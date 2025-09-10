#include "StdAfx.h"
#include "FileDuplicateChecker.h"
#include "../CadLogger.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <wincrypt.h>
#include <fstream>
#include <algorithm>

FileDuplicateChecker::FileDuplicateChecker()
{
    // 设置记录文件路径
    wchar_t appDataPath[MAX_PATH];
    if (SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath) == S_OK) {
        CreateDirectory(appDataPath, nullptr);
        m_recordsFilePath = std::wstring(appDataPath) + L"\\file_upload_records.json";
    }
    
    // 加载现有记录
    loadRecords();
}

FileDuplicateChecker::~FileDuplicateChecker()
{
    saveRecords();
}

std::wstring FileDuplicateChecker::calculateFileMD5(const std::wstring& filePath)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BYTE rgbFile[1024];
    DWORD cbRead = 0;
    BYTE rgbHash[16];
    DWORD cbHash = 16;
    std::wstring result;
    
    do {
        if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            acutPrintf(_T("无法获取加密上下文"));
            break;
        }
        
        if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
            acutPrintf(_T("无法创建MD5哈希对象"));
            break;
        }
        
        hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            acutPrintf(_T("无法打开文件计算MD5: %s"), filePath.c_str());
            break;
        }
        
        while (ReadFile(hFile, rgbFile, sizeof(rgbFile), &cbRead, nullptr) && cbRead > 0) {
            if (!CryptHashData(hHash, rgbFile, cbRead, 0)) {
                acutPrintf(_T("计算MD5哈希失败"));
                break;
            }
        }
        
        if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
            acutPrintf(_T("获取MD5哈希值失败"));
            break;
        }
        
        // 转换为十六进制字符串
        wchar_t hexString[33];
        for (DWORD i = 0; i < cbHash; i++) {
            swprintf_s(&hexString[i * 2], 3, L"%02x", rgbHash[i]);
        }
        result = hexString;
        
    } while (false);
    
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
    if (hHash) {
        CryptDestroyHash(hHash);
    }
    if (hProv) {
        CryptReleaseContext(hProv, 0);
    }
    
    return result;
}

bool FileDuplicateChecker::isFileAlreadyUploaded(const std::wstring& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 检查文件是否存在
    if (!PathFileExists(filePath.c_str())) {
        return false;
    }
    
    // 计算文件MD5
    std::wstring md5Hash = calculateFileMD5(filePath);
    if (md5Hash.empty()) {
        return false;
    }
    
    // 检查是否已记录
    auto it = m_fileRecords.find(md5Hash);
    if (it != m_fileRecords.end()) {
        const FileFingerprint& record = it->second;
        
        // 验证文件是否仍然存在且未修改
        if (PathFileExists(record.filePath.c_str()) && record.isUploaded) {
            WIN32_FILE_ATTRIBUTE_DATA fileData;
            WIN32_FILE_ATTRIBUTE_DATA recordData;
            
            if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileData) &&
                GetFileAttributesEx(record.filePath.c_str(), GetFileExInfoStandard, &recordData)) {
                
                // 检查文件大小和修改时间
                LARGE_INTEGER fileSize, recordSize;
                fileSize.LowPart = fileData.nFileSizeLow;
                fileSize.HighPart = fileData.nFileSizeHigh;
                recordSize.LowPart = recordData.nFileSizeLow;
                recordSize.HighPart = recordData.nFileSizeHigh;
                
                if (fileSize.QuadPart == record.fileSize &&
                    CompareFileTime(&fileData.ftLastWriteTime, &recordData.ftLastWriteTime) == 0) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool FileDuplicateChecker::isFileAlreadyUploaded(const std::wstring& fileName, const std::wstring& buildingName,
                                                const std::wstring& specialty, const std::wstring& version,
                                                const std::wstring& designUnit, const std::wstring& creator)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::wstring metadataFingerprint = generateMetadataFingerprint(fileName, buildingName, specialty, 
                                                                  version, designUnit, creator);
    
    auto it = m_metadataRecords.find(metadataFingerprint);
    if (it != m_metadataRecords.end()) {
        std::wstring md5Hash = it->second;
        auto fileIt = m_fileRecords.find(md5Hash);
        if (fileIt != m_fileRecords.end()) {
            const FileFingerprint& record = fileIt->second;
            return record.isUploaded;
        }
    }
    
    return false;
}

void FileDuplicateChecker::addUploadedFile(const std::wstring& filePath, const std::wstring& serverPath,
                                         const std::wstring& fileName, const std::wstring& buildingName,
                                         const std::wstring& specialty, const std::wstring& version,
                                         const std::wstring& designUnit, const std::wstring& creator)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 生成文件指纹
    FileFingerprint fingerprint = generateFingerprint(filePath);
    fingerprint.serverPath = serverPath;
    fingerprint.isUploaded = true;
    
    // 添加到记录
    m_fileRecords[fingerprint.md5Hash] = fingerprint;
    
    // 添加元数据记录
    std::wstring metadataFingerprint = generateMetadataFingerprint(fileName, buildingName, specialty,
                                                                  version, designUnit, creator);
    m_metadataRecords[metadataFingerprint] = fingerprint.md5Hash;
    
    // 保存记录
    saveRecords();
    
    if (m_fileRecordAddedCallback) {
        m_fileRecordAddedCallback(filePath, serverPath);
    }
}

void FileDuplicateChecker::updateFileRecord(const std::wstring& filePath, const std::wstring& newServerPath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::wstring md5Hash = calculateFileMD5(filePath);
    auto it = m_fileRecords.find(md5Hash);
    if (it != m_fileRecords.end()) {
        it->second.serverPath = newServerPath;
        saveRecords();
        
        if (m_fileRecordUpdatedCallback) {
            m_fileRecordUpdatedCallback(filePath, newServerPath);
        }
    }
}

void FileDuplicateChecker::removeFileRecord(const std::wstring& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::wstring md5Hash = calculateFileMD5(filePath);
    auto it = m_fileRecords.find(md5Hash);
    if (it != m_fileRecords.end()) {
        m_fileRecords.erase(it);
        
        // 清理元数据记录
        for (auto metaIt = m_metadataRecords.begin(); metaIt != m_metadataRecords.end();) {
            if (metaIt->second == md5Hash) {
                metaIt = m_metadataRecords.erase(metaIt);
            } else {
                ++metaIt;
            }
        }
        
        saveRecords();
    }
}

void FileDuplicateChecker::cleanupInvalidRecords()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 清理不存在的文件记录
    for (auto it = m_fileRecords.begin(); it != m_fileRecords.end();) {
        if (!PathFileExists(it->second.filePath.c_str())) {
            it = m_fileRecords.erase(it);
        } else {
            ++it;
        }
    }
    
    // 清理无效的元数据记录
    for (auto it = m_metadataRecords.begin(); it != m_metadataRecords.end();) {
        if (m_fileRecords.find(it->second) == m_fileRecords.end()) {
            it = m_metadataRecords.erase(it);
        } else {
            ++it;
        }
    }
    
    saveRecords();
}

std::vector<std::wstring> FileDuplicateChecker::getDuplicateFiles(const std::wstring& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::wstring> duplicates;
    
    std::wstring md5Hash = calculateFileMD5(filePath);
    if (md5Hash.empty()) {
        return duplicates;
    }
    
    for (const auto& pair : m_fileRecords) {
        if (pair.first == md5Hash && pair.second.filePath != filePath) {
            duplicates.push_back(pair.second.filePath);
        }
    }
    
    return duplicates;
}

FileDuplicateChecker::FileFingerprint FileDuplicateChecker::generateFingerprint(const std::wstring& filePath)
{
    FileFingerprint fingerprint;
    fingerprint.filePath = filePath;
    
    // 提取文件名
    size_t lastSlash = filePath.find_last_of(L"\\");
    if (lastSlash != std::wstring::npos) {
        fingerprint.fileName = filePath.substr(lastSlash + 1);
    } else {
        fingerprint.fileName = filePath;
    }
    
    // 计算MD5
    fingerprint.md5Hash = calculateFileMD5(filePath);
    
    // 获取文件信息
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileData)) {
        LARGE_INTEGER fileSize;
        fileSize.LowPart = fileData.nFileSizeLow;
        fileSize.HighPart = fileData.nFileSizeHigh;
        fingerprint.fileSize = fileSize.QuadPart;
        
        FileTimeToSystemTime(&fileData.ftLastWriteTime, &fingerprint.lastModified);
    }
    
    fingerprint.isUploaded = false;
    
    return fingerprint;
}

std::wstring FileDuplicateChecker::generateMetadataFingerprint(const std::wstring& fileName, const std::wstring& buildingName,
                                                             const std::wstring& specialty, const std::wstring& version,
                                                             const std::wstring& designUnit, const std::wstring& creator)
{
    // 组合所有元数据生成唯一指纹
    std::wstring combined = fileName + L"|" + buildingName + L"|" + specialty + L"|" + 
                           version + L"|" + designUnit + L"|" + creator;
    
    // 使用简单的哈希算法
    std::hash<std::wstring> hasher;
    size_t hashValue = hasher(combined);
    
    wchar_t hashStr[32];
    swprintf_s(hashStr, L"%zu", hashValue);
    
    return std::wstring(hashStr);
}

void FileDuplicateChecker::saveRecords()
{
    // 这里应该实现JSON文件的保存
    // 由于移除了Qt的JSON支持，需要使用其他方法
    // 暂时使用简单的文本格式保存
    
    std::wofstream file(m_recordsFilePath);
    if (!file.is_open()) {
        acutPrintf(_T("无法保存文件记录: %s"), m_recordsFilePath.c_str());
        return;
    }
    
    // 保存文件记录
    file << L"[FileRecords]\n";
    for (const auto& pair : m_fileRecords) {
        const FileFingerprint& fp = pair.second;
        file << L"MD5=" << pair.first << L"\n";
        file << L"FilePath=" << fp.filePath << L"\n";
        file << L"FileName=" << fp.fileName << L"\n";
        file << L"FileSize=" << fp.fileSize << L"\n";
        file << L"ServerPath=" << fp.serverPath << L"\n";
        file << L"IsUploaded=" << (fp.isUploaded ? L"1" : L"0") << L"\n";
        file << L"---\n";
    }
    
    // 保存元数据记录
    file << L"[MetadataRecords]\n";
    for (const auto& pair : m_metadataRecords) {
        file << L"Metadata=" << pair.first << L"\n";
        file << L"MD5=" << pair.second << L"\n";
        file << L"---\n";
    }
    
    file.close();
}

void FileDuplicateChecker::loadRecords()
{
    std::wifstream file(m_recordsFilePath);
    if (!file.is_open()) {
        // 文件不存在，这是正常的
        return;
    }
    
    std::wstring line;
    std::wstring section;
    FileFingerprint currentFp;
    std::wstring currentMD5;
    std::wstring metadataKey, metadataValue;
    
    while (std::getline(file, line)) {
        // 去除行尾的回车换行
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        
        if (line.empty()) continue;
        
        if (line == L"[FileRecords]") {
            section = L"FileRecords";
            continue;
        } else if (line == L"[MetadataRecords]") {
            section = L"MetadataRecords";
            continue;
        } else if (line == L"---") {
            if (section == L"FileRecords" && !currentMD5.empty()) {
                m_fileRecords[currentMD5] = currentFp;
                currentFp = FileFingerprint();
                currentMD5.clear();
            } else if (section == L"MetadataRecords" && !metadataKey.empty() && !metadataValue.empty()) {
                m_metadataRecords[metadataKey] = metadataValue;
                metadataKey.clear();
                metadataValue.clear();
            }
            continue;
        }
        
        size_t equalPos = line.find(L'=');
        if (equalPos == std::wstring::npos) continue;
        
        std::wstring key = line.substr(0, equalPos);
        std::wstring value = line.substr(equalPos + 1);
        
        if (section == L"FileRecords") {
            if (key == L"MD5") {
                currentMD5 = value;
                currentFp.md5Hash = value;
            } else if (key == L"FilePath") {
                currentFp.filePath = value;
            } else if (key == L"FileName") {
                currentFp.fileName = value;
            } else if (key == L"FileSize") {
                currentFp.fileSize = _wtoll(value.c_str());
            } else if (key == L"ServerPath") {
                currentFp.serverPath = value;
            } else if (key == L"IsUploaded") {
                currentFp.isUploaded = (value == L"1");
            }
        } else if (section == L"MetadataRecords") {
            if (key == L"Metadata") {
                metadataKey = value;
            } else if (key == L"MD5") {
                metadataValue = value;
            }
        }
    }
    
    file.close();
} 
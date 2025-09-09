#include "StdAfx.h"
#include "SheetFileManager.h"
#include "../../views/SheetListWindow.h"
#include "../../services/SearchTextInDwg.h"
#include "../CadLogger.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <fstream>
#include <regex>
#include <algorithm>
#include <thread>
#include <sstream>  // 添加这个包含

SheetFileManager::SheetFileManager()
    : m_hSession(nullptr)
    , m_hConnect(nullptr)
    , m_retryTimer(nullptr)
    , m_maxRetryCount(3)
    , m_maxConcurrentUploads(3)
    , m_currentUploads(0)
{
    // 设置缓存目录
    wchar_t appDataPath[MAX_PATH];
    if (SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath) == S_OK) {
        m_cacheDirectory = std::wstring(appDataPath) + L"\\SheetCache";
        CreateDirectory(m_cacheDirectory.c_str(), nullptr);
    }
    
    // 设置服务器地址
    m_serverBaseUrl = L"localhost";
    
    // 初始化WinHTTP
    m_hSession = WinHttpOpen(L"SheetFileManager/1.0",
                            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                            WINHTTP_NO_PROXY_NAME,
                            WINHTTP_NO_PROXY_BYPASS,
                            0);
    if (m_hSession) {
        m_hConnect = WinHttpConnect(m_hSession, m_serverBaseUrl.c_str(), 8080, 0);
    }
    // 加载缓存索引
    loadCacheIndex();
}

SheetFileManager::~SheetFileManager()
{
    saveCacheIndex();
    
    // 清理WinHTTP资源
    if (m_hConnect) {
        WinHttpCloseHandle(m_hConnect);
    }
    if (m_hSession) {
        WinHttpCloseHandle(m_hSession);
    }
    // 清理定时器
    if (m_retryTimer) {
        CloseHandle(m_retryTimer);
    }
}

std::wstring SheetFileManager::generateFileName(const std::wstring& buildingName, const std::wstring& specialty,
                                               const std::wstring& version, const std::wstring& designUnit,
                                               const std::wstring& creator, const std::wstring& sheetName)
{
    // 清理输入参数，移除特殊字符
    auto cleanString = [](const std::wstring& str) -> std::wstring {
        std::wstring cleaned = str;
        // 移除或替换特殊字符
        std::wregex invalidChars(L"[<>:\"/\\\\|?*]");
        cleaned = std::regex_replace(cleaned, invalidChars, L"_");
        std::replace(cleaned.begin(), cleaned.end(), L' ', L'_');
        return cleaned;
    };
    
    std::wstring fileName = cleanString(buildingName) + L"_" +
                           cleanString(specialty) + L"_" +
                           cleanString(version) + L"_" +
                           cleanString(designUnit) + L"_" +
                           cleanString(creator) + L"_" +
                           cleanString(sheetName);
    return fileName;
}

bool SheetFileManager::needsReupload(const SheetData& oldData,
                                    const SheetData& newData)
{
    return (oldData.buildingName != newData.buildingName ||
            oldData.specialty != newData.specialty ||
            oldData.version != newData.version ||
            oldData.designUnit != newData.designUnit ||
            oldData.name != newData.name);
}

std::wstring SheetFileManager::getLocalCachePath(const std::wstring& serverFileName)
{
    return m_cacheDirectory + L"\\" + serverFileName;
}

bool SheetFileManager::isFileCached(const std::wstring& serverFileName)
{
    std::wstring localPath = getLocalCachePath(serverFileName);
    return PathFileExists(localPath.c_str()) && m_fileCache.find(serverFileName) != m_fileCache.end();
}

bool SheetFileManager::downloadFile(const std::wstring& serverFileName, const std::wstring& localPath)
{
    if (isFileCached(serverFileName) && localPath.empty()) {
        return true; // 文件已缓存
    }
    
    std::wstring targetPath = localPath.empty() ? getLocalCachePath(serverFileName) : localPath;
    
    if (!m_hConnect) {
        CadLogger::LogError(_T("WinHTTP连接未初始化"));
        return false;
    }
    
    std::wstring requestPath = L"/download/" + serverFileName;
    HINTERNET hRequest = WinHttpOpenRequest(m_hConnect,
                                           L"GET",
                                           requestPath.c_str(),
                                           nullptr,
                                           WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           0);
    
    if (!hRequest) {
        CadLogger::LogError(_T("创建下载请求失败"));
        return false;
    }
    
    BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!result) {
        CadLogger::LogError(_T("发送下载请求失败"));
        WinHttpCloseHandle(hRequest);
        return false;
    }
    
    result = WinHttpReceiveResponse(hRequest, nullptr);
    if (!result) {
        CadLogger::LogError(_T("接收下载响应失败"));
        WinHttpCloseHandle(hRequest);
        return false;
    }
    
    // 检查HTTP状态码
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                       WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
    
    if (statusCode != 200) {
        CadLogger::LogError(_T("下载失败，HTTP状态码: %d"), statusCode);
        WinHttpCloseHandle(hRequest);
        return false;
    }
    
    // 读取文件内容并保存
    std::ofstream file(targetPath, std::ios::binary);
    if (!file.is_open()) {
        CadLogger::LogError(_T("无法创建本地文件: %s"), targetPath.c_str());
        WinHttpCloseHandle(hRequest);
        return false;
    }
    
    DWORD bytesAvailable = 0;
    DWORD bytesRead = 0;
    char buffer[8192];
    
    do {
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            break;
        }
        
        if (bytesAvailable == 0) {
            break;
        }
        
        DWORD bytesToRead = min(bytesAvailable, sizeof(buffer));
        if (!WinHttpReadData(hRequest, buffer, bytesToRead, &bytesRead)) {
            break;
        }
        
        file.write(buffer, bytesRead);
        
    } while (bytesAvailable > 0);
    
    file.close();
    WinHttpCloseHandle(hRequest);
    
    // 更新缓存索引
    FileInfo fileInfo;
    fileInfo.serverName = serverFileName;
    fileInfo.localPath = targetPath;
    fileInfo.isCached = true;
    GetSystemTime(&fileInfo.lastModified);
    
    std::lock_guard<std::mutex> lock(m_taskMutex);
    m_fileCache[serverFileName] = fileInfo;
    
    if (m_downloadCompletedCallback) {
        m_downloadCompletedCallback(serverFileName, true);
    }
    
    CadLogger::LogInfo(_T("文件下载成功: %s"), serverFileName.c_str());
    return true;
}

bool SheetFileManager::uploadFile(const UploadTask& task)
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    // 检查并发上传数量限制
    if (m_currentUploads >= m_maxConcurrentUploads) {
        m_uploadTasks.push_back(task);
        return true;
    }
    
    processNextUploadTask();
    return true;
}

void SheetFileManager::addUploadTask(const UploadTask& task)
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    m_uploadTasks.push_back(task);
    if (m_taskQueueChangedCallback) {
        m_taskQueueChangedCallback((int)m_uploadTasks.size(), 0, 0);
    }
}

void SheetFileManager::startBatchUpload()
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    processNextUploadTask();
}

void SheetFileManager::processNextUploadTask()
{
    if (m_uploadTasks.empty() || m_currentUploads >= m_maxConcurrentUploads) {
        return;
    }
    
    UploadTask task = m_uploadTasks.front();
    m_uploadTasks.erase(m_uploadTasks.begin());
    m_currentUploads++;
    
    // 在新线程中执行上传
    std::thread uploadThread([this, task]() {
        this->performUpload(task);
    });
    uploadThread.detach();
}

void SheetFileManager::performUpload(const UploadTask& task)
{
    if (!m_hConnect) {
        CadLogger::LogError(_T("WinHTTP连接未初始化"));
        m_currentUploads--;
        if (m_uploadCompletedCallback) {
            m_uploadCompletedCallback(task.serverFileName, false);
        }
        return;
    }
    
    // 检查文件是否存在
    if (!PathFileExists(task.localFilePath.c_str())) {
        CadLogger::LogError(_T("上传文件不存在: %s"), task.localFilePath.c_str());
        m_currentUploads--;
        if (m_uploadCompletedCallback) {
            m_uploadCompletedCallback(task.serverFileName, false);
        }
        return;
    }
    
    HINTERNET hRequest = WinHttpOpenRequest(m_hConnect,
                                           L"POST",
                                           L"/upload",
                                           nullptr,
                                           WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           0);
    
    if (!hRequest) {
        CadLogger::LogError(_T("创建上传请求失败"));
        m_currentUploads--;
        if (m_uploadCompletedCallback) {
            m_uploadCompletedCallback(task.serverFileName, false);
        }
        return;
    }
    
    // 构建multipart/form-data内容
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string contentType = "Content-Type: multipart/form-data; boundary=" + boundary;
    
    // 读取文件内容
    std::ifstream file(task.localFilePath, std::ios::binary);
    if (!file.is_open()) {
        CadLogger::LogError(_T("无法打开上传文件: %s"), task.localFilePath.c_str());
        WinHttpCloseHandle(hRequest);
        m_currentUploads--;
        if (m_uploadCompletedCallback) {
            m_uploadCompletedCallback(task.serverFileName, false);
        }
        return;
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 构建POST数据
    std::ostringstream postData;
    postData << "--" << boundary << "\r\n";
    postData << "Content-Disposition: form-data; name=\"file\"; filename=\"";
    
    // 转换文件名为UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, task.serverFileName.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8FileName(utf8Size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, task.serverFileName.c_str(), -1, &utf8FileName[0], utf8Size, nullptr, nullptr);
    
    postData << utf8FileName << "\"\r\n";
    postData << "Content-Type: application/octet-stream\r\n\r\n";
    
    std::string postDataStr = postData.str();
    
    // 添加结束边界
    std::string endBoundary = "\r\n--" + boundary + "--\r\n";
    
    // 计算总大小
    DWORD totalSize = (DWORD)(postDataStr.length() + fileSize + endBoundary.length());
    
    // 设置请求头
    std::wstring headers = L"Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW";
    
    BOOL result = WinHttpSendRequest(hRequest,
                                    headers.c_str(),
                                    (DWORD)headers.length(),
                                    WINHTTP_NO_REQUEST_DATA,
                                    0,
                                    totalSize,
                                    0);
    
    if (!result) {
        CadLogger::LogError(_T("发送上传请求失败"));
        file.close();
        WinHttpCloseHandle(hRequest);
        m_currentUploads--;
        if (m_uploadCompletedCallback) {
            m_uploadCompletedCallback(task.serverFileName, false);
        }
        return;
    }
    
    // 发送POST数据头部
    DWORD bytesWritten = 0;
    WinHttpWriteData(hRequest, postDataStr.c_str(), (DWORD)postDataStr.length(), &bytesWritten);
    
    // 发送文件内容
    char buffer[8192];
    DWORD totalBytesUploaded = 0;
    
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        
        if (bytesRead > 0) {
            if (!WinHttpWriteData(hRequest, buffer, (DWORD)bytesRead, &bytesWritten)) {
                CadLogger::LogError(_T("写入文件数据失败"));
                break;
            }
            
            totalBytesUploaded += bytesWritten;
            
            // 报告上传进度
            if (m_uploadProgressCallback) {
                m_uploadProgressCallback(task.serverFileName, totalBytesUploaded, (DWORD)fileSize);
            }
        }
    }
    
    // 发送结束边界
    WinHttpWriteData(hRequest, endBoundary.c_str(), (DWORD)endBoundary.length(), &bytesWritten);
    
    file.close();
    
    // 接收响应
    result = WinHttpReceiveResponse(hRequest, nullptr);
    if (!result) {
        CadLogger::LogError(_T("接收上传响应失败"));
        WinHttpCloseHandle(hRequest);
        m_currentUploads--;
        if (m_uploadCompletedCallback) {
            m_uploadCompletedCallback(task.serverFileName, false);
        }
        return;
    }
    
    // 检查HTTP状态码
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                       WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
    
    bool success = (statusCode == 200);
    
    WinHttpCloseHandle(hRequest);
    m_currentUploads--;
    
    if (success) {
        CadLogger::LogInfo(_T("文件上传成功: %s"), task.serverFileName.c_str());
        updateTextIndexForUploadedFile(task);
    } else {
        CadLogger::LogError(_T("文件上传失败: %s, HTTP状态码: %d"), task.serverFileName.c_str(), statusCode);
    }
    
    if (m_uploadCompletedCallback) {
        m_uploadCompletedCallback(task.serverFileName, success);
    }
    
    // 处理下一个上传任务
    processNextUploadTask();
}

bool SheetFileManager::deleteFile(const std::wstring& serverFileName)
{
    if (!m_hConnect) {
        return false;
    }
    
    std::wstring requestPath = L"/delete/" + serverFileName;
    HINTERNET hRequest = WinHttpOpenRequest(m_hConnect,
                                           L"DELETE",
                                           requestPath.c_str(),
                                           nullptr,
                                           WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           0);
    
    if (!hRequest) {
        return false;
    }
    
    BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (result) {
        result = WinHttpReceiveResponse(hRequest, nullptr);
    }
    
    DWORD statusCode = 0;
    if (result) {
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
    }
    
    WinHttpCloseHandle(hRequest);
    
    bool success = (statusCode == 200);
    if (success) {
        // 从缓存中移除
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_fileCache.erase(serverFileName);
        
        // 删除本地缓存文件
        std::wstring localPath = getLocalCachePath(serverFileName);
        DeleteFile(localPath.c_str());
    }
    
    return success;
}

std::wstring SheetFileManager::getNextVersion(const std::wstring& baseName)
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    auto it = m_versionHistory.find(baseName);
    if (it != m_versionHistory.end()) {
        // 解析当前版本号并递增
        std::wstring currentVersion = it->second;
        try {
            int version = std::stoi(currentVersion);
            return std::to_wstring(version + 1);
        } catch (...) {
            return L"1";
        }
    }
    
    return L"1";
}

std::vector<std::wstring> SheetFileManager::getVersionHistory(const std::wstring& baseName)
{
    std::vector<std::wstring> versions;
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    // 这里应该从服务器或数据库查询版本历史
    // 暂时返回空列表
    return versions;
}

bool SheetFileManager::rollbackToVersion(const std::wstring& baseName, const std::wstring& version)
{
    // 实现版本回滚逻辑
    return false;
}

void SheetFileManager::clearExpiredCache(int daysOld)
{
    SYSTEMTIME currentTime;
    GetSystemTime(&currentTime);
    
    FILETIME currentFileTime;
    SystemTimeToFileTime(&currentTime, &currentFileTime);
    
    ULARGE_INTEGER currentTime64;
    currentTime64.LowPart = currentFileTime.dwLowDateTime;
    currentTime64.HighPart = currentFileTime.dwHighDateTime;
    
    ULARGE_INTEGER expireTime64;
    expireTime64.QuadPart = currentTime64.QuadPart - ((ULONGLONG)daysOld * 24 * 60 * 60 * 10000000ULL);
    
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    for (auto it = m_fileCache.begin(); it != m_fileCache.end();) {
        FILETIME fileTime;
        SystemTimeToFileTime(&it->second.lastModified, &fileTime);
        
        ULARGE_INTEGER fileTime64;
        fileTime64.LowPart = fileTime.dwLowDateTime;
        fileTime64.HighPart = fileTime.dwHighDateTime;
        
        if (fileTime64.QuadPart < expireTime64.QuadPart) {
            // 删除过期的缓存文件
            DeleteFile(it->second.localPath.c_str());
            it = m_fileCache.erase(it);
        } else {
            ++it;
        }
    }
}

void SheetFileManager::clearAllCache()
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    for (const auto& pair : m_fileCache) {
        DeleteFile(pair.second.localPath.c_str());
    }
    
    m_fileCache.clear();
}

__int64 SheetFileManager::getCacheSize()
{
    __int64 totalSize = 0;
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    for (const auto& pair : m_fileCache) {
        WIN32_FILE_ATTRIBUTE_DATA fileData;
        if (GetFileAttributesEx(pair.second.localPath.c_str(), GetFileExInfoStandard, &fileData)) {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = fileData.nFileSizeLow;
            fileSize.HighPart = fileData.nFileSizeHigh;
            totalSize += fileSize.QuadPart;
        }
    }
    
    return totalSize;
}

void SheetFileManager::cancelAllTasks()
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    m_uploadTasks.clear();
}

void SheetFileManager::retryFailedTasks()
{
    // 实现重试失败任务的逻辑
}

std::vector<std::wstring> SheetFileManager::getFailedTasks()
{
    // 返回失败任务列表
    return std::vector<std::wstring>();
}

void SheetFileManager::clearFailedTasks()
{
    // 清理失败任务
}

bool SheetFileManager::hasFailedTasks() const
{
    // 检查是否有失败任务
    return false;
}

std::wstring SheetFileManager::resolveFileNameConflict(const std::wstring& baseName, const std::wstring& extension)
{
    return generateUniqueFileName(baseName, extension);
}

std::wstring SheetFileManager::generateUniqueFileName(const std::wstring& baseName, const std::wstring& extension)
{
    std::wstring fileName = baseName + extension;
    int counter = 1;
    
    while (PathFileExists(fileName.c_str())) {
        fileName = baseName + L"_" + std::to_wstring(counter) + extension;
        counter++;
    }
    
    return fileName;
}

std::wstring SheetFileManager::calculateMD5(const std::wstring& filePath)
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
            break;
        }
        
        if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
            break;
        }
        
        hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            break;
        }
        
        while (ReadFile(hFile, rgbFile, sizeof(rgbFile), &cbRead, nullptr) && cbRead > 0) {
            if (!CryptHashData(hHash, rgbFile, cbRead, 0)) {
                break;
            }
        }
        
        if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
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

bool SheetFileManager::verifyFileIntegrity(const std::wstring& filePath, const std::wstring& expectedMD5)
{
    std::wstring actualMD5 = calculateMD5(filePath);
    return _wcsicmp(actualMD5.c_str(), expectedMD5.c_str()) == 0;
}

void SheetFileManager::handleUploadResponse(HINTERNET hRequest, const UploadTask& task)
{
    // 处理上传响应的逻辑已在performUpload中实现
}

void SheetFileManager::handleDownloadResponse(HINTERNET hRequest, const std::wstring& fileName)
{
    // 处理下载响应的逻辑已在downloadFile中实现
}

void SheetFileManager::updateCacheIndex()
{
    saveCacheIndex();
}

void SheetFileManager::loadCacheIndex()
{
    std::wstring indexPath = m_cacheDirectory + L"\\cache_index.json";
    
    // 这里应该实现JSON文件的读取
    // 由于移除了Qt的JSON支持，需要使用其他JSON库或自定义格式
    // 暂时留空，实际使用时可以使用Windows API或第三方JSON库
}

void SheetFileManager::saveCacheIndex()
{
    std::wstring indexPath = m_cacheDirectory + L"\\cache_index.json";
    
    // 这里应该实现JSON文件的写入
    // 由于移除了Qt的JSON支持，需要使用其他JSON库或自定义格式
    // 暂时留空，实际使用时可以使用Windows API或第三方JSON库
}

void SheetFileManager::handleNetworkError(DWORD error, const std::wstring& operation)
{
    CadLogger::LogError(_T("网络操作失败: %s, 错误代码: %d"), operation.c_str(), error);
}

void SheetFileManager::scheduleRetry(const UploadTask& task)
{
    // 实现重试调度逻辑
    if (task.retryCount < m_maxRetryCount) {
        UploadTask retryTask = task;
        retryTask.retryCount++;
        
        // 延迟重试
        std::thread retryThread([this, retryTask]() {
            Sleep(5000); // 5秒后重试
            this->addUploadTask(retryTask);
        });
        retryThread.detach();
    }
}

void SheetFileManager::updateTextIndexForUploadedFile(const UploadTask& task)
{
    // 为上传的文件更新文本索引
    std::wstring errorMsg;
    if (!SearchTextInDwg::buildTextIndexForDrawing(task.localFilePath, errorMsg)) {
        CadLogger::LogWarning(_T("为上传文件建立文本索引失败: %s"), errorMsg.c_str());
    }
}

// SheetStatusManager 实现

SheetStatusManager::SheetStatusManager()
{
}

void SheetStatusManager::setStatus(const std::wstring& fileName, Status status)
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    
    Status oldStatus = getStatus(fileName);
    m_statusMap[fileName] = status;
    
    if (m_statusChangedCallback && oldStatus != status) {
        m_statusChangedCallback(fileName, oldStatus, status);
    }
}

SheetStatusManager::Status SheetStatusManager::getStatus(const std::wstring& fileName) const
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    
    auto it = m_statusMap.find(fileName);
    if (it != m_statusMap.end()) {
        return it->second;
    }
    
    return PendingUpload;
}

std::wstring SheetStatusManager::getStatusText(Status status) const
{
    switch (status) {
        case PendingUpload: return L"待上传";
        case Uploading: return L"上传中";
        case Uploaded: return L"已上传";
        case Downloading: return L"下载中";
        case Downloaded: return L"已下载";
        case Modified: return L"已修改";
        case Error: return L"错误";
        case Deleted: return L"已删除";
        default: return L"未知";
    }
}

std::vector<std::wstring> SheetStatusManager::getFilesByStatus(Status status) const
{
    std::vector<std::wstring> files;
    std::lock_guard<std::mutex> lock(m_statusMutex);
    
    for (const auto& pair : m_statusMap) {
        if (pair.second == status) {
            files.push_back(pair.first);
        }
    }
    
    return files;
}

void SheetStatusManager::markForReupload(const std::wstring& fileName)
{
    setStatus(fileName, PendingUpload);
}

void SheetStatusManager::clearStatus(const std::wstring& fileName)
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_statusMap.erase(fileName);
}
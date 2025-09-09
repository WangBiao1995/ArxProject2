#pragma once

#include <string>
#include <unordered_map>
#include <windows.h>
#include <mutex>
#include <functional>

// 文件重复检查器
class FileDuplicateChecker
{
public:
    struct FileFingerprint {
        std::wstring filePath;
        std::wstring fileName;
        std::wstring md5Hash;
        __int64 fileSize;
        SYSTEMTIME lastModified;
        std::wstring serverPath;
        bool isUploaded;
    };

    // 回调函数类型定义
    typedef std::function<void(const std::wstring&, const std::wstring&)> DuplicateFileFoundCallback;
    typedef std::function<void(const std::wstring&, const std::wstring&)> FileRecordAddedCallback;
    typedef std::function<void(const std::wstring&, const std::wstring&)> FileRecordUpdatedCallback;

    FileDuplicateChecker();
    ~FileDuplicateChecker();

    // 计算文件MD5哈希值
    std::wstring calculateFileMD5(const std::wstring& filePath);
    
    // 检查文件是否已上传（基于MD5）
    bool isFileAlreadyUploaded(const std::wstring& filePath);
    
    // 检查文件是否已上传（基于文件名和元数据）
    bool isFileAlreadyUploaded(const std::wstring& fileName, const std::wstring& buildingName, 
                              const std::wstring& specialty, const std::wstring& version,
                              const std::wstring& designUnit, const std::wstring& creator);
    
    // 添加已上传文件记录
    void addUploadedFile(const std::wstring& filePath, const std::wstring& serverPath, 
                        const std::wstring& fileName, const std::wstring& buildingName,
                        const std::wstring& specialty, const std::wstring& version,
                        const std::wstring& designUnit, const std::wstring& creator);
    
    // 更新文件记录
    void updateFileRecord(const std::wstring& filePath, const std::wstring& newServerPath);
    
    // 删除文件记录
    void removeFileRecord(const std::wstring& filePath);
    
    // 清理无效记录
    void cleanupInvalidRecords();
    
    // 获取重复文件列表
    std::vector<std::wstring> getDuplicateFiles(const std::wstring& filePath);
    
    // 保存/加载记录
    void saveRecords();
    void loadRecords();

    // 回调函数设置
    void setDuplicateFileFoundCallback(DuplicateFileFoundCallback callback) { m_duplicateFileFoundCallback = callback; }
    void setFileRecordAddedCallback(FileRecordAddedCallback callback) { m_fileRecordAddedCallback = callback; }
    void setFileRecordUpdatedCallback(FileRecordUpdatedCallback callback) { m_fileRecordUpdatedCallback = callback; }

private:
    // 生成文件指纹
    FileFingerprint generateFingerprint(const std::wstring& filePath);
    
    // 生成元数据指纹
    std::wstring generateMetadataFingerprint(const std::wstring& fileName, const std::wstring& buildingName,
                                           const std::wstring& specialty, const std::wstring& version,
                                           const std::wstring& designUnit, const std::wstring& creator);
    
    // 成员变量
    std::unordered_map<std::wstring, FileFingerprint> m_fileRecords; // MD5 -> FileFingerprint
    std::unordered_map<std::wstring, std::wstring> m_metadataRecords; // 元数据指纹 -> MD5
    std::mutex m_mutex;
    std::wstring m_recordsFilePath;
    
    // 回调函数
    DuplicateFileFoundCallback m_duplicateFileFoundCallback;
    FileRecordAddedCallback m_fileRecordAddedCallback;
    FileRecordUpdatedCallback m_fileRecordUpdatedCallback;
}; 
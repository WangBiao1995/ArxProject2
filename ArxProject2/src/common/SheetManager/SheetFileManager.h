#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <windows.h>
#include <winhttp.h>
#include <functional>
// 修改包含路径，使用models中的SheetData
#include "../../models/SheetData.h"
#include "../../models/SheetManagerModel.h"
#include <sstream>  // 添加这个包含

// 前向声明
class CWnd;

// 图纸文件管理器 - 处理文件命名、版本控制、缓存等
class SheetFileManager
{
public:
    // 回调函数类型定义
    typedef std::function<void(const std::wstring&, __int64, __int64)> UploadProgressCallback;
    typedef std::function<void(const std::wstring&, bool)> UploadCompletedCallback;
    typedef std::function<void(const std::wstring&, bool)> DownloadCompletedCallback;
    typedef std::function<void(int, int, int)> TaskQueueChangedCallback;

    SheetFileManager();
    ~SheetFileManager();

    // 基础功能
    std::wstring generateFileName(const std::wstring& buildingName, const std::wstring& specialty, 
                                 const std::wstring& version, const std::wstring& designUnit, 
                                 const std::wstring& creator, const std::wstring& sheetName);
    
    bool needsReupload(const SheetData& oldData, 
                      const SheetData& newData);
    
    std::wstring getLocalCachePath(const std::wstring& serverFileName);
    bool isFileCached(const std::wstring& serverFileName);
    
    // 文件操作
    bool downloadFile(const std::wstring& serverFileName, const std::wstring& localPath = L"");
    bool uploadFile(const UploadTask& task);
    bool deleteFile(const std::wstring& serverFileName);
    
    // 版本控制
    std::wstring getNextVersion(const std::wstring& baseName);
    std::vector<std::wstring> getVersionHistory(const std::wstring& baseName);
    bool rollbackToVersion(const std::wstring& baseName, const std::wstring& version);
    
    // 缓存管理
    void clearExpiredCache(int daysOld = 30);
    void clearAllCache();
    __int64 getCacheSize();
    
    // 并发控制
    void addUploadTask(const UploadTask& task);
    void startBatchUpload();
    void cancelAllTasks();
    
    // 错误处理
    void retryFailedTasks();
    std::vector<std::wstring> getFailedTasks();
    void clearFailedTasks();
    
    // 新增：检查是否有失败任务的辅助方法
    bool hasFailedTasks() const;

    // 回调函数设置
    void setUploadProgressCallback(UploadProgressCallback callback) { m_uploadProgressCallback = callback; }
    void setUploadCompletedCallback(UploadCompletedCallback callback) { m_uploadCompletedCallback = callback; }
    void setDownloadCompletedCallback(DownloadCompletedCallback callback) { m_downloadCompletedCallback = callback; }
    void setTaskQueueChangedCallback(TaskQueueChangedCallback callback) { m_taskQueueChangedCallback = callback; }

private:
    // 文件命名冲突处理
    std::wstring resolveFileNameConflict(const std::wstring& baseName, const std::wstring& extension);
    std::wstring generateUniqueFileName(const std::wstring& baseName, const std::wstring& extension);
    
    // 文件完整性验证
    std::wstring calculateMD5(const std::wstring& filePath);
    bool verifyFileIntegrity(const std::wstring& filePath, const std::wstring& expectedMD5);
    
    // 网络操作
    void processNextUploadTask();
    void performUpload(const UploadTask& task);  // 添加这个声明
    void handleUploadResponse(HINTERNET hRequest, const UploadTask& task);
    void handleDownloadResponse(HINTERNET hRequest, const std::wstring& fileName);
    
    // 缓存管理
    void updateCacheIndex();
    void loadCacheIndex();
    void saveCacheIndex();
    
    // 错误处理
    void handleNetworkError(DWORD error, const std::wstring& operation);
    void scheduleRetry(const UploadTask& task);
    
    // 文本索引更新
    void updateTextIndexForUploadedFile(const UploadTask& task);
    
    // 成员变量
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    mutable std::mutex m_taskMutex;
    std::vector<UploadTask> m_uploadTasks;
    std::unordered_map<std::wstring, FileInfo> m_fileCache;
    std::unordered_map<std::wstring, std::wstring> m_versionHistory;
    HANDLE m_retryTimer;
    std::wstring m_cacheDirectory;
    std::wstring m_serverBaseUrl;
    int m_maxRetryCount;
    int m_maxConcurrentUploads;
    int m_currentUploads;
    
    // 回调函数
    UploadProgressCallback m_uploadProgressCallback;
    UploadCompletedCallback m_uploadCompletedCallback;
    DownloadCompletedCallback m_downloadCompletedCallback;
    TaskQueueChangedCallback m_taskQueueChangedCallback;
};

// 图纸状态管理器
class SheetStatusManager
{
public:
    enum Status {
        PendingUpload,    // 待上传
        Uploading,        // 上传中
        Uploaded,         // 已上传
        Downloading,      // 下载中
        Downloaded,       // 已下载
        Modified,         // 已修改
        Error,            // 错误
        Deleted           // 已删除
    };

    SheetStatusManager();
    
    void setStatus(const std::wstring& fileName, Status status);
    Status getStatus(const std::wstring& fileName) const;
    std::wstring getStatusText(Status status) const;
    std::vector<std::wstring> getFilesByStatus(Status status) const;
    
    void markForReupload(const std::wstring& fileName);
    void clearStatus(const std::wstring& fileName);
    
    // 状态变化回调
    typedef std::function<void(const std::wstring&, Status, Status)> StatusChangedCallback;
    void setStatusChangedCallback(StatusChangedCallback callback) { m_statusChangedCallback = callback; }

private:
    std::unordered_map<std::wstring, Status> m_statusMap;
    mutable std::mutex m_statusMutex;
    StatusChangedCallback m_statusChangedCallback;
}; 
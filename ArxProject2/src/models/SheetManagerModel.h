#pragma once
#include <string>
#include <windows.h>

struct FileInfo {
    std::wstring originalName;
    std::wstring serverName;
    std::wstring localPath;
    std::wstring serverPath;
    std::wstring md5Hash;
    SYSTEMTIME lastModified;
    __int64 fileSize;
    bool isCached;
    bool isUploaded;
};

struct UploadTask {
    std::wstring localFilePath;
    std::wstring serverFileName;
    std::wstring buildingName;
    std::wstring specialty;
    std::wstring version;
    std::wstring designUnit;
    std::wstring creator;
    std::wstring sheetName;
    SYSTEMTIME createTime;
    int retryCount;
    bool isCompleted;
};

// 图纸状态枚举
enum class SheetStatus {
    PendingUpload,    // 待上传
    Uploading,        // 上传中
    Uploaded,         // 已上传
    Downloading,      // 下载中
    Downloaded,       // 已下载
    Modified,         // 已修改
    Error,            // 错误
    Deleted           // 已删除
};


#pragma once

#include <string>
#include <vector>
#include <windows.h>

// 前向声明
class SheetFileManager;

// 文本搜索结果结构体
struct TextSearchResult {
    std::wstring filePath;        // 图纸文件路径
    std::wstring textContent;     // 找到的文本内容
    std::wstring layerName;       // 图层名
    double posX;                  // X坐标
    double posY;                  // Y坐标
    double posZ;                  // Z坐标
    std::wstring entityHandle;    // 图元句柄
    SYSTEMTIME lastModified;      // 文件修改时间
};

class SearchTextInDwg
{
public:
    SearchTextInDwg();
    ~SearchTextInDwg();

    // 设置文件管理器（用于获取缓存路径）
    static void setFileManager(SheetFileManager* fileManager);
    
    // 创建文本索引表
    static bool createTextIndexTable();
    
    // 为指定图纸建立文本索引
    static bool buildTextIndexForDrawing(const std::wstring& filePath, std::wstring& errorMsg);
    
    // 为指定大楼的所有图纸建立文本索引（使用缓存路径）
    static bool buildTextIndexForBuilding(const std::wstring& buildingName, std::wstring& errorMsg);
    
    // 为指定大楼的所有图纸建立文本索引（使用服务器文件名列表）
    static bool buildTextIndexForBuildingFromServer(const std::wstring& buildingName, 
                                                   const std::vector<std::wstring>& serverFileNames, 
                                                   std::wstring& errorMsg);
    
    // 搜索包含指定文本的图纸
    static std::vector<TextSearchResult> searchTextInDrawings(const std::wstring& searchText, const std::wstring& buildingName = L"");
    
    // 搜索包含指定文本的图纸
    static std::vector<TextSearchResult> searchTextInDrawings(const std::wstring& searchText, 
                                                             const std::wstring& buildingName,
                                                             std::wstring& errorMsg);
    
    // 增量更新文本索引（只更新修改过的文件）
    static bool updateTextIndexIncremental(const std::wstring& buildingName, std::wstring& errorMsg);
    
    // 清空指定大楼的文本索引
    static bool clearTextIndexForBuilding(const std::wstring& buildingName, std::wstring& errorMsg);

    static void locateText();
    static bool testDatabaseConnection();
    static bool testSearchWithParameters();
    
private:
    static SheetFileManager* s_fileManager;  // 文件管理器实例
    
    // 从DWG文件中提取所有文本信息
    static bool extractTextFromDwg(const std::wstring& localFilePath, 
                              const std::wstring& fileIdentifier,  // 新增参数
                              std::vector<TextSearchResult>& textList, 
                              std::wstring& errorMsg);
    
    // 保存文本索引到数据库
    static bool saveTextIndexToDatabase(const std::vector<TextSearchResult>& textList, std::wstring& errorMsg);
    
    // 获取文件的最后修改时间
    static SYSTEMTIME getFileLastModified(const std::wstring& filePath);
    
    // 检查文件是否需要重新索引
    static bool needsReindexing(const std::wstring& filePath, const SYSTEMTIME& currentModified);
    
    // 获取缓存目录中的所有DWG文件
    static std::vector<std::wstring> getCachedDrawingFiles(const std::wstring& buildingName);
    
    // 从服务器文件名获取本地缓存路径
    static std::wstring getLocalCachePathFromServerName(const std::wstring& serverFileName);
    
    // 从本地缓存路径获取服务器文件名
    static std::wstring getServerFileNameFromLocalPath(const std::wstring& localPath);
    static bool cleanupOldPathData(std::wstring& errorMsg);
};


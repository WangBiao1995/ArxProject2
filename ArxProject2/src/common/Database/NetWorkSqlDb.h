#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <winhttp.h>
#include <nlohmann/json.hpp>

// 前向声明
struct TextSearchResult;

// 建筑信息数据结构
struct BuildingInfo {
    int id;
    std::wstring building_name;
    std::wstring address;
    std::wstring total_area;
    std::wstring floors;
    std::wstring design_unit;
    std::wstring create_time;
    std::wstring creator;
    std::wstring create_datetime;
    std::wstring update_datetime;
};

// 图纸信息数据结构
struct SheetInfo {
    int id;
    std::wstring name;
    int building;
    std::wstring building_name;
    std::wstring building_name_display;
    std::wstring specialty;
    std::wstring format;
    std::wstring status;
    std::wstring version;
    std::wstring design_unit;
    std::wstring create_time;
    std::wstring creator;
    bool is_selected;
    std::wstring file_path;
    long long file_size;
    std::wstring md5_hash;
    std::wstring create_datetime;
    std::wstring update_datetime;
};

// CAD文本索引数据结构
struct CadTextIndex {
    int id;
    std::wstring file_path;
    std::wstring text_content;
    std::wstring layer_name;
    double pos_x;
    double pos_y;
    double pos_z;
    std::wstring entity_handle;
    std::wstring last_modified;
    int sheet;
    std::wstring sheet_name;
    std::wstring create_datetime;
    std::wstring update_datetime;
};

// API响应结构
struct ApiResponse {
    int code;
    std::wstring msg;
    nlohmann::json data;
    bool success;
};

// 分页响应结构
struct PaginatedResponse {
    int count;
    std::wstring next;
    std::wstring previous;
    std::vector<nlohmann::json> results;
};

const std::wstring _token = L"JWT eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ0b2tlbl90eXBlIjoiYWNjZXNzIiwiZXhwIjoxNzU3OTg4MjA1LCJpYXQiOjE3NTc5MDE4MDUsImp0aSI6ImU4ZDM0MTc1MjJhYzRiYTk5NTY5MjVlNjgwNTc5NDI2IiwidXNlcl9pZCI6MX0.OAOXwdQacSDPwKKt29_9PkocdUDaND-t3BAj1c3O43c";
class NetWorkSqlDb
{
public:
    NetWorkSqlDb();
    ~NetWorkSqlDb();

    
    // 初始化和配置
    static bool initialize(const std::wstring& baseUrl, const std::wstring& token = _token);
    static void setAuthToken(const std::wstring& token);
    static void setBaseUrl(const std::wstring& baseUrl);

    // 建筑信息管理 (BuildingInfo)
    static bool getBuildings(std::vector<BuildingInfo>& buildings, 
                            int page = 1, int pageSize = 999999,
                            const std::wstring& search = L"",
                            const std::wstring& designUnit = L"",
                            const std::wstring& creator = L"",
                            const std::wstring& ordering = L"-create_datetime",
                            std::wstring& errorMsg = std::wstring());
    
    static bool getBuilding(int id, BuildingInfo& building, std::wstring& errorMsg = std::wstring());
    static bool createBuilding(const BuildingInfo& building, BuildingInfo& result, std::wstring& errorMsg = std::wstring());
    static bool updateBuilding(int id, const BuildingInfo& building, BuildingInfo& result, std::wstring& errorMsg = std::wstring());
    static bool deleteBuilding(int id, std::wstring& errorMsg = std::wstring());

    // 图纸信息管理 (SheetInfo)
    static bool getSheets(std::vector<SheetInfo>& sheets,
                         int page = 1, int pageSize = 999999,
                         const std::wstring& search = L"",
                         const std::wstring& specialty = L"",
                         const std::wstring& format = L"",
                         const std::wstring& status = L"",
                         const std::wstring& designUnit = L"",
                         const std::wstring& creator = L"",
                         int building = 0,
                         const std::wstring& ordering = L"-create_datetime",
                         std::wstring& errorMsg = std::wstring());
    
    static bool getSheet(int id, SheetInfo& sheet, std::wstring& errorMsg = std::wstring());
    static bool createSheet(const SheetInfo& sheet, SheetInfo& result, std::wstring& errorMsg = std::wstring());
    static bool updateSheet(int id, const SheetInfo& sheet, SheetInfo& result, std::wstring& errorMsg = std::wstring());
    static bool deleteSheet(int id, std::wstring& errorMsg = std::wstring());
    static bool getSheetsByBuilding(int buildingId, std::vector<SheetInfo>& sheets, std::wstring& errorMsg = std::wstring());
    static bool searchSheetsByText(const std::wstring& text, std::vector<SheetInfo>& sheets, std::wstring& errorMsg = std::wstring());

    // CAD文本索引管理 (CadTextIndex)
    static bool getTextIndexes(std::vector<CadTextIndex>& indexes,
                              int page = 1, int pageSize = 999999,
                              const std::wstring& search = L"",
                              const std::wstring& layerName = L"",
                              int sheet = 0,
                              const std::wstring& ordering = L"-create_datetime",
                              std::wstring& errorMsg = std::wstring());
    
    static bool getTextIndex(int id, CadTextIndex& index, std::wstring& errorMsg = std::wstring());
    static bool createTextIndex(const CadTextIndex& index, CadTextIndex& result, std::wstring& errorMsg = std::wstring());
    static bool updateTextIndex(int id, const CadTextIndex& index, CadTextIndex& result, std::wstring& errorMsg = std::wstring());
    static bool deleteTextIndex(int id, std::wstring& errorMsg = std::wstring());
    static bool searchTextContent(const std::wstring& text, std::vector<CadTextIndex>& indexes, std::wstring& errorMsg = std::wstring());
    static bool getTextIndexesByFile(const std::wstring& filePath, std::vector<CadTextIndex>& indexes, std::wstring& errorMsg = std::wstring());

    // 批量操作
    static bool batchInsertTextIndexes(const std::vector<TextSearchResult>& textList, std::wstring& errorMsg = std::wstring());

private:
    static std::wstring m_baseUrl;
    static std::wstring m_authToken;
    static HINTERNET m_hSession;
    static HINTERNET m_hConnect;

    // HTTP请求辅助方法
    static bool makeHttpRequest(const std::wstring& method, const std::wstring& endpoint, 
                               const nlohmann::json& data, ApiResponse& response, std::wstring& errorMsg);
    static bool makeHttpRequest(const std::wstring& method, const std::wstring& endpoint, 
                               ApiResponse& response, std::wstring& errorMsg);
    
    // 数据转换辅助方法
    static nlohmann::json buildingToJson(const BuildingInfo& building);
    static nlohmann::json sheetToJson(const SheetInfo& sheet);
    static nlohmann::json textIndexToJson(const CadTextIndex& index);
    
    static BuildingInfo jsonToBuilding(const nlohmann::json& json);
    static SheetInfo jsonToSheet(const nlohmann::json& json);
    static CadTextIndex jsonToTextIndex(const nlohmann::json& json);
    
    // 字符串转换辅助方法
    static std::wstring stringToWstring(const std::string& str);
    static std::string wstringToString(const std::wstring& wstr);
    static std::string gbkToUtf8(const std::string& strGBK);  // 新增
    static std::wstring urlEncode(const std::wstring& str);
    
    // 错误处理
    static std::wstring getLastError();
    static bool isSuccessCode(int code);

    static std::wstring utf8ToWstring(const std::string& str);
    
    static std::string wstringToUtf8(const std::wstring& wstr);
    // 测试方法
    static void testChineseConversion();
};

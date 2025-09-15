#include "StdAfx.h"
#include "NetWorkSqlDb.h"
#include "../CadLogger.h"
#include "../../services/SearchTextInDwg.h"
#include <sstream>
#include <iomanip>

// 静态成员变量定义
std::wstring NetWorkSqlDb::m_baseUrl = L"http://192.168.1.77:8000";
std::wstring NetWorkSqlDb::m_authToken = L"";
HINTERNET NetWorkSqlDb::m_hSession = NULL;
HINTERNET NetWorkSqlDb::m_hConnect = NULL;

NetWorkSqlDb::NetWorkSqlDb()
{
}

NetWorkSqlDb::~NetWorkSqlDb()
{
    if (m_hConnect) {
      WinHttpCloseHandle(m_hConnect);
        m_hConnect = NULL;
    }
    if (m_hSession) {
      WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
    }
}

// 初始化网络连接
bool NetWorkSqlDb::initialize(const std::wstring& baseUrl, const std::wstring& token)
{
    try {
        m_baseUrl = baseUrl;
        m_authToken = token;
        
        // 初始化WinHTTP会话
        m_hSession = WinHttpOpen(L"ArxProject2/1.0", 
                                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                WINHTTP_NO_PROXY_NAME, 
                                WINHTTP_NO_PROXY_BYPASS, 0);
        
        if (!m_hSession) {
            (_T("初始化WinHTTP会话失败"));
            return false;
        }
        
        // 解析URL获取主机名和端口
        URL_COMPONENTS urlComp = {0};
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.dwSchemeLength = -1;
        urlComp.dwHostNameLength = -1;
        urlComp.dwUrlPathLength = -1;
        urlComp.dwExtraInfoLength = -1;
        
        if (!WinHttpCrackUrl(baseUrl.c_str(), 0, 0, &urlComp)) {
            CadLogger::LogError(_T("解析URL失败"));
            return false;
        }
        
        std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
        INTERNET_PORT port = urlComp.nPort;
        
        // 建立连接
        m_hConnect = WinHttpConnect(m_hSession, hostName.c_str(), port, 0);
        if (!m_hConnect) {
            CadLogger::LogError(_T("建立HTTP连接失败"));
            return false;
        }
        
        CadLogger::LogInfo(_T("网络数据库初始化成功: %s"), baseUrl.c_str());
        return true;
        
    } catch (...) {
        CadLogger::LogError(_T("网络数据库初始化时发生异常"));
        return false;
    }
}

void NetWorkSqlDb::setAuthToken(const std::wstring& token)
{
    m_authToken = token;
}

void NetWorkSqlDb::setBaseUrl(const std::wstring& baseUrl)
{
    m_baseUrl = baseUrl;
}

// 建筑信息管理方法
bool NetWorkSqlDb::getBuildings(std::vector<BuildingInfo>& buildings, 
                               int page, int pageSize,
                               const std::wstring& search,
                               const std::wstring& designUnit,
                               const std::wstring& creator,
                               const std::wstring& ordering,
                               std::wstring& errorMsg)
{
    try {
        // 构建查询参数
        std::wstringstream params;
        params << L"page=" << page << L"&limit=" << pageSize;
        
        if (!search.empty()) {
            params << L"&search=" << urlEncode(search);
        }
        if (!designUnit.empty()) {
            params << L"&design_unit=" << urlEncode(designUnit);
        }
        if (!creator.empty()) {
            params << L"&creator=" << urlEncode(creator);
        }
        if (!ordering.empty()) {
            params << L"&ordering=" << urlEncode(ordering);
        }
        
        std::wstring endpoint = L"/api/drawing/buildings/?" + params.str();
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        // 解析分页响应
        if (response.data.is_array()) {
            for (const auto& item : response.data) {
                buildings.push_back(jsonToBuilding(item));
            }
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"获取建筑信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::getBuilding(int id, BuildingInfo& building, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/buildings/" + std::to_wstring(id) + L"/";
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        building = jsonToBuilding(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"获取单个建筑信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::createBuilding(const BuildingInfo& building, BuildingInfo& result, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/buildings/";
        nlohmann::json data = buildingToJson(building);

        ApiResponse response;
        if (!makeHttpRequest(L"POST", endpoint, data, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        result = jsonToBuilding(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"创建建筑信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::updateBuilding(int id, const BuildingInfo& building, BuildingInfo& result, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/buildings/" + std::to_wstring(id) + L"/";
        nlohmann::json data = buildingToJson(building);
        
        ApiResponse response;
        if (!makeHttpRequest(L"PUT", endpoint, data, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        result = jsonToBuilding(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"更新建筑信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::deleteBuilding(int id, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/buildings/" + std::to_wstring(id) + L"/";
        
        ApiResponse response;
        if (!makeHttpRequest(L"DELETE", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"删除建筑信息时发生异常";
        return false;
    }
}

// 图纸信息管理方法
bool NetWorkSqlDb::getSheets(std::vector<SheetInfo>& sheets,
                            int page, int pageSize,
                            const std::wstring& search,
                            const std::wstring& specialty,
                            const std::wstring& format,
                            const std::wstring& status,
                            const std::wstring& designUnit,
                            const std::wstring& creator,
                            int building,
                            const std::wstring& ordering,
                            std::wstring& errorMsg)
{
    try {
        // 构建查询参数
        std::wstringstream params;
        params << L"page=" << page << L"&limit=" << pageSize;
        
        if (!search.empty()) {
            params << L"&search=" << urlEncode(search);
        }
        if (!specialty.empty()) {
            params << L"&specialty=" << urlEncode(specialty);
        }
        if (!format.empty()) {
            params << L"&format=" << urlEncode(format);
        }
        if (!status.empty()) {
            params << L"&status=" << urlEncode(status);
        }
        if (!designUnit.empty()) {
            params << L"&design_unit=" << urlEncode(designUnit);
        }
        if (!creator.empty()) {
            params << L"&creator=" << urlEncode(creator);
        }
        if (building > 0) {
            params << L"&building=" << building;
        }
        if (!ordering.empty()) {
            params << L"&ordering=" << urlEncode(ordering);
        }
        
        std::wstring endpoint = L"/api/drawing/sheets/?" + params.str();
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        // 解析分页响应
        if (response.data.is_array()) {
            for (const auto& item : response.data) {
                sheets.push_back(jsonToSheet(item));
            }
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"获取图纸信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::getSheet(int id, SheetInfo& sheet, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/sheets/" + std::to_wstring(id) + L"/";
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        sheet = jsonToSheet(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"获取单个图纸信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::createSheet(const SheetInfo& sheet, SheetInfo& result, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/sheets/";
        nlohmann::json data = sheetToJson(sheet);
        
        ApiResponse response;
        if (!makeHttpRequest(L"POST", endpoint, data, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
              return false;
        }
        
        result = jsonToSheet(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"创建图纸信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::updateSheet(int id, const SheetInfo& sheet, SheetInfo& result, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/sheets/" + std::to_wstring(id) + L"/";
        nlohmann::json data = sheetToJson(sheet);
        
        ApiResponse response;
        if (!makeHttpRequest(L"PUT", endpoint, data, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        result = jsonToSheet(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"更新图纸信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::deleteSheet(int id, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/sheets/" + std::to_wstring(id) + L"/";
        
        ApiResponse response;
        if (!makeHttpRequest(L"DELETE", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"删除图纸信息时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::getSheetsByBuilding(int buildingId, std::vector<SheetInfo>& sheets, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/sheets/by_building/?building_id=" + std::to_wstring(buildingId);
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        // 解析数组响应
        if (response.data.is_array()) {
            for (const auto& item : response.data) {
                sheets.push_back(jsonToSheet(item));
            }
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"根据建筑获取图纸列表时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::searchSheetsByText(const std::wstring& text, std::vector<SheetInfo>& sheets, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/sheets/search_by_text/?text=" + urlEncode(text);
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        // 解析数组响应
        if (response.data.is_array()) {
            for (const auto& item : response.data) {
                sheets.push_back(jsonToSheet(item));
            }
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"根据文本搜索图纸时发生异常";
        return false;
    }
}

// CAD文本索引管理方法
bool NetWorkSqlDb::getTextIndexes(std::vector<CadTextIndex>& indexes,
                                 int page, int pageSize,
                                 const std::wstring& search,
                                 const std::wstring& layerName,
                                 int sheet,
                                 const std::wstring& ordering,
                                 std::wstring& errorMsg)
{
    try {
        // 构建查询参数
        std::wstringstream params;
        params << L"page=" << page << L"&limit=" << pageSize;
        
        if (!search.empty()) {
            params << L"&search=" << urlEncode(search);
        }
        if (!layerName.empty()) {
            params << L"&layer_name=" << urlEncode(layerName);
        }
        if (sheet > 0) {
            params << L"&sheet=" << sheet;
        }
        if (!ordering.empty()) {
            params << L"&ordering=" << urlEncode(ordering);
        }
        
        std::wstring endpoint = L"/api/drawing/text-index/?" + params.str();
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        // 解析分页响应
        if (response.data.is_array()) {
            for (const auto& item : response.data) {
                indexes.push_back(jsonToTextIndex(item));
            }
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"获取CAD文本索引时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::getTextIndex(int id, CadTextIndex& index, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/text-index/" + std::to_wstring(id) + L"/";
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        index = jsonToTextIndex(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"获取单个CAD文本索引时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::createTextIndex(const CadTextIndex& index, CadTextIndex& result, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/text-index/";
        nlohmann::json data = textIndexToJson(index);
        
        ApiResponse response;
        if (!makeHttpRequest(L"POST", endpoint, data, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        result = jsonToTextIndex(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"创建CAD文本索引时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::updateTextIndex(int id, const CadTextIndex& index, CadTextIndex& result, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/text-index/" + std::to_wstring(id) + L"/";
        nlohmann::json data = textIndexToJson(index);
        
        ApiResponse response;
        if (!makeHttpRequest(L"PUT", endpoint, data, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        result = jsonToTextIndex(response.data);
        return true;
        
    } catch (...) {
        errorMsg = L"更新CAD文本索引时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::deleteTextIndex(int id, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/text-index/" + std::to_wstring(id) + L"/";
        
        ApiResponse response;
        if (!makeHttpRequest(L"DELETE", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"删除CAD文本索引时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::searchTextContent(const std::wstring& text, std::vector<CadTextIndex>& indexes, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/text-index/search_text/?text=" + urlEncode(text);
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        // 解析数组响应
        if (response.data.is_array()) {
            for (const auto& item : response.data) {
                indexes.push_back(jsonToTextIndex(item));
            }
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"搜索文本内容时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::getTextIndexesByFile(const std::wstring& filePath, std::vector<CadTextIndex>& indexes, std::wstring& errorMsg)
{
    try {
        std::wstring endpoint = L"/api/drawing/text-index/by_file/?file_path=" + urlEncode(filePath);
        
        ApiResponse response;
        if (!makeHttpRequest(L"GET", endpoint, response, errorMsg)) {
            return false;
        }
        
        if (!isSuccessCode(response.code)) {
            errorMsg = response.msg;
            return false;
        }
        
        // 解析数组响应
        if (response.data.is_array()) {
            for (const auto& item : response.data) {
                indexes.push_back(jsonToTextIndex(item));
            }
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"根据文件路径获取文本索引时发生异常";
        return false;
    }
}

// 批量操作
bool NetWorkSqlDb::batchInsertTextIndexes(const std::vector<TextSearchResult>& textList, std::wstring& errorMsg)
{
    try {
        if (textList.empty()) {
            return true;
        }
        
        acutPrintf(_T("开始批量插入 %d 条文本索引记录...\n"), (int)textList.size());
        
        // 将TextSearchResult转换为CadTextIndex
        std::vector<CadTextIndex> indexes;
        for (const auto& result : textList) {
            CadTextIndex index;
            index.file_path = result.filePath;
            index.text_content = result.textContent;
            index.layer_name = result.layerName;
            index.pos_x = result.posX;
            index.pos_y = result.posY;
            index.pos_z = result.posZ;
            index.entity_handle = result.entityHandle;
            index.last_modified = L"";
            index.sheet = 0; // 需要根据实际情况设置
            indexes.push_back(index);
        }
        
        // 分批处理，每批100条
        const size_t batchSize = 100;
        for (size_t start = 0; start < indexes.size(); start += batchSize) {
            size_t end = (start + batchSize < indexes.size()) ? (start + batchSize) : indexes.size();
            
            // 构建批量插入的JSON数据
            nlohmann::json batchData = nlohmann::json::array();
            for (size_t i = start; i < end; ++i) {
                batchData.push_back(textIndexToJson(indexes[i]));
            }
            
            std::wstring endpoint = L"/api/drawing/text-index/";
            ApiResponse response;
            if (!makeHttpRequest(L"POST", endpoint, batchData, response, errorMsg)) {
                return false;
            }
            
            if (!isSuccessCode(response.code)) {
                errorMsg = response.msg;
                return false;
            }
            
            acutPrintf(_T("已处理 %d/%d 条记录\n"), (int)end, (int)indexes.size());
        }
        
        acutPrintf(_T("批量插入完成！\n"));
        return true;
        
    } catch (...) {
        errorMsg = L"批量插入文本索引时发生异常";
        return false;
    }
}

// HTTP请求辅助方法
bool NetWorkSqlDb::makeHttpRequest(const std::wstring& method, const std::wstring& endpoint, 
                                  const nlohmann::json& data, ApiResponse& response, std::wstring& errorMsg)
{
    try {
        if (!m_hConnect) {
            errorMsg = L"HTTP连接未初始化";
            return false;
        }
        
        // 打印完整的请求地址
        std::wstring fullUrl = m_baseUrl + endpoint;
        acutPrintf(_T("发送HTTP请求: %s %s\n"), method.c_str(), fullUrl.c_str());
        
        // 创建请求
        HINTERNET hRequest = WinHttpOpenRequest(m_hConnect, method.c_str(), endpoint.c_str(),
                                               NULL, WINHTTP_NO_REFERER, 
                                               WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        
        if (!hRequest) {
            errorMsg = L"创建HTTP请求失败";
            return false;
        }
        
        // 设置请求头
        std::wstring headers = L"Content-Type: application/json; charset=utf-8\r\n";
        if (!m_authToken.empty()) {
            headers += L"Authorization: " + m_authToken + L"\r\n";
        }
        
        WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
        
        // 准备请求数据 - 添加异常处理
        std::string jsonStr;
        try {
            jsonStr = data.dump();
        } catch (const nlohmann::json::type_error& e) {
            errorMsg = L"JSON序列化失败: " + stringToWstring(e.what());
            WinHttpCloseHandle(hRequest);
            return false;
        } catch (const std::exception& e) {
            errorMsg = L"JSON序列化异常: " + stringToWstring(e.what());
            WinHttpCloseHandle(hRequest);
            return false;
        } catch (...) {
            errorMsg = L"JSON序列化发生未知异常";
            WinHttpCloseHandle(hRequest);
            return false;
        }
        
        // 添加调试信息
        acutPrintf(_T("发送的JSON数据: %s\n"), stringToWstring(jsonStr).c_str());
        
        // 发送请求 - 直接使用UTF-8字符串
        DWORD dataSize = (DWORD)jsonStr.length();
        BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                        (LPVOID)jsonStr.c_str(), dataSize, dataSize, 0);
        
        if (!result) {
            errorMsg = L"发送HTTP请求失败";
            WinHttpCloseHandle(hRequest);
            return false;
        }
        
        // 接收响应
        result = WinHttpReceiveResponse(hRequest, NULL);
        if (!result) {
            errorMsg = L"接收HTTP响应失败";
            WinHttpCloseHandle(hRequest);
            return false;
        }
        
        // 读取响应数据
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        std::string responseData;
        
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                break;
            }
            
            if (dwSize == 0) break;
            
            char* buffer = new char[dwSize + 1];
            if (!WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded)) {
                delete[] buffer;
                break;
            }
            
            buffer[dwDownloaded] = '\0';
            responseData += buffer;
            delete[] buffer;
            
        } while (dwSize > 0);
        
        // 解析响应
        try {
            nlohmann::json jsonResponse = nlohmann::json::parse(responseData);
            response.code = jsonResponse.value("code", 0);
            response.msg = stringToWstring(jsonResponse.value("msg", ""));
            response.data = jsonResponse.value("data", nlohmann::json());
            response.success = isSuccessCode(response.code);
        } catch (...) {
            errorMsg = L"解析响应JSON失败";
            WinHttpCloseHandle(hRequest);
            return false;
        }
        
        WinHttpCloseHandle(hRequest);
        return true;
        
    } catch (...) {
        errorMsg = L"HTTP请求时发生异常";
        return false;
    }
}

bool NetWorkSqlDb::makeHttpRequest(const std::wstring& method, const std::wstring& endpoint, 
                                  ApiResponse& response, std::wstring& errorMsg)
{
    return makeHttpRequest(method, endpoint, nlohmann::json(), response, errorMsg);
}

// 数据转换辅助方法
nlohmann::json NetWorkSqlDb::buildingToJson(const BuildingInfo& building)
{
    nlohmann::json json;
    
    try {
       
        // 必填字段 - 建筑名称
        if (!building.building_name.empty()) {
            json["building_name"] = wstringToUtf8(building.building_name);
        } else {
            json["building_name"] = ""; // 确保字段存在
        }
        
        // 可选字段 - 只有在非空时才添加
        if (!building.address.empty()) {
            json["address"] = wstringToUtf8(building.address);
        }
        
        if (!building.total_area.empty()) {
            json["total_area"] = wstringToUtf8(building.total_area);
        }
        
        if (!building.floors.empty()) {
            json["floors"] = wstringToUtf8(building.floors);
        }
        
        if (!building.design_unit.empty()) {
            json["design_unit"] = wstringToUtf8(building.design_unit);
        }
        
        if (!building.create_time.empty()) {
            json["create_time"] = wstringToUtf8(building.create_time);
        }
        
        if (!building.creator.empty()) {
            json["creator"] = wstringToUtf8(building.creator);
        }
        
        // 注意：id、create_datetime、update_datetime 是服务器生成的字段
        // 在创建时不应该包含在请求中，只在更新时可能需要
        
        // 调试输出：输出到VS输出窗口
        try {
            std::string jsonStr = json.dump(4); // 格式化输出，缩进4个空格
            std::wstring wJsonStr = utf8ToWstring(jsonStr); // 使用 utf8ToWstring 转换
            
            // 输出到VS输出窗口
            OutputDebugString(L"=== buildingToJson 生成的JSON数据 ===\n");
            OutputDebugString(wJsonStr.c_str());
            OutputDebugString(L"\n");
            
            // 输出原始字段值用于对比
            OutputDebugString(L"=== 原始字段值 ===\n");
            std::wstring debugMsg = L"building_name: " + building.building_name + L"\n";
            OutputDebugString(debugMsg.c_str());
            
            debugMsg = L"address: " + building.address + L"\n";
            OutputDebugString(debugMsg.c_str());
            
            debugMsg = L"total_area: " + building.total_area + L"\n";
            OutputDebugString(debugMsg.c_str());
            
            debugMsg = L"floors: " + building.floors + L"\n";
            OutputDebugString(debugMsg.c_str());
            
            debugMsg = L"design_unit: " + building.design_unit + L"\n";
            OutputDebugString(debugMsg.c_str());
            
            debugMsg = L"create_time: " + building.create_time + L"\n";
            OutputDebugString(debugMsg.c_str());
            
            debugMsg = L"creator: " + building.creator + L"\n";
            OutputDebugString(debugMsg.c_str());
            
            OutputDebugString(L"=== 调试信息结束 ===\n");
            
        } catch (const std::exception& e) {
            OutputDebugStringA("输出JSON数据时发生异常: ");
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
        } catch (...) {
            OutputDebugString(L"输出JSON数据时发生未知异常\n");
        }
        
    } catch (const std::exception& e) {
        OutputDebugStringA("buildingToJson 发生异常: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        // 返回一个空的JSON对象
        return nlohmann::json();
    } catch (...) {
        OutputDebugString(L"buildingToJson 发生未知异常\n");
        return nlohmann::json();
    }
    
    return json;
}

nlohmann::json NetWorkSqlDb::sheetToJson(const SheetInfo& sheet)
{
    nlohmann::json json;
    json["name"] = wstringToString(sheet.name);
    json["building"] = sheet.building;
    json["building_name"] = wstringToString(sheet.building_name);
    json["specialty"] = wstringToString(sheet.specialty);
    json["format"] = wstringToString(sheet.format);
    json["status"] = wstringToString(sheet.status);
    json["version"] = wstringToString(sheet.version);
    json["design_unit"] = wstringToString(sheet.design_unit);
    json["create_time"] = wstringToString(sheet.create_time);
    json["creator"] = wstringToString(sheet.creator);
    json["is_selected"] = sheet.is_selected;
    json["file_path"] = wstringToString(sheet.file_path);
    json["file_size"] = sheet.file_size;
    json["md5_hash"] = wstringToString(sheet.md5_hash);
    return json;
}

nlohmann::json NetWorkSqlDb::textIndexToJson(const CadTextIndex& index)
{
    nlohmann::json json;
    json["file_path"] = wstringToString(index.file_path);
    json["text_content"] = wstringToString(index.text_content);
    json["layer_name"] = wstringToString(index.layer_name);
    json["pos_x"] = index.pos_x;
    json["pos_y"] = index.pos_y;
    json["pos_z"] = index.pos_z;
    json["entity_handle"] = wstringToString(index.entity_handle);
    json["last_modified"] = wstringToString(index.last_modified);
    json["sheet"] = index.sheet;
    return json;
}

BuildingInfo NetWorkSqlDb::jsonToBuilding(const nlohmann::json& json)
{
    BuildingInfo building;
    building.id = json.value("id", 0);
    building.building_name = stringToWstring(json.value("building_name", ""));
    building.address = stringToWstring(json.value("address", ""));
    building.total_area = stringToWstring(json.value("total_area", ""));
    building.floors = stringToWstring(json.value("floors", ""));
    building.design_unit = stringToWstring(json.value("design_unit", ""));
    building.create_time = stringToWstring(json.value("create_time", ""));
    building.creator = stringToWstring(json.value("creator", ""));
    building.create_datetime = stringToWstring(json.value("create_datetime", ""));
    building.update_datetime = stringToWstring(json.value("update_datetime", ""));
    return building;
}

SheetInfo NetWorkSqlDb::jsonToSheet(const nlohmann::json& json)
{
    SheetInfo sheet;
    sheet.id = json.value("id", 0);
    sheet.name = stringToWstring(json.value("name", ""));
    sheet.building = json.value("building", 0);
    sheet.building_name = stringToWstring(json.value("building_name", ""));
    sheet.building_name_display = stringToWstring(json.value("building_name_display", ""));
    sheet.specialty = stringToWstring(json.value("specialty", ""));
    sheet.format = stringToWstring(json.value("format", ""));
    sheet.status = stringToWstring(json.value("status", ""));
    sheet.version = stringToWstring(json.value("version", ""));
    sheet.design_unit = stringToWstring(json.value("design_unit", ""));
    sheet.create_time = stringToWstring(json.value("create_time", ""));
    sheet.creator = stringToWstring(json.value("creator", ""));
    sheet.is_selected = json.value("is_selected", false);
    sheet.file_path = stringToWstring(json.value("file_path", ""));
    sheet.file_size = json.value("file_size", 0);
    sheet.md5_hash = stringToWstring(json.value("md5_hash", ""));
    sheet.create_datetime = stringToWstring(json.value("create_datetime", ""));
    sheet.update_datetime = stringToWstring(json.value("update_datetime", ""));
    return sheet;
}

CadTextIndex NetWorkSqlDb::jsonToTextIndex(const nlohmann::json& json)
{
    CadTextIndex index;
    index.id = json.value("id", 0);
    index.file_path = stringToWstring(json.value("file_path", ""));
    index.text_content = stringToWstring(json.value("text_content", ""));
    index.layer_name = stringToWstring(json.value("layer_name", ""));
    index.pos_x = json.value("pos_x", 0.0);
    index.pos_y = json.value("pos_y", 0.0);
    index.pos_z = json.value("pos_z", 0.0);
    index.entity_handle = stringToWstring(json.value("entity_handle", ""));
    index.last_modified = stringToWstring(json.value("last_modified", ""));
    index.sheet = json.value("sheet", 0);
    index.sheet_name = stringToWstring(json.value("sheet_name", ""));
    index.create_datetime = stringToWstring(json.value("create_datetime", ""));
    index.update_datetime = stringToWstring(json.value("update_datetime", ""));
    return index;
}

// 字符串转换辅助方法
std::wstring NetWorkSqlDb::stringToWstring(const std::string& str)
{
    if (str.empty()) return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size <= 0) return L"";
    
    std::wstring wstr(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

std::string NetWorkSqlDb::wstringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (size <= 0) return "";
    
    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, NULL, NULL);
    return str;
}

std::wstring NetWorkSqlDb::urlEncode(const std::wstring& str)
{
    std::wstring result;
    for (wchar_t c : str) {
        if (isalnum(c) || c == L'-' || c == L'_' || c == L'.' || c == L'~') {
            result += c;
        } else {
            std::wstringstream ss;
            ss << L"%" << std::uppercase << std::hex << (int)c;
            result += ss.str();
        }
    }
    return result;
}

// 错误处理
std::wstring NetWorkSqlDb::getLastError()
{
    DWORD error = GetLastError();
    std::wstringstream ss;
    ss << L"错误代码: " << error;
    return ss.str();
}

bool NetWorkSqlDb::isSuccessCode(int code)
{
    return code == 2000; // 根据API文档，2000表示成功
}

// 新增：专门用于从UTF-8转换的方法
std::wstring NetWorkSqlDb::utf8ToWstring(const std::string& str)
{
    if (str.empty()) return L"";
    
    try {
        // 输出原始UTF-8字符串的调试信息
        OutputDebugStringA("utf8ToWstring 输入: ");
        OutputDebugStringA(str.c_str());
        OutputDebugStringA("\n");
        
        // 使用UTF-8编码转换
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
        if (size <= 0) {
            OutputDebugStringA("utf8ToWstring: MultiByteToWideChar 失败\n");
            return L"";
        }
        
        std::wstring wstr(size - 1, 0);
        int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
        if (result == 0) {
            OutputDebugStringA("utf8ToWstring: MultiByteToWideChar 转换失败\n");
            return L"";
        }
        
        // 输出转换结果
        OutputDebugString(L"utf8ToWstring 输出: ");
        OutputDebugString(wstr.c_str());
        OutputDebugString(L"\n");
        
        return wstr;
    } catch (const std::exception& e) {
        OutputDebugStringA("utf8ToWstring 异常: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        return L"";
    } catch (...) {
        OutputDebugString(L"utf8ToWstring 未知异常\n");
        return L"";
    }
}

std::string NetWorkSqlDb::wstringToUtf8(const std::wstring& wstr)
{
	if (wstr.empty()) return "";

	int size = WideCharToMultiByte(
		CP_UTF8,            // ✅ 关键：用 UTF-8
		0,
		wstr.c_str(),
		-1,
		NULL,
		0,
		NULL,
		NULL
	);

	if (size <= 0) {
		OutputDebugStringA("wstringToUtf8: WideCharToMultiByte 获取长度失败\n");
		return "";
	}

	std::string str(size - 1, 0);
	int result = WideCharToMultiByte(
		CP_UTF8,            // ✅
		0,
		wstr.c_str(),
		-1,
		&str[0],
		size,
		NULL,
		NULL
	);

	if (result == 0) {
		OutputDebugStringA("wstringToUtf8: WideCharToMultiByte 转换失败\n");
		return "";
	}

	OutputDebugStringA("wstringToUtf8 输出 (UTF-8): ");
	OutputDebugStringA(str.c_str());
	OutputDebugStringA("\n");

	return str;
}


// 测试方法：验证中文字符串转换
void NetWorkSqlDb::testChineseConversion()
{
    // 测试1：直接使用字符串字面量
    std::wstring test1 = L"测试大楼2";
    OutputDebugString(L"测试1 - 直接字面量: ");
    OutputDebugString(test1.c_str());
    OutputDebugString(L"\n");
    
    // 测试2：使用UTF-8转换
    std::string utf8Result = wstringToUtf8(test1);
    std::wstring backToWide = utf8ToWstring(utf8Result);
    OutputDebugString(L"测试2 - UTF-8往返: ");
    OutputDebugString(backToWide.c_str());
    OutputDebugString(L"\n");
    
    // 测试3：使用系统默认编码
    std::string ansiResult = wstringToString(test1);
    std::wstring backToWideAnsi = stringToWstring(ansiResult);
    OutputDebugString(L"测试3 - ANSI往返: ");
    OutputDebugString(backToWideAnsi.c_str());
    OutputDebugString(L"\n");
}

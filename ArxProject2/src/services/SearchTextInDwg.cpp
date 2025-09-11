#include "StdAfx.h"
#include "../common/Database/SqlDB.h"
#include "../common/Document/DwgDatabaseUtil.h"
#include "../common/Entity/TextUtil.h"
#include "../common/CadLogger.h"
#include "SearchTextInDwg.h"
#include "../common/SheetManager/SheetFileManager.h"
#include <shlwapi.h>
#include <regex>
#include <vector>
#include <algorithm>
#include <filesystem>


// 静态成员初始化
SheetFileManager* SearchTextInDwg::s_fileManager = nullptr;

void SearchTextInDwg::setFileManager(SheetFileManager* fileManager)
{
    s_fileManager = fileManager;
}

SearchTextInDwg::SearchTextInDwg()
{
}

SearchTextInDwg::~SearchTextInDwg()
{
}

// 创建文本索引表
bool SearchTextInDwg::createTextIndexTable()
{
    std::wstring createTableSql = LR"(
        CREATE TABLE IF NOT EXISTS cad_text_index (
            id SERIAL PRIMARY KEY,
            file_path TEXT NOT NULL,
            text_content TEXT NOT NULL,
            layer_name TEXT NOT NULL,
            pos_x DOUBLE PRECISION,
            pos_y DOUBLE PRECISION,
            pos_z DOUBLE PRECISION,
            entity_handle TEXT,
            last_modified TIMESTAMP,
            CONSTRAINT uq_text UNIQUE(file_path, entity_handle, text_content)
        )
    )";
    
    std::wstring errorMsg;
    if (!SqlDB::executeQuery(createTableSql, errorMsg)) {
        acutPrintf(_T("\nERROR: Failed to create text index table\n"));
        acutPrintf(_T("Error details: %s\n"), errorMsg.c_str());
        // 临时注释掉 CadLogger
        // acutPrintf(_T("Failed to create text index table: %s"), errorMsg.c_str());
        return false;
    }
    
    // 创建文本内容索引 - 使用普通B-tree索引而不是GIN全文索引
    std::wstring createIndexSql = LR"(
        CREATE INDEX IF NOT EXISTS idx_text_content 
        ON cad_text_index (text_content)
    )";
    
    if (!SqlDB::executeQuery(createIndexSql, errorMsg)) {
        acutPrintf(_T("创建文本内容索引失败: %s"), errorMsg.c_str());
        return false;
    }
    
    // 创建文件路径索引
    std::wstring createFileIndexSql = LR"(
        CREATE INDEX IF NOT EXISTS idx_file_path 
        ON cad_text_index (file_path)
    )";
    
    if (!SqlDB::executeQuery(createFileIndexSql, errorMsg)) {
        acutPrintf(_T("创建文件路径索引失败: %s"), errorMsg.c_str());
        return false;
    }

    acutPrintf(_T("\nText index table created successfully\n"));
    //acutPrintf(_T("文本索引表创建成功"));
    return true;
}

// 为指定图纸建立文本索引
bool SearchTextInDwg::buildTextIndexForDrawing(const std::wstring& filePath, std::wstring& errorMsg)
{
    if (!PathFileExists(filePath.c_str())) {
        errorMsg = L"文件不存在: " + filePath;
        return false;
    }
    
    // 获取服务器文件名
    std::wstring serverFileName = getServerFileNameFromLocalPath(filePath);
    
    acutPrintf(_T("处理文件: 本地路径=%s, 服务器文件名=%s"), 
               filePath.c_str(), serverFileName.c_str());
    
    // 检查是否需要重新索引
    //SYSTEMTIME currentModified = getFileLastModified(filePath);
    //if (!needsReindexing(serverFileName, currentModified)) {
    //   acutPrintf(_T("文件未修改，跳过索引: %s"), serverFileName.c_str());
    //    return true;
    //}
    
    // 先删除该文件的旧索引（使用服务器文件名）
    std::wstring deleteSql = L"DELETE FROM cad_text_index WHERE file_path = '" + serverFileName + L"'";
    if (!SqlDB::executeQuery(deleteSql, errorMsg)) {
        errorMsg = L"删除旧索引失败: " + errorMsg;
        return false;
    }
    
    // 提取文本信息，直接传入正确的文件标识
    std::vector<TextSearchResult> textList;
    if (!extractTextFromDwg(filePath, serverFileName, textList, errorMsg)) {
        return false;
    }
    
    // 保存到数据库
    if (!saveTextIndexToDatabase(textList, errorMsg)) {
        return false;
    }
    
    acutPrintf(_T("成功为服务器文件 %s 建立了 %d 条文本索引"), 
               serverFileName.c_str(), (int)textList.size());
    return true;
}

// 从DWG文件中提取所有文本信息
bool SearchTextInDwg::extractTextFromDwg(const std::wstring& localFilePath, 
                                         const std::wstring& fileIdentifier,
                                         std::vector<TextSearchResult>& textList, 
                                         std::wstring& errorMsg)
{
    textList.clear();
    
    try {
        // 创建数据库对象来读取DWG文件
        AcDbDatabase* pDb = new AcDbDatabase(false, true);
        
        const TCHAR* tFilePath = localFilePath.c_str();  // 用于读取文件
        
        // 读取DWG文件
        Acad::ErrorStatus es = pDb->readDwgFile(tFilePath);
        if (es != Acad::eOk) {
            delete pDb;
            errorMsg = L"无法读取DWG文件: " + localFilePath + L", 错误代码: " + std::to_wstring((int)es);
            return false;
        }
        
        // 获取文件修改时间（使用本地文件路径）
        SYSTEMTIME lastModified = getFileLastModified(localFilePath);
        
        // 获取块表
        AcDbBlockTable* pBlockTable = nullptr;
        es = pDb->getBlockTable(pBlockTable, AcDb::kForRead);
        if (es != Acad::eOk) {
            delete pDb;
            errorMsg = L"无法获取块表: " + std::to_wstring((int)es);
            return false;
        }
        
        // 获取模型空间块表记录
        AcDbBlockTableRecord* pModelSpace = nullptr;
        es = pBlockTable->getAt(ACDB_MODEL_SPACE, pModelSpace, AcDb::kForRead);
        pBlockTable->close();
        
        if (es != Acad::eOk) {
            delete pDb;
            errorMsg = L"无法获取模型空间: " + std::to_wstring((int)es);
            return false;
        }
        
        // 遍历模型空间中的所有实体
        AcDbBlockTableRecordIterator* pIterator = nullptr;
        es = pModelSpace->newIterator(pIterator);
        if (es != Acad::eOk) {
            pModelSpace->close();
            delete pDb;
            errorMsg = L"无法创建迭代器: " + std::to_wstring((int)es);
            return false;
        }
        
        for (pIterator->start(); !pIterator->done(); pIterator->step()) {
            AcDbEntity* pEntity = nullptr;
            es = pIterator->getEntity(pEntity, AcDb::kForRead);
            if (es != Acad::eOk) {
                continue;
            }
            
            // 获取图层名
            AcDbObjectId layerId = pEntity->layerId();
            AcDbLayerTableRecord* pLayerRecord = nullptr;
            std::wstring layerName = L"0"; // 默认图层
            if (acdbOpenObject(pLayerRecord, layerId, AcDb::kForRead) == Acad::eOk) {
                ACHAR* pLayerName = nullptr;
                if (pLayerRecord->getName(pLayerName) == Acad::eOk) {
                    layerName = pLayerName;
                    acutDelString(pLayerName);
                }
                pLayerRecord->close();
            }
            
            // 获取实体句柄
            AcDbHandle handle;
            pEntity->getAcDbHandle(handle);
            ACHAR handleStr[17];
            handle.getIntoAsciiBuffer(handleStr);
            std::wstring entityHandle = handleStr;
            
            // 检查实体类型并提取文本
            if (pEntity->isKindOf(AcDbText::desc())) {
                // 单行文字
                AcDbText* pText = AcDbText::cast(pEntity);
                if (pText) {
                    TextSearchResult result;
                    result.filePath = fileIdentifier;  // 直接使用传入的文件标识
                    result.textContent = pText->textString();
                    result.layerName = layerName;
                    
                    AcGePoint3d position = pText->position();
                    result.posX = position.x;
                    result.posY = position.y;
                    result.posZ = position.z;
                    result.entityHandle = entityHandle;
                    result.lastModified = lastModified;
                    
                    if (!result.textContent.empty()) {
                        textList.push_back(result);
                    }
                }
            }
            else if (pEntity->isKindOf(AcDbMText::desc())) {
                // 多行文字
                AcDbMText* pMText = AcDbMText::cast(pEntity);
                if (pMText) {
                    TextSearchResult result;
                    result.filePath = fileIdentifier;
                    result.textContent = pMText->contents();
                    result.layerName = layerName;
                    
                    AcGePoint3d position = pMText->location();
                    result.posX = position.x;
                    result.posY = position.y;
                    result.posZ = position.z;
                    result.entityHandle = entityHandle;
                    result.lastModified = lastModified;
                    
                    if (!result.textContent.empty()) {
                        textList.push_back(result);
                    }
                }
            }
            else if (pEntity->isKindOf(AcDbAttribute::desc())) {
                // 属性文字
                AcDbAttribute* pAttr = AcDbAttribute::cast(pEntity);
                if (pAttr) {
                    TextSearchResult result;
                    result.filePath = fileIdentifier;
                    result.textContent = pAttr->textString();
                    result.layerName = layerName;
                    
                    AcGePoint3d position = pAttr->position();
                    result.posX = position.x;
                    result.posY = position.y;
                    result.posZ = position.z;
                    result.entityHandle = entityHandle;
                    result.lastModified = lastModified;
                    
                    if (!result.textContent.empty()) {
                        textList.push_back(result);
                    }
                }
            }
            else if (pEntity->isKindOf(AcDbBlockReference::desc())) {
                // 块参照中的属性文字
                AcDbBlockReference* pBlockRef = AcDbBlockReference::cast(pEntity);
                if (pBlockRef) {
                    AcDbObjectIterator* pAttrIter = pBlockRef->attributeIterator();
                    if (pAttrIter) {
                        for (pAttrIter->start(); !pAttrIter->done(); pAttrIter->step()) {
                            AcDbAttribute* pAttr = nullptr;
                            if (acdbOpenObject(pAttr, pAttrIter->objectId(), AcDb::kForRead) == Acad::eOk) {
                                TextSearchResult result;
                                result.filePath = fileIdentifier;
                                result.textContent = pAttr->textString();
                                result.layerName = layerName;
                                
                                AcGePoint3d position = pAttr->position();
                                result.posX = position.x;
                                result.posY = position.y;
                                result.posZ = position.z;
                                result.entityHandle = entityHandle;
                                result.lastModified = lastModified;
                                
                                if (!result.textContent.empty()) {
                                    textList.push_back(result);
                                }
                                pAttr->close();
                            }
                        }
                        delete pAttrIter;
                    }
                }
            }
            
            pEntity->close();
        }
        
        delete pIterator;
        pModelSpace->close();
        delete pDb;
        
        return true;
        
    } catch (...) {
        errorMsg = L"提取DWG文本时发生异常";
        return false;
    }
}

// 保存文本索引到数据库 - 优化版本
bool SearchTextInDwg::saveTextIndexToDatabase(const std::vector<TextSearchResult>& textList, std::wstring& errorMsg)
{
    if (textList.empty()) {
        return true;
    }
    
    acutPrintf(_T("开始批量保存 %d 条文本索引...\n"), (int)textList.size());
    
    try {
        // 使用批量插入方法替代原来的逐条插入
        if (!SqlDB::batchInsertTextIndex(textList, errorMsg)) {
            acutPrintf(_T("批量保存文本索引失败: %s\n"), errorMsg.c_str());
            return false;
        }
        
        acutPrintf(_T("成功批量保存 %d 条文本索引\n"), (int)textList.size());
        return true;
        
    } catch (...) {
        errorMsg = L"保存文本索引时发生异常";
        return false;
    }
}

// 搜索包含指定文本的图纸（简化版本，不带错误信息）
std::vector<TextSearchResult> SearchTextInDwg::searchTextInDrawings(const std::wstring& searchText, const std::wstring& buildingName)
{
    std::wstring errorMsg;
    return searchTextInDrawings(searchText, buildingName, errorMsg);
}

// 搜索包含指定文本的图纸（带错误信息返回）
std::vector<TextSearchResult> SearchTextInDwg::searchTextInDrawings(const std::wstring& searchText, const std::wstring& buildingName, std::wstring& errorMsg)
{
    std::vector<TextSearchResult> results;
    
    // 去除前后空格
    std::wstring trimmedText = searchText;
    trimmedText.erase(0, trimmedText.find_first_not_of(L" \t\n\r"));
    trimmedText.erase(trimmedText.find_last_not_of(L" \t\n\r") + 1);
    
    if (trimmedText.empty()) {
        errorMsg = L"搜索文本不能为空";
        return results;
    }
    
    try {
        // 构建搜索SQL
        std::wstring sql = LR"(
            SELECT file_path, text_content, layer_name, pos_x, pos_y, pos_z, entity_handle, last_modified
            FROM cad_text_index 
            WHERE text_content LIKE '%)" + trimmedText + L"%'";
        
        // 如果指定了大楼名称，添加文件路径过滤
        if (!buildingName.empty()) {
            sql += L" AND file_path LIKE '%" + buildingName + L"%'";
        }
        sql += L" ORDER BY file_path, pos_x, pos_y";

        // 执行查询
        std::vector<std::vector<std::wstring>> queryResults;
        if (!SqlDB::executeSelectQuery(sql, queryResults, errorMsg)) {
            acutPrintf(_T("查询执行失败: %s\n"), errorMsg.c_str());
            return results;
        }
        
        // 将查询结果转换为 TextSearchResult 对象
        for (const auto& row : queryResults) {
            if (row.size() >= 8) {  // 确保有足够的列
                TextSearchResult result;
                result.filePath = row[0];
                result.textContent = row[1];
                result.layerName = row[2];
                result.posX = _wtof(row[3].c_str());
                result.posY = _wtof(row[4].c_str());
                result.posZ = _wtof(row[5].c_str());
                result.entityHandle = row[6];
                
                // 解析时间戳字符串为 SYSTEMTIME
                // 这里需要根据数据库返回的时间格式进行解析
                // 暂时设置为零值，后续可以完善时间解析
                memset(&result.lastModified, 0, sizeof(SYSTEMTIME));
                
                results.push_back(result);
            }
        }
        
        acutPrintf(_T("搜索文本 '%s' 完成，找到 %d 条结果\n"), trimmedText.c_str(), (int)results.size());
        
    } catch (...) {
        errorMsg = L"搜索文本时发生异常";
    }
    
    return results;
}

// 为指定大楼的所有图纸建立文本索引（使用缓存路径）
bool SearchTextInDwg::buildTextIndexForBuilding(const std::wstring& buildingName, std::wstring& errorMsg)
{
    // 获取缓存中该建筑的所有DWG文件
    std::vector<std::wstring> drawingFiles = getCachedDrawingFiles(buildingName);
    
    if (drawingFiles.empty()) {
        errorMsg = L"未在缓存中找到大楼 " + buildingName + L" 的图纸文件";
        return false;
    }
    
    // 为每个图纸文件建立索引
    int successCount = 0;
    for (const std::wstring& filePath : drawingFiles) {
        std::wstring fileErrorMsg;
        if (buildTextIndexForDrawing(filePath, fileErrorMsg)) {
            successCount++;
        } else {
            acutPrintf(_T("为文件 %s 建立索引失败: %s"), filePath.c_str(), fileErrorMsg.c_str());
        }
    }
    
    acutPrintf(_T("为大楼 %s 成功建立了 %d/%d 个图纸的文本索引"), 
                      buildingName.c_str(), successCount, (int)drawingFiles.size());
    
    return successCount > 0;
}

// 为指定大楼的所有图纸建立文本索引（使用服务器文件名列表）
bool SearchTextInDwg::buildTextIndexForBuildingFromServer(const std::wstring& buildingName, 
                                                         const std::vector<std::wstring>& serverFileNames, 
                                                         std::wstring& errorMsg)
{
    if (!s_fileManager) {
        errorMsg = L"文件管理器未初始化";
        return false;
    }
    
    std::vector<std::wstring> cachedFiles;
    
    // 检查每个服务器文件是否已缓存
    for (const std::wstring& serverFileName : serverFileNames) {
        if (s_fileManager->isFileCached(serverFileName)) {
            std::wstring localPath = s_fileManager->getLocalCachePath(serverFileName);
            if (!localPath.empty() && PathFileExists(localPath.c_str())) {
                cachedFiles.push_back(localPath);  // 传入本地缓存路径
                acutPrintf(_T("找到缓存文件: 服务器名=%s, 本地路径=%s"), 
                          serverFileName.c_str(), localPath.c_str());
            }
        } else {
            acutPrintf(_T("文件 %s 未缓存，需要先下载"), serverFileName.c_str());
        }
    }
    
    if (cachedFiles.empty()) {
        errorMsg = L"没有可用的缓存文件用于建立索引";
        return false;
    }
    
    // 为缓存的文件建立索引（传入本地路径，方法内部会转换为服务器文件名）
    int successCount = 0;
    for (const std::wstring& localPath : cachedFiles) {
        std::wstring fileErrorMsg;
        if (buildTextIndexForDrawing(localPath, fileErrorMsg)) {  // 传入本地路径
            successCount++;
        } else {
            acutPrintf(_T("为文件 %s 建立索引失败: %s"), localPath.c_str(), fileErrorMsg.c_str());
        }
    }
    
    acutPrintf(_T("为大楼 %s 成功建立了 %d/%d 个图纸的文本索引"), 
               buildingName.c_str(), successCount, (int)cachedFiles.size());
    
    return successCount > 0;
}

// 获取文件的最后修改时间
SYSTEMTIME SearchTextInDwg::getFileLastModified(const std::wstring& filePath)
{
    SYSTEMTIME systemTime = {0};
    
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileData)) {
        FileTimeToSystemTime(&fileData.ftLastWriteTime, &systemTime);
    }
    
    return systemTime;
}

// 检查文件是否需要重新索引
bool SearchTextInDwg::needsReindexing(const std::wstring& filePath, const SYSTEMTIME& currentModified)
{
    // 获取服务器文件名
    std::wstring serverFileName = getServerFileNameFromLocalPath(filePath);
    
    // 查询数据库中该服务器文件的最后修改时间
    std::wstring sql = L"SELECT last_modified FROM cad_text_index WHERE file_path = '" + serverFileName + L"' LIMIT 1";
    
    // 使用ODBC执行查询
    // 暂时总是返回true，表示需要重新索引
    return true;
}

// 增量更新文本索引（只更新修改过的文件）
bool SearchTextInDwg::updateTextIndexIncremental(const std::wstring& buildingName, std::wstring& errorMsg)
{
    // 获取缓存中该建筑的所有图纸文件
    std::vector<std::wstring> drawingFiles = getCachedDrawingFiles(buildingName);
    
    if (drawingFiles.empty()) {
        errorMsg = L"未在缓存中找到大楼 " + buildingName + L" 的图纸文件";
        return false;
    }
    
    // 检查需要更新的文件
    std::vector<std::wstring> filesToUpdate;
    for (const std::wstring& filePath : drawingFiles) {
        SYSTEMTIME currentModified = getFileLastModified(filePath);
        if (needsReindexing(filePath, currentModified)) {
            filesToUpdate.push_back(filePath);
        }
    }
    
    if (filesToUpdate.empty()) {
        acutPrintf(_T("大楼 %s 的所有图纸索引都是最新的"), buildingName.c_str());
        return true;
    }
    
    // 更新需要重新索引的文件
    int successCount = 0;
    for (const std::wstring& filePath : filesToUpdate) {
        std::wstring fileErrorMsg;
        if (buildTextIndexForDrawing(filePath, fileErrorMsg)) {
            successCount++;
        } else {
            acutPrintf(_T("更新文件 %s 的索引失败: %s"), filePath.c_str(), fileErrorMsg.c_str());
        }
    }
    
    acutPrintf(_T("为大楼 %s 成功更新了 %d/%d 个图纸的文本索引"), 
                      buildingName.c_str(), successCount, (int)filesToUpdate.size());
    
    return successCount > 0;
}

// 清空指定大楼的文本索引
bool SearchTextInDwg::clearTextIndexForBuilding(const std::wstring& buildingName, std::wstring& errorMsg)
{
    // 注意：这里的 file_path 现在存储的是服务器文件名，可以直接按建筑名称过滤
    // 例如：建筑A_结构_v1_设计院_张三_基础平面图.dwg
    std::wstring sql = L"DELETE FROM cad_text_index WHERE file_path LIKE '%" + buildingName + L"%'";
    
    if (!SqlDB::executeQuery(sql, errorMsg)) {
        errorMsg = L"清空大楼文本索引失败: " + errorMsg;
        return false;
    }
    
    acutPrintf(_T("成功清空大楼 %s 的文本索引"), buildingName.c_str());
    return true;
}

// 定位文本功能
void SearchTextInDwg::locateText()
{
   acutPrintf(_T("定位文本功能"));
}

// 添加一个测试方法
bool SearchTextInDwg::testDatabaseConnection()
{
    std::wstring errorMsg;
    
    // 测试基本查询
    std::wstring testSql = L"SELECT COUNT(*) FROM cad_text_index";
    
    if (!SqlDB::executeQuery(testSql, errorMsg)) {
        acutPrintf(_T("测试查询失败: %s"), errorMsg.c_str());
        return false;
    }
    
   acutPrintf(_T("数据库连接正常"));
    
    // 测试直接查询"休息室"
    std::wstring directSql = L"SELECT text_content FROM cad_text_index WHERE text_content = '休息室'";
    if (SqlDB::executeQuery(directSql, errorMsg)) {
       acutPrintf(_T("直接查询'休息室'成功"));
    } else {
        acutPrintf(_T("直接查询失败: %s"), errorMsg.c_str());
    }
    
    // 测试LIKE查询
    std::wstring likeSql = L"SELECT text_content FROM cad_text_index WHERE text_content LIKE '%休息室%'";
    if (SqlDB::executeQuery(likeSql, errorMsg)) {
       acutPrintf(_T("LIKE查询'%休息室%'成功"));
    } else {
        acutPrintf(_T("LIKE查询失败: %s"), errorMsg.c_str());
    }
    
    return true;
}

// 添加一个简化的测试方法
bool SearchTextInDwg::testSearchWithParameters()
{
    std::wstring searchText = L"休息室";
    std::wstring buildingName = L"";  // 不指定大楼名称
    
    std::wstring sql = L"SELECT text_content FROM cad_text_index WHERE text_content LIKE '%" + searchText + L"%'";
    
    std::wstring errorMsg;
    if (!SqlDB::executeQuery(sql, errorMsg)) {
        acutPrintf(_T("测试查询执行失败: %s"), errorMsg.c_str());
        return false;
    }
    
   acutPrintf(_T("测试查询执行成功"));
   acutPrintf(_T("测试查询找到结果"));
    return true;
}

std::wstring SearchTextInDwg::getLocalCachePathFromServerName(const std::wstring& serverFileName)
{
    if (!s_fileManager) {
        return L"";
    }
    return s_fileManager->getLocalCachePath(serverFileName);
}

std::vector<std::wstring> SearchTextInDwg::getCachedDrawingFiles(const std::wstring& buildingName)
{
    std::vector<std::wstring> dwgFiles;
    
    if (!s_fileManager) {
        return dwgFiles;
    }
    
    // 获取缓存目录
    std::wstring cacheDir = s_fileManager->getLocalCachePath(L"");
    if (cacheDir.empty()) {
        return dwgFiles;
    }
    
    // 移除末尾的文件名部分，只保留目录
    size_t lastSlash = cacheDir.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        cacheDir = cacheDir.substr(0, lastSlash + 1);
    }
    
    try {
        // 使用 std::filesystem 扫描缓存目录中的DWG文件
        std::filesystem::path cachePath(cacheDir);
        if (std::filesystem::exists(cachePath)) {
            for (const auto& entry : std::filesystem::directory_iterator(cachePath)) {
                if (entry.is_regular_file()) {
                    std::wstring fileName = entry.path().filename().wstring();
                    std::wstring extension = entry.path().extension().wstring();
                    
                    // 检查是否是DWG文件且文件名包含建筑名称
                    if (_wcsicmp(extension.c_str(), L".dwg") == 0 && 
                        fileName.find(buildingName) != std::wstring::npos) {
                        dwgFiles.push_back(entry.path().wstring());
                    }
                }
            }
        }
    }
    catch (const std::exception&) {
        // 如果 std::filesystem 失败，回退到 Win32 API
        std::wstring searchPath = cacheDir + L"*.dwg";
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring fileName = findData.cFileName;
                    if (fileName.find(buildingName) != std::wstring::npos) {
                        std::wstring filePath = cacheDir + fileName;
                        if (PathFileExists(filePath.c_str())) {
                            dwgFiles.push_back(filePath);
                        }
                    }
                }
            } while (FindNextFile(hFind, &findData));
            FindClose(hFind);
        }
    }
    
    return dwgFiles;
}

// 从本地缓存路径获取服务器文件名
std::wstring SearchTextInDwg::getServerFileNameFromLocalPath(const std::wstring& localPath)
{
    if (!s_fileManager) {
        // 如果没有文件管理器，直接提取文件名
        size_t lastSlash = localPath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            return localPath.substr(lastSlash + 1);
        }
        return localPath;
    }
    
    // 获取缓存目录路径
    std::wstring cacheDir = s_fileManager->getLocalCachePath(L"");
    
    // 移除末尾可能的文件名部分，只保留目录
    size_t lastSlash = cacheDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        cacheDir = cacheDir.substr(0, lastSlash + 1);
    }
    
    // 标准化路径分隔符
    std::replace(cacheDir.begin(), cacheDir.end(), L'/', L'\\');
    std::wstring normalizedLocalPath = localPath;
    std::replace(normalizedLocalPath.begin(), normalizedLocalPath.end(), L'/', L'\\');
    
    // 检查本地路径是否在缓存目录中
    if (normalizedLocalPath.length() > cacheDir.length()) {
        std::wstring pathPrefix = normalizedLocalPath.substr(0, cacheDir.length());
        if (_wcsicmp(pathPrefix.c_str(), cacheDir.c_str()) == 0) {
            // 提取服务器文件名（去掉缓存目录前缀）
            return normalizedLocalPath.substr(cacheDir.length());
        }
    }
    
    // 如果不在缓存目录中，直接提取文件名
    lastSlash = normalizedLocalPath.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        return normalizedLocalPath.substr(lastSlash + 1);
    }
    
    return normalizedLocalPath;
}

// 添加数据清理方法
bool SearchTextInDwg::cleanupOldPathData(std::wstring& errorMsg)
{
    acutPrintf(_T("开始清理旧的路径格式数据...\n"));
    
    // 删除包含完整路径的旧记录（包含 :\ 或 / 的记录）
    std::wstring cleanupSql = L"DELETE FROM cad_text_index WHERE file_path LIKE '%:\\%' OR file_path LIKE '%/%'";
    
    if (!SqlDB::executeQuery(cleanupSql, errorMsg)) {
        acutPrintf(_T("清理旧数据失败: %s\n"), errorMsg.c_str());
        return false;
    }
    
    acutPrintf(_T("已清理旧的路径格式数据\n"));
    return true;
}

#include "StdAfx.h"
#include "ComponentLabelService.h"
#include "../common/CadLogger.h"
#include <time.h>

// 静态成员初始化
const TCHAR* ComponentLabelService::APP_NAME = _T("TSTArxProject2");
bool ComponentLabelService::s_isAppRegistered = false;

ComponentLabelService::ComponentLabelService()
{
}

ComponentLabelService::~ComponentLabelService()
{
}

ComponentLabelResult ComponentLabelService::AddLabelsToEntities(
    const AcDbObjectIdArray& entityIds, 
    const ComponentLabelData& labelData)
{
    ComponentLabelResult result;
    result.totalCount = entityIds.length();
    result.successCount = 0;
    
    if (result.totalCount == 0) {
        result.errorMessage = L"没有选中的实体";
        return result;
    }
   
    // 步骤1：注册应用程序名称
    if (!RegisterAppName(APP_NAME)) {
        result.errorMessage = L"注册应用程序名称失败";
        return result;
    }
    
    // 获取当前文档
    AcApDocument* curDoc = acDocManager->curDocument();
    if (curDoc == nullptr) {
        result.errorMessage = L"无法获取当前文档";
        return result;
    }
    
    // 锁定文档
    Acad::ErrorStatus lockStatus = acDocManager->lockDocument(curDoc, AcAp::kWrite);
    if (lockStatus != Acad::eOk) {
        result.errorMessage = L"无法锁定当前文档";
        return result;
    }
    
    acutPrintf(L"\n文档锁定成功");
    
    // 创建完整的标签数据
    ComponentLabelData fullLabelData = labelData;
    fullLabelData.createTime = GetCurrentTimeString();
    fullLabelData.creator = GetCurrentUserName();
    
    try {
        // 步骤2和3：为每个实体处理
        for (int i = 0; i < result.totalCount; i++) {
            try {
                // 获取ads_name
                ads_name en;
                if (acdbGetAdsName(en, entityIds[i]) != Acad::eOk) {
                    acutPrintf(L"\n实体 %d：无法获取ads_name", i + 1);
                    continue;
                }
                
                // 通过ads_name重新获取ObjectId
                AcDbObjectId objId;
                if (acdbGetObjectId(objId, en) != Acad::eOk) {
                    acutPrintf(L"\n实体 %d：无法重新获取ObjectId", i + 1);
                    continue;
                }
                
                // 打开对象（现在文档已经锁定）
                AcDbEntity* pEnt = nullptr;
                Acad::ErrorStatus es = acdbOpenObject(pEnt, objId, AcDb::kForWrite);
                
                if (es != Acad::eOk) {
                    acutPrintf(L"\n实体 %d：无法以写模式打开，错误代码: %d", i + 1, (int)es);
                    continue;
                }
                
                // 检查实体类型
								/*if (!IsEntityTypeSupported(objId)) {
										pEnt->close();
										acutPrintf(L"\n实体 %d：类型不支持", i + 1);
										continue;
								}*/
                
                // 构造扩展数据
                const TCHAR* maintenanceDate = fullLabelData.maintenanceDate.c_str();
                const TCHAR* responsiblePerson = fullLabelData.responsiblePerson.c_str();
                const TCHAR* createTime = fullLabelData.createTime.c_str();
                const TCHAR* creator = fullLabelData.creator.c_str();
                
                resbuf* pRb = acutBuildList(
                    AcDb::kDxfRegAppName, APP_NAME,        // 应用程序名称（必须是第一个）
                    AcDb::kDxfXdAsciiString, maintenanceDate,    // 维护日期
                    AcDb::kDxfXdAsciiString, responsiblePerson,  // 责任人
                    AcDb::kDxfXdAsciiString, createTime,         // 创建时间
                    AcDb::kDxfXdAsciiString, creator,            // 创建者
                    RTNONE                                 // 结束
                );
                
                // 写入XData
                if (pRb != nullptr) {
                    es = pEnt->setXData(pRb);
                    if (es == Acad::eOk) {
                        result.successCount++;
                        acutPrintf(L"\n实体 %d：扩展数据写入成功", i + 1);
                    } else {
                        acutPrintf(L"\n实体 %d：扩展数据写入失败，错误代码: %d", i + 1, (int)es);
                    }
                    acutRelRb(pRb); // 释放内存
                } else {
                    acutPrintf(L"\n实体 %d：构造扩展数据失败", i + 1);
                }
                
                pEnt->close();
                
            } catch (...) {
                acutPrintf(L"\n实体 %d：处理时发生异常", i + 1);
            }
        }
    }
    catch (...) {
        acutPrintf(L"\n处理实体时发生异常");
    }
    
    // 解锁文档
    acDocManager->unlockDocument(curDoc);
    acutPrintf(L"\n文档解锁完成");
    
    result.success = (result.successCount > 0);
    
    if (result.successCount < result.totalCount) {
        wchar_t buffer[256];
        swprintf_s(buffer, 256, L"部分实体添加标签失败，成功: %d/%d", 
                  result.successCount, result.totalCount);
        result.errorMessage = buffer;
    }
    
    return result;
}

bool ComponentLabelService::ReadLabelFromEntity(
    AcDbObjectId entityId, 
    ComponentLabelData& labelData, 
    std::wstring& errorMsg)
{
    AcDbEntity* pEnt = nullptr;
    Acad::ErrorStatus es = acdbOpenObject(pEnt, entityId, AcDb::kForRead);
    
    if (es != Acad::eOk) {
        errorMsg = L"无法打开实体";
        return false;
    }
    
    try {
        // 获取扩展数据 - 使用正确的方法
        struct resbuf* pXData = pEnt->xData(APP_NAME);
        pEnt->close();
        
        if (pXData == nullptr) {
            errorMsg = L"实体没有标签数据";
            return false;
        }
        
        // 解析扩展数据
        bool parseResult = ParseXData(pXData, labelData);
        FreeXData(pXData);
        
        if (!parseResult) {
            errorMsg = L"解析标签数据失败";
            return false;
        }
        
        return true;
    }
    catch (...) {
        pEnt->close();
        errorMsg = L"读取标签数据时发生异常";
        return false;
    }
}

bool ComponentLabelService::UpdateEntityLabel(
    AcDbObjectId entityId, 
    const ComponentLabelData& labelData, 
    std::wstring& errorMsg)
{
    // 检查实体类型是否支持
    if (!IsEntityTypeSupported(entityId)) {
        errorMsg = L"不支持的实体类型";
        return false;
    }
    
    try {
        AcDbEntity* pEnt = nullptr;
        
        // 直接以写模式打开实体（参考示例代码的方式）
        Acad::ErrorStatus es = acdbOpenObject(pEnt, entityId, AcDb::kForWrite);
        
        if (es != Acad::eOk) {
            wchar_t buffer[256];
            swprintf_s(buffer, 256, L"无法打开实体进行写操作，错误代码: %d", (int)es);
            errorMsg = buffer;
            return false;
        }
        
        // 创建扩展数据（使用与参考代码相同的方式）
        struct resbuf* pXData = CreateXData(labelData);
        if (pXData == nullptr) {
            pEnt->close();
            errorMsg = L"创建扩展数据失败";
            return false;
        }
        
        // 直接设置扩展数据（与参考代码相同）
        es = pEnt->setXData(pXData);
        
        // 释放扩展数据内存（与参考代码相同）
        acutRelRb(pXData);  // 使用acutRelRb而不是自定义的FreeXData
        
        // 关闭实体
        pEnt->close();
        
        if (es != Acad::eOk) {
            wchar_t buffer[256];
            swprintf_s(buffer, 256, L"设置扩展数据失败，错误代码: %d", (int)es);
            errorMsg = buffer;
            return false;
        }
        
        return true;
    }
    catch (...) {
        errorMsg = L"更新标签数据时发生异常";
        return false;
    }
}

bool ComponentLabelService::RemoveLabelFromEntity(AcDbObjectId entityId, std::wstring& errorMsg)
{
    AcDbEntity* pEnt = nullptr;
    Acad::ErrorStatus es = acdbOpenObject(pEnt, entityId, AcDb::kForWrite);
    
    if (es != Acad::eOk) {
        errorMsg = L"无法打开实体进行写操作";
        return false;
    }
    
    try {
        // 创建空的扩展数据来删除现有数据
        struct resbuf* pXData = acutBuildList(AcDb::kDxfRegAppName, APP_NAME, RTNONE);
        
        if (pXData != nullptr) {
            es = pEnt->setXData(pXData);
            FreeXData(pXData);
        }
        
        pEnt->close();
        
        if (es != Acad::eOk) {
            errorMsg = L"删除扩展数据失败";
            return false;
        }
        
        return true;
    }
    catch (...) {
        pEnt->close();
        errorMsg = L"删除标签数据时发生异常";
        return false;
    }
}

bool ComponentLabelService::HasLabel(AcDbObjectId entityId)
{
    AcDbEntity* pEnt = nullptr;
    Acad::ErrorStatus es = acdbOpenObject(pEnt, entityId, AcDb::kForRead);
    
    if (es != Acad::eOk) {
        return false;
    }
    
    // 使用正确的方法获取扩展数据
    struct resbuf* pXData = pEnt->xData(APP_NAME);
    pEnt->close();
    
    bool hasData = (pXData != nullptr);
    if (pXData != nullptr) {
        FreeXData(pXData);
    }
    
    return hasData;
}

AcDbObjectIdArray ComponentLabelService::GetAllLabeledEntities()
{
    AcDbObjectIdArray labeledEntities;
    
    // 获取当前数据库
    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
    if (pDb == nullptr) {
        return labeledEntities;
    }
    
    // 遍历模型空间中的所有实体
    AcDbBlockTable* pBlockTable = nullptr;
    if (pDb->getBlockTable(pBlockTable, AcDb::kForRead) != Acad::eOk) {
        return labeledEntities;
    }
    
    AcDbBlockTableRecord* pModelSpace = nullptr;
    if (pBlockTable->getAt(ACDB_MODEL_SPACE, pModelSpace, AcDb::kForRead) != Acad::eOk) {
        pBlockTable->close();
        return labeledEntities;
    }
    
    AcDbBlockTableRecordIterator* pIter = nullptr;
    if (pModelSpace->newIterator(pIter) != Acad::eOk) {
        pModelSpace->close();
        pBlockTable->close();
        return labeledEntities;
    }
    
    // 遍历所有实体
    for (pIter->start(); !pIter->done(); pIter->step()) {
        AcDbObjectId entityId;
        if (pIter->getEntityId(entityId) == Acad::eOk) {
            if (HasLabel(entityId)) {
                labeledEntities.append(entityId);
            }
        }
    }
    
    delete pIter;
    pModelSpace->close();
    pBlockTable->close();
    
    return labeledEntities;
}

AcDbObjectIdArray ComponentLabelService::SearchLabeledEntities(
    const std::wstring& maintenanceDate,
    const std::wstring& responsiblePerson)
{
    AcDbObjectIdArray matchedEntities;
    AcDbObjectIdArray allLabeledEntities = GetAllLabeledEntities();
    
    for (int i = 0; i < allLabeledEntities.length(); i++) {
        ComponentLabelData labelData;
        std::wstring errorMsg;
        
        if (ReadLabelFromEntity(allLabeledEntities[i], labelData, errorMsg)) {
            bool matches = true;
            
            // 检查维护日期条件
            if (!maintenanceDate.empty() && labelData.maintenanceDate.find(maintenanceDate) == std::wstring::npos) {
                matches = false;
            }
            
            // 检查责任人条件
            if (!responsiblePerson.empty() && labelData.responsiblePerson.find(responsiblePerson) == std::wstring::npos) {
                matches = false;
            }
            
            if (matches) {
                matchedEntities.append(allLabeledEntities[i]);
            }
        }
    }
    
    return matchedEntities;
}

bool ComponentLabelService::RegisterAppName(const ACHAR* appName)
{
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc == nullptr) {
		acutPrintf(_T("\n没有找到当前文档"));
		return false;
	}

	// 🔒 加锁当前文档（类似 .NET 的 DocumentLock）
	acDocManager->lockDocument(pDoc);

	AcDbDatabase* pDb = pDoc->database();
	AcDbRegAppTable* pTable = nullptr;

	if (pDb->getRegAppTable(pTable, AcDb::kForWrite) == Acad::eOk)
	{
		if (!pTable->has(appName))
		{
			AcDbRegAppTableRecord* pRec = new AcDbRegAppTableRecord();
			pRec->setName(appName);
			if (pTable->add(pRec) == Acad::eOk)
			{
				acutPrintf(_T("\nAppName %s 注册成功"), appName);
			}
			else
			{
				acutPrintf(_T("\nAppName %s 注册失败"), appName);
			}
			pRec->close();
		}
		else
		{
			acutPrintf(_T("\nAppName %s 已存在"), appName);
		}
		pTable->close();
	}
  
	// 🔓 解锁
	acDocManager->unlockDocument(pDoc);

  return true;
}

struct resbuf* ComponentLabelService::CreateXData(const ComponentLabelData& labelData)
{
    // 转换字符串为TCHAR*
    const TCHAR* maintenanceDate = labelData.maintenanceDate.c_str();
    const TCHAR* responsiblePerson = labelData.responsiblePerson.c_str();
    const TCHAR* createTime = labelData.createTime.c_str();
    const TCHAR* creator = labelData.creator.c_str();
    
    // 创建扩展数据链表
    struct resbuf* pXData = acutBuildList(
        AcDb::kDxfRegAppName, APP_NAME,                    // 应用程序名称
        AcDb::kDxfXdControlString, _T("{"),                // 开始标记
        AcDb::kDxfXdAsciiString, _T("MAINTENANCE_DATE"),   // 维护日期标识
        AcDb::kDxfXdAsciiString, maintenanceDate,          // 维护日期值
        AcDb::kDxfXdAsciiString, _T("RESPONSIBLE_PERSON"), // 责任人标识
        AcDb::kDxfXdAsciiString, responsiblePerson,        // 责任人值
        AcDb::kDxfXdAsciiString, _T("CREATE_TIME"),        // 创建时间标识
        AcDb::kDxfXdAsciiString, createTime,               // 创建时间值
        AcDb::kDxfXdAsciiString, _T("CREATOR"),            // 创建者标识
        AcDb::kDxfXdAsciiString, creator,                  // 创建者值
        AcDb::kDxfXdControlString, _T("}"),                // 结束标记
        RTNONE
    );
    
    return pXData;
}

bool ComponentLabelService::ParseXData(struct resbuf* pXData, ComponentLabelData& labelData)
{
    if (pXData == nullptr) {
        return false;
    }
    
    struct resbuf* pCurrent = pXData;
    std::wstring currentKey;
    
    while (pCurrent != nullptr) {
        if (pCurrent->restype == AcDb::kDxfXdAsciiString) {
            std::wstring value = pCurrent->resval.rstring;
            
            if (currentKey == L"MAINTENANCE_DATE") {
                labelData.maintenanceDate = value;
                currentKey.clear();
            } else if (currentKey == L"RESPONSIBLE_PERSON") {
                labelData.responsiblePerson = value;
                currentKey.clear();
            } else if (currentKey == L"CREATE_TIME") {
                labelData.createTime = value;
                currentKey.clear();
            } else if (currentKey == L"CREATOR") {
                labelData.creator = value;
                currentKey.clear();
            } else if (value == L"MAINTENANCE_DATE" || 
                      value == L"RESPONSIBLE_PERSON" || 
                      value == L"CREATE_TIME" || 
                      value == L"CREATOR") {
                currentKey = value;
            }
        }
        pCurrent = pCurrent->rbnext;
    }
    
    return !labelData.maintenanceDate.empty() && !labelData.responsiblePerson.empty();
}

void ComponentLabelService::FreeXData(struct resbuf* pRb)
{
    if (pRb != nullptr) {
        acutRelRb(pRb);
    }
}

std::wstring ComponentLabelService::GetCurrentTimeString()
{
    time_t rawtime;
    struct tm timeinfo;
    wchar_t buffer[80];
    
    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);
    
    wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &timeinfo);
    
    return std::wstring(buffer);
}

std::wstring ComponentLabelService::GetCurrentUserName()
{
    wchar_t username[256];
    DWORD size = sizeof(username) / sizeof(wchar_t);
    
    if (GetUserNameW(username, &size)) {
        return std::wstring(username);
    }
    
    return L"Unknown";
}

bool ComponentLabelService::IsEntityTypeSupported(AcDbObjectId entityId)
{
    AcDbEntity* pEnt = nullptr;
    Acad::ErrorStatus es = acdbOpenObject(pEnt, entityId, AcDb::kForRead);
    
    if (es != Acad::eOk) {
        return false;
    }
    
    bool supported = false;
    
    // 检查是否为支持的实体类型（线段、块参照等）
    if (pEnt->isKindOf(AcDbLine::desc()) ||
        pEnt->isKindOf(AcDbBlockReference::desc()) ||
        pEnt->isKindOf(AcDbCircle::desc()) ||
        pEnt->isKindOf(AcDbArc::desc()) ||
        pEnt->isKindOf(AcDbPolyline::desc())) {
        supported = true;
    }
    
    pEnt->close();
    return supported;
} 
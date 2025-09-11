#include "StdAfx.h"
#include "DwgViewerService.h"
#include <shlwapi.h>
#include <afxwin.h>
#include "../common/CadLogger.h"
#include "../common/SymbolTable/ViewUtil.h"

// 解决 acedCommand 编译错误的宏定义
#ifndef ACEDCOMMAND_USE_ACEDCOMMANDS
#define ACEDCOMMAND_USE_ACEDCOMMANDS
#endif

DwgViewerService::DwgViewerService()
{
}

DwgViewerService::~DwgViewerService()
{
}

bool DwgViewerService::openDwgAndLocateText(const std::wstring& filePath, const std::wstring& entityHandle, std::wstring& errorMsg)
{
    if (!PathFileExists(filePath.c_str())) {
        errorMsg = L"文件不存在: " + filePath;
        return false;
    }
    
    try {
        // 检查当前文档是否已经打开指定的文件
        AcApDocument* pCurrentDoc = acDocManager->curDocument();
        bool needToOpen = true;
        
        if (pCurrentDoc) {
            // 获取当前文档的文件路径
            const ACHAR* currentPath = pCurrentDoc->fileName();
            if (currentPath) {
                std::wstring currentFilePath = currentPath;
                
                // 比较路径是否相同（忽略大小写）
                if (_wcsicmp(currentFilePath.c_str(), filePath.c_str()) == 0) {
                    needToOpen = false;
                   acutPrintf(_T("文件已经打开，直接使用当前文档"));
                }
            }
        }
        
        // 如果需要打开文件
        if (needToOpen) {
            const ACHAR* tFilePath = filePath.c_str();
            
            // 使用acDocManager打开文件
            Acad::ErrorStatus es = acDocManager->appContextOpenDocument(tFilePath);
            if (es != Acad::eOk) {
                errorMsg = L"无法打开DWG文件: " + filePath + L", 错误代码: " + std::to_wstring((int)es);
                return false;
            }
            
           acutPrintf(_T("成功打开DWG文件: %s"), filePath.c_str());
        }
        
        // 获取当前数据库
        AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
        if (!pDb) {
            errorMsg = L"无法获取当前数据库";
            return false;
        }
        
        // 根据实体句柄查找并高亮显示实体
        bool success = highlightEntityByHandle(pDb, entityHandle);
        
        if (success) {
           acutPrintf(_T("成功定位到实体: %s"), entityHandle.c_str());
        } else {
            errorMsg = L"无法定位到指定的文本实体";
        }
        
        return success;
        
    } catch (...) {
        errorMsg = L"处理DWG文件时发生异常";
        return false;
    }
}

bool DwgViewerService::openDwgAndLocatePosition(const std::wstring& filePath, double x, double y, double z, std::wstring& errorMsg)
{
    if (!PathFileExists(filePath.c_str())) {
        errorMsg = L"文件不存在: " + filePath;
        return false;
    }
    
    try {
        // 检查当前文档是否已经打开指定的文件
        AcApDocument* pCurrentDoc = acDocManager->curDocument();
        bool needToOpen = true;
        
        if (pCurrentDoc) {
            // 获取当前文档的文件路径
            const ACHAR* currentPath = pCurrentDoc->fileName();
            if (currentPath) {
                std::wstring currentFilePath = currentPath;
                
                // 比较路径是否相同（忽略大小写）
                if (_wcsicmp(currentFilePath.c_str(), filePath.c_str()) == 0) {
                    needToOpen = false;
                   acutPrintf(_T("文件已经打开，直接使用当前文档"));
                }
            }
        }
        
        // 如果需要打开文件
        if (needToOpen) {
            const ACHAR* tFilePath = filePath.c_str();
            
            // 使用acDocManager打开文件
            Acad::ErrorStatus es = acDocManager->appContextOpenDocument(tFilePath);
            if (es != Acad::eOk) {
                errorMsg = L"无法打开DWG文件: " + filePath + L", 错误代码: " + std::to_wstring((int)es);
                return false;
            }
            
           acutPrintf(_T("成功打开DWG文件: %s"), filePath.c_str());
        }
        
        // 获取当前数据库
        AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
        if (!pDb) {
            errorMsg = L"无法获取当前数据库";
            return false;
        }
        
        // 缩放到指定位置
        bool success = zoomToPosition(x, y, z);
        
        if (success) {
           acutPrintf(_T("成功定位到位置: (%f, %f, %f)"), x, y, z);
        } else {
            errorMsg = L"无法定位到指定位置";
        }
        
        return success;
        
    } catch (...) {
        errorMsg = L"处理DWG文件时发生异常";
        return false;
    }
}

bool DwgViewerService::canOpenDwgFile(const std::wstring& filePath, std::wstring& errorMsg)
{
    if (!PathFileExists(filePath.c_str())) {
        errorMsg = L"文件不存在: " + filePath;
        return false;
    }
    
    try {
        AcDbDatabase* pDb = new AcDbDatabase(false, true);
        const TCHAR* tFilePath = convertPathToTChar(filePath);
        
        if (!tFilePath) {
            delete pDb;
            errorMsg = L"路径转换失败";
            return false;
        }
        
        Acad::ErrorStatus es = pDb->readDwgFile(tFilePath);
        delete pDb;
        
        if (es != Acad::eOk) {
            errorMsg = L"无法读取DWG文件: " + filePath + L", 错误代码: " + std::to_wstring((int)es);
            return false;
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"检查DWG文件时发生异常";
        return false;
    }
}

const TCHAR* DwgViewerService::convertPathToTChar(const std::wstring& filePath)
{
    static std::wstring staticPath;
    staticPath = filePath;
    return staticPath.c_str();
}

bool DwgViewerService::highlightEntityByHandle(AcDbDatabase* pDb, const std::wstring& entityHandle)
{
    if (!pDb || entityHandle.empty()) {
        return false;
    }
    
    try {
        // 静态变量记录当前高亮的实体
        static AcDbObjectId lastHighlightedEntity = AcDbObjectId::kNull;
        
        // 取消前一个高亮对象
        if (lastHighlightedEntity != AcDbObjectId::kNull) {
            AcDbEntity* pLastEntity = nullptr;
            if (acdbOpenObject(pLastEntity, lastHighlightedEntity, AcDb::kForRead) == Acad::eOk) {
                pLastEntity->unhighlight();
                pLastEntity->close();
               acutPrintf(_T("取消前一个高亮对象"));
            }
            lastHighlightedEntity = AcDbObjectId::kNull;
        }
        
        // 将wstring句柄转换为AcDbHandle
        unsigned long long handleValue = std::stoull(entityHandle, nullptr, 16);
        
        // 创建AcDbHandle对象
        AcDbHandle handle(handleValue);
        
        // 根据句柄获取对象ID
        AcDbObjectId objId;
        if (pDb->getAcDbObjectId(objId, false, handle) != Acad::eOk) {
            acutPrintf(_T("无法根据句柄找到对象: %s"), entityHandle.c_str());
            return false;
        }
        
        // 打开实体对象
        AcDbEntity* pEntity = nullptr;
        if (acdbOpenObject(pEntity, objId, AcDb::kForRead) != Acad::eOk) {
            acutPrintf(_T("无法打开实体对象"));
            return false;
        }
        
        // 获取实体的包围框
        AcDbExtents extents;
        if (pEntity->getGeomExtents(extents) == Acad::eOk) {
            // 缩放到实体位置
            AcGePoint3d center = extents.minPoint() + (extents.maxPoint() - extents.minPoint()) * 0.5;
            zoomToPosition(center.x, center.y, center.z);
            
            // 高亮显示当前实体
            pEntity->highlight();
            
            // 记录当前高亮的实体ID
            lastHighlightedEntity = objId;
            
           acutPrintf(_T("成功定位并高亮实体"));
        }
        
        pEntity->close();
        return true;
        
    } catch (...) {
        acutPrintf(_T("定位实体时发生异常"));
        return false;
    }
}

bool DwgViewerService::zoomToPosition(double x, double y, double z)
{
    try {
        // 获取当前视图
        AcDbViewTableRecord currentView;
        CViewUtil::GetCurrentView(currentView);
        
        // 设置新的中心点
        AcGePoint2d newCenter(x, y);
        currentView.setCenterPoint(newCenter);
        
        // 设置合适的缩放比例
        double scaleFactor = 0.05; // 放大20倍，可以根据需要调整
        
        // 获取当前视图的高度和宽度
        double currentHeight = currentView.height();
        double currentWidth = currentView.width();
        
        // 应用缩放因子
        double newHeight = currentHeight * scaleFactor;
        double newWidth = currentWidth * scaleFactor;
        
        currentView.setHeight(newHeight);
        currentView.setWidth(newWidth);
        
        // 设置新的视图
        Acad::ErrorStatus es = acedSetCurrentView(&currentView, NULL);
        
        return (es == Acad::eOk);
        
    } catch (...) {
        return false;
    }
} 
#pragma once

#include <string>
#include "SearchTextInDwg.h"

// 移除这行，因为arxHeaders.h应该通过StdAfx.h包含
// #include <arxHeaders.h>

class DwgViewerService
{
public:
    DwgViewerService();
    ~DwgViewerService();

    // 打开DWG文件并定位到指定的文本实体
    static bool openDwgAndLocateText(const std::wstring& filePath, const std::wstring& entityHandle, std::wstring& errorMsg);
    
    // 打开DWG文件并定位到指定坐标位置
    static bool openDwgAndLocatePosition(const std::wstring& filePath, double x, double y, double z, std::wstring& errorMsg);
    
    // 检查DWG文件是否可以打开
    static bool canOpenDwgFile(const std::wstring& filePath, std::wstring& errorMsg);

private:
    // 将std::wstring路径转换为TCHAR*格式
    static const TCHAR* convertPathToTChar(const std::wstring& filePath);
    
    // 根据实体句柄查找并高亮显示实体
    static bool highlightEntityByHandle(AcDbDatabase* pDb, const std::wstring& entityHandle);
    
    // 缩放到指定坐标位置
    static bool zoomToPosition(double x, double y, double z);
}; 
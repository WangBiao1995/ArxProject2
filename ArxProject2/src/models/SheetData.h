#pragma once

#include <string>

// 图纸数据结构体
struct SheetData {
    std::wstring name;           // 图纸名称
    std::wstring buildingName;   // 建筑名称
    std::wstring specialty;      // 专业
    std::wstring format;         // 格式
    std::wstring status;         // 状态
    std::wstring version;        // 版本
    std::wstring designUnit;     // 设计单位
    std::wstring createTime;     // 创建时间
    std::wstring creator;        // 创建者
    bool isSelected;             // 是否选中
    std::wstring filePath;       // 文件路径
}; 
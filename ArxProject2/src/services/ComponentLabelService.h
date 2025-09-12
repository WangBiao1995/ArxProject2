#pragma once

#include <vector>
#include <string>

// 构件标签数据结构
struct ComponentLabelData
{
    std::wstring maintenanceDate;   // 维护日期
    std::wstring responsiblePerson; // 责任人
    std::wstring createTime;        // 创建时间
    std::wstring creator;           // 创建者
    
    ComponentLabelData() = default;
    ComponentLabelData(const std::wstring& date, const std::wstring& person)
        : maintenanceDate(date), responsiblePerson(person) {}
};

// 构件标签操作结果
struct ComponentLabelResult
{
    bool success;
    int totalCount;
    int successCount;
    std::wstring errorMessage;
    
    ComponentLabelResult() : success(false), totalCount(0), successCount(0) {}
};

/// <summary>
/// 给构件打标签，命令行输入(entget (car (entsel))) 可查看结果
/// </summary>
class ComponentLabelService
{
public:
    ComponentLabelService();
    ~ComponentLabelService();

    // 为选中的实体添加标签数据
    static ComponentLabelResult AddLabelsToEntities(
        const AcDbObjectIdArray& entityIds, 
        const ComponentLabelData& labelData
    );
    
    // 从实体读取标签数据
    static bool ReadLabelFromEntity(
        AcDbObjectId entityId, 
        ComponentLabelData& labelData, 
        std::wstring& errorMsg
    );
    
    // 更新实体的标签数据
    static bool UpdateEntityLabel(
        AcDbObjectId entityId, 
        const ComponentLabelData& labelData, 
        std::wstring& errorMsg
    );
    
    // 删除实体的标签数据
    static bool RemoveLabelFromEntity(
        AcDbObjectId entityId, 
        std::wstring& errorMsg
    );
    
    // 检查实体是否有标签数据
    static bool HasLabel(AcDbObjectId entityId);
    
    // 获取所有带标签的实体
    static AcDbObjectIdArray GetAllLabeledEntities();
    
    // 根据条件搜索带标签的实体
    static AcDbObjectIdArray SearchLabeledEntities(
        const std::wstring& maintenanceDate = L"",
        const std::wstring& responsiblePerson = L""
    );

private:
    // 应用程序名称常量
    static const TCHAR* APP_NAME;
    
    // 注册状态标记
    static bool s_isAppRegistered;
    
    // 注册应用程序名称
    static bool RegisterAppName(const ACHAR* appName);
    
    // 创建扩展数据
    static struct resbuf* CreateXData(const ComponentLabelData& labelData);
    
    // 解析扩展数据
    static bool ParseXData(struct resbuf* pXData, ComponentLabelData& labelData);
    
    // 释放扩展数据内存
    static void FreeXData(struct resbuf* pRb);
    
    // 获取当前时间字符串
    static std::wstring GetCurrentTimeString();
    
    // 获取当前用户名
    static std::wstring GetCurrentUserName();
    
    // 验证实体类型是否支持标签
    static bool IsEntityTypeSupported(AcDbObjectId entityId);
}; 
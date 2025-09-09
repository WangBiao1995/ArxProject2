// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- BuildBuildingTableWindow.cpp : Implementation of BuildBuildingTableWindow
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "../../resource.h"
#include "BuildBuildingTableWindow.h"
#include "../common/Database/SqlDB.h"
#include "../common/CadLogger.h"
#include <sstream>
#include <iomanip>

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(BuildBuildingTableWindow, CAdUiBaseDialog)

// 静态成员初始化
BuildBuildingTableWindow* BuildBuildingTableWindow::m_pInstance = nullptr;

BEGIN_MESSAGE_MAP(BuildBuildingTableWindow, CAdUiBaseDialog)
    ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
    ON_BN_CLICKED(IDC_FILTER_BUTTON, &BuildBuildingTableWindow::OnBnClickedFilterButton)
    ON_BN_CLICKED(IDC_RESET_FILTER_BUTTON, &BuildBuildingTableWindow::OnBnClickedResetFilterButton)
    ON_BN_CLICKED(IDC_SAVE_BUTTON, &BuildBuildingTableWindow::OnBnClickedSaveButton)
    ON_NOTIFY(NM_RCLICK, IDC_BUILDING_TABLE, &BuildBuildingTableWindow::OnNMRClickBuildingTable)
    ON_NOTIFY(LVN_ENDLABELEDIT, IDC_BUILDING_TABLE, &BuildBuildingTableWindow::OnLvnEndlabeleditBuildingTable)
    ON_WM_SIZE()
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
// 单例模式实现
BuildBuildingTableWindow* BuildBuildingTableWindow::getInstance(CWnd *pParent, HINSTANCE hInstance)
{
    if (m_pInstance == nullptr) {
        m_pInstance = new BuildBuildingTableWindow(pParent, hInstance);
    }
    return m_pInstance;
}

void BuildBuildingTableWindow::destroyInstance()
{
    if (m_pInstance != nullptr) {
        if (m_pInstance->GetSafeHwnd()) {
            m_pInstance->DestroyWindow();
        }
        delete m_pInstance;
        m_pInstance = nullptr;
    }
}

//-----------------------------------------------------------------------------
BuildBuildingTableWindow::BuildBuildingTableWindow(CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) 
    : CAdUiBaseDialog(BuildBuildingTableWindow::IDD, pParent, hInstance)
{
}

BuildBuildingTableWindow::~BuildBuildingTableWindow()
{
    // 如果这是单例实例，则清空静态指针
    if (m_pInstance == this) {
        m_pInstance = nullptr;
    }
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::DoDataExchange(CDataExchange *pDX)
{
    CAdUiBaseDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_BUILDING_TABLE, m_buildingTable);
    DDX_Control(pDX, IDC_BUILDING_NAME_EDIT, m_buildingNameEdit);
    DDX_Control(pDX, IDC_DESIGN_UNIT_EDIT, m_designUnitEdit);
    DDX_Control(pDX, IDC_CREATE_TIME_PICKER, m_createTimePicker);
}

//-----------------------------------------------------------------------------
BOOL BuildBuildingTableWindow::OnInitDialog()
{
    CAdUiBaseDialog::OnInitDialog();
    
    // 设置窗口最小尺寸
    SetWindowPos(NULL, 0, 0, 600, 450, SWP_NOMOVE | SWP_NOZORDER);
    
    // 初始化控件
    initializeControls();
    
    // 设置表格列
    setupTableColumns();
    
    // 创建数据库表
    createBuildingTable();
    
    // 加载数据
    loadDataFromDatabase();
    
    return TRUE;
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::initializeControls()
{
    // 设置窗口标题
    SetWindowText(_T("大楼列表"));
    
    // 设置时间选择器默认值为当前时间
    CTime currentTime = CTime::GetCurrentTime();
    m_createTimePicker.SetTime(&currentTime);
    
    // 设置表格样式
    m_buildingTable.SetExtendedStyle(
        LVS_EX_FULLROWSELECT | 
        LVS_EX_GRIDLINES | 
        LVS_EX_HEADERDRAGDROP |
        LVS_EX_LABELTIP
    );
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::setupTableColumns()
{
    // 清除现有列
    while (m_buildingTable.GetHeaderCtrl()->GetItemCount() > 0) {
        m_buildingTable.DeleteColumn(0);
    }
    
    // 添加列，调整列宽以适应600像素宽度的窗口
    m_buildingTable.InsertColumn(0, _T("大楼名称"), LVCFMT_LEFT, 90);
    m_buildingTable.InsertColumn(1, _T("地址"), LVCFMT_LEFT, 120);
    m_buildingTable.InsertColumn(2, _T("总面积"), LVCFMT_LEFT, 70);
    m_buildingTable.InsertColumn(3, _T("层数"), LVCFMT_LEFT, 50);
    m_buildingTable.InsertColumn(4, _T("设计单位"), LVCFMT_LEFT, 100);
    m_buildingTable.InsertColumn(5, _T("创建时间"), LVCFMT_LEFT, 80);
    m_buildingTable.InsertColumn(6, _T("创建人"), LVCFMT_LEFT, 60);
}

//-----------------------------------------------------------------------------
bool BuildBuildingTableWindow::createBuildingTable()
{
    std::wstring sql = L"CREATE TABLE IF NOT EXISTS building_info ("
        L"id SERIAL PRIMARY KEY, "
        L"building_name VARCHAR(255) NOT NULL, "
        L"address VARCHAR(500), "
        L"total_area VARCHAR(100), "
        L"floors VARCHAR(50), "
        L"design_unit VARCHAR(255), "
        L"create_time DATE DEFAULT CURRENT_DATE, "
        L"creator VARCHAR(100) DEFAULT '系统用户', "
        L"created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        L")";
    
    std::wstring errorMsg;
    if (!SqlDB::executeQuery(sql, errorMsg)) {
        CString errMsg(errorMsg.c_str());
        CadLogger::LogError(_T("创建大楼信息表失败: %s"), errMsg);
        return false;
    }
    
    CadLogger::LogInfo(_T("大楼信息表创建成功或已存在"));
    return true;
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::loadDataFromDatabase()
{
    // 清空现有数据
    m_buildingDataList.clear();
    m_buildingTable.DeleteAllItems();
    
    // 查询数据库
    std::wstring sql = L"SELECT id, building_name, address, total_area, floors, design_unit, "
        L"TO_CHAR(create_time, 'YYYY-MM-DD') as create_time, creator "
        L"FROM building_info ORDER BY created_at DESC";
    
    std::wstring errorMsg;
    
    // 这里需要实现一个查询方法，由于原有的SqlDB类主要用于执行语句，我们需要扩展它
    // 暂时使用简单的方法，实际项目中应该扩展SqlDB类
    
    // 添加一些示例数据用于演示
    BuildingData sampleData1;
    sampleData1.id = 1;
    sampleData1.buildingName = L"测试大楼1";
    sampleData1.address = L"北京市朝阳区";
    sampleData1.totalArea = L"10000";
    sampleData1.floors = L"20";
    sampleData1.designUnit = L"北京设计院";
    sampleData1.createTime = getCurrentTimeString();
    sampleData1.creator = L"系统用户";
    m_buildingDataList.push_back(sampleData1);
    
    BuildingData sampleData2;
    sampleData2.id = 2;
    sampleData2.buildingName = L"测试大楼2";
    sampleData2.address = L"上海市浦东新区";
    sampleData2.totalArea = L"15000";
    sampleData2.floors = L"25";
    sampleData2.designUnit = L"上海设计院";
    sampleData2.createTime = getCurrentTimeString();
    sampleData2.creator = L"系统用户";
    m_buildingDataList.push_back(sampleData2);
    
    // 填充表格
    populateTableFromData();
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::populateTableFromData()
{
    m_buildingTable.DeleteAllItems();
    
    for (int i = 0; i < static_cast<int>(m_buildingDataList.size()); ++i) {
        const BuildingData& data = m_buildingDataList[i];
        
        // 转换wstring到CString
        CString buildingName(data.buildingName.c_str());
        CString address(data.address.c_str());
        CString totalArea(data.totalArea.c_str());
        CString floors(data.floors.c_str());
        CString designUnit(data.designUnit.c_str());
        CString createTime(data.createTime.c_str());
        CString creator(data.creator.c_str());
        
        int nItem = m_buildingTable.InsertItem(i, buildingName);
        m_buildingTable.SetItemText(nItem, 1, address);
        m_buildingTable.SetItemText(nItem, 2, totalArea);
        m_buildingTable.SetItemText(nItem, 3, floors);
        m_buildingTable.SetItemText(nItem, 4, designUnit);
        m_buildingTable.SetItemText(nItem, 5, createTime);
        m_buildingTable.SetItemText(nItem, 6, creator);
        
        // 存储数据ID到项目数据中
        m_buildingTable.SetItemData(nItem, static_cast<DWORD_PTR>(data.id));
    }
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::saveDataToDatabase()
{
    // 先清空数据库中的旧数据
    std::wstring clearSql = L"DELETE FROM building_info";
    std::wstring errorMsg;
    if (!SqlDB::executeQuery(clearSql, errorMsg)) {
        CString errMsg(errorMsg.c_str());
        CadLogger::LogError(_T("清空大楼数据失败: %s"), errMsg);
        AfxMessageBox(_T("保存失败：无法清空旧数据"));
        return;
    }
    
    // 从表格中读取数据并保存到数据库
    int itemCount = m_buildingTable.GetItemCount();
    int successCount = 0;
    
    for (int i = 0; i < itemCount; ++i) {
        BuildingData data;
        
        // 从CString转换到wstring
        CString temp;
        temp = m_buildingTable.GetItemText(i, 0);
        data.buildingName = std::wstring(temp);
        
        temp = m_buildingTable.GetItemText(i, 1);
        data.address = std::wstring(temp);
        
        temp = m_buildingTable.GetItemText(i, 2);
        data.totalArea = std::wstring(temp);
        
        temp = m_buildingTable.GetItemText(i, 3);
        data.floors = std::wstring(temp);
        
        temp = m_buildingTable.GetItemText(i, 4);
        data.designUnit = std::wstring(temp);
        
        temp = m_buildingTable.GetItemText(i, 5);
        data.createTime = std::wstring(temp);
        
        temp = m_buildingTable.GetItemText(i, 6);
        data.creator = std::wstring(temp);
        
        // 如果创建时间为空，设置当前时间
        if (data.createTime.empty()) {
            data.createTime = getCurrentTimeString();
        }
        
        // 如果创建人为空，设置默认值
        if (data.creator.empty()) {
            data.creator = L"系统用户";
        }
        
        if (insertBuildingData(data)) {
            successCount++;
        }
    }
    
    CString msg;
    msg.Format(_T("成功保存 %d/%d 条记录到数据库"), successCount, itemCount);
    AfxMessageBox(msg);
    CadLogger::LogInfo(_T("保存大楼数据: %s"), msg);
}

//-----------------------------------------------------------------------------
bool BuildBuildingTableWindow::insertBuildingData(const BuildingData& data)
{
    // 构建SQL语句，注意转义单引号
    std::wstring sql = L"INSERT INTO building_info (building_name, address, total_area, floors, design_unit, create_time, creator) VALUES ('";
    sql += data.buildingName + L"', '";
    sql += data.address + L"', '";
    sql += data.totalArea + L"', '";
    sql += data.floors + L"', '";
    sql += data.designUnit + L"', '";
    sql += data.createTime + L"', '";
    sql += data.creator + L"')";
    
    std::wstring errorMsg;
    if (!SqlDB::executeQuery(sql, errorMsg)) {
        CString errMsg(errorMsg.c_str());
        CadLogger::LogError(_T("插入大楼数据失败: %s"), errMsg);
        return false;
    }
    
    return true;
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::filterData()
{
    CString buildingName, designUnit;
    m_buildingNameEdit.GetWindowText(buildingName);
    m_designUnitEdit.GetWindowText(designUnit);
    
    CTime selectedTime;
    m_createTimePicker.GetTime(selectedTime);
    CString createTime = selectedTime.Format(_T("%Y-%m-%d"));
    
    // 简单的内存筛选（实际项目中应该使用数据库查询）
    m_buildingTable.DeleteAllItems();
    
    for (int i = 0; i < static_cast<int>(m_buildingDataList.size()); ++i) {
        const BuildingData& data = m_buildingDataList[i];
        bool match = true;
        
        // 检查大楼名称
        if (!buildingName.IsEmpty()) {
            CString dataName(data.buildingName.c_str());
            if (dataName.Find(buildingName) == -1) {
                match = false;
            }
        }
        
        // 检查设计单位
        if (match && !designUnit.IsEmpty()) {
            CString dataUnit(data.designUnit.c_str());
            if (dataUnit.Find(designUnit) == -1) {
                match = false;
            }
        }
        
        // 检查创建时间（简单匹配日期）
        if (match && !createTime.IsEmpty()) {
            CString dataTime(data.createTime.c_str());
            if (dataTime.Find(createTime) == -1) {
                match = false;
            }
        }
        
        if (match) {
            CString buildingNameStr(data.buildingName.c_str());
            CString addressStr(data.address.c_str());
            CString totalAreaStr(data.totalArea.c_str());
            CString floorsStr(data.floors.c_str());
            CString designUnitStr(data.designUnit.c_str());
            CString createTimeStr(data.createTime.c_str());
            CString creatorStr(data.creator.c_str());
            
            int nItem = m_buildingTable.InsertItem(m_buildingTable.GetItemCount(), buildingNameStr);
            m_buildingTable.SetItemText(nItem, 1, addressStr);
            m_buildingTable.SetItemText(nItem, 2, totalAreaStr);
            m_buildingTable.SetItemText(nItem, 3, floorsStr);
            m_buildingTable.SetItemText(nItem, 4, designUnitStr);
            m_buildingTable.SetItemText(nItem, 5, createTimeStr);
            m_buildingTable.SetItemText(nItem, 6, creatorStr);
            m_buildingTable.SetItemData(nItem, static_cast<DWORD_PTR>(data.id));
        }
    }
    
    CString msg;
    msg.Format(_T("筛选完成，共找到 %d 条记录"), m_buildingTable.GetItemCount());
    CadLogger::LogInfo(_T("筛选大楼数据: %s"), msg);
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::resetFilter()
{
    // 清空筛选条件
    m_buildingNameEdit.SetWindowText(_T(""));
    m_designUnitEdit.SetWindowText(_T(""));
    
    CTime currentTime = CTime::GetCurrentTime();
    m_createTimePicker.SetTime(&currentTime);
    
    // 重新显示所有数据
    populateTableFromData();
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::insertRow()
{
    // 获取当前选中的行，如果没有选中行则在末尾插入
    int selectedItem = m_buildingTable.GetNextItem(-1, LVNI_SELECTED);
    if (selectedItem == -1) {
        selectedItem = m_buildingTable.GetItemCount();
    }
    
    // 插入新行
    int newItem = m_buildingTable.InsertItem(selectedItem, _T(""));
    CString currentTimeStr(getCurrentTimeString().c_str());
    m_buildingTable.SetItemText(newItem, 5, currentTimeStr);
    m_buildingTable.SetItemText(newItem, 6, _T("系统用户"));
    
    // 选中新行并开始编辑
    m_buildingTable.SetItemState(newItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    m_buildingTable.EditLabel(newItem);
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::deleteRow()
{
    int selectedItem = m_buildingTable.GetNextItem(-1, LVNI_SELECTED);
    if (selectedItem == -1) {
        AfxMessageBox(_T("请先选择要删除的行"));
        return;
    }
    
    CString msg;
    msg.Format(_T("确定要删除第 %d 行吗？"), selectedItem + 1);
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES) {
        m_buildingTable.DeleteItem(selectedItem);
        
        // 选中下一行或上一行
        int itemCount = m_buildingTable.GetItemCount();
        if (itemCount > 0) {
            if (selectedItem >= itemCount) {
                selectedItem = itemCount - 1;
            }
            m_buildingTable.SetItemState(selectedItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
    }
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::showContextMenu(CPoint point)
{
    CMenu menu;
    menu.CreatePopupMenu();
    
    menu.AppendMenu(MF_STRING, 1001, _T("插入行"));
    menu.AppendMenu(MF_STRING, 1002, _T("删除行"));
    
    // 如果没有选中行，禁用删除操作
    int selectedItem = m_buildingTable.GetNextItem(-1, LVNI_SELECTED);
    if (selectedItem == -1) {
        menu.EnableMenuItem(1002, MF_GRAYED);
    }
    
    int cmd = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, this);
    
    switch (cmd) {
    case 1001:
        insertRow();
        break;
    case 1002:
        deleteRow();
        break;
    }
}

//-----------------------------------------------------------------------------
std::wstring BuildBuildingTableWindow::getCurrentTimeString()
{
    CTime currentTime = CTime::GetCurrentTime();
    CString timeStr = currentTime.Format(_T("%Y-%m-%d"));
    return std::wstring(timeStr);
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::resizeControls(int cx, int cy)
{
    if (!::IsWindow(m_buildingTable.GetSafeHwnd())) {
        return;
    }
    
    // 设置最小窗口尺寸
    if (cx < 500) cx = 500;
    if (cy < 350) cy = 350;
    
    // 调整表格大小
    CRect tableRect;
    m_buildingTable.GetWindowRect(&tableRect);
    ScreenToClient(&tableRect);
    
    tableRect.right = cx - 15;
    tableRect.bottom = cy - 45;
    m_buildingTable.MoveWindow(&tableRect);
    
    // 调整按钮位置
    CWnd* pSaveBtn = GetDlgItem(IDC_SAVE_BUTTON);
    CWnd* pOKBtn = GetDlgItem(IDOK);
    CWnd* pCancelBtn = GetDlgItem(IDCANCEL);
    
    if (pSaveBtn && pOKBtn && pCancelBtn) {
        int buttonY = cy - 25;
        
        CRect saveRect, okRect, cancelRect;
        pSaveBtn->GetWindowRect(&saveRect);
        pOKBtn->GetWindowRect(&okRect);
        pCancelBtn->GetWindowRect(&cancelRect);
        ScreenToClient(&saveRect);
        ScreenToClient(&okRect);
        ScreenToClient(&cancelRect);
        
        // 重新定位按钮
        saveRect.MoveToXY(cx - 155, buttonY);
        okRect.MoveToXY(cx - 100, buttonY);
        cancelRect.MoveToXY(cx - 50, buttonY);
        
        pSaveBtn->MoveWindow(&saveRect);
        pOKBtn->MoveWindow(&okRect);
        pCancelBtn->MoveWindow(&cancelRect);
    }
}

//-----------------------------------------------------------------------------
// 消息处理函数
//-----------------------------------------------------------------------------
LRESULT BuildBuildingTableWindow::OnAcadKeepFocus(WPARAM, LPARAM)
{
    return (TRUE);
}

void BuildBuildingTableWindow::OnBnClickedFilterButton()
{
    filterData();
}

void BuildBuildingTableWindow::OnBnClickedResetFilterButton()
{
    resetFilter();
}

void BuildBuildingTableWindow::OnBnClickedSaveButton()
{
    saveDataToDatabase();
}

void BuildBuildingTableWindow::OnNMRClickBuildingTable(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    
    CPoint point;
    GetCursorPos(&point);
    showContextMenu(point);
    
    *pResult = 0;
}

void BuildBuildingTableWindow::OnLvnEndlabeleditBuildingTable(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    
    // 允许编辑
    *pResult = TRUE;
}

void BuildBuildingTableWindow::OnSize(UINT nType, int cx, int cy)
{
    CAdUiBaseDialog::OnSize(nType, cx, cy);
    
    if (nType != SIZE_MINIMIZED) {
        resizeControls(cx, cy);
    }
}

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

//// 静态成员初始化
//BuildBuildingTableWindow* BuildBuildingTableWindow::m_pInstance = nullptr;

BEGIN_MESSAGE_MAP(BuildBuildingTableWindow, CAdUiBaseDialog)
    ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
    ON_BN_CLICKED(IDC_FILTER_BUTTON, &BuildBuildingTableWindow::OnBnClickedFilterButton)
    ON_BN_CLICKED(IDC_RESET_FILTER_BUTTON, &BuildBuildingTableWindow::OnBnClickedResetFilterButton)
    ON_NOTIFY(NM_RCLICK, IDC_BUILDING_TABLE, &BuildBuildingTableWindow::OnNMRClickBuildingTable)
    ON_NOTIFY(LVN_ENDLABELEDIT, IDC_BUILDING_TABLE, &BuildBuildingTableWindow::OnLvnEndlabeleditBuildingTable)
    ON_NOTIFY(NM_DBLCLK, IDC_BUILDING_TABLE, &BuildBuildingTableWindow::OnNMDblclkBuildingTable)      // 添加双击消息
    ON_NOTIFY(LVN_KEYDOWN, IDC_BUILDING_TABLE, &BuildBuildingTableWindow::OnLvnKeydownBuildingTable)  // 添加按键消息
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_BUILDING_TABLE, &BuildBuildingTableWindow::OnLvnItemchangedBuildingTable)  // 添加这行
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()  // 新增：添加最小窗口尺寸限制消息映射
END_MESSAGE_MAP()

BuildBuildingTableWindow::BuildBuildingTableWindow(CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) 
    : CAdUiBaseDialog(BuildBuildingTableWindow::IDD, pParent, hInstance)
    , m_pEditCtrl(nullptr)
    , m_nEditItem(-1)
    , m_nEditSubItem(-1)
{
}

BuildBuildingTableWindow::~BuildBuildingTableWindow()
{

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
    
    // 初始化控件
    initializeControls();
    
    // 设置表格列
    setupTableColumns();
    
    // 创建数据库表
    createBuildingTable();
    
    // 加载数据
    loadDataFromDatabase();
    
   
    // 窗口最大化显示
    //ShowWindow(SW_SHOWMAXIMIZED);
    
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
    
    // 设置表格样式 - 启用编辑功能
    m_buildingTable.SetExtendedStyle(
        LVS_EX_FULLROWSELECT |      // 整行选择
        LVS_EX_GRIDLINES |          // 显示网格线
        LVS_EX_HEADERDRAGDROP |     // 允许拖拽列头
        LVS_EX_LABELTIP |           // 显示标签提示
        LVS_EX_INFOTIP              // 显示信息提示
    );
    
    // 启用表格的标签编辑功能（这是关键）
    DWORD dwStyle = m_buildingTable.GetStyle();
    dwStyle |= LVS_EDITLABELS;  // 添加编辑标签样式
    m_buildingTable.ModifyStyle(0, LVS_EDITLABELS);
}

//-----------------------------------------------------------------------------
void BuildBuildingTableWindow::setupTableColumns()
{
    // 检查表格控件是否有效
    if (!::IsWindow(m_buildingTable.GetSafeHwnd())) {
        CadLogger::LogError(_T("表格控件未正确初始化"));
        return;
    }
    
    // 检查表头控件是否有效
    CHeaderCtrl* pHeader = m_buildingTable.GetHeaderCtrl();
    if (!pHeader || !::IsWindow(pHeader->GetSafeHwnd())) {
        CadLogger::LogError(_T("表头控件获取失败"));
        return;
    }
    
    // 清除现有列
    while (pHeader->GetItemCount() > 0) {
        m_buildingTable.DeleteColumn(0);
    }

    // 获取表格客户区宽度用于按权重分配列宽
    CRect clientRect;
    m_buildingTable.GetClientRect(&clientRect);
    int totalWidth = clientRect.Width() - 30; // 减少边距，确保有足够空间
    
    // 如果窗口最大化，使用更精确的宽度计算
    if (IsZoomed()) {
        CRect windowRect;
        GetWindowRect(&windowRect);
        // 使用窗口实际宽度减去边框和滚动条
        totalWidth = windowRect.Width() - 60; // 减去窗口边框、滚动条等
    }
    
    // 根据内容长度重新调整权重分配
    struct ColumnInfo {
        LPCTSTR title;
        int weight;
        int format;
    };
    
    ColumnInfo columns[] = {
        { _T("大楼名称"), 18, LVCFMT_LEFT },   // 18% - 大楼名称通常较长，如"A座办公楼"、"北京国际贸易中心"
        { _T("地址"), 25, LVCFMT_LEFT },       // 25% - 地址最长，如"北京市朝阳区建国门外大街1号"
        { _T("总面积"), 10, LVCFMT_LEFT },     // 10% - 数字+单位，如"10000㎡"
        { _T("层数"), 8, LVCFMT_LEFT },        // 8% - 简短数字，如"20层"
        { _T("设计单位"), 20, LVCFMT_LEFT },   // 20% - 单位名称较长，如"中国建筑设计研究院"
        { _T("创建时间"), 12, LVCFMT_LEFT },   // 12% - 日期格式，如"2024-01-15"
        { _T("创建人"), 7, LVCFMT_LEFT }       // 7% - 人名最短，如"张三"、"李四"
    };
    
    // 插入列并按权重设置宽度
    for (int i = 0; i < 7; i++) {
        int columnWidth = (totalWidth * columns[i].weight) / 100;
        // 设置最小列宽，确保内容可读
        int minWidth;
        switch (i) {
            case 0: minWidth = 80;  break;  // 大楼名称最小80
            case 1: minWidth = 100; break;  // 地址最小100
            case 2: minWidth = 60;  break;  // 总面积最小60
            case 3: minWidth = 40;  break;  // 层数最小40
            case 4: minWidth = 90;  break;  // 设计单位最小90
            case 5: minWidth = 70;  break;  // 创建时间最小70
            case 6: minWidth = 50;  break;  // 创建人最小50
            default: minWidth = 50; break;
        }
        
        if (columnWidth < minWidth) columnWidth = minWidth;
        m_buildingTable.InsertColumn(i, columns[i].title, columns[i].format, columnWidth);
    }
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
        CadLogger::LogError(_T("Failed to create building_info table: %s"), errMsg);
        return false;
    }
    
    CadLogger::LogInfo(_T("Building info table created successfully or already exists"));
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
    std::vector<std::vector<std::wstring>> results;
    
    // 执行查询
    if (SqlDB::executeSelectQuery(sql, results, errorMsg)) {
        // 处理查询结果
        for (const auto& row : results) {
            if (row.size() >= 8) {  // 确保有足够的列
                BuildingData data;
                data.id = _wtoi(row[0].c_str());
                data.buildingName = row[1];
                data.address = row[2];
                data.totalArea = row[3];
                data.floors = row[4];
                data.designUnit = row[5];
                data.createTime = row[6];
                data.creator = row[7];
                
                m_buildingDataList.push_back(data);
            }
        }
        
        CadLogger::LogInfo(_T("Loaded %d building records from database"), static_cast<int>(m_buildingDataList.size()));
    } else {
        // 查询失败，记录错误并使用示例数据
        CString errMsg(errorMsg.c_str());
        CadLogger::LogWarning(_T("Failed to load data from database: %s, using sample data"), errMsg);
        
        // 添加示例数据作为备用
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
    }
    
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
        CadLogger::LogError(_T("Failed to clear building data: %s"), errMsg);
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
    CadLogger::LogInfo(_T("Saved building data: %s"), msg);
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
        CadLogger::LogError(_T("Failed to insert building data: %s"), errMsg);
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
    CadLogger::LogInfo(_T("Filtered building data: %s"), msg);
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

//-----------------------------------------------------------------------------
// 重写OnOK方法，点击确定时自动保存数据
void BuildBuildingTableWindow::OnOK()
{
    // 如果正在编辑，先结束编辑
    if (m_pEditCtrl) {
        endEdit(true);
    }
    
    // 自动保存数据到数据库
    saveDataToDatabase();
    
    // 关闭对话框
    CAdUiBaseDialog::OnOK();
}

//-----------------------------------------------------------------------------
// 重写OnCancel方法，取消时不保存数据
void BuildBuildingTableWindow::OnCancel()
{
    // 如果正在编辑，取消编辑
    if (m_pEditCtrl) {
        endEdit(false);
    }
    
    // 询问是否要保存更改
    if (!m_buildingDataList.empty()) {
        int result = AfxMessageBox(_T("是否要保存更改？"), MB_YESNOCANCEL | MB_ICONQUESTION);
        if (result == IDYES) {
            saveDataToDatabase();
            CAdUiBaseDialog::OnOK();  // 保存后正常关闭
        } else if (result == IDNO) {
            CAdUiBaseDialog::OnCancel();  // 不保存直接关闭
        }
        // IDCANCEL 不做任何操作，继续停留在对话框
    } else {
        CAdUiBaseDialog::OnCancel();
    }
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
    
    // 检查是否有有效的编辑文本
    if (pDispInfo->item.pszText != NULL && pDispInfo->item.pszText[0] != '\0') {
        // 接受编辑
        *pResult = TRUE;
        
        // 可以在这里添加数据验证逻辑
        int nItem = pDispInfo->item.iItem;
        int nSubItem = pDispInfo->item.iSubItem;
        CString newText = pDispInfo->item.pszText;
        
        // 记录编辑操作
        CadLogger::LogInfo(_T("表格编辑: 行%d 列%d 新值: %s"), nItem, nSubItem, newText);
    } else {
        // 拒绝编辑（如果文本为空或无效）
        *pResult = FALSE;
    }
}

//-----------------------------------------------------------------------------
// 添加双击编辑功能
void BuildBuildingTableWindow::OnNMDblclkBuildingTable(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    
    if (pNMItemActivate->iItem >= 0 && pNMItemActivate->iSubItem >= 0) {
        // 开始编辑选中的单元格
        startEdit(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
    }
    
    *pResult = 0;
}

//-----------------------------------------------------------------------------
// 添加按键处理，支持F2键编辑
void BuildBuildingTableWindow::OnLvnKeydownBuildingTable(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
    
    if (pLVKeyDow->wVKey == VK_F2) {
        // F2键开始编辑当前选中的单元格
        int selectedItem = m_buildingTable.GetNextItem(-1, LVNI_SELECTED);
        if (selectedItem >= 0) {
            startEdit(selectedItem, 0); // 默认编辑第一列
        }
    }
    else if (pLVKeyDow->wVKey == VK_ESCAPE) {
        // ESC键取消编辑
        if (m_pEditCtrl) {
            endEdit(false);
        }
    }
    else if (pLVKeyDow->wVKey == VK_RETURN) {
        // 回车键保存编辑
        if (m_pEditCtrl) {
            endEdit(true);
        }
    }
    
    *pResult = 0;
}

void BuildBuildingTableWindow::OnLvnItemchangedBuildingTable(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    
    // 如果正在编辑且选择项发生变化，结束编辑
    if (m_pEditCtrl && pNMLV->uNewState & LVIS_SELECTED) {
        endEdit(true);
    }
    
    *pResult = 0;
}

void BuildBuildingTableWindow::OnSize(UINT nType, int cx, int cy)
{
    CAdUiBaseDialog::OnSize(nType, cx, cy);
    
    if (nType != SIZE_MINIMIZED) {  // 修改：只有在非最小化时才调整控件
        resizeControls(cx, cy);
    }
}

// 新增：添加最小窗口尺寸限制
void BuildBuildingTableWindow::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    lpMMI->ptMinTrackSize.x = MIN_WIDTH;
    lpMMI->ptMinTrackSize.y = MIN_HEIGHT;
    
    CAdUiBaseDialog::OnGetMinMaxInfo(lpMMI);
}

// 使用常量来确保一致性
void BuildBuildingTableWindow::resizeControls(int cx, int cy)
{
    if (!::IsWindow(m_buildingTable.GetSafeHwnd())) {
        return;
    }
    
    // 设置最小窗口尺寸（使用常量）
    if (cx < MIN_WIDTH) cx = MIN_WIDTH;
    if (cy < MIN_HEIGHT) cy = MIN_HEIGHT;
    
    // 调整表格大小 - 减少底部空间，让布局更紧凑
    CRect tableRect;
    m_buildingTable.GetWindowRect(&tableRect);
    ScreenToClient(&tableRect);
    
    tableRect.right = cx - 15;
    tableRect.bottom = cy - 40;  // 从45减少到40，减少5像素的缓冲空间
    m_buildingTable.MoveWindow(&tableRect);
    
    // 重新计算列宽度（新增）
    resizeTableColumns();
    
    // 调整底部按钮位置 - 只处理确定和取消按钮
    CWnd* pOKBtn = GetDlgItem(IDOK);
    CWnd* pCancelBtn = GetDlgItem(IDCANCEL);
    
    if (pOKBtn && pCancelBtn) {
        // 获取按钮的原始宽度，但设置固定高度
        CRect okRect;
        pOKBtn->GetWindowRect(&okRect);
        ScreenToClient(&okRect);
        
        int buttonWidth = okRect.Width();     // 使用原始宽度
        int buttonHeight = BUTTON_HEIGHT;     // 使用常量高度
        int buttonSpacing = 10;               // 按钮间距
        
        // 计算按钮Y坐标：窗口高度 - 8像素底部距离 - 按钮高度
        int buttonY = cy - 8 - buttonHeight;
        
        // 从右到左排列按钮（只有确定和取消）
        int cancelX = cx - 15 - buttonWidth;  // 取消按钮（最右）
        int okX = cancelX - buttonWidth - buttonSpacing;  // 确定按钮
        
        // 移动按钮到新位置，设置固定高度
        pCancelBtn->MoveWindow(cancelX, buttonY, buttonWidth, buttonHeight);
        pOKBtn->MoveWindow(okX, buttonY, buttonWidth, buttonHeight);
    }
    
    // 调整筛选区域的控件位置
    adjustFilterControls(cx);
}

// 修改 adjustFilterControls() 方法，使用常量高度
void BuildBuildingTableWindow::adjustFilterControls(int cx)
{
    // 调整筛选组框的宽度
    CWnd* pFilterGroup = GetDlgItem(IDC_FILTER_GROUP);
    if (pFilterGroup) {
        CRect groupRect;
        pFilterGroup->GetWindowRect(&groupRect);
        ScreenToClient(&groupRect);
        groupRect.right = cx - 15;
        pFilterGroup->MoveWindow(&groupRect);
    }
    
    // 获取筛选区域的控件
    CWnd* pCreateTimePicker = GetDlgItem(IDC_CREATE_TIME_PICKER);
    CWnd* pFilterBtn = GetDlgItem(IDC_FILTER_BUTTON);
    CWnd* pResetBtn = GetDlgItem(IDC_RESET_FILTER_BUTTON);
    
    if (pCreateTimePicker && pFilterBtn && pResetBtn) {
        // 获取日期选择器的位置
        CRect timePickerRect;
        pCreateTimePicker->GetWindowRect(&timePickerRect);
        ScreenToClient(&timePickerRect);
        
        // 获取按钮的原始尺寸
        CRect filterRect, resetRect;
        pFilterBtn->GetWindowRect(&filterRect);
        pResetBtn->GetWindowRect(&resetRect);
        ScreenToClient(&filterRect);
        ScreenToClient(&resetRect);
        
        int buttonWidth = filterRect.Width();   // 保持原始宽度
        int buttonHeight = BUTTON_HEIGHT;       // 使用常量高度
        int buttonY = filterRect.top;           // 保持原始Y坐标
        int buttonSpacing = 8;                  // 按钮间的间距
        int gapFromTimePicker = 15;             // 与日期选择器的间距
        
        // 计算筛选按钮的位置（紧跟在日期选择器右侧）
        int filterX = timePickerRect.right + gapFromTimePicker;
        int resetX = filterX + buttonWidth + buttonSpacing;
        
        // 移动按钮到新位置，使用常量高度
        pFilterBtn->MoveWindow(filterX, buttonY, buttonWidth, buttonHeight);
        pResetBtn->MoveWindow(resetX, buttonY, buttonWidth, buttonHeight);
    }
}

// 实现 startEdit 方法
void BuildBuildingTableWindow::startEdit(int nItem, int nSubItem)
{
    // 如果已经在编辑，先结束当前编辑
    if (m_pEditCtrl) {
        endEdit(true);
    }
    
    // 检查参数有效性
    if (nItem < 0 || nItem >= m_buildingTable.GetItemCount() || 
        nSubItem < 0 || nSubItem >= m_buildingTable.GetHeaderCtrl()->GetItemCount()) {
        return;
    }
    
    // 记录编辑位置
    m_nEditItem = nItem;
    m_nEditSubItem = nSubItem;
    
    // 创建编辑控件
    m_pEditCtrl = createEditControl(nItem, nSubItem);
    if (m_pEditCtrl) {
        m_pEditCtrl->SetFocus();
        m_pEditCtrl->SetSel(0, -1); // 选中所有文本
    }
}

// 实现 endEdit 方法
void BuildBuildingTableWindow::endEdit(bool bSave)
{
    if (!m_pEditCtrl) {
        return;
    }
    
    if (bSave && m_nEditItem >= 0 && m_nEditSubItem >= 0) {
        // 保存编辑的内容
        CString strText;
        m_pEditCtrl->GetWindowText(strText);
        m_buildingTable.SetItemText(m_nEditItem, m_nEditSubItem, strText);
        
        // 记录编辑操作
        CadLogger::LogInfo(_T("表格编辑: 行%d 列%d 新值: %s"), 
                          m_nEditItem, m_nEditSubItem, strText);
    }
    
    // 销毁编辑控件
    m_pEditCtrl->DestroyWindow();
    delete m_pEditCtrl;
    m_pEditCtrl = nullptr;
    
    // 重置编辑位置
    m_nEditItem = -1;
    m_nEditSubItem = -1;
    
    // 重新设置焦点到列表控件
    m_buildingTable.SetFocus();
}

// 实现 createEditControl 方法
CEdit* BuildBuildingTableWindow::createEditControl(int nItem, int nSubItem)
{
    // 获取单元格的矩形区域
    CRect rect;
    if (!m_buildingTable.GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect)) {
        return nullptr;
    }
    
    // 调整矩形位置（相对于列表控件）
    if (nSubItem == 0) {
        // 第一列需要特殊处理
        CRect labelRect;
        m_buildingTable.GetSubItemRect(nItem, nSubItem, LVIR_LABEL, labelRect);
        rect = labelRect;
    }
    
    // 创建编辑控件
    CEdit* pEdit = new CEdit();
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    
    if (!pEdit->Create(dwStyle, rect, &m_buildingTable, 1001)) {
        delete pEdit;
        return nullptr;
    }
    
    // 设置字体（与列表控件相同）
    CFont* pFont = m_buildingTable.GetFont();
    if (pFont) {
        pEdit->SetFont(pFont);
    }
    
    // 设置初始文本
    CString strText = m_buildingTable.GetItemText(nItem, nSubItem);
    pEdit->SetWindowText(strText);
    
    return pEdit;
}

// 重写 PreTranslateMessage 来处理编辑控件的消息
BOOL BuildBuildingTableWindow::PreTranslateMessage(MSG* pMsg)
{
    if (m_pEditCtrl && pMsg->hwnd == m_pEditCtrl->GetSafeHwnd()) {
        if (pMsg->message == WM_KEYDOWN) {
            if (pMsg->wParam == VK_RETURN) {
                // 回车键保存并结束编辑
                endEdit(true);
                return TRUE;
            }
            else if (pMsg->wParam == VK_ESCAPE) {
                // ESC键取消编辑
                endEdit(false);
                return TRUE;
            }
            else if (pMsg->wParam == VK_TAB) {
                // Tab键移动到下一个单元格
                endEdit(true);
                int nextSubItem = m_nEditSubItem + 1;
                if (nextSubItem >= m_buildingTable.GetHeaderCtrl()->GetItemCount()) {
                    nextSubItem = 0;
                    int nextItem = m_nEditItem + 1;
                    if (nextItem < m_buildingTable.GetItemCount()) {
                        startEdit(nextItem, nextSubItem);
                    }
                } else {
                    startEdit(m_nEditItem, nextSubItem);
                }
                return TRUE;
            }
        }
    }
    
    return CAdUiBaseDialog::PreTranslateMessage(pMsg);
}

// 新增方法：重新计算表格列宽度
void BuildBuildingTableWindow::resizeTableColumns()
{
    CRect clientRect;
    m_buildingTable.GetClientRect(&clientRect);
    int totalWidth = clientRect.Width() - 30; // 减少边距
    
    // 如果窗口最大化，使用更精确的宽度计算
    if (IsZoomed()) {
        CRect windowRect;
        GetWindowRect(&windowRect);
        totalWidth = windowRect.Width() - 60;
    }
    
    // 权重数组（与setupTableColumns中保持一致）
    int weights[] = { 18, 25, 10, 8, 20, 12, 7 };
    
    // 重新设置各列宽度
    for (int i = 0; i < 7; i++) {
        int columnWidth = (totalWidth * weights[i]) / 100;
        // 设置最小列宽
        int minWidth;
        switch (i) {
            case 0: minWidth = 80;  break;  // 大楼名称
            case 1: minWidth = 100; break;  // 地址
            case 2: minWidth = 60;  break;  // 总面积
            case 3: minWidth = 40;  break;  // 层数
            case 4: minWidth = 90;  break;  // 设计单位
            case 5: minWidth = 70;  break;  // 创建时间
            case 6: minWidth = 50;  break;  // 创建人
            default: minWidth = 50; break;
        }
        
        if (columnWidth < minWidth) columnWidth = minWidth;
        m_buildingTable.SetColumnWidth(i, columnWidth);
    }
}


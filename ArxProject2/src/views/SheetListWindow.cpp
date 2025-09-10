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
//----- SheetListWindow.cpp : Implementation of SheetListWindow
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "../../resource.h"
#include "SheetListWindow.h"
#include "../models/SheetData.h"
#include "../common/Database/SqlDB.h"
#include "../common/CadLogger.h"
#include <afxdlgs.h>
#include <sstream>
#include <iomanip>

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(SheetListWindow, CAdUiBaseDialog)

// 静态成员初始化
SheetListWindow* SheetListWindow::m_pInstance = nullptr;

BEGIN_MESSAGE_MAP(SheetListWindow, CAdUiBaseDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
	ON_BN_CLICKED(IDC_SHEET_FILTER_BUTTON, &SheetListWindow::OnBnClickedSheetFilterButton)
	ON_BN_CLICKED(IDC_SHEET_RESET_FILTER_BUTTON, &SheetListWindow::OnBnClickedSheetResetFilterButton)
	ON_BN_CLICKED(IDC_SHEET_UPLOAD_BUTTON, &SheetListWindow::OnBnClickedSheetUploadButton)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_NOTIFY(NM_RCLICK, IDC_SHEET_TABLE, &SheetListWindow::OnNMRClickSheetTable)
	ON_COMMAND(ID_CONTEXT_INSERT_ROW, &SheetListWindow::OnContextMenuInsertRow)
	ON_COMMAND(ID_CONTEXT_DELETE_ROW, &SheetListWindow::OnContextMenuDeleteRow)
	ON_COMMAND(ID_CONTEXT_SELECT_FILE, &SheetListWindow::OnContextMenuSelectFile)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SHEET_TABLE, &SheetListWindow::OnLvnItemchangedSheetTable)
	ON_NOTIFY(NM_DBLCLK, IDC_SHEET_TABLE, &SheetListWindow::OnNMDblclkSheetTable)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_SHEET_TABLE, &SheetListWindow::OnLvnEndlabeleditSheetTable)
	ON_NOTIFY(LVN_KEYDOWN, IDC_SHEET_TABLE, &SheetListWindow::OnLvnKeydownSheetTable)
	ON_EN_CHANGE(IDC_SHEET_BUILDING_NAME_EDIT, &SheetListWindow::OnEnChangeSheetBuildingNameEdit)
	ON_BN_CLICKED(IDC_SHEET_SEARCH_GROUP, &SheetListWindow::OnBnClickedSheetSearchGroup)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
// 单例模式实现
SheetListWindow* SheetListWindow::getInstance(CWnd* pParent, HINSTANCE hInstance)
{
	if (m_pInstance == nullptr) {
		m_pInstance = new SheetListWindow(pParent, hInstance);
	}
	return m_pInstance;
}

void SheetListWindow::destroyInstance()
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
SheetListWindow::SheetListWindow(CWnd* pParent, HINSTANCE hInstance)
	: CAdUiBaseDialog(SheetListWindow::IDD, pParent, hInstance)
	, m_contextMenuRow(-1)
	, m_fileManager(nullptr)
	, m_statusManager(nullptr)
	, m_pEditCtrl(nullptr)
	, m_nEditItem(-1)
	, m_nEditSubItem(-1)
{
}

SheetListWindow::~SheetListWindow()
{
	// 清理编辑控件
	if (m_pEditCtrl) {
		EndEdit(false);
	}
	
	// 清理资源
	if (m_fileManager) {
		delete m_fileManager;
		m_fileManager = nullptr;
	}
	if (m_statusManager) {
		delete m_statusManager;
		m_statusManager = nullptr;
	}
	
	// 清空单例指针
	if (m_pInstance == this) {
		m_pInstance = nullptr;
	}
}

//-----------------------------------------------------------------------------
void SheetListWindow::DoDataExchange(CDataExchange* pDX)
{
	CAdUiBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SHEET_BUILDING_NAME_EDIT, m_buildingNameEdit);
	DDX_Control(pDX, IDC_SHEET_SPECIALTY_EDIT, m_specialtyEdit);
	DDX_Control(pDX, IDC_SHEET_DESIGN_UNIT_EDIT, m_designUnitEdit);
	DDX_Control(pDX, IDC_SHEET_CREATE_TIME_PICKER, m_createTimePicker);
	DDX_Control(pDX, IDC_SHEET_FILTER_BUTTON, m_filterButton);
	DDX_Control(pDX, IDC_SHEET_RESET_FILTER_BUTTON, m_resetFilterButton);
	DDX_Control(pDX, IDC_SHEET_UPLOAD_BUTTON, m_uploadButton);
	DDX_Control(pDX, IDC_SHEET_TABLE, m_sheetTable);
}

BOOL SheetListWindow::OnInitDialog()
{
	CAdUiBaseDialog::OnInitDialog();
	
	// 初始化控件
	InitializeControls();
	
	// 设置表格
	SetupSheetTable();
	
	// 创建数据库表并加载数据
	if (CreateSheetTable()) {
		LoadDataFromDatabase();
	} else {
		// 如果创建表失败，使用示例数据
		PopulateTableData();
	}
	
	// 创建右键菜单
	if (m_contextMenu.CreatePopupMenu()) {
		m_contextMenu.AppendMenu(MF_STRING, ID_CONTEXT_INSERT_ROW, _T("插入行"));
		m_contextMenu.AppendMenu(MF_SEPARATOR);
		m_contextMenu.AppendMenu(MF_STRING, ID_CONTEXT_DELETE_ROW, _T("删除行"));
		m_contextMenu.AppendMenu(MF_SEPARATOR);
		m_contextMenu.AppendMenu(MF_STRING, ID_CONTEXT_SELECT_FILE, _T("选择文件"));
	}
	
	// 窗口最大化显示（参照BuildBuildingTableWindow）
	ShowWindow(SW_SHOWMAXIMIZED);
	
	return TRUE;
}

void SheetListWindow::InitializeControls()
{
	// 设置窗口标题
	SetWindowText(_T("图纸列表"));
	
	// 设置日期时间控件的格式和默认值
	CTime currentTime = CTime::GetCurrentTime();
	m_createTimePicker.SetTime(&currentTime);
	
	// 设置表格样式 - 启用编辑功能（参照BuildBuildingTableWindow）
	m_sheetTable.SetExtendedStyle(
		LVS_EX_FULLROWSELECT |      // 整行选择
		LVS_EX_GRIDLINES |          // 显示网格线
		LVS_EX_CHECKBOXES |         // 显示复选框
		LVS_EX_HEADERDRAGDROP |     // 允许拖拽列头
		LVS_EX_LABELTIP |           // 显示标签提示
		LVS_EX_INFOTIP              // 显示信息提示
	);
	
	// 启用表格的标签编辑功能
	DWORD dwStyle = m_sheetTable.GetStyle();
	dwStyle |= LVS_EDITLABELS;
	m_sheetTable.ModifyStyle(0, LVS_EDITLABELS);
}

void SheetListWindow::SetupSheetTable()
{
	// 清除现有列
	while (m_sheetTable.GetHeaderCtrl()->GetItemCount() > 0) {
		m_sheetTable.DeleteColumn(0);
	}
	
	// 插入列，调整列宽以适应800像素宽度的窗口
	m_sheetTable.InsertColumn(COL_SELECT, _T("选择"), LVCFMT_CENTER, 50);
	m_sheetTable.InsertColumn(COL_NAME, _T("图纸名称"), LVCFMT_LEFT, 120);
	m_sheetTable.InsertColumn(COL_BUILDING, _T("大楼名称"), LVCFMT_LEFT, 100);
	m_sheetTable.InsertColumn(COL_SPECIALTY, _T("专业"), LVCFMT_LEFT, 80);
	m_sheetTable.InsertColumn(COL_FORMAT, _T("图纸格式"), LVCFMT_LEFT, 70);
	m_sheetTable.InsertColumn(COL_STATUS, _T("图纸状态"), LVCFMT_LEFT, 70);
	m_sheetTable.InsertColumn(COL_VERSION, _T("版本号"), LVCFMT_LEFT, 60);
	m_sheetTable.InsertColumn(COL_DESIGN_UNIT, _T("设计单位"), LVCFMT_LEFT, 100);
	m_sheetTable.InsertColumn(COL_CREATE_TIME, _T("创建时间"), LVCFMT_LEFT, 80);
	m_sheetTable.InsertColumn(COL_CREATOR, _T("创建人"), LVCFMT_LEFT, 60);
	m_sheetTable.InsertColumn(COL_OPERATION, _T("操作"), LVCFMT_CENTER, 80);
}

void SheetListWindow::PopulateTableData()
{
	// 清空现有数据
	m_sheetDataList.clear();
	m_sheetTable.DeleteAllItems();
	
	// 添加示例数据
	auto data1 = std::make_shared<SheetData>();
	data1->name = _T("建筑平面图");
	data1->buildingName = _T("A座办公楼");
	data1->specialty = _T("结构");
	data1->format = _T("dwg/dxf");
	data1->status = _T("已上传");
	data1->version = _T("v1.0");
	data1->designUnit = _T("建筑设计院");
	data1->createTime = _T("2024-01-15");
	data1->creator = _T("张三");
	data1->isSelected = false;
	data1->filePath = _T("");
	m_sheetDataList.push_back(data1);
	
	auto data2 = std::make_shared<SheetData>();
	data2->name = _T("结构施工图");
	data2->buildingName = _T("B座住宅楼");
	data2->specialty = _T("结构");
	data2->format = _T("pdf");
	data2->status = _T("解析中");
	data2->version = _T("v2.1");
	data2->designUnit = _T("结构设计院");
	data2->createTime = _T("2024-01-16");
	data2->creator = _T("李四");
	data2->isSelected = false;
	data2->filePath = _T("");
	m_sheetDataList.push_back(data2);
	
	auto data3 = std::make_shared<SheetData>();
	data3->name = _T("给排水图");
	data3->buildingName = _T("C座商业楼");
	data3->specialty = _T("机电");
	data3->format = _T("jpeg");
	data3->status = _T("待上传");
	data3->version = _T("v1.5");
	data3->designUnit = _T("机电设计院");
	data3->createTime = _T("2024-01-17");
	data3->creator = _T("王五");
	data3->isSelected = false;
	data3->filePath = _T("");
	m_sheetDataList.push_back(data3);
	
	// 保存原始数据用于筛选恢复
	m_originalDataList = m_sheetDataList;
	
	// 填充表格
	UpdateTableWithFilteredData(m_sheetDataList);
}

void SheetListWindow::UpdateTableWithFilteredData(const std::vector<std::shared_ptr<SheetData>>& filteredData)
{
	// 清空表格
	m_sheetTable.DeleteAllItems();
	
	// 填充数据
	for (size_t i = 0; i < filteredData.size(); ++i) {
		const auto& data = filteredData[i];
		
		// 插入行
		int nItem = m_sheetTable.InsertItem(static_cast<int>(i), _T(""));
		
		// 设置复选框状态
		m_sheetTable.SetCheck(nItem, data->isSelected);
		
		// 填充各列数据
		m_sheetTable.SetItemText(nItem, COL_NAME, data->name.c_str());
		m_sheetTable.SetItemText(nItem, COL_BUILDING, data->buildingName.c_str());
		m_sheetTable.SetItemText(nItem, COL_SPECIALTY, data->specialty.c_str());
		m_sheetTable.SetItemText(nItem, COL_FORMAT, data->format.c_str());
		m_sheetTable.SetItemText(nItem, COL_STATUS, data->status.c_str());
		m_sheetTable.SetItemText(nItem, COL_VERSION, data->version.c_str());
		m_sheetTable.SetItemText(nItem, COL_DESIGN_UNIT, data->designUnit.c_str());
		m_sheetTable.SetItemText(nItem, COL_CREATE_TIME, data->createTime.c_str());
		m_sheetTable.SetItemText(nItem, COL_CREATOR, data->creator.c_str());
		m_sheetTable.SetItemText(nItem, COL_OPERATION, _T("选择文件"));
		
		// 设置行数据指针
		m_sheetTable.SetItemData(nItem, static_cast<DWORD_PTR>(i));
	}
}

//-----------------------------------------------------------------------------
//----- 消息处理函数

LRESULT SheetListWindow::OnAcadKeepFocus(WPARAM, LPARAM)
{
	return TRUE;
}

void SheetListWindow::OnBnClickedSheetFilterButton()
{
	// 获取筛选条件
	CString buildingName, specialty, designUnit;
	m_buildingNameEdit.GetWindowText(buildingName);
	m_specialtyEdit.GetWindowText(specialty);
	m_designUnitEdit.GetWindowText(designUnit);
	
	CTime createTime;
	m_createTimePicker.GetTime(createTime);
	
	// 执行筛选
	auto filteredData = FilterSheetData(buildingName, specialty, designUnit, createTime);
	
	// 更新表格显示
	UpdateTableWithFilteredData(filteredData);
	
	// 显示筛选结果
	CString message;
	message.Format(_T("筛选完成！\n\n筛选条件：\n大楼名称：%s\n专业：%s\n设计单位：%s\n创建时间：%s\n\n筛选结果：共找到 %d 条记录"),
		buildingName.IsEmpty() ? _T("全部") : buildingName,
		specialty.IsEmpty() ? _T("全部") : specialty,
		designUnit.IsEmpty() ? _T("全部") : designUnit,
		createTime.Format(_T("%Y-%m-%d")),
		static_cast<int>(filteredData.size()));
	
	AfxMessageBox(message);
	CadLogger::LogInfo(_T("筛选图纸数据: %s"), message);
}

void SheetListWindow::OnBnClickedSheetResetFilterButton()
{
	ResetFilter();
	AfxMessageBox(_T("筛选条件已重置，显示所有数据。"));
}

void SheetListWindow::OnBnClickedSheetUploadButton()
{
	auto selectedSheets = GetSelectedSheets();
	
	if (selectedSheets.empty()) {
		AfxMessageBox(_T("请先选择要上传的图纸！"));
		return;
	}
	
	// 确认上传
	CString message;
	message.Format(_T("确定要上传 %d 个图纸文件吗？"), static_cast<int>(selectedSheets.size()));
	
	if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) == IDYES) {
		UploadSelectedSheets();
	}
}

void SheetListWindow::OnSize(UINT nType, int cx, int cy)
{
	CAdUiBaseDialog::OnSize(nType, cx, cy);
	
	if (nType != SIZE_MINIMIZED) {
		ResizeControls(cx, cy);
	}
}

void SheetListWindow::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = MIN_WIDTH;
	lpMMI->ptMinTrackSize.y = MIN_HEIGHT;
	
	CAdUiBaseDialog::OnGetMinMaxInfo(lpMMI);
}

void SheetListWindow::OnNMRClickSheetTable(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	if (pNMItemActivate->iItem >= 0) {
		m_contextMenuRow = pNMItemActivate->iItem;
		
		CPoint point;
		GetCursorPos(&point);
		ShowContextMenu(point);
	}
	
	*pResult = 0;
}

void SheetListWindow::OnContextMenuInsertRow()
{
	if (m_contextMenuRow >= 0) {
		InsertNewRow(m_contextMenuRow);
	}
}

void SheetListWindow::OnContextMenuDeleteRow()
{
	if (m_contextMenuRow >= 0 && m_contextMenuRow < m_sheetTable.GetItemCount()) {
		CString message;
		message.Format(_T("确定要删除第 %d 行吗？"), m_contextMenuRow + 1);
		
		if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) == IDYES) {
			DeleteRow(m_contextMenuRow);
		}
	}
}

void SheetListWindow::OnContextMenuSelectFile()
{
	if (m_contextMenuRow >= 0 && m_contextMenuRow < m_sheetTable.GetItemCount()) {
		SelectFileForRow(m_contextMenuRow);
	}
}

void SheetListWindow::OnLvnItemchangedSheetTable(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	
	// 如果正在编辑且选择项发生变化，结束编辑
	if (m_pEditCtrl && pNMLV->uNewState & LVIS_SELECTED) {
		EndEdit(true);
	}
	
	// 处理复选框状态变化
	if (pNMLV->uChanged & LVIF_STATE) {
		int nItem = pNMLV->iItem;
		if (nItem >= 0 && nItem < static_cast<int>(m_sheetDataList.size())) {
			BOOL bChecked = m_sheetTable.GetCheck(nItem);
			m_sheetDataList[nItem]->isSelected = (bChecked == TRUE);
		}
	}
	
	*pResult = 0;
}

void SheetListWindow::OnNMDblclkSheetTable(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	if (pNMItemActivate->iItem >= 0 && pNMItemActivate->iSubItem >= 0) {
		// 双击开始编辑选中的单元格
		StartEdit(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
	}
	
	*pResult = 0;
}

void SheetListWindow::OnLvnEndlabeleditSheetTable(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	
	// 检查是否有有效的编辑文本
	if (pDispInfo->item.pszText != NULL && pDispInfo->item.pszText[0] != '\0') {
		// 接受编辑
		*pResult = TRUE;
		
		// 记录编辑操作
		int nItem = pDispInfo->item.iItem;
		int nSubItem = pDispInfo->item.iSubItem;
		CString newText = pDispInfo->item.pszText;
		
		CadLogger::LogInfo(_T("图纸表格编辑: 行%d 列%d 新值: %s"), nItem, nSubItem, newText);
	} else {
		// 拒绝编辑（如果文本为空或无效）
		*pResult = FALSE;
	}
}

void SheetListWindow::OnLvnKeydownSheetTable(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	
	if (pLVKeyDow->wVKey == VK_F2) {
		// F2键开始编辑当前选中的单元格
		int selectedItem = m_sheetTable.GetNextItem(-1, LVNI_SELECTED);
		if (selectedItem >= 0) {
			StartEdit(selectedItem, 1); // 默认编辑图纸名称列
		}
	}
	else if (pLVKeyDow->wVKey == VK_ESCAPE) {
		// ESC键取消编辑
		if (m_pEditCtrl) {
			EndEdit(false);
		}
	}
	else if (pLVKeyDow->wVKey == VK_RETURN) {
		// 回车键保存编辑
		if (m_pEditCtrl) {
			EndEdit(true);
		}
	}
	
	*pResult = 0;
}

void SheetListWindow::OnOK()
{
	// 如果正在编辑，先结束编辑
	if (m_pEditCtrl) {
		EndEdit(true);
	}
	
	// 保存数据到数据库
	SaveDataToDatabase();
	
	// 隐藏窗口而不是销毁（单例模式）
	ShowWindow(SW_HIDE);
}

void SheetListWindow::OnCancel()
{
	// 如果正在编辑，取消编辑
	if (m_pEditCtrl) {
		EndEdit(false);
	}
	
	// 隐藏窗口而不是销毁（单例模式）
	ShowWindow(SW_HIDE);
}

BOOL SheetListWindow::PreTranslateMessage(MSG* pMsg)
{
	// 处理编辑控件的消息（参照BuildBuildingTableWindow）
	if (m_pEditCtrl && pMsg->hwnd == m_pEditCtrl->GetSafeHwnd()) {
		if (pMsg->message == WM_KEYDOWN) {
			if (pMsg->wParam == VK_RETURN) {
				// 回车键保存并结束编辑
				EndEdit(true);
				return TRUE;
			}
			else if (pMsg->wParam == VK_ESCAPE) {
				// ESC键取消编辑
				EndEdit(false);
				return TRUE;
			}
			else if (pMsg->wParam == VK_TAB) {
				// Tab键移动到下一个单元格
				EndEdit(true);
				int nextSubItem = m_nEditSubItem + 1;
				if (nextSubItem >= m_sheetTable.GetHeaderCtrl()->GetItemCount()) {
					nextSubItem = 1; // 跳过选择列
					int nextItem = m_nEditItem + 1;
					if (nextItem < m_sheetTable.GetItemCount()) {
						StartEdit(nextItem, nextSubItem);
					}
				} else {
					StartEdit(m_nEditItem, nextSubItem);
				}
				return TRUE;
			}
		}
	}
	
	return CAdUiBaseDialog::PreTranslateMessage(pMsg);
}

//-----------------------------------------------------------------------------
//----- 辅助方法实现

void SheetListWindow::ResizeControls(int cx, int cy)
{
	if (!::IsWindow(m_sheetTable.GetSafeHwnd())) {
		return;
	}
	
	// 设置最小窗口尺寸
	if (cx < MIN_WIDTH) cx = MIN_WIDTH;
	if (cy < MIN_HEIGHT) cy = MIN_HEIGHT;
	
	// 调整表格大小 - 为底部按钮留出足够空间
	CRect tableRect;
	m_sheetTable.GetWindowRect(&tableRect);
	ScreenToClient(&tableRect);
	
	tableRect.right = cx - 15;
	tableRect.bottom = cy - 35;  // 为10像素底部距离 + 14像素按钮高度 + 11像素缓冲
	m_sheetTable.MoveWindow(&tableRect);
	
	// 调整底部按钮位置 - 距离底部10像素，按钮高度14像素
	CWnd* pOKBtn = GetDlgItem(IDOK);
	CWnd* pCancelBtn = GetDlgItem(IDCANCEL);
	
	if (pOKBtn && pCancelBtn) {
		// 获取按钮的原始宽度，但设置固定高度为14像素
		CRect okRect;
		pOKBtn->GetWindowRect(&okRect);
		ScreenToClient(&okRect);
		
		int buttonWidth = okRect.Width();   // 使用原始宽度
		int buttonHeight = BUTTON_HEIGHT;   // 固定高度为14像素
		int buttonSpacing = 10;             // 按钮间距
		
		// 计算按钮Y坐标：窗口高度 - 10像素底部距离 - 14像素按钮高度
		int buttonY = cy - 10 - buttonHeight;
		
		// 从右到左排列按钮
		int cancelX = cx - 15 - buttonWidth;  // 取消按钮（最右）
		int okX = cancelX - buttonWidth - buttonSpacing;  // 确定按钮
		
		// 移动按钮到新位置，设置固定高度14像素
		pCancelBtn->MoveWindow(cancelX, buttonY, buttonWidth, buttonHeight);
		pOKBtn->MoveWindow(okX, buttonY, buttonWidth, buttonHeight);
	}
	
	// 调整筛选区域的控件位置
	AdjustFilterControls(cx);
}

void SheetListWindow::AdjustFilterControls(int cx)
{
	// 调整筛选组框的宽度
	CWnd* pFilterGroup = GetDlgItem(IDC_SHEET_SEARCH_GROUP);
	if (pFilterGroup) {
		CRect groupRect;
		pFilterGroup->GetWindowRect(&groupRect);
		ScreenToClient(&groupRect);
		groupRect.right = cx - 15;
		pFilterGroup->MoveWindow(&groupRect);
	}
	
	// 获取筛选区域的控件
	CWnd* pCreateTimePicker = GetDlgItem(IDC_SHEET_CREATE_TIME_PICKER);
	CWnd* pFilterBtn = GetDlgItem(IDC_SHEET_FILTER_BUTTON);
	CWnd* pResetBtn = GetDlgItem(IDC_SHEET_RESET_FILTER_BUTTON);
	CWnd* pUploadBtn = GetDlgItem(IDC_SHEET_UPLOAD_BUTTON);
	
	if (pCreateTimePicker && pFilterBtn && pResetBtn && pUploadBtn) {
		// 获取日期选择器的位置
		CRect timePickerRect;
		pCreateTimePicker->GetWindowRect(&timePickerRect);
		ScreenToClient(&timePickerRect);
		
		// 获取按钮的原始尺寸
		CRect filterRect, resetRect, uploadRect;
		pFilterBtn->GetWindowRect(&filterRect);
		pResetBtn->GetWindowRect(&resetRect);
		pUploadBtn->GetWindowRect(&uploadRect);
		ScreenToClient(&filterRect);
		ScreenToClient(&resetRect);
		ScreenToClient(&uploadRect);
		
		int filterButtonWidth = filterRect.Width();
		int resetButtonWidth = resetRect.Width();
		int uploadButtonWidth = uploadRect.Width();
		int buttonHeight = BUTTON_HEIGHT;  // 使用常量定义的按钮高度
		int buttonY = filterRect.top;
		int buttonSpacing = 10;
		int gapFromTimePicker = 15;
		
		// 计算筛选按钮的位置（紧跟在日期选择器右侧）
		int filterX = timePickerRect.right + gapFromTimePicker;
		int resetX = filterX + filterButtonWidth + buttonSpacing;
		int uploadX = resetX + resetButtonWidth + buttonSpacing;
		
		// 如果上传按钮超出窗口范围，则右对齐
		if (uploadX + uploadButtonWidth > cx - 15) {
			uploadX = cx - 15 - uploadButtonWidth;
			// 如果空间不够，调整重置按钮位置
			if (resetX + resetButtonWidth + buttonSpacing > uploadX) {
				resetX = uploadX - resetButtonWidth - buttonSpacing;
				// 如果还是不够，调整筛选按钮位置
				if (filterX + filterButtonWidth + buttonSpacing > resetX) {
					filterX = resetX - filterButtonWidth - buttonSpacing;
				}
			}
		}
		
		// 移动按钮到新位置，设置统一高度14像素
		pFilterBtn->MoveWindow(filterX, buttonY, filterButtonWidth, buttonHeight);
		pResetBtn->MoveWindow(resetX, buttonY, resetButtonWidth, buttonHeight);
		pUploadBtn->MoveWindow(uploadX, buttonY, uploadButtonWidth, buttonHeight);
	}
}

std::vector<std::shared_ptr<SheetData>> SheetListWindow::FilterSheetData(
	const CString& buildingName,
	const CString& specialty,
	const CString& designUnit,
	const CTime& createTime)
{
	std::vector<std::shared_ptr<SheetData>> filteredData;
	
	CString createTimeStr = createTime.Format(_T("%Y-%m-%d"));
	
	// 转换CString为std::wstring进行比较
	std::wstring wBuildingName = buildingName.IsEmpty() ? L"" : std::wstring(buildingName);
	std::wstring wSpecialty = specialty.IsEmpty() ? L"" : std::wstring(specialty);
	std::wstring wDesignUnit = designUnit.IsEmpty() ? L"" : std::wstring(designUnit);
	std::wstring wCreateTime = createTimeStr.IsEmpty() ? L"" : std::wstring(createTimeStr);
	
	for (const auto& data : m_originalDataList) {
		bool matchBuilding = buildingName.IsEmpty() || 
			data->buildingName.find(wBuildingName) != std::wstring::npos;
		bool matchSpecialty = specialty.IsEmpty() || 
			data->specialty.find(wSpecialty) != std::wstring::npos;
		bool matchDesignUnit = designUnit.IsEmpty() || 
			data->designUnit.find(wDesignUnit) != std::wstring::npos;
		bool matchCreateTime = createTimeStr.IsEmpty() || 
			data->createTime.find(wCreateTime) != std::wstring::npos;
		
		if (matchBuilding && matchSpecialty && matchDesignUnit && matchCreateTime) {
			filteredData.push_back(data);
		}
	}
	
	return filteredData;
}

void SheetListWindow::ResetFilter()
{
	m_buildingNameEdit.SetWindowText(_T(""));
	m_specialtyEdit.SetWindowText(_T(""));
	m_designUnitEdit.SetWindowText(_T(""));
	
	CTime currentTime = CTime::GetCurrentTime();
	m_createTimePicker.SetTime(&currentTime);
	
	// 恢复显示所有数据
	UpdateTableWithFilteredData(m_originalDataList);
	m_sheetDataList = m_originalDataList;
}

void SheetListWindow::InsertNewRow(int row)
{
	// 获取当前选中的行，如果没有选中行则在末尾插入
	int selectedItem = m_sheetTable.GetNextItem(-1, LVNI_SELECTED);
	if (selectedItem == -1) {
		selectedItem = m_sheetTable.GetItemCount();
	}
	
	// 创建新数据
	auto newData = std::make_shared<SheetData>();
	newData->name = L"新建图纸";
	newData->buildingName = L"新建大楼";
	newData->specialty = L"结构";
	newData->format = L"dwg/dxf";
	newData->status = L"待上传";
	newData->version = L"v1.0";
	newData->designUnit = L"新建设计院";
	newData->createTime = GetCurrentTimeString();
	newData->creator = L"系统用户";
	newData->isSelected = false;
	newData->filePath = L"";
	
	// 插入到数据列表
	if (row >= static_cast<int>(m_sheetDataList.size())) {
		m_sheetDataList.push_back(newData);
		m_originalDataList.push_back(newData);
	} else {
		m_sheetDataList.insert(m_sheetDataList.begin() + row, newData);
		m_originalDataList.insert(m_originalDataList.begin() + row, newData);
	}
	
	// 插入新行到表格
	int newItem = m_sheetTable.InsertItem(selectedItem, _T(""));
	m_sheetTable.SetItemText(newItem, COL_NAME, newData->name.c_str());
	m_sheetTable.SetItemText(newItem, COL_BUILDING, newData->buildingName.c_str());
	m_sheetTable.SetItemText(newItem, COL_SPECIALTY, newData->specialty.c_str());
	m_sheetTable.SetItemText(newItem, COL_FORMAT, newData->format.c_str());
	m_sheetTable.SetItemText(newItem, COL_STATUS, newData->status.c_str());
	m_sheetTable.SetItemText(newItem, COL_VERSION, newData->version.c_str());
	m_sheetTable.SetItemText(newItem, COL_DESIGN_UNIT, newData->designUnit.c_str());
	m_sheetTable.SetItemText(newItem, COL_CREATE_TIME, newData->createTime.c_str());
	m_sheetTable.SetItemText(newItem, COL_CREATOR, newData->creator.c_str());
	m_sheetTable.SetItemText(newItem, COL_OPERATION, _T("选择文件"));
	
	// 选中新行并开始编辑
	m_sheetTable.SetItemState(newItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_sheetTable.EditLabel(newItem);
}

void SheetListWindow::DeleteRow(int row)
{
	if (row >= 0 && row < static_cast<int>(m_sheetDataList.size())) {
		// 从数据列表中删除
		m_sheetDataList.erase(m_sheetDataList.begin() + row);
		
		// 同时从原始数据列表中删除
		if (row < static_cast<int>(m_originalDataList.size())) {
			m_originalDataList.erase(m_originalDataList.begin() + row);
		}
		
		// 从表格中删除
		m_sheetTable.DeleteItem(row);
		
		// 选中下一行或上一行
		int itemCount = m_sheetTable.GetItemCount();
		if (itemCount > 0) {
			if (row >= itemCount) {
				row = itemCount - 1;
			}
			m_sheetTable.SetItemState(row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}
	}
}

void SheetListWindow::SelectFileForRow(int row)
{
	if (row >= 0 && row < static_cast<int>(m_sheetDataList.size())) {
		// 文件选择对话框
		CFileDialog dlg(TRUE, nullptr, nullptr,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			_T("AutoCAD文件 (*.dwg;*.dxf)|*.dwg;*.dxf|PDF文件 (*.pdf)|*.pdf|图片文件 (*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.gif)|*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.gif|所有支持的文件 (*.dwg;*.dxf;*.pdf;*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.gif)|*.dwg;*.dxf;*.pdf;*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.gif||"),
			this);
		
		if (dlg.DoModal() == IDOK) {
			CString fileName = dlg.GetPathName();
			CString fileTitle = dlg.GetFileTitle();
			CString fileExt = dlg.GetFileExt();
			
			// 更新数据
			auto& data = m_sheetDataList[row];
			data->filePath = fileName;
			data->name = fileTitle;
			
			// 根据文件扩展名确定格式
			fileExt.MakeLower();
			if (fileExt == _T("dwg") || fileExt == _T("dxf")) {
				data->format = _T("dwg/dxf");
			} else if (fileExt == _T("pdf")) {
				data->format = _T("pdf");
			} else if (fileExt == _T("png") || fileExt == _T("jpg") || fileExt == _T("jpeg") ||
				fileExt == _T("bmp") || fileExt == _T("tiff") || fileExt == _T("gif")) {
				data->format = _T("图片");
			} else {
				data->format = _T("未知格式");
			}
			
			// 更新表格显示
			m_sheetTable.SetItemText(row, COL_NAME, data->name.c_str());
			m_sheetTable.SetItemText(row, COL_FORMAT, data->format.c_str());
			
			CString message;
			message.Format(_T("已选择文件：%s\n图纸名称：%s\n图纸格式：%s"),
				fileName, data->name, data->format);
			AfxMessageBox(message);
			
			CadLogger::LogInfo(_T("选择图纸文件: %s"), fileName);
		}
	}
}

std::vector<std::shared_ptr<SheetData>> SheetListWindow::GetSelectedSheets()
{
	std::vector<std::shared_ptr<SheetData>> selectedSheets;
	
	for (const auto& data : m_sheetDataList) {
		if (data->isSelected && !data->filePath.empty()) {
			selectedSheets.push_back(data);
		}
	}
	
	return selectedSheets;
}

void SheetListWindow::UploadSelectedSheets()
{
	// 这里将在后续实现业务逻辑时调用文件管理器
	AfxMessageBox(_T("上传功能将在后续实现业务逻辑时完成"));
	CadLogger::LogInfo(_T("图纸上传功能调用"));
}

void SheetListWindow::ShowContextMenu(CPoint point)
{
	if (m_contextMenu.GetSafeHmenu()) {
		// 如果没有选中行，禁用删除操作
		int selectedItem = m_sheetTable.GetNextItem(-1, LVNI_SELECTED);
		if (selectedItem == -1) {
			m_contextMenu.EnableMenuItem(ID_CONTEXT_DELETE_ROW, MF_GRAYED);
		} else {
			m_contextMenu.EnableMenuItem(ID_CONTEXT_DELETE_ROW, MF_ENABLED);
		}
		
		m_contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	}
}

// 编辑功能实现（参照BuildBuildingTableWindow）
void SheetListWindow::StartEdit(int nItem, int nSubItem)
{
	// 如果已经在编辑，先结束当前编辑
	if (m_pEditCtrl) {
		EndEdit(true);
	}
	
	// 检查参数有效性
	if (nItem < 0 || nItem >= m_sheetTable.GetItemCount() || 
		nSubItem < 0 || nSubItem >= m_sheetTable.GetHeaderCtrl()->GetItemCount()) {
		return;
	}
	
	// 跳过选择列（第0列）
	if (nSubItem == 0) {
		nSubItem = 1;
	}
	
	// 记录编辑位置
	m_nEditItem = nItem;
	m_nEditSubItem = nSubItem;
	
	// 创建编辑控件
	m_pEditCtrl = CreateEditControl(nItem, nSubItem);
	if (m_pEditCtrl) {
		m_pEditCtrl->SetFocus();
		m_pEditCtrl->SetSel(0, -1); // 选中所有文本
	}
}

void SheetListWindow::EndEdit(bool bSave)
{
	if (!m_pEditCtrl) {
		return;
	}
	
	if (bSave && m_nEditItem >= 0 && m_nEditSubItem >= 0) {
		// 保存编辑的内容
		CString strText;
		m_pEditCtrl->GetWindowText(strText);
		m_sheetTable.SetItemText(m_nEditItem, m_nEditSubItem, strText);
		
		// 记录编辑操作
		CadLogger::LogInfo(_T("图纸表格编辑: 行%d 列%d 新值: %s"), 
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
	m_sheetTable.SetFocus();
}

CEdit* SheetListWindow::CreateEditControl(int nItem, int nSubItem)
{
	// 获取单元格的矩形区域
	CRect rect;
	if (!m_sheetTable.GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect)) {
		return nullptr;
	}
	
	// 调整矩形位置（相对于列表控件）
	if (nSubItem == 0) {
		// 第一列需要特殊处理
		CRect labelRect;
		m_sheetTable.GetSubItemRect(nItem, nSubItem, LVIR_LABEL, labelRect);
		rect = labelRect;
	}
	
	// 创建编辑控件
	CEdit* pEdit = new CEdit();
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
	
	if (!pEdit->Create(dwStyle, rect, &m_sheetTable, 1001)) {
		delete pEdit;
		return nullptr;
	}
	
	// 设置字体（与列表控件相同）
	CFont* pFont = m_sheetTable.GetFont();
	if (pFont) {
		pEdit->SetFont(pFont);
	}
	
	// 设置初始文本
	CString strText = m_sheetTable.GetItemText(nItem, nSubItem);
	pEdit->SetWindowText(strText);
	
	return pEdit;
}

std::wstring SheetListWindow::GetCurrentTimeString()
{
	CTime currentTime = CTime::GetCurrentTime();
	CString timeStr = currentTime.Format(_T("%Y-%m-%d"));
	return std::wstring(timeStr);
}

bool SheetListWindow::CreateSheetTable()
{
	// 这里将在后续实现业务逻辑时调用数据库管理器
	std::wstring sql = L"CREATE TABLE IF NOT EXISTS sheet_info ("
		L"id SERIAL PRIMARY KEY, "
		L"name VARCHAR(255) NOT NULL, "
		L"building_name VARCHAR(255), "
		L"specialty VARCHAR(100), "
		L"format VARCHAR(50), "
		L"status VARCHAR(50), "
		L"version VARCHAR(20), "
		L"design_unit VARCHAR(255), "
		L"create_time DATE DEFAULT CURRENT_DATE, "
		L"creator VARCHAR(100) DEFAULT '系统用户', "
		L"is_selected BOOLEAN DEFAULT FALSE, "
		L"file_path TEXT, "
		L"created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
		L")";
	
	std::wstring errorMsg;
	if (!SqlDB::executeQuery(sql, errorMsg)) {
		CString errMsg(errorMsg.c_str());
		CadLogger::LogError(_T("创建图纸信息表失败: %s"), errMsg);
		return false;
	}
	
	CadLogger::LogInfo(_T("图纸信息表创建成功或已存在"));
	return true;
}

void SheetListWindow::LoadDataFromDatabase()
{
	// 这里将在后续实现业务逻辑时调用数据库管理器
	CadLogger::LogInfo(_T("从数据库加载图纸数据"));
	PopulateTableData(); // 暂时使用示例数据
}

void SheetListWindow::SaveDataToDatabase()
{
	// 这里将在后续实现业务逻辑时调用数据库管理器
	CadLogger::LogInfo(_T("保存图纸数据到数据库"));
}

void SheetListWindow::OnEnChangeSheetBuildingNameEdit()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CAdUiBaseDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}

void SheetListWindow::OnBnClickedSheetSearchGroup()
{
	// TODO: 在此添加控件通知处理程序代码
}

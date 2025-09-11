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
#include <src/services/SearchTextInDwg.h>

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(SheetListWindow, CAdUiBaseDialog)

// 移除静态成员初始化
// SheetListWindow* SheetListWindow::m_pInstance = nullptr;

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
	ON_WM_TIMER()  // 新增：定时器消息映射
	ON_MESSAGE(WM_UPLOAD_PROGRESS, OnUploadProgressMessage)
	ON_MESSAGE(WM_UPLOAD_COMPLETED, OnUploadCompletedMessage)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
// 单例模式实现
// 移除单例方法实现
/*
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
*/

//-----------------------------------------------------------------------------
SheetListWindow::SheetListWindow(CWnd* pParent, HINSTANCE hInstance)
	: CAdUiBaseDialog(SheetListWindow::IDD, pParent, hInstance)
	, m_contextMenuRow(-1)
	, m_pEditCtrl(nullptr)
	, m_pComboCtrl(nullptr)  // 新增
	, m_pButtonCtrl(nullptr) // 新增
	, m_nEditItem(-1)
	, m_nEditSubItem(-1)
	, m_fileManager(nullptr)      // 添加这行
	, m_statusManager(nullptr)    // 添加这行
	, m_isUploading(false)  // 添加这行
{
}

SheetListWindow::~SheetListWindow()
{
	// 清理编辑控件
	if (m_pEditCtrl) {
		EndEdit(false);
	}
	if (m_pComboCtrl) {
		m_pComboCtrl->DestroyWindow();
		delete m_pComboCtrl;
		m_pComboCtrl = nullptr;
	}
	if (m_pButtonCtrl) {
		m_pButtonCtrl->DestroyWindow();
		delete m_pButtonCtrl;
		m_pButtonCtrl = nullptr;
	}
	
	// 清理文件管理器
	if (m_fileManager) {
		delete m_fileManager;
		m_fileManager = nullptr;
	}
	
	if (m_statusManager) {
		delete m_statusManager;
		m_statusManager = nullptr;
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
	// 添加进度条控件绑定（如果资源中有对应ID）
	// DDX_Control(pDX, IDC_UPLOAD_PROGRESS, m_uploadProgressCtrl);
	// DDX_Control(pDX, IDC_UPLOAD_STATUS, m_uploadStatusLabel);
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
	
	// 初始化文件管理器
	try {
		m_fileManager = new SheetFileManager();
		m_statusManager = new SheetStatusManager();
		
		// 设置回调函数
		m_fileManager->setUploadProgressCallback([this](const std::wstring& fileName, __int64 bytesSent, __int64 bytesTotal) {
			// 创建数据结构
			UploadProgressData* pData = new UploadProgressData();
			pData->fileName = fileName;
			pData->bytesSent = bytesSent;
			pData->bytesTotal = bytesTotal;
			
			// 发送消息到主线程
			PostMessage(WM_UPLOAD_PROGRESS, 0, reinterpret_cast<LPARAM>(pData));
		});
		
		m_fileManager->setUploadCompletedCallback([this](const std::wstring& fileName, bool success) {
			// 创建数据结构
			UploadCompletedData* pData = new UploadCompletedData();
			pData->fileName = fileName;
			pData->success = success;
			
			// 发送消息到主线程
			PostMessage(WM_UPLOAD_COMPLETED, 0, reinterpret_cast<LPARAM>(pData));
		});
		
		m_statusManager->setStatusChangedCallback([this](const std::wstring& fileName, SheetStatusManager::Status oldStatus, SheetStatusManager::Status newStatus) {
			OnFileStatusChanged(fileName, oldStatus, newStatus);
		});
		
		CadLogger::LogInfo(_T("文件管理器初始化成功"));
		
	} catch (...) {
		CadLogger::LogError(_T("文件管理器初始化失败"));
		m_fileManager = nullptr;
		m_statusManager = nullptr;
	}
	
	// 创建进度条和状态标签
	CreateProgressControls();
	
	// 窗口最大化显示（参照BuildBuildingTableWindow）
	//ShowWindow(SW_SHOWMAXIMIZED);
	
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
	
	// 设置行高以容纳按钮控件
	CImageList imageList;
	imageList.Create(1, 24, ILC_COLOR, 1, 1); // 设置24像素高度
	m_sheetTable.SetImageList(&imageList, LVSIL_SMALL);
	imageList.Detach(); // 分离图像列表，让表格控件管理
	
	// 设置表头字体为粗体
	CHeaderCtrl* pHeader = m_sheetTable.GetHeaderCtrl();
	if (pHeader) {
		CFont* pFont = GetFont();
		if (pFont) {
			LOGFONT lf;
			pFont->GetLogFont(&lf);
			lf.lfWeight = FW_BOLD;  // 设置为粗体
			m_headerFont.CreateFontIndirect(&lf);
			pHeader->SetFont(&m_headerFont);
		}
	}
}

void SheetListWindow::SetupSheetTable()
{
	// 清除现有列
	while (m_sheetTable.GetHeaderCtrl()->GetItemCount() > 0) {
		m_sheetTable.DeleteColumn(0);
	}
	
	// 获取表格客户区宽度用于按权重分配列宽
	CRect clientRect;
	m_sheetTable.GetClientRect(&clientRect);
	int totalWidth = clientRect.Width() - 30; // 减少边距，确保有足够空间
	
	// 如果窗口最大化，使用更精确的宽度计算
	if (IsZoomed()) {
		CRect windowRect;
		GetWindowRect(&windowRect);
		// 使用窗口实际宽度减去边框和滚动条
		totalWidth = windowRect.Width() - 60; // 减去窗口边框、滚动条等
	}
	
	// 重新调整权重分配，确保所有列都能在窗口内显示
	struct ColumnInfo {
		LPCTSTR title;
		int weight;
		int format;
	};
	
	ColumnInfo columns[] = {
		{ _T("选择"), 4, LVCFMT_CENTER },      // 4%
		{ _T("图纸名称"), 16, LVCFMT_LEFT },   // 16% - 减少
		{ _T("大楼名称"), 14, LVCFMT_LEFT },   // 14% - 减少
		{ _T("专业"), 9, LVCFMT_LEFT },        // 9% - 减少
		{ _T("图纸格式"), 8, LVCFMT_LEFT },    // 8% - 减少
		{ _T("图纸状态"), 8, LVCFMT_LEFT },    // 8% - 减少
		{ _T("版本号"), 6, LVCFMT_LEFT },      // 6%
		{ _T("设计单位"), 12, LVCFMT_LEFT },   // 12% - 减少
		{ _T("创建时间"), 8, LVCFMT_LEFT },    // 8%
		{ _T("创建人"), 6, LVCFMT_LEFT },      // 6% - 减少
		{ _T("操作"), 9, LVCFMT_CENTER }       // 9% - 确保操作列可见
	};
	
	// 插入列并按权重设置宽度
	for (int i = 0; i < 11; i++) {
		int columnWidth = (totalWidth * columns[i].weight) / 100;
		// 设置最小列宽，操作列需要更大的最小宽度
		int minWidth = (i == COL_OPERATION) ? 80 : 50;
		if (columnWidth < minWidth) columnWidth = minWidth;
		m_sheetTable.InsertColumn(i, columns[i].title, columns[i].format, columnWidth);
	}
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
		
		// 操作列显示按钮文本
		if (data->filePath.empty()) {
			m_sheetTable.SetItemText(nItem, COL_OPERATION, _T("选择文件"));
		} else {
			m_sheetTable.SetItemText(nItem, COL_OPERATION, _T("重新选择"));
		}
		
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
	
	if (pNMItemActivate->iItem >= 0) {
		// 如果双击的是操作列，创建按钮控件
		if (pNMItemActivate->iSubItem == COL_OPERATION) {
			CreateButtonControl(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
		}
		// 如果双击的是专业列，创建下拉框控件
		else if (pNMItemActivate->iSubItem == COL_SPECIALTY) {
			CreateComboControl(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
		}
		// 如果双击的是其他列，开始编辑
		else if (pNMItemActivate->iSubItem > 0) {
			StartEdit(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
		}
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
	
	// 关闭对话框
	CAdUiBaseDialog::OnOK();
}

void SheetListWindow::OnCancel()
{
	// 如果正在编辑，取消编辑
	if (m_pEditCtrl) {
		EndEdit(false);
	}
	
	// 关闭对话框
	CAdUiBaseDialog::OnCancel();
}

BOOL SheetListWindow::PreTranslateMessage(MSG* pMsg)
{
	// 处理编辑控件的消息
	if (m_pEditCtrl && pMsg->hwnd == m_pEditCtrl->GetSafeHwnd()) {
		if (pMsg->message == WM_KEYDOWN) {
			if (pMsg->wParam == VK_RETURN) {
				EndEdit(true);
				return TRUE;
			}
			else if (pMsg->wParam == VK_ESCAPE) {
				EndEdit(false);
				return TRUE;
			}
		}
	}
	
	// 处理下拉框控件的消息
	if (m_pComboCtrl && pMsg->hwnd == m_pComboCtrl->GetSafeHwnd()) {
		if (pMsg->message == WM_KEYDOWN) {
			if (pMsg->wParam == VK_RETURN) {
				EndEdit(true);
				return TRUE;
			}
			else if (pMsg->wParam == VK_ESCAPE) {
				EndEdit(false);
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
	
	// 重新计算列宽度
	ResizeTableColumns();
	
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
	
	// 调整进度条位置
	if (::IsWindow(m_uploadProgressCtrl.GetSafeHwnd())) {
		CRect uploadBtnRect;
		m_uploadButton.GetWindowRect(&uploadBtnRect);
		ScreenToClient(&uploadBtnRect);
		
		CRect progressRect;
		progressRect.left = uploadBtnRect.left;
		progressRect.right = uploadBtnRect.right + 200;
		progressRect.top = uploadBtnRect.bottom + 10;
		progressRect.bottom = progressRect.top + 20;
		
		m_uploadProgressCtrl.MoveWindow(&progressRect);
		
		// 调整状态标签位置
		CRect labelRect;
		labelRect.left = progressRect.left;
		labelRect.right = progressRect.right;
		labelRect.top = progressRect.bottom + 5;
		labelRect.bottom = labelRect.top + 20;
		
		m_uploadStatusLabel.MoveWindow(&labelRect);
	}
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
	// 处理编辑控件
	if (m_pEditCtrl) {
		if (bSave && m_nEditItem >= 0 && m_nEditSubItem >= 0) {
			CString strText;
			m_pEditCtrl->GetWindowText(strText);
			m_sheetTable.SetItemText(m_nEditItem, m_nEditSubItem, strText);
			
			// 同步更新底层数据模型
			UpdateDataModel(m_nEditItem, m_nEditSubItem, strText);
			
			CadLogger::LogInfo(_T("图纸表格编辑: 行%d 列%d 新值: %s"), 
							  m_nEditItem, m_nEditSubItem, strText);
		}
		m_pEditCtrl->DestroyWindow();
		delete m_pEditCtrl;
		m_pEditCtrl = nullptr;
	}
	
	// 处理下拉框控件
	if (m_pComboCtrl) {
		if (bSave && m_nEditItem >= 0 && m_nEditSubItem >= 0) {
			CString strText;
			m_pComboCtrl->GetWindowText(strText);
			m_sheetTable.SetItemText(m_nEditItem, m_nEditSubItem, strText);
			
			// 同步更新底层数据模型
			UpdateDataModel(m_nEditItem, m_nEditSubItem, strText);
			
			CadLogger::LogInfo(_T("图纸表格专业选择: 行%d 新值: %s"), m_nEditItem, strText);
		}
		m_pComboCtrl->DestroyWindow();
		delete m_pComboCtrl;
		m_pComboCtrl = nullptr;
	}
	
	// 处理按钮控件
	if (m_pButtonCtrl) {
		m_pButtonCtrl->DestroyWindow();
		delete m_pButtonCtrl;
		m_pButtonCtrl = nullptr;
	}
	
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

CComboBox* SheetListWindow::CreateComboControl(int nItem, int nSubItem)
{
	// 如果已经在编辑，先结束当前编辑
	EndEdit(true);
	
	// 检查参数有效性
	if (nItem < 0 || nItem >= m_sheetTable.GetItemCount() || 
		nSubItem != COL_SPECIALTY) {
		return nullptr;
	}
	
	// 记录编辑位置
	m_nEditItem = nItem;
	m_nEditSubItem = nSubItem;
	
	// 获取单元格的矩形区域
	CRect rect;
	if (!m_sheetTable.GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect)) {
		return nullptr;
	}
	
	// 创建下拉框控件
	m_pComboCtrl = new CComboBox();
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL;
	
	// 调整高度以容纳下拉列表
	rect.bottom += 150;
	
	if (!m_pComboCtrl->Create(dwStyle, rect, &m_sheetTable, 1002)) {
		delete m_pComboCtrl;
		m_pComboCtrl = nullptr;
		return nullptr;
	}
	
	// 初始化专业选项
	InitializeSpecialtyCombo(m_pComboCtrl);
	
	// 设置字体
	CFont* pFont = m_sheetTable.GetFont();
	if (pFont) {
		m_pComboCtrl->SetFont(pFont);
	}
	
	// 设置当前选中项
	CString currentText = m_sheetTable.GetItemText(nItem, nSubItem);
	int index = m_pComboCtrl->FindStringExact(-1, currentText);
	if (index != CB_ERR) {
		m_pComboCtrl->SetCurSel(index);
	}
	
	m_pComboCtrl->SetFocus();
	m_pComboCtrl->ShowDropDown(TRUE);
	
	return m_pComboCtrl;
}

CButton* SheetListWindow::CreateButtonControl(int nItem, int nSubItem)
{
	// 如果已经在编辑，先结束当前编辑
	EndEdit(true);
	
	// 检查参数有效性
	if (nItem < 0 || nItem >= m_sheetTable.GetItemCount() || 
		nSubItem != COL_OPERATION) {
		return nullptr;
	}
	
	// 记录编辑位置
	m_nEditItem = nItem;
	m_nEditSubItem = nSubItem;
	
	// 获取单元格的矩形区域
	CRect rect;
	if (!m_sheetTable.GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect)) {
		return nullptr;
	}
	
	// 调整按钮大小，确保有足够的高度和边距
	rect.DeflateRect(3, 2); // 减少边距以获得更大的按钮区域
	
	// 确保按钮有足够的高度
	if (rect.Height() < 20) {
		int centerY = rect.CenterPoint().y;
		rect.top = centerY - 10;
		rect.bottom = centerY + 10;
	}
	
	// 创建按钮控件
	m_pButtonCtrl = new CButton();
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER | WS_BORDER;
	
	if (!m_pButtonCtrl->Create(_T("选择文件"), dwStyle, rect, &m_sheetTable, 1003)) {
		delete m_pButtonCtrl;
		m_pButtonCtrl = nullptr;
		return nullptr;
	}
	
	// 设置字体
	CFont* pFont = m_sheetTable.GetFont();
	if (pFont) {
		m_pButtonCtrl->SetFont(pFont);
	}
	
	// 设置按钮为默认按钮样式，确保有边框
	m_pButtonCtrl->ModifyStyle(0, BS_DEFPUSHBUTTON);
	
	// 强制重绘按钮
	m_pButtonCtrl->Invalidate();
	m_pButtonCtrl->UpdateWindow();
	
	// 设置定时器，延迟执行文件选择操作
	SetTimer(1004, 100, nullptr); // 100ms后执行
	
	return m_pButtonCtrl;
}

std::wstring SheetListWindow::GetCurrentTimeString()
{
	CTime currentTime = CTime::GetCurrentTime();
	CString timeStr = currentTime.Format(_T("%Y-%m-%d"));
	return std::wstring(timeStr);
}

//-----------------------------------------------------------------------------
//----- 数据库操作方法实现

void SheetListWindow::LoadDataFromDatabase()
{
	CadLogger::LogInfo(_T("开始从数据库加载图纸数据"));
	
	// 清空现有数据
	m_sheetDataList.clear();
	m_originalDataList.clear();
	
	try {
		// 使用SqlDB加载数据
		std::vector<DbSheetData> dbSheetList;
		std::wstring errorMsg;
		
		if (!SqlDB::loadSheetData(dbSheetList, errorMsg)) {
			CadLogger::LogError(_T("从数据库加载图纸数据失败: %s"), errorMsg.c_str());
			// 如果加载失败，使用示例数据
			PopulateTableData();
			return;
		}
		
		// 检查是否获取到数据
		if (dbSheetList.empty()) {
			CadLogger::LogInfo(_T("数据库中没有图纸数据，使用示例数据"));
			// 如果数据库为空，使用示例数据
			PopulateTableData();
			return;
		}
		
		// 转换数据格式
		for (const auto& dbSheet : dbSheetList) {
			auto sheetData = std::make_shared<SheetData>();
			sheetData->name = dbSheet.name;
			sheetData->buildingName = dbSheet.buildingName;
			sheetData->specialty = dbSheet.specialty;
			sheetData->format = dbSheet.format;
			sheetData->status = dbSheet.status;
			sheetData->version = dbSheet.version;
			sheetData->designUnit = dbSheet.designUnit;
			sheetData->createTime = dbSheet.createTime;
			sheetData->creator = dbSheet.creator;
			sheetData->isSelected = dbSheet.isSelected;
			sheetData->filePath = dbSheet.filePath;
			
			m_sheetDataList.push_back(sheetData);
		}
		
		// 保存原始数据用于筛选恢复
		m_originalDataList = m_sheetDataList;
		
		// 更新表格显示
		UpdateTableWithFilteredData(m_sheetDataList);
		
		CadLogger::LogInfo(_T("成功从数据库加载 %d 条图纸数据"), static_cast<int>(m_sheetDataList.size()));
		
	} catch (...) {
		CadLogger::LogError(_T("加载图纸数据时发生异常"));
		// 异常时使用示例数据
		PopulateTableData();
	}
}

void SheetListWindow::SaveDataToDatabase()
{
	CadLogger::LogInfo(_T("开始保存图纸数据到数据库"));
	
	try {
		// 转换数据格式
		std::vector<DbSheetData> dbSheetList;
		
		for (const auto& sheet : m_sheetDataList) {
			DbSheetData dbSheet;
			dbSheet.id = 0; // 数据库自动生成ID
			dbSheet.name = sheet->name;
			dbSheet.buildingName = sheet->buildingName;
			dbSheet.specialty = sheet->specialty;
			dbSheet.format = sheet->format;
			dbSheet.status = sheet->status;
			dbSheet.version = sheet->version;
			dbSheet.designUnit = sheet->designUnit;
			dbSheet.createTime = sheet->createTime;
			dbSheet.creator = sheet->creator;
			dbSheet.isSelected = sheet->isSelected;
			dbSheet.filePath = sheet->filePath;
			
			dbSheetList.push_back(dbSheet);
		}
		
		// 保存到数据库
		std::wstring errorMsg;
		if (!SqlDB::saveSheetData(dbSheetList, errorMsg)) {
			CadLogger::LogError(_T("保存图纸数据到数据库失败: %s"), errorMsg.c_str());
			AfxMessageBox(_T("保存数据失败！"));
			return;
		}
		
		CadLogger::LogInfo(_T("成功保存 %d 条图纸数据到数据库"), static_cast<int>(dbSheetList.size()));
		
	} catch (...) {
		CadLogger::LogError(_T("保存图纸数据时发生异常"));
		AfxMessageBox(_T("保存数据时发生异常！"));
	}
}

bool SheetListWindow::CreateSheetTable()
{
	CadLogger::LogInfo(_T("开始创建图纸数据表"));
	
	try {
		// 定义表结构
		std::wstring tableStructure = LR"(
			(
				id SERIAL PRIMARY KEY,
				name VARCHAR(255) NOT NULL,
				building_name VARCHAR(255),
				specialty VARCHAR(100),
				format VARCHAR(50),
				status VARCHAR(50),
				version VARCHAR(20),
				design_unit VARCHAR(255),
				create_time VARCHAR(50),
				creator VARCHAR(100),
				is_selected BOOLEAN DEFAULT FALSE,
				file_path TEXT,
				created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
				updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
			)
		)";
		
		// 创建表
		if (!SqlDB::createSheetTable(L"sheet_data", tableStructure)) {
			CadLogger::LogError(_T("创建图纸数据表失败"));
			return false;
		}
		
		CadLogger::LogInfo(_T("图纸数据表创建成功"));
		return true;
		
	} catch (...) {
		CadLogger::LogError(_T("创建图纸数据表时发生异常"));
		return false;
	}
}

//-----------------------------------------------------------------------------
//----- 文件上传方法实现

void SheetListWindow::UploadSelectedSheets()
{
	CadLogger::LogInfo(_T("开始上传选中的图纸"));
	
	try {
		// 获取选中的图纸
		auto selectedSheets = GetSelectedSheets();
		
		if (selectedSheets.empty()) {
			AfxMessageBox(_T("请先选择要上传的图纸！"));
			return;
		}
		
		// 检查文件管理器是否已初始化
		if (!m_fileManager) {
			m_fileManager = new SheetFileManager();
			
			// 设置回调函数
			m_fileManager->setUploadProgressCallback([this](const std::wstring& fileName, __int64 bytesSent, __int64 bytesTotal) {
				// 创建数据结构
				UploadProgressData* pData = new UploadProgressData();
				pData->fileName = fileName;
				pData->bytesSent = bytesSent;
				pData->bytesTotal = bytesTotal;
				
				// 发送消息到主线程
				PostMessage(WM_UPLOAD_PROGRESS, 0, reinterpret_cast<LPARAM>(pData));
			});
			
			m_fileManager->setUploadCompletedCallback([this](const std::wstring& fileName, bool success) {
				// 创建数据结构
				UploadCompletedData* pData = new UploadCompletedData();
				pData->fileName = fileName;
				pData->success = success;
				
				// 发送消息到主线程
				PostMessage(WM_UPLOAD_COMPLETED, 0, reinterpret_cast<LPARAM>(pData));
			});
		}
		
		if (!m_statusManager) {
			m_statusManager = new SheetStatusManager();
			
			// 设置状态变化回调
			m_statusManager->setStatusChangedCallback([this](const std::wstring& fileName, SheetStatusManager::Status oldStatus, SheetStatusManager::Status newStatus) {
				OnFileStatusChanged(fileName, oldStatus, newStatus);
			});
		}
		
		// 确认上传
		CString message;
		message.Format(_T("确定要上传 %d 个图纸文件吗？\n\n选中的文件：\n"), static_cast<int>(selectedSheets.size()));
		
		for (const auto& sheet : selectedSheets) {
			CString fileName = PathFindFileName(sheet->filePath.c_str());
			message += fileName + _T("\n");
		}
		
		if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) != IDYES) {
			return;
		}
		
		// 创建上传任务
		int taskCount = 0;
		for (const auto& sheet : selectedSheets) {
			// 检查文件是否存在
			if (!PathFileExists(sheet->filePath.c_str())) {
				CString msg;
				msg.Format(_T("文件不存在：%s"), sheet->filePath.c_str());
				CadLogger::LogWarning(msg);
				continue;
			}
			
			// 生成服务器文件名
			std::wstring serverFileName = m_fileManager->generateFileName(
				sheet->buildingName, sheet->specialty, sheet->version,
				sheet->designUnit, sheet->creator, sheet->name);
			
			// 添加文件扩展名
			CString ext = PathFindExtension(sheet->filePath.c_str());
			serverFileName += ext.GetString();
			
			// 创建上传任务
			UploadTask task;
			task.localFilePath = sheet->filePath;
			task.serverFileName = serverFileName;
			task.buildingName = sheet->buildingName;
			task.specialty = sheet->specialty;
			task.version = sheet->version;
			task.designUnit = sheet->designUnit;
			task.creator = sheet->creator;
			task.sheetName = sheet->name;
			GetSystemTime(&task.createTime);
			task.retryCount = 0;
			task.isCompleted = false;
			
			// 添加到上传队列
			m_fileManager->addUploadTask(task);
			
			// 设置初始状态
			m_statusManager->setStatus(serverFileName, SheetStatusManager::PendingUpload);
			
			taskCount++;
		}
		
		if (taskCount > 0) {
			// 开始批量上传
			m_fileManager->startBatchUpload();
			
			CString msg;
			msg.Format(_T("已添加 %d 个上传任务，正在后台处理..."), taskCount);
			AfxMessageBox(msg);
			
			CadLogger::LogInfo(_T("已添加 %d 个上传任务"), taskCount);
		} else {
			AfxMessageBox(_T("没有有效的文件可以上传！"));
		}
		
	} catch (...) {
		CadLogger::LogError(_T("上传图纸时发生异常"));
		AfxMessageBox(_T("上传过程中发生异常！"));
	}
}

std::vector<std::shared_ptr<SheetData>> SheetListWindow::GetSelectedSheets()
{
	std::vector<std::shared_ptr<SheetData>> selectedSheets;
	
	try {
		for (const auto& sheet : m_sheetDataList) {
			if (sheet->isSelected && !sheet->filePath.empty()) {
				// 检查文件是否存在
				if (PathFileExists(sheet->filePath.c_str())) {
					selectedSheets.push_back(sheet);
				} else {
					CadLogger::LogWarning(_T("选中的文件不存在: %s"), sheet->filePath.c_str());
				}
			}
		}
		
	} catch (...) {
		CadLogger::LogError(_T("获取选中图纸时发生异常"));
	}
	
	return selectedSheets;
}

//-----------------------------------------------------------------------------
//----- 上传回调方法实现

void SheetListWindow::OnUploadProgress(const std::wstring& fileName, __int64 bytesSent, __int64 bytesTotal)
{
    try {
        if (bytesTotal > 0) {
            int progress = static_cast<int>((bytesSent * 100) / bytesTotal);
            
            // 显示并更新进度条
            if (!m_isUploading) {
                m_isUploading = true;
                m_uploadProgressCtrl.ShowWindow(SW_SHOW);
                m_uploadStatusLabel.ShowWindow(SW_SHOW);
            }
            
            // 更新进度条
            m_uploadProgressCtrl.SetPos(progress);
            
            // 更新状态标签
            CString statusText;
            statusText.Format(_T("正在上传: %s (%d%%)"), 
                            CString(fileName.c_str()), progress);
            m_uploadStatusLabel.SetWindowText(statusText);
            
            // 同时更新窗口标题
            CString title;
            title.Format(_T("图纸列表 - %s"), statusText);
            SetWindowText(title);
        }
        
    } catch (...) {
        // 静默处理异常
    }
}

void SheetListWindow::OnUploadCompleted(const std::wstring& fileName, bool success)
{
    try {
        if (success) {
            // 显示完成状态
            m_uploadProgressCtrl.SetPos(100);
            m_uploadStatusLabel.SetWindowText(_T("上传完成"));
            
            // 更新对应图纸的状态
            UpdateSheetStatus(fileName, _T("已上传"));
            
            // 触发文本索引更新
            UpdateTextIndexForUploadedFile(fileName);
            
        } else {
            // 显示失败状态
            m_uploadStatusLabel.SetWindowText(_T("上传失败"));
            
            // 更新对应图纸的状态
            UpdateSheetStatus(fileName, _T("上传失败"));
        }
        
        // 延迟隐藏进度条（3秒后）
        SetTimer(2003, 3000, nullptr);
        
        // 刷新表格显示
        Invalidate();
        
    } catch (...) {
        m_uploadStatusLabel.SetWindowText(_T("处理上传结果时发生异常"));
    }
}

void SheetListWindow::OnFileStatusChanged(const std::wstring& fileName, SheetStatusManager::Status oldStatus, SheetStatusManager::Status newStatus)
{
	try {
		CString statusText = m_statusManager->getStatusText(newStatus).c_str();
		CadLogger::LogInfo(_T("文件状态变化: %s -> %s"), fileName.c_str(), statusText);
		
		// 更新UI中对应的状态显示
		UpdateSheetStatus(fileName, statusText);
		
	} catch (...) {
		CadLogger::LogError(_T("处理文件状态变化时发生异常"));
	}
}

//-----------------------------------------------------------------------------
//----- 辅助方法实现

void SheetListWindow::UpdateSheetStatus(const std::wstring& serverFileName, const CString& newStatus)
{
	try {
		// 遍历数据列表，找到对应的图纸并更新状态
		for (size_t i = 0; i < m_sheetDataList.size(); ++i) {
			const auto& sheet = m_sheetDataList[i];
			
			// 生成对应的服务器文件名进行比较
			std::wstring expectedServerName = m_fileManager->generateFileName(
				sheet->buildingName, sheet->specialty, sheet->version,
				sheet->designUnit, sheet->creator, sheet->name);
			
			CString ext = PathFindExtension(sheet->filePath.c_str());
			expectedServerName += ext.GetString();
			
			if (expectedServerName == serverFileName) {
				// 更新数据模型
				sheet->status = newStatus.GetString();
				
				// 更新表格显示 - 这里需要根据实际的表格结构来更新
				// 假设状态列是第5列（COL_STATUS）
				m_sheetTable.SetItemText(static_cast<int>(i), COL_STATUS, newStatus);
				break;
			}
		}
		
	} catch (...) {
		CadLogger::LogError(_T("更新图纸状态时发生异常"));
	}
}

void SheetListWindow::UpdateTextIndexForUploadedFile(const std::wstring& serverFileName)
{
	try {
		// 找到对应的本地文件路径
		std::wstring localFilePath;
		
		for (const auto& sheet : m_sheetDataList) {
			std::wstring expectedServerName = m_fileManager->generateFileName(
				sheet->buildingName, sheet->specialty, sheet->version,
				sheet->designUnit, sheet->creator, sheet->name);
			
			CString ext = PathFindExtension(sheet->filePath.c_str());
			expectedServerName += ext.GetString();
			
			if (expectedServerName == serverFileName) {
				localFilePath = sheet->filePath;
				break;
			}
		}
		
		if (!localFilePath.empty()) {
			// 为上传的文件建立文本索引
			std::wstring errorMsg;
			if (!SearchTextInDwg::buildTextIndexForDrawing(localFilePath, errorMsg)) {
				CadLogger::LogWarning(_T("为上传文件建立文本索引失败: %s"), errorMsg.c_str());
			} else {
				CadLogger::LogInfo(_T("成功为上传文件建立文本索引: %s"), serverFileName.c_str());
			}
		}
		
	} catch (...) {
		CadLogger::LogError(_T("更新文本索引时发生异常"));
	}
}

//-----------------------------------------------------------------------------
//----- 在OnInitDialog中添加文件管理器初始化

// 在OnInitDialog方法中添加以下代码：
/*
BOOL SheetListWindow::OnInitDialog()
{
    CAdUiBaseDialog::OnInitDialog();
    
    // ... 现有的初始化代码 ...
    
    // 初始化文件管理器
    try {
        m_fileManager = new SheetFileManager();
        m_statusManager = new SheetStatusManager();
        
        // 设置回调函数
        m_fileManager->setUploadProgressCallback([this](const std::wstring& fileName, __int64 bytesSent, __int64 bytesTotal) {
            OnUploadProgress(fileName, bytesSent, bytesTotal);
        });
        
        m_fileManager->setUploadCompletedCallback([this](const std::wstring& fileName, bool success) {
            OnUploadCompleted(fileName, success);
        });
        
        m_statusManager->setStatusChangedCallback([this](const std::wstring& fileName, SheetStatusManager::Status oldStatus, SheetStatusManager::Status newStatus) {
            OnFileStatusChanged(fileName, oldStatus, newStatus);
        });
        
        CadLogger::LogInfo(_T("文件管理器初始化成功"));
        
    } catch (...) {
        CadLogger::LogError(_T("文件管理器初始化失败"));
        m_fileManager = nullptr;
        m_statusManager = nullptr;
    }
    
    // ... 其余初始化代码 ...
    
    return TRUE;
}
*/

// 添加新方法：重新计算表格列宽度
void SheetListWindow::ResizeTableColumns()
{
	CRect clientRect;
	m_sheetTable.GetClientRect(&clientRect);
	int totalWidth = clientRect.Width() - 30; // 减少边距
	
	// 如果窗口最大化，使用更精确的宽度计算
	if (IsZoomed()) {
		CRect windowRect;
		GetWindowRect(&windowRect);
		totalWidth = windowRect.Width() - 60;
	}
	
	// 权重数组（与SetupSheetTable中保持一致）
	int weights[] = { 4, 16, 14, 9, 8, 8, 6, 12, 8, 6, 9 };
	
	// 重新设置各列宽度
	for (int i = 0; i < 11; i++) {
		int columnWidth = (totalWidth * weights[i]) / 100;
		// 设置最小列宽，操作列需要更大的最小宽度
		int minWidth = (i == COL_OPERATION) ? 80 : 50;
		if (columnWidth < minWidth) columnWidth = minWidth;
		m_sheetTable.SetColumnWidth(i, columnWidth);
	}
}

// 新增：初始化专业下拉框
void SheetListWindow::InitializeSpecialtyCombo(CComboBox* pCombo)
{
	if (!pCombo) return;
	
	pCombo->ResetContent();
	pCombo->AddString(_T("结构"));
	pCombo->AddString(_T("围护(含室外)"));
	pCombo->AddString(_T("装饰装修"));
	pCombo->AddString(_T("给水排水"));
	pCombo->AddString(_T("供热采暖"));
	pCombo->AddString(_T("空调通风"));
	pCombo->AddString(_T("电气"));
	pCombo->AddString(_T("电梯"));
	pCombo->AddString(_T("建筑智能化(含消防)"));
}

// 修改：获取专业文本
CString SheetListWindow::GetSpecialtyText(int specialtyIndex)
{
	switch (specialtyIndex) {
		case SPECIALTY_STRUCTURE: return _T("结构");
		case SPECIALTY_ENVELOPE: return _T("围护(含室外)");
		case SPECIALTY_DECORATION: return _T("装饰装修");
		case SPECIALTY_WATER: return _T("给水排水");
		case SPECIALTY_HEATING: return _T("供热采暖");
		case SPECIALTY_HVAC: return _T("空调通风");
		case SPECIALTY_ELECTRICAL: return _T("电气");
		case SPECIALTY_ELEVATOR: return _T("电梯");
		case SPECIALTY_INTELLIGENT: return _T("建筑智能化(含消防)");
		default: return _T("结构");
	}
}

// 修改：获取专业索引
int SheetListWindow::GetSpecialtyIndex(const CString& specialtyText)
{
	if (specialtyText == _T("结构")) return SPECIALTY_STRUCTURE;
	if (specialtyText == _T("围护(含室外)")) return SPECIALTY_ENVELOPE;
	if (specialtyText == _T("装饰装修")) return SPECIALTY_DECORATION;
	if (specialtyText == _T("给水排水")) return SPECIALTY_WATER;
	if (specialtyText == _T("供热采暖")) return SPECIALTY_HEATING;
	if (specialtyText == _T("空调通风")) return SPECIALTY_HVAC;
	if (specialtyText == _T("电气")) return SPECIALTY_ELECTRICAL;
	if (specialtyText == _T("电梯")) return SPECIALTY_ELEVATOR;
	if (specialtyText == _T("建筑智能化(含消防)")) return SPECIALTY_INTELLIGENT;
	return SPECIALTY_STRUCTURE;
}

// 添加定时器处理
void SheetListWindow::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1004) {
        KillTimer(1004);
        
        // 执行文件选择操作
        if (m_nEditItem >= 0) {
            SelectFileForRow(m_nEditItem);
        }
        
        // 销毁按钮控件
        if (m_pButtonCtrl) {
            m_pButtonCtrl->DestroyWindow();
            delete m_pButtonCtrl;
            m_pButtonCtrl = nullptr;
        }
        
        // 重置编辑状态
        m_nEditItem = -1;
        m_nEditSubItem = -1;
        
        return;
    }
    else if (nIDEvent == 2003) {
        // 隐藏进度条和状态标签
        KillTimer(2003);
        m_uploadProgressCtrl.ShowWindow(SW_HIDE);
        m_uploadStatusLabel.ShowWindow(SW_HIDE);
        m_isUploading = false;
        
        // 恢复窗口标题
        SetWindowText(_T("图纸列表"));
        
        return;
    }
    
    CAdUiBaseDialog::OnTimer(nIDEvent);
}

//-----------------------------------------------------------------------------
//----- 缺失的消息处理方法实现

void SheetListWindow::OnEnChangeSheetBuildingNameEdit()
{
    // 处理大楼名称编辑框内容变化事件
    // 这个方法通常用于实时筛选或验证输入
    
    try {
        CString buildingName;
        m_buildingNameEdit.GetWindowText(buildingName);
        
        // 记录输入变化（可选）
        CadLogger::LogInfo(_T("大楼名称输入变化: %s"), buildingName);
        
        // 如果需要实时筛选，可以在这里添加逻辑
        // 但通常建议使用定时器避免频繁操作
        
    } catch (...) {
        CadLogger::LogError(_T("处理大楼名称编辑框变化时发生异常"));
    }
}

void SheetListWindow::OnBnClickedSheetSearchGroup()
{
    // 处理搜索组框按钮点击事件
    // 这个方法可能用于展开/收起搜索区域或执行搜索操作
    
    try {
        CadLogger::LogInfo(_T("搜索组框按钮被点击"));
        
        // 根据实际需求实现功能
        // 例如：展开/收起筛选区域
        // 或者：执行快速搜索
        // 或者：重置搜索条件
        
        // 示例：显示提示信息
        // AfxMessageBox(_T("搜索功能开发中..."));
        
    } catch (...) {
        CadLogger::LogError(_T("处理搜索组框按钮点击时发生异常"));
    }
}

LRESULT SheetListWindow::OnUploadProgressMessage(WPARAM wParam, LPARAM lParam)
{
    UploadProgressData* pData = reinterpret_cast<UploadProgressData*>(lParam);
    if (pData) {
        OnUploadProgress(pData->fileName, pData->bytesSent, pData->bytesTotal);
        delete pData;  // 释放内存
    }
    return 0;
}

LRESULT SheetListWindow::OnUploadCompletedMessage(WPARAM wParam, LPARAM lParam)
{
    UploadCompletedData* pData = reinterpret_cast<UploadCompletedData*>(lParam);
    if (pData) {
        OnUploadCompleted(pData->fileName, pData->success);
        delete pData;  // 释放内存
    }
    return 0;
}

// 新增方法：创建进度条控件
void SheetListWindow::CreateProgressControls()
{
    // 获取上传按钮的位置，在其下方创建进度条
    CRect uploadBtnRect;
    m_uploadButton.GetWindowRect(&uploadBtnRect);
    ScreenToClient(&uploadBtnRect);
    
    // 计算进度条位置
    CRect progressRect;
    progressRect.left = uploadBtnRect.left;
    progressRect.right = uploadBtnRect.right + 200;  // 扩展宽度
    progressRect.top = uploadBtnRect.bottom + 10;    // 距离按钮10像素
    progressRect.bottom = progressRect.top + 20;     // 高度20像素
    
    // 创建进度条
    m_uploadProgressCtrl.Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 
                               progressRect, this, 2001);
    m_uploadProgressCtrl.SetRange(0, 100);
    m_uploadProgressCtrl.SetPos(0);
    m_uploadProgressCtrl.ShowWindow(SW_HIDE);  // 初始隐藏
    
    // 计算状态标签位置
    CRect labelRect;
    labelRect.left = progressRect.left;
    labelRect.right = progressRect.right;
    labelRect.top = progressRect.bottom + 5;
    labelRect.bottom = labelRect.top + 20;
    
    // 创建状态标签
    m_uploadStatusLabel.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT,
                              labelRect, this, 2002);
    m_uploadStatusLabel.ShowWindow(SW_HIDE);  // 初始隐藏
}

// 新增方法：更新数据模型
void SheetListWindow::UpdateDataModel(int nItem, int nSubItem, const CString& newValue)
{
	try {
		// 检查索引有效性
		if (nItem < 0 || nItem >= static_cast<int>(m_sheetDataList.size())) {
			CadLogger::LogWarning(_T("更新数据模型时行索引无效: %d"), nItem);
			return;
		}
		
		auto& sheetData = m_sheetDataList[nItem];
		std::wstring newValueW = newValue.GetString();
		
		// 根据列索引更新对应的数据字段
		switch (nSubItem) {
			case COL_NAME:
				sheetData->name = newValueW;
				break;
			case COL_BUILDING:
				sheetData->buildingName = newValueW;
				break;
			case COL_SPECIALTY:
				sheetData->specialty = newValueW;
				break;
			case COL_FORMAT:
				sheetData->format = newValueW;
				break;
			case COL_STATUS:
				sheetData->status = newValueW;
				break;
			case COL_VERSION:
				sheetData->version = newValueW;
				break;
			case COL_DESIGN_UNIT:
				sheetData->designUnit = newValueW;
				break;
			case COL_CREATE_TIME:
				sheetData->createTime = newValueW;
				break;
			case COL_CREATOR:
				sheetData->creator = newValueW;
				break;
			default:
				CadLogger::LogWarning(_T("未处理的列索引: %d"), nSubItem);
				break;
		}
		
		// 同时更新原始数据列表（用于筛选恢复）
		for (auto& originalData : m_originalDataList) {
			if (originalData->name == sheetData->name && 
				originalData->buildingName == sheetData->buildingName) {
				// 找到对应的原始数据并更新
				switch (nSubItem) {
					case COL_NAME:
						originalData->name = newValueW;
						break;
					case COL_BUILDING:
						originalData->buildingName = newValueW;
						break;
					case COL_SPECIALTY:
						originalData->specialty = newValueW;
						break;
					case COL_FORMAT:
						originalData->format = newValueW;
						break;
					case COL_STATUS:
						originalData->status = newValueW;
						break;
					case COL_VERSION:
						originalData->version = newValueW;
						break;
					case COL_DESIGN_UNIT:
						originalData->designUnit = newValueW;
						break;
					case COL_CREATE_TIME:
						originalData->createTime = newValueW;
						break;
					case COL_CREATOR:
						originalData->creator = newValueW;
						break;
				}
				break;
			}
		}
		
		CadLogger::LogInfo(_T("数据模型已更新: 行%d 列%d 新值: %s"), nItem, nSubItem, newValue);
		
	} catch (...) {
		CadLogger::LogError(_T("更新数据模型时发生异常"));
	}
}

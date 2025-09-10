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
//----- SheetListWindow.h : Declaration of the SheetListWindow
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
#include "../../resource.h"

#include "adui.h"
#include <afxcmn.h>
#include <afxdtctl.h>
#include <vector>
#include <memory>
#include <string>
#include "src/common/SheetManager/SheetFileManager.h"

// 前向声明
struct SheetData;


#define WM_UPLOAD_PROGRESS (WM_USER + 1001)
#define WM_UPLOAD_COMPLETED (WM_USER + 1002)

// 添加结构体用于传递数据
struct UploadProgressData {
	std::wstring fileName;
	__int64 bytesSent;
	__int64 bytesTotal;
};

struct UploadCompletedData {
	std::wstring fileName;
	bool success;
};


//-----------------------------------------------------------------------------
class SheetListWindow : public CAdUiBaseDialog {
	DECLARE_DYNAMIC(SheetListWindow)

public:
	// 构造函数和析构函数
	SheetListWindow(CWnd* pParent = nullptr, HINSTANCE hInstance = nullptr);
	virtual ~SheetListWindow();

	enum { IDD = IDD_SheetListWindow };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	
	afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
	afx_msg void OnBnClickedSheetFilterButton();
	afx_msg void OnBnClickedSheetResetFilterButton();
	afx_msg void OnBnClickedSheetUploadButton();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnNMRClickSheetTable(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenuInsertRow();
	afx_msg void OnContextMenuDeleteRow();
	afx_msg void OnContextMenuSelectFile();
	afx_msg void OnLvnItemchangedSheetTable(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkSheetTable(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnEndlabeleditSheetTable(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnKeydownSheetTable(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeSheetBuildingNameEdit();
	afx_msg void OnBnClickedSheetSearchGroup();
	afx_msg void OnTimer(UINT_PTR nIDEvent);  // 新增：定时器处理

	DECLARE_MESSAGE_MAP()

private:
	// 控件变量
	CEdit m_buildingNameEdit;
	CEdit m_specialtyEdit;
	CEdit m_designUnitEdit;
	CDateTimeCtrl m_createTimePicker;
	CButton m_filterButton;
	CButton m_resetFilterButton;
	CButton m_uploadButton;
	CListCtrl m_sheetTable;
	CFont m_headerFont;  // 新增：表头粗体字体
	
	// 右键菜单
	CMenu m_contextMenu;
	int m_contextMenuRow;
	
	// 编辑控件相关（参照BuildBuildingTableWindow）
	CEdit* m_pEditCtrl;
	CComboBox* m_pComboCtrl;  // 新增：专业选择下拉框
	CButton* m_pButtonCtrl;   // 新增：操作按钮
	int m_nEditItem;
	int m_nEditSubItem;
	
	// 数据成员
	std::vector<std::shared_ptr<SheetData>> m_sheetDataList;
	std::vector<std::shared_ptr<SheetData>> m_originalDataList; // 用于筛选恢复
	
	// 文件管理器（后续实现业务逻辑时使用）
	SheetFileManager* m_fileManager;
	SheetStatusManager* m_statusManager;
	
	// 界面相关方法
	void InitializeControls();
	void SetupSheetTable();
	void PopulateTableData();
	void UpdateTableWithFilteredData(const std::vector<std::shared_ptr<SheetData>>& filteredData);
	void ResizeControls(int cx, int cy);
	void AdjustFilterControls(int cx);
	void ResizeTableColumns(); // 新增：重新计算表格列宽度
	
	// 数据操作方法
	void LoadDataFromDatabase();
	void SaveDataToDatabase();
	bool CreateSheetTable();
	
	// 筛选相关方法
	std::vector<std::shared_ptr<SheetData>> FilterSheetData(
		const CString& buildingName,
		const CString& specialty, 
		const CString& designUnit,
		const CTime& createTime);
	void ResetFilter();
	
	// 表格操作方法
	void InsertNewRow(int row);
	void DeleteRow(int row);
	void SelectFileForRow(int row);
	void UpdateRowData(int row);
	
	// 编辑功能方法（参照BuildBuildingTableWindow）
	void StartEdit(int nItem, int nSubItem);
	void EndEdit(bool bSave);
	CEdit* CreateEditControl(int nItem, int nSubItem);
	CComboBox* CreateComboControl(int nItem, int nSubItem);  // 新增：创建下拉框控件
	CButton* CreateButtonControl(int nItem, int nSubItem);   // 新增：创建按钮控件
	
	// 上传相关方法
	void UploadSelectedSheets();
	std::vector<std::shared_ptr<SheetData>> GetSelectedSheets();
	
	// 辅助方法
	void ShowContextMenu(CPoint point);
	CString GetSpecialtyText(int specialtyIndex);
	int GetSpecialtyIndex(const CString& specialtyText);
	CString GetStatusText(int statusIndex);
	int GetStatusIndex(const CString& statusText);
	std::wstring GetCurrentTimeString();
	
	// 专业类型相关方法
	void InitializeSpecialtyCombo(CComboBox* pCombo);
	
	// 常量定义（调整按钮高度和最小窗口尺寸）
	static const int MIN_WIDTH = 700;   // 增加最小宽度
	static const int MIN_HEIGHT = 350;
	static const int BUTTON_HEIGHT = 25; // 按钮高度改为14像素
	
	// 表格列索引
	enum ColumnIndex {
		COL_SELECT = 0,      // 选择状态
		COL_NAME = 1,        // 图纸名称
		COL_BUILDING = 2,    // 大楼名称
		COL_SPECIALTY = 3,   // 专业
		COL_FORMAT = 4,      // 图纸格式
		COL_STATUS = 5,      // 图纸状态
		COL_VERSION = 6,     // 版本号
		COL_DESIGN_UNIT = 7, // 设计单位
		COL_CREATE_TIME = 8, // 创建时间
		COL_CREATOR = 9,     // 创建人
		COL_OPERATION = 10   // 操作
	};
	
	// 专业类型枚举
	enum SpecialtyType {
		SPECIALTY_STRUCTURE = 0,     // 结构
		SPECIALTY_ENVELOPE = 1,      // 围(含室外)
		SPECIALTY_DECORATION = 2,    // 装饰装修
		SPECIALTY_WATER = 3,         // 给水排水
		SPECIALTY_HEATING = 4,       // 供热采暖
		SPECIALTY_HVAC = 5,          // 空调通风
		SPECIALTY_ELECTRICAL = 6,    // 电气
		SPECIALTY_ELEVATOR = 7,      // 电梯
		SPECIALTY_INTELLIGENT = 8    // 建筑智能化(含消防)
	};
	
	// 上传回调方法
	void OnUploadProgress(const std::wstring& fileName, __int64 bytesSent, __int64 bytesTotal);
	void OnUploadCompleted(const std::wstring& fileName, bool success);
	void OnFileStatusChanged(const std::wstring& fileName, SheetStatusManager::Status oldStatus, SheetStatusManager::Status newStatus);
	
	// 辅助方法
	void UpdateSheetStatus(const std::wstring& serverFileName, const CString& newStatus);
	void UpdateTextIndexForUploadedFile(const std::wstring& serverFileName);

	// 在 SheetListWindow.h 中添加
protected:
	afx_msg LRESULT OnUploadProgressMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUploadCompletedMessage(WPARAM wParam, LPARAM lParam);
};

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
//----- BuildBuildingTableWindow.h : Declaration of the BuildBuildingTableWindow
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
#include "adui.h"
#include "../../resource.h"
#include <afxdtctl.h>  // 添加这个头文件用于CDateTimeCtrl
#include <vector>
#include <string>

// 大楼数据结构
struct BuildingData {
    int id;
    std::wstring buildingName;
    std::wstring address;
    std::wstring totalArea;
    std::wstring floors;
    std::wstring designUnit;
    std::wstring createTime;
    std::wstring creator;
    
    BuildingData() : id(0) {}
};

//-----------------------------------------------------------------------------
class BuildBuildingTableWindow : public CAdUiBaseDialog {
    DECLARE_DYNAMIC(BuildBuildingTableWindow)

public:
    BuildBuildingTableWindow(CWnd *pParent = NULL, HINSTANCE hInstance = NULL);
    virtual ~BuildBuildingTableWindow();

    enum { IDD = IDD_BuildBuildingTableWindow };


protected:
    virtual void DoDataExchange(CDataExchange *pDX);
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);  // 添加这行
    afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
    afx_msg void OnBnClickedFilterButton();
    afx_msg void OnBnClickedResetFilterButton();
    afx_msg void OnBnClickedSaveButton();
    afx_msg void OnNMRClickBuildingTable(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnEndlabeleditBuildingTable(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMDblclkBuildingTable(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnKeydownBuildingTable(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnItemchangedBuildingTable(NMHDR *pNMHDR, LRESULT *pResult);  // 添加这行
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
 
    
    // 控件变量
    CListCtrl m_buildingTable;
    CEdit m_buildingNameEdit;
    CEdit m_designUnitEdit;
    CDateTimeCtrl m_createTimePicker;
    
    // 添加用于编辑的控件
    CEdit* m_pEditCtrl;          // 编辑控件指针
    int m_nEditItem;             // 正在编辑的行
    int m_nEditSubItem;          // 正在编辑的列
    
    // 数据管理
    std::vector<BuildingData> m_buildingDataList;
    
    // 私有方法
    void initializeControls();
    void setupTableColumns();
    void loadDataFromDatabase();
    void saveDataToDatabase();
    void filterData();
    void resetFilter();
    void insertRow();
    void deleteRow();
    void showContextMenu(CPoint point);
    bool createBuildingTable();
    bool insertBuildingData(const BuildingData& data);
    void populateTableFromData();
    void resizeControls(int cx, int cy);
    void adjustFilterControls(int cx);
    std::wstring getCurrentTimeString();
    
    // 添加编辑相关方法
    void startEdit(int nItem, int nSubItem);
    void endEdit(bool bSave);
    CEdit* createEditControl(int nItem, int nSubItem);
    
   
};

#pragma once
#include "../../resource.h"

// CTestDialog 对话框类 - 图纸数字化档案管理系统
class CTestDialog : public CAcUiDialog
{
    DECLARE_DYNAMIC(CTestDialog)

public:
    CTestDialog(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~CTestDialog();

// 对话框数据
    enum { IDD = IDD_DIALOG1 };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual void PostNcDestroy();  // 添加PostNcDestroy声明
    virtual BOOL PreTranslateMessage(MSG* pMsg);  // 预处理消息

    DECLARE_MESSAGE_MAP()

private:
    // 控件成员变量
    CStatic m_titleLabel;           // 标题标签
    CStatic m_userLabel;            // 用户标签
    CStatic m_buildingMgmtLabel;    // 大楼管理标签
    CButton m_buildingListBtn;      // 大楼列表按钮
    CButton m_drawingListBtn;       // 图纸列表按钮
    CStatic m_currentBuildingLabel; // 当前大楼标签
    CComboBox m_currentBuildingCombo; // 当前大楼下拉框
    CStatic m_drawingViewLabel;     // 图纸查看标签
    CEdit m_searchEdit;             // 搜索编辑框
    CButton m_searchBtn;            // 搜索按钮
    CTreeCtrl m_drawingTree;        // 图纸树形控件
    CStatic m_toolboxLabel;         // 工具箱标签
    CButton m_drawingComparisonBtn; // 图纸对比按钮
    CButton m_engineeringCalcBtn;   // 工程量计算按钮
    CButton m_exportReportBtn;      // 导出报表按钮
    CButton m_imageMgmtBtn;         // 影像管理按钮
    CButton m_componentTaggingBtn;  // 构件打标签按钮
    CButton m_modifyUploadBtn;      // 图纸修改并上传按钮

    // 数据成员变量
    CString m_strSearchText;        // 搜索文本
    int m_nCurrentBuilding;         // 当前选中的大楼索引

public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnClose();  // 添加关闭消息处理
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
    
    // 按钮消息处理函数
    afx_msg void OnBnClickedBuildingList();
    afx_msg void OnBnClickedDrawingList();
    afx_msg void OnBnClickedSearch();
    afx_msg void OnBnClickedDrawingComparison();
    afx_msg void OnBnClickedEngineeringCalc();
    afx_msg void OnBnClickedExportReport();
    afx_msg void OnBnClickedImageMgmt();
    afx_msg void OnBnClickedComponentTagging();
    afx_msg void OnBnClickedModifyUpload();
    
    // 控件消息处理函数
    afx_msg void OnCbnSelchangeCurrentBuilding();
    afx_msg void OnTvnSelchangedDrawingTree(NMHDR *pNMHDR, LRESULT *pResult);
    
    // 静态指针用于管理非模态对话框实例
    static CTestDialog* s_pModelessDialog;

private:
    // 初始化函数
    void InitializeControls();
    void InitializeBuildingCombo();
    void InitializeDrawingTree();
    void UpdateUserInfo();
    
    // 功能函数
    void SearchDrawings();
    void LoadBuildingData(int buildingIndex);
};

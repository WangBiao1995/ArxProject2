#pragma once
#include "../../resource.h"
#include "adui.h"  // 添加AutoCAD UI头文件
#include "../services/SearchTextInDwg.h"
#include "../services/DwgViewerService.h"
#include "../common/Database/SqlDB.h"
#include "../common/SheetManager/SheetFileManager.h"
#include "../models/SheetManagerModel.h"
#include <vector>
#include <memory>

// CTestDialog 对话框类 - 图纸数字化档案管理系统
class CTestDialog : public CAdUiBaseDialog  // 修改基类
{
    DECLARE_DYNAMIC(CTestDialog)

public:
    CTestDialog(CWnd* pParent = nullptr, HINSTANCE hInstance = nullptr);   // 修改构造函数
    virtual ~CTestDialog();

// 对话框数据
    enum { IDD = IDD_DIALOG1 };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual void PostNcDestroy();  // 添加PostNcDestroy声明
    virtual BOOL PreTranslateMessage(MSG* pMsg);  // 预处理消息
    afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);  // AutoCAD焦点处理

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

    // 后台服务成员
    std::unique_ptr<SheetFileManager> m_pSheetFileManager;  // 图纸文件管理器
    std::vector<TextSearchResult> m_searchResults;          // 搜索结果
    std::vector<DbSheetData> m_buildingData;               // 大楼数据
    
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnClose();  // 添加关闭消息处理
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    
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
    void InitializeServices();          // 初始化后台服务
    
    // 功能函数
    void SearchDrawings();
    void LoadBuildingData(int buildingIndex);
    void PopulateBuildingComboFromDatabase();   // 从数据库加载大楼数据
    void UpdateDrawingTreeWithSearchResults(const std::vector<TextSearchResult>& results);
    void ClearSearchResults();
    HTREEITEM FindOrCreateCategoryItem(const CString& category);
    CString ExtractCategoryFromFilePath(const std::wstring& filePath);
    CString ExtractDrawingNameFromFilePath(const std::wstring& filePath);
    void OpenDrawingAndLocateText(const std::wstring& filePath, const TextSearchResult& textResult);
    
    // 网络和缓存相关函数
    std::vector<std::wstring> GetAvailableDrawingsFromServer();
    bool DownloadDrawingIfNeeded(const std::wstring& serverFileName);
    std::wstring GetLocalFilePath(const std::wstring& serverFileName);
    
    // 辅助函数
    std::wstring CStringToWString(const CString& str);
    CString WStringToCString(const std::wstring& wstr);
public:
    afx_msg void OnStnClickedTitleLabel();
};

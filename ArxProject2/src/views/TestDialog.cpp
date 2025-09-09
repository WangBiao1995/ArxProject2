#include "../../stdafx.h"
#include "TestDialog.h"


// CTestDialog 对话框

IMPLEMENT_DYNAMIC(CTestDialog, CAcUiDialog)

// 静态成员变量定义
CTestDialog* CTestDialog::s_pModelessDialog = nullptr;

CTestDialog::CTestDialog(CWnd* pParent /*=nullptr*/)
    : CAcUiDialog(CTestDialog::IDD, pParent)
    , m_strSearchText(_T(""))
    , m_nCurrentBuilding(0)
{
}

CTestDialog::~CTestDialog()
{
    // 如果当前实例是静态指针指向的实例，清空静态指针
    if (s_pModelessDialog == this) {
        s_pModelessDialog = nullptr;
    }
}

void CTestDialog::DoDataExchange(CDataExchange* pDX)
{
    CAcUiDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TITLE_LABEL, m_titleLabel);
    DDX_Control(pDX, IDC_USER_LABEL, m_userLabel);
    DDX_Control(pDX, IDC_BUILDING_MANAGEMENT_LABEL, m_buildingMgmtLabel);
    DDX_Control(pDX, IDC_BUILDING_LIST_BUTTON, m_buildingListBtn);
    DDX_Control(pDX, IDC_DRAWING_LIST_BUTTON, m_drawingListBtn);
    DDX_Control(pDX, IDC_CURRENT_BUILDING_LABEL, m_currentBuildingLabel);
    DDX_Control(pDX, IDC_CURRENT_BUILDING_COMBO, m_currentBuildingCombo);
    DDX_Control(pDX, IDC_DRAWING_VIEW_LABEL, m_drawingViewLabel);
    DDX_Control(pDX, IDC_SEARCH_EDIT, m_searchEdit);
    DDX_Control(pDX, IDC_SEARCH_BUTTON, m_searchBtn);
    DDX_Control(pDX, IDC_DRAWING_TREE, m_drawingTree);
    DDX_Control(pDX, IDC_TOOLBOX_LABEL, m_toolboxLabel);
    DDX_Control(pDX, IDC_DRAWING_COMPARISON_BUTTON, m_drawingComparisonBtn);
    DDX_Control(pDX, IDC_ENGINEERING_CALC_BUTTON, m_engineeringCalcBtn);
    DDX_Control(pDX, IDC_EXPORT_REPORT_BUTTON, m_exportReportBtn);
    DDX_Control(pDX, IDC_IMAGE_MANAGEMENT_BUTTON, m_imageMgmtBtn);
    DDX_Control(pDX, IDC_COMPONENT_TAGGING_BUTTON, m_componentTaggingBtn);
    DDX_Control(pDX, IDC_MODIFY_UPLOAD_BUTTON, m_modifyUploadBtn);
    DDX_Text(pDX, IDC_SEARCH_EDIT, m_strSearchText);
    DDX_CBIndex(pDX, IDC_CURRENT_BUILDING_COMBO, m_nCurrentBuilding);
}

BEGIN_MESSAGE_MAP(CTestDialog, CAcUiDialog)
    ON_BN_CLICKED(IDOK, &CTestDialog::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CTestDialog::OnBnClickedCancel)
    ON_WM_CLOSE()
    ON_WM_ACTIVATE()
    ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
    
    // 按钮消息映射
    ON_BN_CLICKED(IDC_BUILDING_LIST_BUTTON, &CTestDialog::OnBnClickedBuildingList)
    ON_BN_CLICKED(IDC_DRAWING_LIST_BUTTON, &CTestDialog::OnBnClickedDrawingList)
    ON_BN_CLICKED(IDC_SEARCH_BUTTON, &CTestDialog::OnBnClickedSearch)
    ON_BN_CLICKED(IDC_DRAWING_COMPARISON_BUTTON, &CTestDialog::OnBnClickedDrawingComparison)
    ON_BN_CLICKED(IDC_ENGINEERING_CALC_BUTTON, &CTestDialog::OnBnClickedEngineeringCalc)
    ON_BN_CLICKED(IDC_EXPORT_REPORT_BUTTON, &CTestDialog::OnBnClickedExportReport)
    ON_BN_CLICKED(IDC_IMAGE_MANAGEMENT_BUTTON, &CTestDialog::OnBnClickedImageMgmt)
    ON_BN_CLICKED(IDC_COMPONENT_TAGGING_BUTTON, &CTestDialog::OnBnClickedComponentTagging)
    ON_BN_CLICKED(IDC_MODIFY_UPLOAD_BUTTON, &CTestDialog::OnBnClickedModifyUpload)
    
    // 控件消息映射
    ON_CBN_SELCHANGE(IDC_CURRENT_BUILDING_COMBO, &CTestDialog::OnCbnSelchangeCurrentBuilding)
    ON_NOTIFY(TVN_SELCHANGED, IDC_DRAWING_TREE, &CTestDialog::OnTvnSelchangedDrawingTree)
END_MESSAGE_MAP()

// CTestDialog 消息处理程序

BOOL CTestDialog::OnInitDialog()
{
    CAcUiDialog::OnInitDialog();

    // 设置对话框标题
    SetWindowText(_T("图纸数字化档案管理系统"));
    
    // 初始化控件
    InitializeControls();
    InitializeBuildingCombo();
    InitializeDrawingTree();
    UpdateUserInfo();

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTestDialog::InitializeControls()
{
    // 设置搜索编辑框的提示文本
    m_searchEdit.SetWindowText(_T(""));
    
    // 设置标题样式（可以通过修改字体来实现）
    CFont titleFont;
    titleFont.CreateFont(
        22,                        // nHeight
        0,                         // nWidth
        0,                         // nEscapement
        0,                         // nOrientation
        FW_BOLD,                   // nWeight
        FALSE,                     // bItalic
        FALSE,                     // bUnderline
        0,                         // cStrikeOut
        ANSI_CHARSET,              // nCharSet
        OUT_DEFAULT_PRECIS,        // nOutPrecision
        CLIP_DEFAULT_PRECIS,       // nClipPrecision
        DEFAULT_QUALITY,           // nQuality
        DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
        _T("Microsoft YaHei UI"));    // lpszFacename
    
    m_titleLabel.SetFont(&titleFont);
}

void CTestDialog::InitializeBuildingCombo()
{
    // 清空下拉框
    m_currentBuildingCombo.ResetContent();
    
    // 添加大楼选项
    m_currentBuildingCombo.AddString(_T("A电力大楼"));
    m_currentBuildingCombo.AddString(_T("B办公大楼"));
    m_currentBuildingCombo.AddString(_T("C住宅楼"));
    
    // 设置默认选择
    m_currentBuildingCombo.SetCurSel(0);
    m_nCurrentBuilding = 0;
}

void CTestDialog::InitializeDrawingTree()
{
    // 清空树形控件
    m_drawingTree.DeleteAllItems();
    
    // 添加根节点
    HTREEITEM hRoot = m_drawingTree.InsertItem(_T("图纸分类"), TVI_ROOT);
    
    // 添加子节点示例
    HTREEITEM hArchitecture = m_drawingTree.InsertItem(_T("建筑图纸"), hRoot);
    m_drawingTree.InsertItem(_T("平面图"), hArchitecture);
    m_drawingTree.InsertItem(_T("立面图"), hArchitecture);
    m_drawingTree.InsertItem(_T("剖面图"), hArchitecture);
    
    HTREEITEM hStructure = m_drawingTree.InsertItem(_T("结构图纸"), hRoot);
    m_drawingTree.InsertItem(_T("基础图"), hStructure);
    m_drawingTree.InsertItem(_T("梁板图"), hStructure);
    m_drawingTree.InsertItem(_T("柱网图"), hStructure);
    
    HTREEITEM hMEP = m_drawingTree.InsertItem(_T("机电图纸"), hRoot);
    m_drawingTree.InsertItem(_T("电气图"), hMEP);
    m_drawingTree.InsertItem(_T("给排水图"), hMEP);
    m_drawingTree.InsertItem(_T("暖通图"), hMEP);
    
    // 展开根节点
    m_drawingTree.Expand(hRoot, TVE_EXPAND);
}

void CTestDialog::UpdateUserInfo()
{
    // 更新用户信息（这里可以从系统获取当前用户信息）
    m_userLabel.SetWindowText(_T("用户：管理员"));
}

BOOL CTestDialog::PreTranslateMessage(MSG* pMsg)
{
    // 处理ESC键，关闭非模态对话框
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
    {
        PostMessage(WM_CLOSE);
        return TRUE;
    }
    
    // 处理回车键，执行搜索操作
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        CWnd* pFocusWnd = GetFocus();
        if (pFocusWnd && pFocusWnd->GetDlgCtrlID() == IDC_SEARCH_EDIT)
        {
            // 如果焦点在搜索框上，执行搜索
            PostMessage(WM_COMMAND, MAKEWPARAM(IDC_SEARCH_BUTTON, BN_CLICKED), 0);
            return TRUE;
        }
    }
    
    return CAcUiDialog::PreTranslateMessage(pMsg);
}

void CTestDialog::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CAcUiDialog::OnActivate(nState, pWndOther, bMinimized);
    
    // 当对话框被激活时，确保可以接收输入
    if (nState == WA_CLICKACTIVE || nState == WA_ACTIVE)
    {
        // 延迟设置焦点，避免被AutoCAD立即抢夺
        PostMessage(WM_COMMAND, MAKEWPARAM(0, 0x1000), 0);
    }
}

LRESULT CTestDialog::OnAcadKeepFocus(WPARAM, LPARAM)
{
    // 返回TRUE告诉AutoCAD保持对话框的焦点
    return TRUE;
}

void CTestDialog::OnBnClickedOk()
{
    UpdateData(TRUE);
    acutPrintf(_T("\n图纸数字化档案管理系统 - 确定操作\n"));
    DestroyWindow();
}

void CTestDialog::OnBnClickedCancel()
{
    acutPrintf(_T("\n图纸数字化档案管理系统 - 取消操作\n"));
    DestroyWindow();
}

void CTestDialog::OnClose()
{
    acutPrintf(_T("\n图纸数字化档案管理系统被关闭\n"));
    DestroyWindow();
}

void CTestDialog::PostNcDestroy()
{
    CAcUiDialog::PostNcDestroy();
    // 非模态对话框需要在PostNcDestroy中删除自己
    delete this;
}

// 按钮事件处理函数
void CTestDialog::OnBnClickedBuildingList()
{
    acutPrintf(_T("\n打开大楼列表窗口\n"));
    // TODO: 实现大楼列表功能
}

void CTestDialog::OnBnClickedDrawingList()
{
    acutPrintf(_T("\n打开图纸列表窗口\n"));
    // TODO: 实现图纸列表功能
}

void CTestDialog::OnBnClickedSearch()
{
    SearchDrawings();
}

void CTestDialog::OnBnClickedDrawingComparison()
{
    acutPrintf(_T("\n启动图纸对比功能\n"));
    // TODO: 实现图纸对比功能
}

void CTestDialog::OnBnClickedEngineeringCalc()
{
    acutPrintf(_T("\n启动工程量计算功能\n"));
    // TODO: 实现工程量计算功能
}

void CTestDialog::OnBnClickedExportReport()
{
    acutPrintf(_T("\n导出计算报表\n"));
    // TODO: 实现导出报表功能
}

void CTestDialog::OnBnClickedImageMgmt()
{
    acutPrintf(_T("\n打开影像管理功能\n"));
    // TODO: 实现影像管理功能
}

void CTestDialog::OnBnClickedComponentTagging()
{
    acutPrintf(_T("\n启动构件打标签功能\n"));
    // TODO: 实现构件打标签功能
}

void CTestDialog::OnBnClickedModifyUpload()
{
    acutPrintf(_T("\n启动图纸修改并上传功能\n"));
    // TODO: 实现图纸修改并上传功能
}

// 控件事件处理函数
void CTestDialog::OnCbnSelchangeCurrentBuilding()
{
    UpdateData(TRUE);
    LoadBuildingData(m_nCurrentBuilding);
}

void CTestDialog::OnTvnSelchangedDrawingTree(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    
    HTREEITEM hSelectedItem = m_drawingTree.GetSelectedItem();
    if (hSelectedItem != NULL)
    {
        CString itemText = m_drawingTree.GetItemText(hSelectedItem);
        acutPrintf(_T("\n选中图纸项目：%s\n"), itemText);
        // TODO: 实现图纸选择后的处理逻辑
    }
    
    *pResult = 0;
}

// 功能实现函数
void CTestDialog::SearchDrawings()
{
    UpdateData(TRUE);
    
    if (m_strSearchText.IsEmpty())
    {
        acutPrintf(_T("\n请输入搜索关键词\n"));
        return;
    }
    
    acutPrintf(_T("\n搜索关键词：%s\n"), m_strSearchText);
    
    // TODO: 实现实际的搜索功能
    // 这里可以根据搜索关键词过滤树形控件中的内容
    // 或者调用后台服务进行搜索
}

void CTestDialog::LoadBuildingData(int buildingIndex)
{
    CString buildingName;
    m_currentBuildingCombo.GetLBText(buildingIndex, buildingName);
    
    acutPrintf(_T("\n加载大楼数据：%s\n"), buildingName);
    
    // TODO: 根据选中的大楼加载相应的图纸数据
    // 更新树形控件中的内容
    InitializeDrawingTree(); // 临时使用，实际应该根据大楼加载不同的数据
}



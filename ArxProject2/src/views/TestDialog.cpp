#include "StdAfx.h"
#include "TestDialog.h"
#include "../views/BuildBuildingTableWindow.h"
#include "../views/SheetListWindow.h"
#include <set>

// CTestDialog 对话框

IMPLEMENT_DYNAMIC(CTestDialog, CAdUiBaseDialog)

// 静态成员变量定义
CTestDialog* CTestDialog::s_pModelessDialog = nullptr;

CTestDialog::CTestDialog(CWnd* pParent /*=nullptr*/, HINSTANCE hInstance /*=nullptr*/)
    : CAdUiBaseDialog(CTestDialog::IDD, pParent, hInstance)
    , m_strSearchText(_T(""))
    , m_nCurrentBuilding(0)
    , m_pSheetFileManager(nullptr)
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
    CAdUiBaseDialog::DoDataExchange(pDX);
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

BEGIN_MESSAGE_MAP(CTestDialog, CAdUiBaseDialog)
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
    CAdUiBaseDialog::OnInitDialog();

    // 设置对话框标题
    SetWindowText(_T("图纸数字化档案管理系统"));
    
    // 初始化后台服务
    InitializeServices();
    
    // 初始化控件
    InitializeControls();
    InitializeBuildingCombo();
    InitializeDrawingTree();
    UpdateUserInfo();

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTestDialog::InitializeServices()
{
    // 初始化数据库连接
    if (!SqlDB::isDatabaseOpen()) {
        if (!SqlDB::initDatabase()) {
            acutPrintf(_T("\n数据库连接失败，使用默认数据\n"));
        }
    }
    
    // 初始化文本搜索索引表
    if (!SearchTextInDwg::createTextIndexTable()) {
        acutPrintf(_T("\n创建文本索引表失败\n"));
    }
    
    // 初始化图纸文件管理器
    m_pSheetFileManager = std::make_unique<SheetFileManager>();
    
    // 设置回调函数
    if (m_pSheetFileManager) {
        m_pSheetFileManager->setUploadProgressCallback([this](const std::wstring& fileName, __int64 current, __int64 total) {
            // 上传进度回调
            double progress = (double)current / total * 100.0;
            CString msg;
            msg.Format(_T("上传进度: %s - %.1f%%"), WStringToCString(fileName), progress);
            acutPrintf(_T("\n%s\n"), msg);
        });
        
        m_pSheetFileManager->setUploadCompletedCallback([this](const std::wstring& fileName, bool success) {
            // 上传完成回调
            CString msg;
            msg.Format(_T("上传%s: %s"), success ? _T("成功") : _T("失败"), WStringToCString(fileName));
            acutPrintf(_T("\n%s\n"), msg);
        });
    }
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
    
    // 从数据库加载大楼数据
    PopulateBuildingComboFromDatabase();
}

void CTestDialog::PopulateBuildingComboFromDatabase()
{
    if (!SqlDB::isDatabaseOpen()) {
        // 如果数据库未连接，使用默认数据
        m_currentBuildingCombo.AddString(_T("A电力大楼"));
        m_currentBuildingCombo.AddString(_T("B办公大楼"));
        m_currentBuildingCombo.AddString(_T("C住宅楼"));
        m_currentBuildingCombo.SetCurSel(0);
        m_nCurrentBuilding = 0;
        return;
    }
    
    // 从数据库查询大楼名称
    std::wstring errorMsg;
    if (SqlDB::loadSheetData(m_buildingData, errorMsg)) {
        // 提取唯一的大楼名称
        std::set<std::wstring> uniqueBuildings;
        for (const auto& sheet : m_buildingData) {
            if (!sheet.buildingName.empty()) {
                uniqueBuildings.insert(sheet.buildingName);
            }
        }
        
        // 添加到下拉框
        for (const auto& building : uniqueBuildings) {
            m_currentBuildingCombo.AddString(WStringToCString(building));
        }
        
        // 设置默认选择
        if (!uniqueBuildings.empty()) {
            m_currentBuildingCombo.SetCurSel(0);
            m_nCurrentBuilding = 0;
        }
    } else {
        acutPrintf(_T("\n加载大楼数据失败: %s\n"), WStringToCString(errorMsg));
        // 使用默认数据
        m_currentBuildingCombo.AddString(_T("A电力大楼"));
        m_currentBuildingCombo.AddString(_T("B办公大楼"));
        m_currentBuildingCombo.AddString(_T("C住宅楼"));
        m_currentBuildingCombo.SetCurSel(0);
        m_nCurrentBuilding = 0;
    }
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
    
    return CAdUiBaseDialog::PreTranslateMessage(pMsg);
}

void CTestDialog::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CAdUiBaseDialog::OnActivate(nState, pWndOther, bMinimized);
    
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
    CAdUiBaseDialog::PostNcDestroy();
    // 非模态对话框需要在PostNcDestroy中删除自己
    delete this;
}

// 按钮事件处理函数
void CTestDialog::OnBnClickedBuildingList()
{
    acutPrintf(_T("\n打开大楼列表窗口\n"));
    
    // 创建并显示大楼列表窗口
    BuildBuildingTableWindow* pBuildingWindow = new BuildBuildingTableWindow(this);
    pBuildingWindow->DoModal();
  /*  pBuildingWindow->Create(IDD_BuildBuildingTableWindow, this);
    pBuildingWindow->ShowWindow(SW_SHOW);*/
}

void CTestDialog::OnBnClickedDrawingList()
{
    acutPrintf(_T("\n打开图纸列表窗口\n"));
    
    // 创建并显示图纸列表窗口
    SheetListWindow* pSheetWindow = new SheetListWindow(this);
    pSheetWindow->DoModal();
		/* pSheetWindow->Create(IDD_SheetListWindow, this);
		 pSheetWindow->ShowWindow(SW_SHOW);*/
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
        
        // 检查是否是文本搜索结果项
        DWORD_PTR itemData = m_drawingTree.GetItemData(hSelectedItem);
        if (itemData != 0) {
            // 这是一个搜索结果项，尝试打开图纸并定位
            int resultIndex = (int)itemData - 1; // 减1因为我们存储时加了1
            if (resultIndex >= 0 && resultIndex < m_searchResults.size()) {
                const TextSearchResult& result = m_searchResults[resultIndex];
                OpenDrawingAndLocateText(result.filePath, result);
            }
        }
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
    
    // 获取当前选择的大楼
    CString currentBuilding;
    m_currentBuildingCombo.GetLBText(m_nCurrentBuilding, currentBuilding);
    
    acutPrintf(_T("\n开始搜索：关键词=%s，大楼=%s\n"), m_strSearchText, currentBuilding);
    
    // 首先从服务器获取可用图纸列表并下载
    std::vector<std::wstring> availableDrawings = GetAvailableDrawingsFromServer();
    if (!availableDrawings.empty()) {
        acutPrintf(_T("\n正在下载图纸文件...\n"));
        for (const auto& drawing : availableDrawings) {
            DownloadDrawingIfNeeded(drawing);
        }
    }
    
    // 执行文本搜索
    std::wstring searchText = CStringToWString(m_strSearchText);
    std::wstring buildingName = CStringToWString(currentBuilding);
    std::wstring errorMsg;
    
    m_searchResults = SearchTextInDwg::searchTextInDrawings(searchText, buildingName, errorMsg);
    
    if (!errorMsg.empty()) {
        // 搜索失败，可能需要建立索引
        CString msg;
        msg.Format(_T("搜索失败: %s\n\n是否为当前大楼建立文本索引？"), WStringToCString(errorMsg));
        
        if (MessageBox(msg, _T("搜索提示"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            acutPrintf(_T("\n正在建立文本索引...\n"));
            
            std::wstring indexErrorMsg;
            if (SearchTextInDwg::buildTextIndexForBuilding(buildingName, indexErrorMsg)) {
                acutPrintf(_T("\n文本索引建立成功，请重新搜索\n"));
                MessageBox(_T("文本索引建立成功，请重新搜索"), _T("索引建立"), MB_OK | MB_ICONINFORMATION);
            } else {
                CString errorText;
                errorText.Format(_T("建立文本索引失败: %s"), WStringToCString(indexErrorMsg));
                MessageBox(errorText, _T("索引建立失败"), MB_OK | MB_ICONERROR);
            }
        }
        return;
    }
    
    // 更新树形控件显示搜索结果
    UpdateDrawingTreeWithSearchResults(m_searchResults);
    
    // 显示搜索结果统计
    if (m_searchResults.empty()) {
        CString msg;
        msg.Format(_T("在大楼 \"%s\" 中未找到包含 \"%s\" 的图纸"), currentBuilding, m_strSearchText);
        MessageBox(msg, _T("搜索结果"), MB_OK | MB_ICONINFORMATION);
    } else {
        // 统计找到的图纸数量
        std::set<std::wstring> uniqueFiles;
        for (const auto& result : m_searchResults) {
            uniqueFiles.insert(result.filePath);
        }
        
        CString msg;
        msg.Format(_T("搜索完成：在 %d 张图纸中找到 %d 处匹配"), 
                   (int)uniqueFiles.size(), (int)m_searchResults.size());
        acutPrintf(_T("\n%s\n"), msg);
    }
}

void CTestDialog::UpdateDrawingTreeWithSearchResults(const std::vector<TextSearchResult>& results)
{
    // 清空当前树形控件
    ClearSearchResults();
    
    if (results.empty()) {
        return;
    }
    
    // 按图纸文件分组显示搜索结果
    std::map<std::wstring, std::vector<int>> fileGroups;
    for (int i = 0; i < results.size(); ++i) {
        fileGroups[results[i].filePath].push_back(i);
    }
    
    // 为每个包含搜索结果的图纸创建树节点
    for (const auto& group : fileGroups) {
        const std::wstring& filePath = group.first;
        const std::vector<int>& resultIndices = group.second;
        
        // 获取图纸分类和名称
        CString category = ExtractCategoryFromFilePath(filePath);
        CString drawingName = ExtractDrawingNameFromFilePath(filePath);
        
        // 查找或创建分类节点
        HTREEITEM categoryItem = FindOrCreateCategoryItem(category);
        
        // 创建图纸节点
        CString drawingText;
        drawingText.Format(_T("%s (%d处匹配)"), drawingName, (int)resultIndices.size());
        HTREEITEM drawingItem = m_drawingTree.InsertItem(drawingText, categoryItem);
        
        // 为每个匹配的文本创建子节点
        for (int index : resultIndices) {
            const TextSearchResult& result = results[index];
            
            CString displayText = WStringToCString(result.textContent);
            if (displayText.GetLength() > 50) {
                displayText = displayText.Left(47) + _T("...");
            }
            
            CString textItemText;
            textItemText.Format(_T("📝 %s [%s]"), displayText, WStringToCString(result.layerName));
            
            HTREEITEM textItem = m_drawingTree.InsertItem(textItemText, drawingItem);
            // 存储搜索结果索引（加1避免0值）
            m_drawingTree.SetItemData(textItem, index + 1);
        }
        
        m_drawingTree.Expand(categoryItem, TVE_EXPAND);
        m_drawingTree.Expand(drawingItem, TVE_EXPAND);
    }
}

void CTestDialog::ClearSearchResults()
{
    m_drawingTree.DeleteAllItems();
    m_searchResults.clear();
}

HTREEITEM CTestDialog::FindOrCreateCategoryItem(const CString& category)
{
    // 查找是否已存在该分类节点
    HTREEITEM hItem = m_drawingTree.GetRootItem();
    while (hItem != NULL) {
        CString itemText = m_drawingTree.GetItemText(hItem);
        if (itemText == category || itemText == (_T("▼") + category)) {
            return hItem;
        }
        hItem = m_drawingTree.GetNextSiblingItem(hItem);
    }
    
    // 创建新的分类节点
    CString categoryText = _T("▼") + category;
    HTREEITEM categoryItem = m_drawingTree.InsertItem(categoryText, TVI_ROOT);
    m_drawingTree.Expand(categoryItem, TVE_EXPAND);
    
    return categoryItem;
}

CString CTestDialog::ExtractCategoryFromFilePath(const std::wstring& filePath)
{
    // 从文件路径中提取分类信息
    CString fileName = WStringToCString(filePath);
    fileName.MakeLower();
    
    // 根据文件名关键词判断分类
    if (fileName.Find(_T("结构")) != -1 || fileName.Find(_T("structure")) != -1) {
        return _T("结构");
    } else if (fileName.Find(_T("建筑")) != -1 || fileName.Find(_T("architecture")) != -1) {
        return _T("建筑");
    } else if (fileName.Find(_T("给排水")) != -1 || fileName.Find(_T("plumbing")) != -1) {
        return _T("给水排水");
    } else if (fileName.Find(_T("电气")) != -1 || fileName.Find(_T("electrical")) != -1) {
        return _T("电气");
    } else if (fileName.Find(_T("暖通")) != -1 || fileName.Find(_T("hvac")) != -1) {
        return _T("空调通风");
    } else if (fileName.Find(_T("装修")) != -1 || fileName.Find(_T("decoration")) != -1) {
        return _T("装饰装修");
    } else {
        return _T("其他");
    }
}

CString CTestDialog::ExtractDrawingNameFromFilePath(const std::wstring& filePath)
{
    // 提取文件名（不含路径和扩展名）
    size_t lastSlash = filePath.find_last_of(L"\\/");
    size_t lastDot = filePath.find_last_of(L'.');
    
    std::wstring fileName;
    if (lastSlash != std::wstring::npos) {
        if (lastDot != std::wstring::npos && lastDot > lastSlash) {
            fileName = filePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
        } else {
            fileName = filePath.substr(lastSlash + 1);
        }
    } else {
        if (lastDot != std::wstring::npos) {
            fileName = filePath.substr(0, lastDot);
        } else {
            fileName = filePath;
        }
    }
    
    return WStringToCString(fileName);
}

void CTestDialog::OpenDrawingAndLocateText(const std::wstring& filePath, const TextSearchResult& textResult)
{
    acutPrintf(_T("\n准备打开图纸并定位文本: %s\n"), WStringToCString(filePath));
    acutPrintf(_T("文本内容: %s\n"), WStringToCString(textResult.textContent));
    acutPrintf(_T("实体句柄: %s\n"), WStringToCString(textResult.entityHandle));
    acutPrintf(_T("位置: %.3f, %.3f, %.3f\n"), textResult.posX, textResult.posY, textResult.posZ);
    
    std::wstring errorMsg;
    bool success = false;
    
    if (!textResult.entityHandle.empty()) {
        // 优先使用实体句柄定位
        success = DwgViewerService::openDwgAndLocateText(filePath, textResult.entityHandle, errorMsg);
    } else if (textResult.posX != 0.0 || textResult.posY != 0.0 || textResult.posZ != 0.0) {
        // 如果没有句柄，使用坐标定位
        success = DwgViewerService::openDwgAndLocatePosition(filePath, textResult.posX, textResult.posY, textResult.posZ, errorMsg);
    } else {
        errorMsg = L"没有可用的定位信息";
    }
    
    if (success) {
        CString msg;
        msg.Format(_T("已成功打开图纸并定位到文本: %s"), WStringToCString(textResult.textContent));
        MessageBox(msg, _T("定位成功"), MB_OK | MB_ICONINFORMATION);
    } else {
        CString msg;
        msg.Format(_T("无法打开图纸或定位到文本\n错误: %s"), WStringToCString(errorMsg));
        MessageBox(msg, _T("定位失败"), MB_OK | MB_ICONWARNING);
    }
}

void CTestDialog::LoadBuildingData(int buildingIndex)
{
    CString buildingName;
    m_currentBuildingCombo.GetLBText(buildingIndex, buildingName);
    
    acutPrintf(_T("\n加载大楼数据：%s\n"), buildingName);
    
    // 清空搜索结果
    ClearSearchResults();
    
    // 根据选中的大楼加载相应的图纸数据
    InitializeDrawingTree(); // 临时使用，实际应该根据大楼加载不同的数据
}

// 网络和缓存相关函数实现
std::vector<std::wstring> CTestDialog::GetAvailableDrawingsFromServer()
{
    std::vector<std::wstring> drawings;
    
    // 这里应该实现从服务器获取图纸列表的逻辑
    // 暂时返回一些示例数据
    drawings.push_back(L"Drawing1.dwg");
    drawings.push_back(L"Drawing2.dwg");
    drawings.push_back(L"Drawing3.dwg");
    
    return drawings;
}

bool CTestDialog::DownloadDrawingIfNeeded(const std::wstring& serverFileName)
{
    if (!m_pSheetFileManager) {
        return false;
    }
    
    // 检查文件是否已缓存
    if (m_pSheetFileManager->isFileCached(serverFileName)) {
        return true; // 已缓存，无需下载
    }
    
    // 下载文件
    return m_pSheetFileManager->downloadFile(serverFileName);
}

std::wstring CTestDialog::GetLocalFilePath(const std::wstring& serverFileName)
{
    if (!m_pSheetFileManager) {
        return L"";
    }
    
    return m_pSheetFileManager->getLocalCachePath(serverFileName);
}

// 辅助函数实现
std::wstring CTestDialog::CStringToWString(const CString& str)
{
    return std::wstring(str.GetString());
}

CString CTestDialog::WStringToCString(const std::wstring& wstr)
{
    return CString(wstr.c_str());
}



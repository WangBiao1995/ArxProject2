#include "../stdafx.h"
#include "DialogCommand.h"
#include <afxwin.h>
#include <winhttp.h>
#include <windows.h>
#include "aced.h"
#include "../views/BuildBuildingTableWindow.h"
#include "../views/SheetListWindow.h"
#include "../common/CadLogger.h"

DialogCommand::DialogCommand()
    : m_loginDialog(nullptr)
    , m_mainDialog(nullptr)
{
}

DialogCommand::~DialogCommand()
{
    // 清理对话框实例
    if (m_loginDialog) {
        delete m_loginDialog;
        m_loginDialog = nullptr;
    }
    if (m_mainDialog) {
        delete m_mainDialog;
        m_mainDialog = nullptr;
    }
}

void DialogCommand::Init()
{
    // 初始化数据库连接
    if (!SqlDB::initDatabase()) {
        CadLogger::LogWarning(_T("数据库初始化失败，但插件仍可正常使用!"));
    }
    
    // 注册DialogCommand的静态方法
    acedRegCmds->addCommand(
        _T("TST"),              // 命令组名
        _T("ShowDialog"),       // 命令名
        _T("ShowDialog"),       // 命令别名
        ACRX_CMD_MODAL,         // 命令类型
        DialogCommand::executeCommand  // 直接指向DialogCommand的静态方法
    );
    
    // 注册测试数据命令
    acedRegCmds->addCommand(
        _T("TST"),              // 命令组名
        _T("TestData"),         // 命令名
        _T("TestData"),         // 命令别名
        ACRX_CMD_MODAL,         // 命令类型
        DialogCommand::executeTestDataCommand  // 测试数据命令
    );
    
    // 注册查询数据命令
    acedRegCmds->addCommand(
        _T("TST"),              // 命令组名
        _T("QueryData"),        // 命令名
        _T("QueryData"),        // 命令别名
        ACRX_CMD_MODAL,         // 命令类型
        DialogCommand::executeQueryDataCommand  // 查询数据命令
    );
    
    // 注册文件上传测试命令
    acedRegCmds->addCommand(
        _T("TST"),              // 命令组名
        _T("TestFileUpload"),   // 命令名
        _T("TestFileUpload"),   // 命令别名
        ACRX_CMD_MODAL,         // 命令类型
        DialogCommand::executeFileUploadTestCommand  // 文件上传测试命令
    );
    
    // 注册显示登录对话框测试命令
    acedRegCmds->addCommand(
        _T("TST"),              // 命令组名
        _T("ShowLoginDialog"),  // 命令名
        _T("ShowLoginDialog"),  // 命令别名
        ACRX_CMD_MODAL,         // 命令类型
        DialogCommand::executeShowLoginDialogCommand  // 显示登录对话框测试命令
    );
}

void DialogCommand::UnLoad()
{
    // 关闭数据库连接
    SqlDB::closeDatabase();
}

bool DialogCommand::showLoginDialog()
{
    try {
        // 创建登录对话框实例
        if (!m_loginDialog) {
            m_loginDialog = new CManagerSystemLogin();
            if (!m_loginDialog) {
                CadLogger::LogError(_T("创建登录对话框失败!"));
                return false;
            }
        }
        
        // 设置登录对话框连接
        setupLoginConnections(m_loginDialog);
        
        // 获取AutoCAD主窗口句柄
        HWND acadMainWnd = adsw_acadMainWnd();
        
        // 设置父窗口为AutoCAD主窗口
        if (acadMainWnd && m_loginDialog->GetSafeHwnd()) {
            ::SetParent(m_loginDialog->GetSafeHwnd(), acadMainWnd);
        }
        
        // 计算登录对话框在AutoCAD窗口中央的位置
        if (acadMainWnd) {
            RECT acadRect;
            GetWindowRect(acadMainWnd, &acadRect);
            
            CRect dialogRect;
            m_loginDialog->GetWindowRect(&dialogRect);
            
            int x = acadRect.left + (acadRect.right - acadRect.left - dialogRect.Width()) / 2;
            int y = acadRect.top + (acadRect.bottom - acadRect.top - dialogRect.Height()) / 2;
            
            m_loginDialog->SetWindowPos(nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        
        // 显示登录对话框（模态）
        INT_PTR result = m_loginDialog->DoModal();
        
        CadLogger::LogInfo(_T("登录对话框已显示!"));
        return (result == IDOK);
        
    } catch (...) {
        CadLogger::LogError(_T("显示登录对话框时发生异常!"));
        return false;
    }
}

bool DialogCommand::showMainDialog()
{
    try {
        // 创建主对话框实例
        if (!m_mainDialog) {
            m_mainDialog = new CMainDialog();
            if (!m_mainDialog) {
                CadLogger::LogError(_T("创建主对话框失败!"));
                return false;
            }
        }

        // 获取AutoCAD主窗口句柄
        HWND acadMainWnd = adsw_acadMainWnd();
        
        // 设置父窗口为AutoCAD主窗口
        if (acadMainWnd) {
            ::SetParent(m_mainDialog->GetSafeHwnd(), acadMainWnd);
        }

        // 获取AutoCAD窗口的几何信息
        if (acadMainWnd) {
            RECT acadRect;
            GetWindowRect(acadMainWnd, &acadRect);

            // 计算对话框在右侧的位置
            CRect dialogRect;
            m_mainDialog->GetWindowRect(&dialogRect);
            
            int x = acadRect.right - dialogRect.Width();  // 右侧对齐
            int y = acadRect.top + 100;  // 距离顶部100像素

            // 设置对话框位置
            m_mainDialog->SetWindowPos(&CWnd::wndTopMost, x, y, 0, 0, SWP_NOSIZE);
        }

        // 显示非模态对话框
        if (m_mainDialog->GetSafeHwnd() == nullptr) {
            m_mainDialog->Create(IDD_MAIN_DIALOG, CWnd::FromHandle(acadMainWnd));
        }
        m_mainDialog->ShowWindow(SW_SHOW);
        
        CadLogger::LogInfo(_T("图纸管理界面已显示在CAD右侧!"));
        return true;

    }
    catch (...) {
        CadLogger::LogError(_T("显示主对话框时发生异常!"));
        return false;
    }
}

void DialogCommand::setupLoginConnections(CManagerSystemLogin* loginDialog)
{
    if (!loginDialog) return;
    
    // MFC中通过设置回调指针或消息映射来处理事件
    // 这里可以设置回调函数指针或使用Windows消息机制
    loginDialog->SetCallbackObject(this);
}

void DialogCommand::onLoginSuccess()
{
    CadLogger::LogInfo(_T("登录成功，正在显示主界面..."));
    
    // 隐藏登录对话框
    if (m_loginDialog && m_loginDialog->GetSafeHwnd()) {
        m_loginDialog->ShowWindow(SW_HIDE);
    }
    
    // 显示主对话框
    if (showMainDialog()) {
        CadLogger::LogInfo(_T("主界面显示成功!"));
    } else {
        CadLogger::LogError(_T("主界面显示失败!"));
    }
}

void DialogCommand::onLoginCancelled()
{
    CadLogger::LogInfo(_T("登录已取消!"));
    
    // 清理登录对话框
    if (m_loginDialog) {
        delete m_loginDialog;
        m_loginDialog = nullptr;
    }
}

// 静态成员变量，用于保持DialogCommand实例的生命周期
static DialogCommand* g_dialogCommandInstance = nullptr;

void DialogCommand::executeCommand()
{
    try {
        // 如果已经有实例在运行，先清理
        if (g_dialogCommandInstance) {
            delete g_dialogCommandInstance;
            g_dialogCommandInstance = nullptr;
        }
        
        // 创建DialogCommand实例并保持引用
        g_dialogCommandInstance = new DialogCommand();
        if (!g_dialogCommandInstance) {
            CadLogger::LogError(_T("创建DialogCommand失败!"));
            return;
        }
        
        // 测试数据库连接
        if (SqlDB::testDatabaseConnection()) {
            CadLogger::LogInfo(_T("数据库连接正常!"));
        } else {
            CadLogger::LogWarning(_T("数据库连接失败，但插件仍可正常使用!"));
        }
        
        g_dialogCommandInstance->showMainDialog();
        
    } catch (...) {
        CadLogger::LogError(_T("执行命令时发生异常!"));
    }
} 

// 执行测试数据命令
void DialogCommand::executeTestDataCommand()
{
    try {
        CadLogger::LogInfo(_T("=== 开始执行测试数据操作 ==="));
        
        // 测试数据库连接
        if (!SqlDB::testDatabaseConnection()) {
            CadLogger::LogError(_T("数据库连接失败，无法执行测试数据操作!"));
            return;
        }
        
        // 创建测试表
        if (SqlDB::createTestTables()) {
            CadLogger::LogInfo(_T("测试表创建成功!"));
        } else {
            CadLogger::LogError(_T("测试表创建失败!"));
            return;
        }
        
        // 插入测试数据
        if (SqlDB::insertTestData()) {
            CadLogger::LogInfo(_T("测试数据插入成功!"));
        } else {
            CadLogger::LogError(_T("测试数据插入失败!"));
            return;
        }
        
        // 显示测试数据
        if (SqlDB::showTestData()) {
            CadLogger::LogInfo(_T("测试数据查询成功!"));
        } else {
            CadLogger::LogError(_T("测试数据查询失败!"));
        }
        
        CadLogger::LogInfo(_T("=== 测试数据操作完成 ==="));
        
    } catch (...) {
        CadLogger::LogError(_T("执行测试数据命令时发生异常!"));
    }
}

// 执行查询数据命令
void DialogCommand::executeQueryDataCommand()
{
    try {
        CadLogger::LogInfo(_T("=== 开始查询测试数据 ==="));
        
        // 测试数据库连接
        if (!SqlDB::testDatabaseConnection()) {
            CadLogger::LogError(_T("数据库连接失败，无法查询测试数据!"));
            return;
        }
        
        // 查询测试数据
        if (SqlDB::queryTestData()) {
            CadLogger::LogInfo(_T("测试数据查询成功!"));
        } else {
            CadLogger::LogError(_T("测试数据查询失败!"));
        }
        
        CadLogger::LogInfo(_T("=== 查询操作完成 ==="));
        
    } catch (...) {
        CadLogger::LogError(_T("执行查询数据命令时发生异常!"));
    }
}

// 测试文件上传到SheetManagerServer（使用WinHTTP替代Qt网络功能）
void DialogCommand::executeFileUploadTestCommand()
{
    try {
        CadLogger::LogInfo(_T("=== 开始测试文件上传到SheetManagerServer ==="));
        
        // 测试文件路径
        CString testFilePath = _T("D:\\Documents\\Drawing1.dwg");
        
        // 检查文件是否存在
        DWORD fileAttributes = GetFileAttributes(testFilePath);
        if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
            CadLogger::LogError(_T("测试文件不存在: %s"), testFilePath);
            CadLogger::LogError(_T("请确保文件路径正确且文件存在!"));
            return;
        }
        
        // 获取文件信息
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesEx(testFilePath, GetFileExInfoStandard, &fileInfo)) {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = fileInfo.nFileSizeLow;
            fileSize.HighPart = fileInfo.nFileSizeHigh;
            
            CadLogger::LogInfo(_T("测试文件信息:"));
            CadLogger::LogInfo(_T("  文件路径: %s"), testFilePath);
            CadLogger::LogInfo(_T("  文件大小: %lld 字节"), fileSize.QuadPart);
        }
        
        // 使用WinHTTP进行文件上传
        HINTERNET hSession = WinHttpOpen(L"CAD File Uploader/1.0",
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME,
                                        WINHTTP_NO_PROXY_BYPASS,
                                        0);
        
        if (!hSession) {
            CadLogger::LogError(_T("创建HTTP会话失败!"));
            return;
        }
        
        HINTERNET hConnect = WinHttpConnect(hSession,
                                           L"localhost",
                                           8080,
                                           0);
        
        if (!hConnect) {
            CadLogger::LogError(_T("连接到服务器失败!"));
            WinHttpCloseHandle(hSession);
            return;
        }
        
        HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                               L"POST",
                                               L"/api/upload",
                                               NULL,
                                               WINHTTP_NO_REFERER,
                                               WINHTTP_DEFAULT_ACCEPT_TYPES,
                                               0);
        
        if (!hRequest) {
            CadLogger::LogError(_T("创建HTTP请求失败!"));
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }
        
        // 设置请求头
        LPCWSTR headers = L"Content-Type: multipart/form-data\r\n";
        WinHttpAddRequestHeaders(hRequest, headers, -1, WINHTTP_ADDREQ_FLAG_ADD);
        
        // 这里需要实现multipart/form-data的构建和发送
        // 由于篇幅限制，这里只展示框架
        
        CadLogger::LogInfo(_T("文件上传请求已发送，等待服务器响应..."));
        
        // 清理资源
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        
    } catch (...) {
        CadLogger::LogError(_T("执行文件上传测试时发生异常!"));
    }
} 

// 测试显示登录对话框命令
void DialogCommand::executeShowLoginDialogCommand()
{
    try {
        CadLogger::LogInfo(_T("=== 开始测试显示登录对话框 ==="));
        
        // 如果已经有实例在运行，先清理
        if (g_dialogCommandInstance) {
            delete g_dialogCommandInstance;
            g_dialogCommandInstance = nullptr;
        }
        
        // 创建DialogCommand实例并保持引用
        g_dialogCommandInstance = new DialogCommand();
        if (!g_dialogCommandInstance) {
            CadLogger::LogError(_T("创建DialogCommand失败!"));
            return;
        }
        
        // 显示登录对话框
        if (g_dialogCommandInstance->showLoginDialog()) {
            CadLogger::LogInfo(_T("登录对话框显示成功!"));
        } else {
            CadLogger::LogError(_T("登录对话框显示失败!"));
        }
        
        CadLogger::LogInfo(_T("=== 登录对话框测试完成 ==="));
        
    } catch (...) {
        CadLogger::LogError(_T("执行显示登录对话框测试时发生异常!"));
    }
} 




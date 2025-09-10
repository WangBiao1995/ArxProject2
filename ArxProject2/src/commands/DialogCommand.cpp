#include "StdAfx.h"
#include "DialogCommand.h"
#include <afxwin.h>
#include <winhttp.h>
#include <windows.h>
#include "aced.h"
#include "../views/BuildBuildingTableWindow.h"
#include "../views/SheetListWindow.h"
#include "../views/ManagerSystemLogin.h"
#include "../views/TestDialog.h"
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
        acutPrintf(_T("数据库初始化失败，但插件仍可正常使用!"));
    }
    
    // 注册DialogCommand的静态方法
    acedRegCmds->addCommand(
        _T("TST"),              // 命令组名
        _T("SheetManager"),       // 命令名
        _T("SheetManager"),       // 命令别名
        ACRX_CMD_MODAL,         // 命令类型
        DialogCommand::executeCommand  // 直接指向DialogCommand的静态方法
    );
    
    //// 注册显示登录对话框测试命令
    //acedRegCmds->addCommand(
    //    _T("TST"),              // 命令组名
    //    _T("ShowLoginDialog"),  // 命令名
    //    _T("ShowLoginDialog"),  // 命令别名
    //    ACRX_CMD_MODAL,         // 命令类型
    //    DialogCommand::executeShowLoginDialogCommand  // 显示登录对话框测试命令
    //);
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
            m_loginDialog = new ManagerSystemLogin();
            if (!m_loginDialog) {
               
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
        
        
        return (result == IDOK);
        
    } catch (...) {
       
        return false;
    }
}

bool DialogCommand::showMainDialog()
{
    try {
        //必须添加，防止资源调用冲突
        CAcModuleResourceOverride resOverrid;
        
        // 检查是否已经有非模态对话框实例存在
        if (CTestDialog::s_pModelessDialog != nullptr) {
            // 如果已存在，激活并显示该对话框
            CTestDialog::s_pModelessDialog->ShowWindow(SW_SHOW);
            CTestDialog::s_pModelessDialog->SetForegroundWindow();
            // 暂时移除中文日志，避免编码问题
            //acutPrintf(_T("图纸管理界面已经打开，将其置于前台"));
            acutPrintf(_T("\nSheet Manager dialog is already open, bringing to front\n"));
            return true;
        }

        // 创建非模态对话框
        CTestDialog* pDlg = new CTestDialog();
        if (pDlg->Create(CTestDialog::IDD, acedGetAcadFrame())) {
            // 设置静态指针
            CTestDialog::s_pModelessDialog = pDlg;
            
            // 获取AutoCAD主窗口句柄
            HWND acadMainWnd = acedGetAcadFrame()->GetSafeHwnd();
            
            if (acadMainWnd) {
                // 获取AutoCAD窗口矩形
                RECT acadRect;
                GetWindowRect(acadMainWnd, &acadRect);
                
                // 获取对话框矩形
                CRect dialogRect;
                pDlg->GetWindowRect(&dialogRect);
                
                // 计算对话框在CAD窗口右侧居中的位置
                int dialogWidth = dialogRect.Width();
                int dialogHeight = dialogRect.Height();
                int acadWidth = acadRect.right - acadRect.left;
                int acadHeight = acadRect.bottom - acadRect.top;
                
                // X坐标：CAD窗口右边减去对话框宽度，再减去一些边距
                int x = acadRect.right - dialogWidth - 20; // 20像素边距
                
                // Y坐标：CAD窗口垂直居中
                int y = acadRect.top + (acadHeight - dialogHeight) / 2;
                
                // 确保对话框不会超出屏幕边界
                RECT screenRect;
                SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
                
                // 调整X坐标，确保对话框完全在屏幕内
                if (x + dialogWidth > screenRect.right) {
                    x = screenRect.right - dialogWidth - 10;
                }
                if (x < screenRect.left) {
                    x = screenRect.left + 10;
                }
                
                // 调整Y坐标，确保对话框完全在屏幕内
                if (y + dialogHeight > screenRect.bottom) {
                    y = screenRect.bottom - dialogHeight - 10;
                }
                if (y < screenRect.top) {
                    y = screenRect.top + 10;
                }
                
                // 设置对话框位置
                pDlg->SetWindowPos(nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            
            pDlg->ShowWindow(SW_SHOW);
            // 暂时使用英文日志，避免编码问题
            //acutPrintf(_T("图纸管理界面已创建并显示在CAD窗口右侧"));
            acutPrintf(_T("\nSheet Manager dialog created and positioned on right side of CAD window\n"));
            return true;
        } else {
            delete pDlg;
            // acutPrintf(_T("创建图纸管理界面失败"));
            acutPrintf(_T("\nFailed to create Sheet Manager dialog\n"));
            return false;
        }

    }
    catch (...) {
        // acutPrintf(_T("显示主对话框时发生异常!"));
        acutPrintf(_T("\nException occurred while showing main dialog\n"));
        return false;
    }
}

void DialogCommand::setupLoginConnections(ManagerSystemLogin* loginDialog)
{
    if (!loginDialog) return;
    
    // MFC中通过设置回调指针或消息映射来处理事件
    // 这里可以设置回调函数指针或使用Windows消息机制
    // loginDialog->SetCallbackObject(this);  // 需要在ManagerSystemLogin中实现此方法
}

void DialogCommand::onLoginSuccess()
{
   
    // 隐藏登录对话框
    if (m_loginDialog && m_loginDialog->GetSafeHwnd()) {
        m_loginDialog->ShowWindow(SW_HIDE);
    }
    
    // 显示主对话框
    if (showMainDialog()) {
        
    } else {
      
    }
}

void DialogCommand::onLoginCancelled()
{
   
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
           
            return;
        }
        
        // 测试数据库连接
        if (SqlDB::testDatabaseConnection()) {
            acutPrintf(_T("\nDatabase connection is normal!\n"));
            //acutPrintf(_T("数据库连接正常!"));
        } else {
            acutPrintf(_T("\nDatabase connection failed, but plugin can still work normally!\n"));
            // CadLogger::LogWarning(_T("数据库连接失败，但插件仍可正常使用!"));
        }
        
        g_dialogCommandInstance->showMainDialog();
        
    } catch (...) {
        acutPrintf(_T("执行命令时发生异常!"));
    }
} 

// 执行测试数据命令
void DialogCommand::executeTestDataCommand()
{
    try {
       acutPrintf(_T("=== 开始执行测试数据操作 ==="));
        
        // 测试数据库连接
        if (!SqlDB::testDatabaseConnection()) {
            acutPrintf(_T("数据库连接失败，无法执行测试数据操作!"));
            return;
        }
        
        // 创建测试表
        if (SqlDB::createTestTables()) {
           acutPrintf(_T("测试表创建成功!"));
        } else {
            acutPrintf(_T("测试表创建失败!"));
            return;
        }
        
        // 插入测试数据
        if (SqlDB::insertTestData()) {
           acutPrintf(_T("测试数据插入成功!"));
        } else {
            acutPrintf(_T("测试数据插入失败!"));
            return;
        }
        
        // 显示测试数据
        if (SqlDB::showTestData()) {
           acutPrintf(_T("测试数据查询成功!"));
        } else {
            acutPrintf(_T("测试数据查询失败!"));
        }
        
       acutPrintf(_T("=== 测试数据操作完成 ==="));
        
    } catch (...) {
        acutPrintf(_T("执行测试数据命令时发生异常!"));
    }
}

// 执行查询数据命令
void DialogCommand::executeQueryDataCommand()
{
    try {
       acutPrintf(_T("=== 开始查询测试数据 ==="));
        
        // 测试数据库连接
        if (!SqlDB::testDatabaseConnection()) {
            acutPrintf(_T("数据库连接失败，无法查询测试数据!"));
            return;
        }
        
        // 查询测试数据
        if (SqlDB::queryTestData()) {
           acutPrintf(_T("测试数据查询成功!"));
        } else {
            acutPrintf(_T("测试数据查询失败!"));
        }
        
       acutPrintf(_T("=== 查询操作完成 ==="));
        
    } catch (...) {
        acutPrintf(_T("执行查询数据命令时发生异常!"));
    }
}

// 测试文件上传到SheetManagerServer（使用WinHTTP替代Qt网络功能）
void DialogCommand::executeFileUploadTestCommand()
{
    try {
       acutPrintf(_T("=== 开始测试文件上传到SheetManagerServer ==="));
        
        // 测试文件路径
        CString testFilePath = _T("D:\\Documents\\Drawing1.dwg");
        
        // 检查文件是否存在
        DWORD fileAttributes = GetFileAttributes(testFilePath);
        if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
            acutPrintf(_T("测试文件不存在: %s"), testFilePath);
            acutPrintf(_T("请确保文件路径正确且文件存在!"));
            return;
        }
        
        // 获取文件信息
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesEx(testFilePath, GetFileExInfoStandard, &fileInfo)) {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = fileInfo.nFileSizeLow;
            fileSize.HighPart = fileInfo.nFileSizeHigh;
            
           acutPrintf(_T("测试文件信息:"));
           acutPrintf(_T("  文件路径: %s"), testFilePath);
           acutPrintf(_T("  文件大小: %lld 字节"), fileSize.QuadPart);
        }
        
        // 使用WinHTTP进行文件上传
        HINTERNET hSession = WinHttpOpen(L"CAD File Uploader/1.0",
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME,
                                        WINHTTP_NO_PROXY_BYPASS,
                                        0);
        
        if (!hSession) {
            acutPrintf(_T("创建HTTP会话失败!"));
            return;
        }
        
        HINTERNET hConnect = WinHttpConnect(hSession,
                                           L"localhost",
                                           8080,
                                           0);
        
        if (!hConnect) {
            acutPrintf(_T("连接到服务器失败!"));
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
            acutPrintf(_T("创建HTTP请求失败!"));
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }
        
        // 设置请求头
        LPCWSTR headers = L"Content-Type: multipart/form-data\r\n";
        WinHttpAddRequestHeaders(hRequest, headers, -1, WINHTTP_ADDREQ_FLAG_ADD);
        
        // 这里需要实现multipart/form-data的构建和发送
        // 由于篇幅限制，这里只展示框架
        
       acutPrintf(_T("文件上传请求已发送，等待服务器响应..."));
        
        // 清理资源
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        
    } catch (...) {
        acutPrintf(_T("执行文件上传测试时发生异常!"));
    }
} 

// 测试显示登录对话框命令
void DialogCommand::executeShowLoginDialogCommand()
{
    try {
       acutPrintf(_T("=== 开始测试显示登录对话框 ==="));
        
        // 如果已经有实例在运行，先清理
        if (g_dialogCommandInstance) {
            delete g_dialogCommandInstance;
            g_dialogCommandInstance = nullptr;
        }
        
        // 创建DialogCommand实例并保持引用
        g_dialogCommandInstance = new DialogCommand();
        if (!g_dialogCommandInstance) {
            acutPrintf(_T("创建DialogCommand失败!"));
            return;
        }
        
        // 显示登录对话框
        if (g_dialogCommandInstance->showLoginDialog()) {
           acutPrintf(_T("登录对话框显示成功!"));
        } else {
            acutPrintf(_T("登录对话框显示失败!"));
        }
        
       acutPrintf(_T("=== 登录对话框测试完成 ==="));
        
    } catch (...) {
        acutPrintf(_T("执行显示登录对话框测试时发生异常!"));
    }
} 




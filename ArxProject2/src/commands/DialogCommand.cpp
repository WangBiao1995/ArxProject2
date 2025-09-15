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
#include "../common/Database/NetWorkSqlDb.h"  // 替换 SqlDB.h

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
    // 初始化网络数据库连接
    if (!NetWorkSqlDb::initialize(L"http://192.168.1.77:8000")) {
        acutPrintf(_T("网络数据库初始化失败，但插件仍可正常使用!"));
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
    // 网络数据库不需要显式关闭连接
    // NetWorkSqlDb 使用静态连接，程序结束时自动清理
}

bool DialogCommand::showLoginDialog()
{
    try {
        // 创建登录对话框实例
        if (!m_loginDialog) {
            m_loginDialog = new ManagerSystemLogin();
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
        //必须添加，防止资源调用冲突
        CAcModuleResourceOverride resOverrid;
        
        // 检查是否已经有非模态对话框实例存在
        if (CTestDialog::s_pModelessDialog != nullptr) {
            // 如果已存在，激活并显示该对话框
            CTestDialog::s_pModelessDialog->ShowWindow(SW_SHOW);
            CTestDialog::s_pModelessDialog->SetForegroundWindow();
            // 暂时移除中文日志，避免编码问题
            // CadLogger::LogInfo(_T("图纸管理界面已经打开，将其置于前台"));
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
            // CadLogger::LogInfo(_T("图纸管理界面已创建并显示在CAD窗口右侧"));
            acutPrintf(_T("\nSheet Manager dialog created and positioned on right side of CAD window\n"));
            return true;
        } else {
            delete pDlg;
            // CadLogger::LogError(_T("创建图纸管理界面失败"));
            acutPrintf(_T("\nFailed to create Sheet Manager dialog\n"));
            return false;
        }

    }
    catch (...) {
        // CadLogger::LogError(_T("显示主对话框时发生异常!"));
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
        
        // 测试网络数据库连接 - 通过获取建筑列表来测试
        std::vector<BuildingInfo> buildings;
        std::wstring errorMsg;
        if (NetWorkSqlDb::getBuildings(buildings, 1, 1, L"", L"", L"", L"", errorMsg)) {
            acutPrintf(_T("\n网络数据库连接正常!\n"));
        } else {
            acutPrintf(_T("\n网络数据库连接失败，但插件仍可正常使用!\n"));
        }
        
        g_dialogCommandInstance->showMainDialog();
        
    } catch (...) {
        CadLogger::LogError(_T("执行命令时发生异常!"));
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




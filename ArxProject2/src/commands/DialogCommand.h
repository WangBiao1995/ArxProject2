#pragma once

#include <afxwin.h>
#include "../common/Database/SqlDB.h"

// 前向声明MFC对话框类
class CManagerSystemLogin;
class CMainDialog;

class DialogCommand
{
public:
    DialogCommand();
    ~DialogCommand();

    static void executeCommand();
    static void executeTestDataCommand();
    static void executeQueryDataCommand();
    static void executeFileUploadTestCommand(); // 文件上传测试方法
    static void executeShowLoginDialogCommand(); // 显示登录对话框测试方法
    static void Init();
    static void UnLoad();
   
    // 回调方法（替代Qt槽函数）
    void onLoginSuccess();
    void onLoginCancelled();

private:
    bool showLoginDialog();
    bool showMainDialog();
    void setupLoginConnections(CManagerSystemLogin* loginDialog);
    
    CManagerSystemLogin* m_loginDialog;
    CMainDialog* m_mainDialog;
}; 
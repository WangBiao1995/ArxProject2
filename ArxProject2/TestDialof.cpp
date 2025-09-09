#include "StdAfx.h"
#include "TestDialof.h"
#include "resource.h"

// CTestDialog 对话框

IMPLEMENT_DYNAMIC(CTestDialog, CAcUiDialog)

// 静态成员变量定义
CTestDialog* CTestDialog::s_pModelessDialog = nullptr;

CTestDialog::CTestDialog(CWnd* pParent /*=nullptr*/)
    : CAcUiDialog(CTestDialog::IDD, pParent)
    , m_strEditText(_T(""))
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
    DDX_Text(pDX, IDC_EDIT1, m_strEditText);
}

BEGIN_MESSAGE_MAP(CTestDialog, CAcUiDialog)
    ON_BN_CLICKED(IDOK, &CTestDialog::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CTestDialog::OnBnClickedCancel)
    ON_WM_CLOSE()
    ON_WM_ACTIVATE()
    ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
END_MESSAGE_MAP()

// CTestDialog 消息处理程序

BOOL CTestDialog::OnInitDialog()
{
    CAcUiDialog::OnInitDialog();

    // 设置对话框标题
    SetWindowText(_T("测试对话框"));
    
    // 设置编辑框默认文本
    m_strEditText = _T("这是一个测试窗口");
    UpdateData(FALSE);

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

BOOL CTestDialog::PreTranslateMessage(MSG* pMsg)
{
    // 处理ESC键，关闭非模态对话框
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
    {
        PostMessage(WM_CLOSE);
        return TRUE;
    }
    
    // 处理回车键，执行确定操作
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        CWnd* pFocusWnd = GetFocus();
        if (pFocusWnd && pFocusWnd->GetDlgCtrlID() == IDC_EDIT1)
        {
            // 如果焦点在编辑框上，让编辑框处理回车（换行）
            return CAcUiDialog::PreTranslateMessage(pMsg);
        }
        else
        {
            // 否则执行确定操作
            PostMessage(WM_COMMAND, IDOK, 0);
            return TRUE;
        }
    }
    
    // 处理键盘消息，确保编辑框能正常接收输入
    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_CHAR)
    {
        CWnd* pFocusWnd = GetFocus();
        if (pFocusWnd && pFocusWnd->GetDlgCtrlID() == IDC_EDIT1)
        {
            // 如果焦点在编辑框上，让编辑框处理消息
            return CAcUiDialog::PreTranslateMessage(pMsg);
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
    
    // 显示用户输入的文本
    CString msg;
    msg.Format(_T("您输入的文本是：%s"), m_strEditText);
    acutPrintf(_T("\n%s\n"), msg);
    
    // 非模态对话框不调用OnOK，而是销毁窗口
    DestroyWindow();
}

void CTestDialog::OnBnClickedCancel()
{
    acutPrintf(_T("\n用户取消了对话框操作\n"));
    // 非模态对话框不调用OnCancel，而是销毁窗口
    DestroyWindow();
}

void CTestDialog::OnClose()
{
    acutPrintf(_T("\n对话框被关闭\n"));
    DestroyWindow();
}

void CTestDialog::PostNcDestroy()
{
    CAcUiDialog::PostNcDestroy();
    // 非模态对话框需要在PostNcDestroy中删除自己
    delete this;
}



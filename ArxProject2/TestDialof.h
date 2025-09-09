#pragma once
#include "StdAfx.h"
#include "resource.h"

// CTestDialog 对话框类
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
    CString m_strEditText;  // 编辑框文本

public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnClose();  // 添加关闭消息处理
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
    
    // 静态指针用于管理非模态对话框实例
    static CTestDialog* s_pModelessDialog;
};

// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- ManagerSystemLogin.cpp : Implementation of ManagerSystemLogin
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "../../resource.h"
#include "ManagerSystemLogin.h"

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(ManagerSystemLogin, CAdUiBaseDialog)



BEGIN_MESSAGE_MAP(ManagerSystemLogin, CAdUiBaseDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
	ON_BN_CLICKED(IDC_LOGIN_BUTTON, OnLoginButtonClicked)
	ON_BN_CLICKED(IDC_EXIT_BUTTON, OnExitButtonClicked)
	ON_WM_CLOSE()
	ON_STN_CLICKED(IDC_LOGIN_USERNAME_LABEL, &ManagerSystemLogin::OnStnClickedLoginUsernameLabel)
END_MESSAGE_MAP()


ManagerSystemLogin::ManagerSystemLogin(CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) 
	: CAdUiBaseDialog(ManagerSystemLogin::IDD, pParent, hInstance)
	, m_strUsername(_T("admin"))  // 默认用户名
	, m_strPassword(_T(""))
{
}

ManagerSystemLogin::~ManagerSystemLogin()
{
	
}

//-----------------------------------------------------------------------------
void ManagerSystemLogin::DoDataExchange(CDataExchange *pDX)
{
	CAdUiBaseDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOGIN_USERNAME_EDIT, m_strUsername);
	DDX_Text(pDX, IDC_LOGIN_PASSWORD_EDIT, m_strPassword);
}

//-----------------------------------------------------------------------------
BOOL ManagerSystemLogin::OnInitDialog()
{
	CAdUiBaseDialog::OnInitDialog();
	
	SetupControls();
	
	// 设置焦点到用户名输入框
	GetDlgItem(IDC_LOGIN_USERNAME_EDIT)->SetFocus();
	
	return FALSE;  // 返回FALSE因为我们手动设置了焦点
}

//-----------------------------------------------------------------------------
void ManagerSystemLogin::SetupControls()
{
	// 设置窗口标题
	SetWindowText(_T("图纸管理系统登录"));
	
	// 设置默认值
	SetDlgItemText(IDC_LOGIN_USERNAME_EDIT, m_strUsername);
	
	// 设置密码框为密码模式（已在资源文件中设置ES_PASSWORD）
	
	// 设置窗口大小为固定大小
	CRect rect;
	GetWindowRect(&rect);
	SetWindowPos(NULL, 0, 0, 380, 300, SWP_NOMOVE | SWP_NOZORDER);
	
	// 居中显示
	CenterWindow();
}

//-----------------------------------------------------------------------------
void ManagerSystemLogin::OnLoginButtonClicked()
{
	UpdateData(TRUE);  // 获取控件数据
	
	// 验证输入
	m_strUsername.Trim();
	m_strPassword.Trim();
	
	if (m_strUsername.IsEmpty() || m_strPassword.IsEmpty()) {
		ShowLoginMessage(_T("用户名和密码不能为空！"), MB_ICONWARNING);
		GetDlgItem(IDC_LOGIN_USERNAME_EDIT)->SetFocus();
		return;
	}
	
	// 验证登录
	if (ValidateLogin(m_strUsername, m_strPassword)) {
		ShowLoginMessage(_T("欢迎使用图纸管理系统！"));
		EndDialog(LOGIN_SUCCESS);
	} else {
		ShowLoginMessage(_T("用户名或密码错误！"), MB_ICONERROR);
		SetDlgItemText(IDC_LOGIN_PASSWORD_EDIT, _T(""));
		GetDlgItem(IDC_LOGIN_PASSWORD_EDIT)->SetFocus();
	}
}

//-----------------------------------------------------------------------------
void ManagerSystemLogin::OnExitButtonClicked()
{
	EndDialog(LOGIN_CANCELLED);
}

//-----------------------------------------------------------------------------
void ManagerSystemLogin::OnClose()
{
	// 当用户点击关闭按钮时，相当于取消登录
	EndDialog(LOGIN_CANCELLED);
}

//-----------------------------------------------------------------------------
BOOL ManagerSystemLogin::ValidateLogin(const CString& username, const CString& password)
{
	// 这里实现实际的登录验证逻辑
	// 目前使用简单的硬编码验证，实际项目中应该连接数据库或调用API
	
	// 示例：管理员账户
	if (username == _T("admin") && password == _T("123456")) {
		return TRUE;
	}
	
	// 示例：普通用户账户
	if (username == _T("user") && password == _T("password")) {
		return TRUE;
	}
	
	// 示例：测试账户
	if (username == _T("test") && password == _T("test123")) {
		return TRUE;
	}
	
	return FALSE;
}

//-----------------------------------------------------------------------------
void ManagerSystemLogin::ShowLoginMessage(const CString& message, UINT type /*= MB_ICONINFORMATION*/)
{
	MessageBox(message, _T("登录提示"), type | MB_OK);
}

//-----------------------------------------------------------------------------
//----- Needed for modeless dialogs to keep focus.
//----- Return FALSE to not keep the focus, return TRUE to keep the focus
LRESULT ManagerSystemLogin::OnAcadKeepFocus(WPARAM, LPARAM)
{
	return (TRUE);
}

void ManagerSystemLogin::OnStnClickedLoginUsernameLabel()
{
	// TODO: 在此添加控件通知处理程序代码
}

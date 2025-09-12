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
//----- ManagerSystemLogin.h : Declaration of the ManagerSystemLogin
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
#include "adui.h"
#include "../../resource.h"

//-----------------------------------------------------------------------------
class ManagerSystemLogin : public CAdUiBaseDialog {
	DECLARE_DYNAMIC(ManagerSystemLogin)

public:
	
	ManagerSystemLogin(CWnd* pParent = NULL, HINSTANCE hInstance = NULL);
	virtual ~ManagerSystemLogin();

	enum { IDD = IDD_ManagerSystemLogin };

	// 登录结果枚举
	enum LoginResult {
		LOGIN_SUCCESS = 1,
		LOGIN_CANCELLED = 0,
		LOGIN_FAILED = -1
	};

protected:
	virtual void DoDataExchange(CDataExchange *pDX);
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
	afx_msg void OnLoginButtonClicked();
	afx_msg void OnExitButtonClicked();
	afx_msg void OnClose();

	DECLARE_MESSAGE_MAP()

private:

	// 控件变量
	CString m_strUsername;
	CString m_strPassword;
	
	// 私有方法
	void SetupControls();
	BOOL ValidateLogin(const CString& username, const CString& password);
	void ShowLoginMessage(const CString& message, UINT type = MB_ICONINFORMATION);
public:
	afx_msg void OnStnClickedLoginUsernameLabel();
};

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
//----- SetComponentLabel.h : Declaration of the SetComponentLabel
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
#include "adui.h"

//-----------------------------------------------------------------------------
//在CAD命令行使用(entget (car (entsel)))可查看扩展数据

class SetComponentLabel : public CAdUiBaseDialog {
	DECLARE_DYNAMIC (SetComponentLabel)

public:
	SetComponentLabel (CWnd *pParent =NULL, HINSTANCE hInstance =NULL) ;

	enum { IDD = IDD_SETCOMPONENTLABEL} ;

	// 设置选中的实体ID数组
	void SetSelectedEntities(const AcDbObjectIdArray& entIds);
	
	// 获取输入的标签数据
	CString GetMaintenanceDate() const { return m_strMaintenanceDate; }
	CString GetResponsiblePerson() const { return m_strResponsiblePerson; }

protected:
	virtual void DoDataExchange (CDataExchange *pDX) ;
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnAcadKeepFocus (WPARAM, LPARAM) ;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

	DECLARE_MESSAGE_MAP()

private:
	// 控件变量
	CString m_strMaintenanceDate;  // 维护日期
	CString m_strResponsiblePerson; // 责任人
	
	// 选中的实体ID数组
	AcDbObjectIdArray m_selectedEntities;
} ;

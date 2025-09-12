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
//----- SetComponentLabel.cpp : Implementation of SetComponentLabel
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include "SetComponentLabel.h"
#include "../services/ComponentLabelService.h"
#include "../common/CadLogger.h"

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC (SetComponentLabel, CAdUiBaseDialog)

BEGIN_MESSAGE_MAP(SetComponentLabel, CAdUiBaseDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
	ON_BN_CLICKED(IDOK, &SetComponentLabel::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &SetComponentLabel::OnBnClickedCancel)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
SetComponentLabel::SetComponentLabel (CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) 
	: CAdUiBaseDialog (SetComponentLabel::IDD, pParent, hInstance)
	, m_strMaintenanceDate(_T(""))
	, m_strResponsiblePerson(_T(""))
{
}

//-----------------------------------------------------------------------------
void SetComponentLabel::DoDataExchange (CDataExchange *pDX) {
	CAdUiBaseDialog::DoDataExchange (pDX) ;
	DDX_Text(pDX, IDC_EDIT1, m_strMaintenanceDate);
	DDX_Text(pDX, IDC_EDIT2, m_strResponsiblePerson);
}

//-----------------------------------------------------------------------------
BOOL SetComponentLabel::OnInitDialog()
{
	CAdUiBaseDialog::OnInitDialog();
	
	// 设置对话框标题
	SetWindowText(_T("构件打标签"));
	
	// 设置默认值
	CTime currentTime = CTime::GetCurrentTime();
	m_strMaintenanceDate = currentTime.Format(_T("%Y-%m-%d"));
	m_strResponsiblePerson = _T("管理员");
	
	UpdateData(FALSE);
	
	return TRUE;
}

//-----------------------------------------------------------------------------
LRESULT SetComponentLabel::OnAcadKeepFocus (WPARAM, LPARAM) {
	return (TRUE) ;
}

//-----------------------------------------------------------------------------
void SetComponentLabel::SetSelectedEntities(const AcDbObjectIdArray& entIds)
{
	m_selectedEntities = entIds;
}

//-----------------------------------------------------------------------------
void SetComponentLabel::OnBnClickedOk()
{
	UpdateData(TRUE);
	
	// 验证输入
	if (m_strMaintenanceDate.IsEmpty()) {
		MessageBox(_T("请输入维护日期"), _T("输入错误"), MB_OK | MB_ICONWARNING);
		return;
	}
	
	if (m_strResponsiblePerson.IsEmpty()) {
		MessageBox(_T("请输入责任人"), _T("输入错误"), MB_OK | MB_ICONWARNING);
		return;
	}
	
	if (m_selectedEntities.length() == 0) {
		MessageBox(_T("没有选中的实体"), _T("操作错误"), MB_OK | MB_ICONWARNING);
		return;
	}
	
	// 准备标签数据
	ComponentLabelData labelData;
	labelData.maintenanceDate = std::wstring(m_strMaintenanceDate.GetString());
	labelData.responsiblePerson = std::wstring(m_strResponsiblePerson.GetString());
	
	// 使用服务类添加标签
	ComponentLabelResult result = ComponentLabelService::AddLabelsToEntities(m_selectedEntities, labelData);
	
	// 显示结果
	if (result.success) {
		CString msg;
		msg.Format(_T("成功为 %d/%d 个构件添加了标签数据"), result.successCount, result.totalCount);
		MessageBox(msg, _T("操作完成"), MB_OK | MB_ICONINFORMATION);
		
		// 输出到命令行
		acutPrintf(_T("\n构件打标签完成：成功 %d 个，总计 %d 个\n"), result.successCount, result.totalCount);
		acutPrintf(_T("维护日期：%s\n"), m_strMaintenanceDate);
		acutPrintf(_T("责任人：%s\n"), m_strResponsiblePerson);
		
		CAdUiBaseDialog::OnOK();
	} else {
		CString msg;
		msg.Format(_T("添加标签失败：%s"), result.errorMessage.c_str());
		MessageBox(msg, _T("操作失败"), MB_OK | MB_ICONERROR);
	}
}

//-----------------------------------------------------------------------------
void SetComponentLabel::OnBnClickedCancel()
{
	acutPrintf(_T("\n构件打标签操作已取消\n"));
	CAdUiBaseDialog::OnCancel();
}

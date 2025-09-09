// TextStyleUtil.cpp: implementation of the CTextStyleUtil class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TextStyleUtil.h"
#include <dbsymtb.h>
#include <acutmem.h>
#include <string>
#include <dbid.h>        // 添加这行 - 定义 AcDbObjectId
#include <dbmain.h>      // 添加这行 - 数据库相关

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTextStyleUtil::CTextStyleUtil()
{

}

CTextStyleUtil::~CTextStyleUtil()
{

}

AcDbObjectId CTextStyleUtil::GetAt( const TCHAR* name )
{
	AcDbObjectId textStyleId;

	if (_tcslen(name) > 0)
	{
		AcDbTextStyleTable* pTextStyleTable = NULL;
		acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pTextStyleTable, AcDb::kForRead);
		pTextStyleTable->getAt(name, textStyleId);	// 如果不存在，textStyleId不会被赋值
		pTextStyleTable->close();
	}

	return textStyleId;
}

void CTextStyleUtil::GetAll( std::vector<std::basic_string<TCHAR>> &textStyles )  // 将CString改为std::basic_string<TCHAR>
{
	textStyles.clear();
	
	AcDbTextStyleTable *pTextStyleTbl = NULL;
	acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pTextStyleTbl, AcDb::kForRead);
	AcDbTextStyleTableIterator *pIt = NULL;
	pTextStyleTbl->newIterator(pIt);
	
	for (; !pIt->done(); pIt->step())
	{
		AcDbTextStyleTableRecord *pRcd = NULL;
		if (pIt->getRecord(pRcd, AcDb::kForRead) == Acad::eOk)
		{
			TCHAR *szName = NULL;
			pRcd->getName(szName);
			if (_tcslen(szName) > 0)		// 过滤掉名称为空的文本样式
			{
				textStyles.push_back(std::basic_string<TCHAR>(szName));  // 显式构造std::basic_string<TCHAR>
			}
			acutDelString(szName);
			
			pRcd->close();
		}
	}
	delete pIt;
	pTextStyleTbl->close();
}

AcDbObjectId CTextStyleUtil::Add( const TCHAR* name, const TCHAR* fontFileName, const TCHAR* bigFontFileName )
{
	Acad::ErrorStatus es;
    AcDbTextStyleTable* pTextStyleTable = NULL;
    es = acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pTextStyleTable, AcDb::kForWrite);    
	
    AcDbTextStyleTableRecord* pTextStyleRecord = new AcDbTextStyleTableRecord();	
	es = pTextStyleRecord->setName(name);
	es = pTextStyleRecord->setBigFontFileName(bigFontFileName);		// 大字体文件
	es = pTextStyleRecord->setFileName(fontFileName);	// 字体文件
	es = pTextStyleRecord->setXScale(1.0);		// 文字高宽比，一般情况都设置为1，除非需要调整文字高宽比
    es = pTextStyleTable->add(pTextStyleRecord);
	AcDbObjectId styleId = pTextStyleRecord->objectId();	
    pTextStyleTable->close();
    pTextStyleRecord->close();
	
    return styleId;
}

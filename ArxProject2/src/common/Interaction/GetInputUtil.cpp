// GetInputUtil.cpp: implementation of the CGetInputUtil class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GetInputUtil.h"
#include "..\Others\ConvertUtil.h"
#include <geassign.h>
#include <vector>
#include <string>
#include <sstream>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGetInputUtil::CGetInputUtil()
{

}

CGetInputUtil::~CGetInputUtil()
{

}

int CGetInputUtil::GetPointReturnCode( const AcGePoint3d &basePoint, const TCHAR* prompt, AcGePoint3d &point )
{
	// 将基点转换为UCS坐标
	AcGePoint3d ucsBasePoint = CConvertUtil::WcsToUcsPoint(basePoint);
	
	int nReturn = acedGetPoint(asDblArray(ucsBasePoint), prompt, asDblArray(point));
	if (nReturn == RTNORM)
	{
		// acedGetPoint得到UCS坐标，转换为WCS
		point = CConvertUtil::UcsToWcsPoint(point);
	}
	
	return nReturn;
}

int CGetInputUtil::GetPointReturnCode( const TCHAR* prompt, AcGePoint3d &point )
{
	int nReturn = acedGetPoint(NULL, prompt, asDblArray(point));
	if (nReturn == RTNORM)
	{
		point = CConvertUtil::UcsToWcsPoint(point);
	}
	
	return nReturn;
}

bool CGetInputUtil::GetPoint( const AcGePoint3d &basePoint, const TCHAR* prompt, AcGePoint3d &point )
{
	return (GetPointReturnCode(basePoint, prompt, point) == RTNORM);
}

bool CGetInputUtil::GetPoint( const TCHAR* prompt, AcGePoint3d &point )
{
	return (GetPointReturnCode(prompt, point) == RTNORM);
}

bool CGetInputUtil::GetKeyword( const TCHAR* prompt, const TCHAR* keywords, const TCHAR* firstDefault, int defaultKeyIndex, int &nRet )
{
	int rc;		// 返回值
	TCHAR kword[20];		// 关键字
	// firstDefault参数输入NULL表示不允许直接按下Enter键
	acedInitGet(firstDefault == NULL ? 1 : 0, keywords);
	
	// 使用 TCHAR 字符串和 boost 算法
	std::basic_string<TCHAR> strPrompt = prompt;
	if (firstDefault != NULL)
	{
		strPrompt += TEXT("<");
		strPrompt += firstDefault;
		strPrompt += TEXT(">:");
	}
	else
	{
		strPrompt += TEXT(":");
	}
	rc = acedGetKword(strPrompt.c_str(), kword);
	
	switch (rc)
	{
	case RTCAN:		// 按下Esc键
		return false;

	case RTNONE:	// 按下Enter键
		nRet = defaultKeyIndex;
		return true;

	case RTNORM:	// 输入了某个关键字
		{
			std::vector<std::basic_string<TCHAR>> items;
			std::basic_string<TCHAR> keywordsStr = keywords;
			boost::split(items, keywordsStr, boost::is_any_of(TEXT(" ")));  // boost::split 支持 TCHAR
	
			std::basic_string<TCHAR> kwStr = kword;
			for (int i = 0; i < (int)items.size(); i++)
			{
				if (boost::iequals(items[i], kwStr))  // boost::iequals 支持 TCHAR
				{
					nRet = i;
					break;
				}
			}
			return true;
		}
	default:
		return false;
	}
}

bool CGetInputUtil::GetReal( const TCHAR* prompt, double defaultVal, int precision, double &ret )
{
	std::basic_string<TCHAR> strPrompt = prompt;
	boost::trim_right(strPrompt);  // boost::trim_right 支持 TCHAR
	boost::trim_right_if(strPrompt, boost::is_any_of(TEXT(":")));  // 去除右边的冒号
	
	// 使用 TCHAR 版本的格式化
	TCHAR buffer[50];
	_stprintf_s(buffer, 50, TEXT("<%.*f>:"), precision, defaultVal);
	strPrompt += buffer;
	
	ret = defaultVal;
	int rc = acedGetReal(strPrompt.c_str(), &ret);
	if (rc == RTNORM || rc == RTNONE)
	{
		return true;
	}
	else
	{
		return false;
	}
}

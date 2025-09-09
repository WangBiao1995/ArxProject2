// BlockUtil.h: interface for the CBlockUtil class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLOCKUTIL_H__7ADDDF39_6E25_4316_A6B7_0B99ECBE3CEB__INCLUDED_)
#define AFX_BLOCKUTIL_H__7ADDDF39_6E25_4316_A6B7_0B99ECBE3CEB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <dbents.h>

class CBlockUtil  
{
public:
	CBlockUtil();
	virtual ~CBlockUtil();

	// ���ָ�����ƵĿ鶨��
	static AcDbObjectId GetBlkDefId(const TCHAR* blkDefName, AcDbDatabase *pDb = acdbHostApplicationServices()->workingDatabase());

	// ��������
	static AcDbObjectId InsertBlockRef(AcDbObjectId blkDefId, const AcGePoint3d &insertPoint, double scale = 1, double rotation = 0);

	// �������Կ飨����ֵȫ��ʹ��Ĭ��ֵ��
	static AcDbObjectId InsertBlockRefWithAttribute(const AcDbObjectId &blkDefId, const AcGePoint3d &insertPoint, 
		double scale = 1, double rotation = 0);
	
	// ������������ĳ������
	static void AppendAttributeToBlkRef( AcDbBlockReference * pBlkRef, AcDbAttributeDefinition * pAttDef );

	// �������Կ��ĳ������ֵ
	static bool SetBlockRefAttribute(AcDbBlockReference *pBlkRef, const TCHAR* tag, const TCHAR* val);

	// ��ÿ������ĳ�����Ե�����
	static bool GetAttributeValue(AcDbBlockReference *pBlkRef, const TCHAR* tag, std::wstring &value);

	// ÿյ����ű���
	static void SetScaleFactor(AcDbBlockReference *pBlkRef, double scale);

	// ��������������ʵ�����ʵ���㣨�����õ��ǿ���հ�Χ����ʵ��Ľ��㣩
	static void IntersectWith(AcDbBlockReference *pBlkRef, AcDbEntity *pEnt, AcGePoint3dArray &intPoints, 
		bool bIntersectWithText = true, bool bProjectToXOY = false, bool bExtendEntArg = false);

	// ����: ��������DWGͼ�θ���һ���鶨�嵽��ǰͼ��
	// ����: const TCHAR * fileName, Dwg�ļ�����ȫ·����
	// ����: const TCHAR * blkDefName, �鶨�������
	// ����ֵ:   AcDbObjectId, ���Ƶ���ǰͼ���ж�Ӧ�Ŀ鶨��ID(�����ǰͼ���д���ͬ���Ŀ鶨�壬ֱ�ӷ��ص�ǰͼ����ͬ���Ŀ鶨���ID)
	static AcDbObjectId CopyBlockDefFromOtherDwg(const TCHAR* fileName, const TCHAR* blkDefName);

	// ��DWG�ļ���Ϊ�鶨����뵽ָ����ͼ�����ݿ�
	// bOverwriteIfExist: ���ָ����ͼ�����ݿ����Ѿ�������ͬ���Ŀ飬�Ƿ񸲸�ԭ�еĶ���
	static AcDbObjectId InsertDwgBlockDef(const TCHAR* dwgFileName, const TCHAR* blkName, bool bOverwriteIfExist, 
		AcDbDatabase *pDb = acdbHostApplicationServices()->workingDatabase());
};

#endif // !defined(AFX_BLOCKUTIL_H__7ADDDF39_6E25_4316_A6B7_0B99ECBE3CEB__INCLUDED_)

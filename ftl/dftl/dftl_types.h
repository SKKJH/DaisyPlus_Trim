/*******************************************************
*
* Copyright (C) 2018-2019 
* Embedded Software Laboratory(ESLab), SUNG KYUN KWAN UNIVERSITY
* 
* This file is part of ESLab's Flash memory firmware
* 
* This source can not be copied and/or distributed without the express
* permission of ESLab
*
* Author: DongYoung Seo (dongyoung.seo@gmail.com)
* ESLab: http://nyx.skku.ac.kr
*
*******************************************************/

#ifndef __DFTL_TYPES_H__
#define __DFTL_TYPES_H__

#define VIRTUAL
#define STATIC

#define INVALID_PPN				(INVALID_INDEX)
#define INVALID_PPN				(INVALID_INDEX)
#define INVALID_VPPN			(INVALID_INDEX)
#define INVALID_LPN				(INVALID_INDEX)
#define INVALID_VBN				(INVALID_INDEX)
#define INVALID_EC				-1				// erase count

typedef enum
{
	IOTYPE_HOST = 0,	// HOST REQUEST
	IOTYPE_GC,			// GC REQUEST
	IOTYPE_META,		// META REQUEST

	IOTYPE_TEST,
	IOTYPE_COUNT,
} IOTYPE;

#ifdef __cplusplus

/*
	// pure virtual class
*/
class FTL_INTERFACE
{
public:
	virtual VOID Initialize(VOID) = 0;
	virtual BOOL Format(VOID) = 0;
	virtual VOID Run(VOID) = 0;
	virtual VOID ReadPage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount) = 0;
	virtual VOID WritePage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount) = 0;
	virtual VOID CallBack(FTL_REQUEST_ID stReqID) = 0;
	virtual VOID IOCtl(IOCTL_TYPE eType) = 0;
};

class COMMON_REQUEST
{
public:
	COMMON_REQUEST(VOID) : m_eIOType(IOTYPE_COUNT) {}

protected:
	IOTYPE	m_eIOType;
};

#endif
#endif		// end of #ifndef __DFTL_INIT_H__
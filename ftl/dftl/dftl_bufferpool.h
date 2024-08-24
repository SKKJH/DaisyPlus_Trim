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

#ifndef __DFTL_BUFFERPOOL_H__
#define __DFTL_BUFFERPOOL_H__

class BUFFER_ENTRY
{
public:
	struct list_head	m_dlList;		// to link free list
	void*				m_pMainBuf;
	void*				m_pSpareBuf;
};

class BUFFER_MGR
{
public:
	VOID			Initialize(VOID);
	BUFFER_ENTRY*	GetEntry(INT32 nIndex);
	BUFFER_ENTRY*	Allocate(VOID);
	VOID			Release(BUFFER_ENTRY* pstEntry);
	BOOL			IsEmpty(VOID) { return GetFreeCount() == 0 ? TRUE : FALSE; }

	INT32			GetFreeCount(VOID)
	{
		return m_nFreeCount;
	}

private:
	INT32				m_nTotalCount;
	BUFFER_ENTRY*		m_pastEntry;

	struct list_head	m_dlFree;		// to link free list
	INT32				m_nFreeCount;
};

#endif
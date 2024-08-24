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

#ifndef __DFTL_GARBAGECOLLECTOR_H__
#define __DFTL_GARBAGECOLLECTOR_H__

#include "dftl_internal.h"

class GC_POLICY_GREEDY
{
public:
	VOID Initialize(IOTYPE eIOType);
	UINT32 GetVictimVBN(VOID);

protected:
	BLOCK_MGR*	m_pstBlockMgr;


private:
};

class GC_DEBUG
{
public:
	typedef enum
	{
		GC_DEBUG_FLAG_READ			= 0x01,
		GC_DEBUG_FLAG_READ_DONE		= 0x02,
		GC_DEBUG_FLAG_WRITE_ISSUE	= 0x04,
		GC_DEBUG_FLAG_WRITE_DONE	= 0x08,
		GC_DEBUG_FLAG_MASK			= 0x0F,
	} GC_DEBUG_FLAG;

	GC_DEBUG() :m_apstVPNStatus(NULL) {};

	VOID Initialize(VOID);
	VOID SetFlag(UINT32 nVPPN, GC_DEBUG_FLAG eFlag);

	GC_DEBUG_FLAG*	m_apstVPNStatus;
	UINT32			m_nRead;
	UINT32			m_nWrite;
};

class GC_MGR : public GC_POLICY_GREEDY
{
public:
	GC_MGR() : m_nVictimVBN(INVALID_VBN) {};

	VOID Initialize(UINT32 nGCTh, IOTYPE eIOType);

	VOID Run(VOID);

	BOOL IsGCRunning(VOID);

	VOID CheckAndStartGC(const char* callerName);

	VOID IncreaseWriteCount(VOID);

#if (SUPPORT_GC_DEBUG == 1)
	GC_DEBUG	m_stDebug;
#endif

private:
	VOID _Read(VOID);

	UINT32		m_nThreshold;
	IOTYPE		m_eIOType;

	// A block GC will be done on the below condition
	// 1. when nCurReadOffset offset goes up to the end of page.
	// 2. nReadLPNCount == m_nWriteIssuedLPNCount		<== Write issue done

	INT32	m_nVictimVBN;			// Victim block
	INT32	m_nVPC;					// Valid Page count of VBN @ GC start
	UINT32	m_nCurReadVPageOffset;	// Read logical page offset
	INT32	m_nWriteCount;	// count of write issued LPN
	INT32	GC_Check;			// Check GC or Not
};

#endif

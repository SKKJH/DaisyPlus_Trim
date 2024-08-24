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

#ifndef __DFTL_REQUEST_META_H__
#define __DFTL_REQUEST_META_H__

#define META_REQUEST_COUNT			(USER_CHANNELS)

typedef enum
{
	META_REQUEST_FREE,
	META_REQUEST_READ_WAIT,			// wait for NAND read
	META_REQUEST_READ_ISSUE,		// NAND IO Issued
	META_REQUEST_READ_DONE,			// operation done

	META_REQUEST_WRITE_WAIT,		// wait for NAND write
	META_REQUEST_WRITE_ISSUE,		// Add to Active Block
	META_REQUEST_WRITE_DONE = META_REQUEST_WRITE_ISSUE,		// operation done,
} META_REQUEST_STATUS;

class META_REQUEST : public COMMON_REQUEST
{
public:
	META_REQUEST() :m_pstBufEntry(NULL) {}

	VOID	Initialize(META_REQUEST_STATUS nStatus, UINT32 m_nMetaLPN, VOID* pstMetaEntry, IOTYPE eIOType);
	BOOL	Run(VOID);

	BUFFER_ENTRY*		GetBuffer(VOID) { return m_pstBufEntry; }

	VOID				SetRequestIndex(INT32 nIndex) { m_nRequestIndex = nIndex; }
	META_REQUEST_STATUS	GetStatus(VOID) { return m_nStatus; }
	VOID				SetStatus(META_REQUEST_STATUS nStatus)	{ m_nStatus = nStatus; }
	UINT32				GetVPPN(VOID) { return m_nVPPN; }
	UINT32				GetLPN(VOID) { return m_nMetaLPN; }
	VOID				SetLPN(UINT32 nLPN) { m_nMetaLPN = nLPN; }

	BOOL				AllocateBuf(VOID);
	VOID				ReleaseBuf(VOID);

	VOID GoToNextStatus(VOID) { m_nStatus = (META_REQUEST_STATUS)(m_nStatus + 1); }

private:
	BOOL	_ProcessRead_Wait(VOID);
	BOOL	_ProcessRead_Done(VOID);
	BOOL	_ProcessWrite_Wait(VOID);
	BOOL	_ProcessWrite_Done(VOID);

	BOOL	_IsWritable(VOID);

	FTL_REQUEST_ID		_GetRquestID(VOID);

public:
	struct list_head	m_dlList;				// list for queueing
												// public for macro operation

private:
	META_REQUEST_STATUS	m_nStatus;

	INT32				m_nMetaLPN;				// Meta LPN
	INT32				m_nVPPN;				// Meta VPPN,

	BUFFER_ENTRY*		m_pstBufEntry;
	VOID*				m_pstMetaEntry;			// Meta Cache Entry pointer

	unsigned int		m_nRequestIndex:FTL_REQUEST_ID_REQUEST_INDEX_BITS;
	unsigned int		m_nMetaEntryIndex : 8;
	unsigned int		m_bBufferHit : 1;		// hit on the write buffering LPN
};

class META_REQUEST_INFO
{
public:
	VOID		Initialize(VOID);

	META_REQUEST*	AllocateRequest(VOID);
	VOID		ReleaseRequest(META_REQUEST* pstRequest);

	VOID		AddToWaitQ(META_REQUEST* pstRequest);
	VOID		AddToIssuedQ(META_REQUEST* pstRequest);
	VOID		AddToDoneQ(META_REQUEST* pstRequest);
	VOID		RemoveFromWaitQ(META_REQUEST* pstRequest);
	VOID		RemoveFromIssuedQ(META_REQUEST* pstRequest);
	VOID		RemoveFromDoneQ(META_REQUEST* pstRequest);

	META_REQUEST*	GetWaitRequest(VOID);
	META_REQUEST*	GetDoneRequest(VOID);

	META_REQUEST* GetRequest(INT32 nIndex) { return &m_astRequestPool[nIndex]; }

private:
	INT32 _GetRequestIndex(META_REQUEST* pstRequest);

private:
	FTL_REQUEST_ID_TYPE		m_nReqType;

	META_REQUEST			m_astRequestPool[META_REQUEST_COUNT];

	struct list_head		m_dlFree;				// free NVMeRequest
	INT32					m_nFreeCount;			// count of free request

	struct list_head		m_dlWait;				// NAND issue wait queue
	INT32					m_nWaitCount;

	struct list_head		m_dlIssued;			// issued queue
	INT32					m_nIssuedCount;

	struct list_head		m_dlDone;				// Read done or Program done
	INT32					m_nDoneCount;
};

#endif
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

#ifndef __DFTL_REQUEST_GC_H__
#define __DFTL_REQUEST_GC_H__

#define GC_REQUEST_COUNT			(NSC_MAX_CHANNELS * USER_WAYS * LPN_PER_PHYSICAL_PAGE * 2/* double buffering */)

typedef enum
{
	GC_REQUEST_FREE,
	GC_REQUEST_READ_WAIT,
	GC_REQUEST_READ_ISSUE,
	GC_REQUEST_READ_DONE,
	GC_REQUEST_WRITE_WAIT = GC_REQUEST_READ_DONE,
	GC_REQUEST_WRITE_ADD_TO_ACTIVE_BLOCK,							// GC request is inserted to active block buffer
	GC_REQUEST_WRITE_DONE = GC_REQUEST_WRITE_ADD_TO_ACTIVE_BLOCK,	// no further work to do
} GC_REQUEST_STATUS;

class GC_REQUEST : public COMMON_REQUEST
{
public:
	VOID	Initialize(GC_REQUEST_STATUS nStatus, UINT32 m_nVPPN, IOTYPE eIOType);
	BOOL	Run(VOID);
	VOID	GCReadDone(VOID);

	BUFFER_ENTRY* GetBuffer(VOID) { return m_pstBufEntry; }

	VOID				SetRequestIndex(INT32 nIndex) { m_nRequestIndex = nIndex; }
	GC_REQUEST_STATUS	GetStatus(VOID) { return m_nStatus; }
	VOID				SetStatus(GC_REQUEST_STATUS nStatus)	{ m_nStatus = nStatus; }
	UINT32				GetVPPN(VOID) { return m_nVPPN; }
	UINT32				GetLPN(VOID) { return m_nLPN; }
	VOID				SetLPN(UINT32 nLPN) { m_nLPN = nLPN; }
	IOTYPE				GetIOType(VOID) { return m_eIOType; }

	VOID GoToNextStatus(VOID) { m_nStatus = (GC_REQUEST_STATUS)(m_nStatus + 1); }

private:
	BOOL	_CheckAndLoadMeta(VOID);

	BOOL	_ProcessRead_Wait(VOID);
	BOOL	_ProcessWrite_Wait(VOID);
	BOOL	_ProcessWrite_Done(VOID);

	BOOL	_IsWritable(VOID);

	BOOL	_AllocateBuf(VOID);
	VOID	_ReleaseBuf(VOID);
	FTL_REQUEST_ID		_GetRquestID(VOID);

	GC_MGR*	_GetGCMgr(VOID);

public:
	struct list_head	m_dlList;				// list for queueing
												// public for macro operation

private:
	GC_REQUEST_STATUS	m_nStatus;

	INT32				m_nLPN;					// LPN
	INT32				m_nVPPN;				// Source VPPN, to get LPN data offset in pMainBuf

	BUFFER_ENTRY*		m_pstBufEntry;

	unsigned int		m_nRequestIndex:FTL_REQUEST_ID_REQUEST_INDEX_BITS;
};

class GC_REQUEST_INFO
{
public:
	VOID		Initialize(VOID);

	GC_REQUEST*	AllocateRequest(VOID);
	VOID		ReleaseRequest(GC_REQUEST* pstRequest);

	VOID		AddToWaitQ(GC_REQUEST* pstRequest);
	VOID		AddToIssuedQ(GC_REQUEST* pstRequest);
	VOID		AddToDoneQ(GC_REQUEST* pstRequest);
	VOID		RemoveFromWaitQ(GC_REQUEST* pstRequest);
	VOID		RemoveFromIssuedQ(GC_REQUEST* pstRequest);
	VOID		RemoveFromDoneQ(GC_REQUEST* pstRequest);

	GC_REQUEST*	GetWaitRequest(VOID);
	GC_REQUEST*	GetDoneRequest(VOID);


	GC_REQUEST* GetRequest(INT32 nIndex) { return &m_astRequestPool[nIndex]; }

private:
	INT32 _GetRequestIndex(GC_REQUEST* pstRequest);

private:
	FTL_REQUEST_ID_TYPE		m_nReqType;

	GC_REQUEST				m_astRequestPool[GC_REQUEST_COUNT];

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
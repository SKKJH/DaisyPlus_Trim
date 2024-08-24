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

#include "dftl_internal.h"

///////////////////////////////////////////////////////////////////////////////
//
//	META REQUEST
//
///////////////////////////////////////////////////////////////////////////////

VOID
META_REQUEST::Initialize(META_REQUEST_STATUS nStatus, UINT32 nMetaLPN, VOID* pstMetaEntry, IOTYPE eIOType)
{
	m_nStatus		= nStatus;
	m_nMetaLPN		= nMetaLPN;
	m_pstMetaEntry	= pstMetaEntry;		// Meta Cache Entry
	m_bBufferHit	= FALSE;
	m_eIOType		= eIOType;
}

BOOL
META_REQUEST::Run(VOID)
{	
	BOOL bSuccess;
	switch (m_nStatus)
	{
	case META_REQUEST_READ_WAIT:
		bSuccess = _ProcessRead_Wait();
		break;

	case META_REQUEST_READ_DONE:
		bSuccess = _ProcessRead_Done();
		break;

	case META_REQUEST_WRITE_WAIT:
		bSuccess = _ProcessWrite_Wait();
		break;

	case META_REQUEST_WRITE_DONE:
		bSuccess = _ProcessWrite_Done();
		break;

	default:
		ASSERT(0);
		bSuccess = FALSE;
		break;
	}

	return bSuccess;
}

BOOL
META_REQUEST::AllocateBuf(VOID)
{
	DEBUG_ASSERT(m_pstBufEntry == NULL);

	// Allocate Buffer
	BUFFER_MGR*		pstBufferMgr = DFTL_GLOBAL::GetBufferMgr();
	m_pstBufEntry = pstBufferMgr->Allocate();

	return (m_pstBufEntry != NULL) ? TRUE : FALSE;
}

VOID
META_REQUEST::ReleaseBuf(VOID)
{
	if (m_pstBufEntry == NULL)
	{
		return;
	}

	BUFFER_MGR*		pstBufferMgr = DFTL_GLOBAL::GetBufferMgr();
	pstBufferMgr->Release(m_pstBufEntry);

	m_pstBufEntry = NULL;
}

BOOL
META_REQUEST::_ProcessRead_Done(VOID)
{
	DEBUG_ASSERT(m_nStatus == META_REQUEST_READ_DONE);

	UINT32				nBufAddr;

#if defined(WIN32)
	if (m_bBufferHit == FALSE)
	{
		VNAND*	pstVNandMgr = DFTL_GLOBAL::GetVNandMgr();
		pstVNandMgr->ReadPageSimul(m_nVPPN, m_pstBufEntry->m_pMainBuf);
	}
#endif

	nBufAddr = (UINT32)m_pstBufEntry->m_pMainBuf + (LOGICAL_PAGE_SIZE * LPN_OFFSET_FROM_VPPN(m_nVPPN));

	// copy L2P to cache entry
	META_MGR*			pstMetaMgr = DFTL_GLOBAL::GetMetaMgr();
	pstMetaMgr->LoadDone((META_CACHE_ENTRY*)m_pstMetaEntry, (VOID*)nBufAddr);

#if defined(SUPPORT_DATA_VERIFICATION) || defined(WIN32)
	// Set LPN on main buffer to data verification
	//DEBUG_ASSERT(((unsigned int *)pstRequest->pSpareBuf)[nLPNOffset] == pstRequest->nLPN);
	if (((UINT32 *)m_pstBufEntry->m_pSpareBuf)[LPN_OFFSET_FROM_VPPN(m_nVPPN)] != m_nMetaLPN)
	{
		PRINTF("[FTL][META] LPN mismatch, request LPN: %d, SpareLPN: %d \n\r",
			m_nMetaLPN, ((UINT32 *)m_pstBufEntry->m_pSpareBuf)[LPN_OFFSET_FROM_VPPN(m_nVPPN)]);
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_MISCOMAPRE);
	}
#endif

	GoToNextStatus();

	META_REQUEST_INFO*	pstRequestInfo = DFTL_GLOBAL::GetRequestMgr()->GetMetaRequestInfo();

	ReleaseBuf();

	// Release Request
	pstRequestInfo->RemoveFromDoneQ(this);
	pstRequestInfo->ReleaseRequest(this);

	return TRUE;
}

FTL_REQUEST_ID
META_REQUEST::_GetRquestID(VOID)
{
	FTL_REQUEST_ID stReqID;

	if (m_nStatus  == META_REQUEST_READ_WAIT)
	{
		stReqID.stCommon.nType = FTL_REQUEST_ID_TYPE_META_READ;
	}
	else if (m_nStatus == META_REQUEST_WRITE_WAIT)
	{
		stReqID.stCommon.nType = FTL_REQUEST_ID_TYPE_WRITE;	// this type will be stored request id of PROGRAM_UNIT
	}
	else
	{
		ASSERT(0);	// Not implemented yet
	}

	stReqID.stMeta.nRequestIndex = m_nRequestIndex;
	DEBUG_ASSERT(stReqID.stMeta.nRequestIndex < META_REQUEST_COUNT);

	return stReqID;
}

BOOL
META_REQUEST::_ProcessRead_Wait(VOID)
{
	ASSERT(m_nStatus == HIL_REQUEST_READ_WAIT);

	REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
	BUFFER_MGR*		pstBufferMgr = DFTL_GLOBAL::GetBufferMgr();

	UINT32			nBufferingVPPN;
	UINT32			nLPN;

	FTL_REQUEST_ID stReqID = _GetRquestID();

	BOOL			bDone = FALSE;

	nLPN = GetLPN();

	// GetPPN
	m_nVPPN = DFTL_GLOBAL::GetMetaL2VMgr()->GetL2V(nLPN);
	DEBUG_ASSERT(INVALID_PPN != m_nVPPN);

	// Check buffering 
	BUFFERING_LPN* pstBufferingLPN = DFTL_GLOBAL::GetMetaActiveBlockBufferingLPN();
	nBufferingVPPN = pstBufferingLPN->ReadLPN(nLPN, m_pstBufEntry);
	if (nBufferingVPPN != INVALID_VPPN)
	{
		DEBUG_ASSERT(m_nVPPN == nBufferingVPPN);
		bDone = TRUE;
		m_bBufferHit = TRUE;
	}
	else
	{
		// VNNAD Read
		DFTL_GLOBAL::GetVNandMgr()->ReadPage(stReqID, m_nVPPN, m_pstBufEntry->m_pMainBuf, m_pstBufEntry->m_pSpareBuf);
		bDone = FALSE;
	}

	FTL_DEBUG_PRINTF("[FTL][META][WAITQ][Read] LPN:%d, Count: %d \r\n", pstRequest->nLPN, pstRequest->nLPNCount);

	GoToNextStatus();

	META_REQUEST_INFO*	pstMetaRequestInfo = pstRequestMgr->GetMetaRequestInfo();

	pstMetaRequestInfo->RemoveFromWaitQ(this);
	if (bDone == TRUE)
	{
		GoToNextStatus();
		pstMetaRequestInfo->AddToDoneQ(this);	// Issued request will be done by FIL call back
	}
	else
	{
		pstMetaRequestInfo->AddToIssuedQ(this);	// Issued request will be done by FIL call back
	}

	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_META_READ);

	return TRUE;
}

BOOL
META_REQUEST::_ProcessWrite_Wait(VOID)
{
	// Get ActiveBlock
	ACTIVE_BLOCK*	pstActiveBlock;
	BOOL			bSuccess;

	if (_IsWritable() == FALSE)
	{
		return FALSE;
	}

	pstActiveBlock = DFTL_GLOBAL::GetActiveBlockMgr()->GetActiveBlock(IOTYPE_META);
	bSuccess = pstActiveBlock->Write(this, IOTYPE_META);
	if (bSuccess == TRUE)
	{
		GoToNextStatus();

		//Remove from WaitQ
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		META_REQUEST_INFO*	pstMETARequestInfo = pstRequestMgr->GetMetaRequestInfo();
		pstMETARequestInfo->RemoveFromWaitQ(this);
		pstMETARequestInfo->AddToDoneQ(this);

		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_META_WRITE);
	}

	return bSuccess;
}

BOOL
META_REQUEST::_IsWritable(VOID)
{
	// check buffer available
	if (DFTL_GLOBAL::GetBufferMgr()->GetFreeCount() == 0)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL
META_REQUEST::_ProcessWrite_Done(VOID)
{
	REQUEST_MGR*		pstRequestMgr		= DFTL_GLOBAL::GetRequestMgr();
	META_REQUEST_INFO*	pstMETARequestInfo	= pstRequestMgr->GetMetaRequestInfo();

	META_MGR*			pstMetaMgr = DFTL_GLOBAL::GetMetaMgr();
	pstMetaMgr->StoreDone((META_CACHE_ENTRY*)m_pstMetaEntry);

	//Remove from DoneQ & Release Request
	pstMETARequestInfo->RemoveFromDoneQ(this);
	pstMETARequestInfo->ReleaseRequest(this);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////
//
//	META REQUEST INFORMATION
//
///////////////////////////////////////////////////////////////////////////////////

VOID
META_REQUEST_INFO::Initialize(VOID)
{
	INIT_LIST_HEAD(&m_dlFree);
	INIT_LIST_HEAD(&m_dlWait);
	INIT_LIST_HEAD(&m_dlIssued);
	INIT_LIST_HEAD(&m_dlDone);

	m_nFreeCount = 0;
	m_nWaitCount = 0;
	m_nIssuedCount = 0;
	m_nDoneCount = 0;

	for (int i = 0; i < META_REQUEST_COUNT; i++)
	{
		INIT_LIST_HEAD(&m_astRequestPool[i].m_dlList);
		m_astRequestPool[i].SetRequestIndex(i);
		ReleaseRequest(&m_astRequestPool[i]);
	}
}

META_REQUEST*
META_REQUEST_INFO::AllocateRequest(VOID)
{
	if (m_nFreeCount == 0)
	{
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlFree) == FALSE);

	if (DFTL_GLOBAL::GetBufferMgr()->IsEmpty() == TRUE)
	{
		return NULL;
	}

	META_REQUEST*	pstRequest;

	pstRequest = list_first_entry(&m_dlFree, META_REQUEST, m_dlList);

	list_del(&pstRequest->m_dlList);

	m_nFreeCount--;

	DEBUG_ASSERT(pstRequest->GetStatus() == META_REQUEST_FREE);

	if (pstRequest->AllocateBuf() == FALSE)
	{
		ASSERT(0);
	}

	return pstRequest;
}

VOID
META_REQUEST_INFO::AddToWaitQ(META_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlWait);
	m_nWaitCount++;

	DEBUG_ASSERT(m_nWaitCount <= META_REQUEST_COUNT);
}

VOID
META_REQUEST_INFO::RemoveFromWaitQ(META_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nWaitCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nWaitCount--;
}

VOID
META_REQUEST_INFO::AddToIssuedQ(META_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlIssued);
	m_nIssuedCount++;

	DEBUG_ASSERT(m_nIssuedCount <= (MAX(META_REQUEST_COUNT, META_REQUEST_COUNT)));
}

VOID
META_REQUEST_INFO::RemoveFromIssuedQ(META_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nIssuedCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nIssuedCount--;
}

VOID
META_REQUEST_INFO::AddToDoneQ(META_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlDone);
	m_nDoneCount++;

	DEBUG_ASSERT(m_nDoneCount <= META_REQUEST_COUNT);
}

VOID
META_REQUEST_INFO::RemoveFromDoneQ(META_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nDoneCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nDoneCount--;
}

META_REQUEST*
META_REQUEST_INFO::GetWaitRequest(VOID)
{
	if (m_nWaitCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlWait) == TRUE);
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlWait) == FALSE);

	META_REQUEST*	pstRequest;
	pstRequest = list_first_entry(&m_dlWait, META_REQUEST, m_dlList);

	return pstRequest;
}

META_REQUEST*
META_REQUEST_INFO::GetDoneRequest(VOID)
{
	if (m_nDoneCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlDone) == TRUE);
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlDone) == FALSE);

	META_REQUEST*	pstRequest;
	pstRequest = list_first_entry(&m_dlDone, META_REQUEST, m_dlList);

	return pstRequest;
}

VOID
META_REQUEST_INFO::ReleaseRequest(META_REQUEST* pstRequest)
{
	pstRequest->ReleaseBuf();

	list_add_tail(&pstRequest->m_dlList, &m_dlFree);
	m_nFreeCount++;
	DEBUG_ASSERT(m_nFreeCount <= META_REQUEST_COUNT);

	pstRequest->SetStatus(META_REQUEST_FREE);
	DEBUG_ASSERT(m_nFreeCount <= META_REQUEST_COUNT);
	DEBUG_ASSERT(_GetRequestIndex(pstRequest) < META_REQUEST_COUNT);
}

INT32
META_REQUEST_INFO::_GetRequestIndex(META_REQUEST* pstRequest)
{
	return pstRequest - &m_astRequestPool[0];
}


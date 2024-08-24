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
//	GC REQUEST
//
///////////////////////////////////////////////////////////////////////////////

VOID
GC_REQUEST::Initialize(GC_REQUEST_STATUS nStatus, UINT32 nVPPN, IOTYPE eIOType)
{
	m_nStatus = nStatus;
	m_nVPPN = nVPPN;
	m_eIOType = eIOType;
}

BOOL
GC_REQUEST::Run(VOID)
{	
	BOOL bSuccess;
	switch (m_nStatus)
	{
	case GC_REQUEST_READ_WAIT:
		bSuccess = _ProcessRead_Wait();
		break;

	case GC_REQUEST_WRITE_WAIT:
		bSuccess = _ProcessWrite_Wait();
		break;

	case GC_REQUEST_WRITE_DONE:
		bSuccess = _ProcessWrite_Done();
		break;

	default:
		ASSERT(0);
		bSuccess = FALSE;
		break;
	}

	return bSuccess;
}

VOID
GC_REQUEST::GCReadDone(VOID)
{
	// Get LPN from spare but current FW does not store LPN to spare area,
	//	(TODO) I need to figure out the layout of spare area, and then store LPN to Spare

	DFTL_GLOBAL*	pstGlobal = DFTL_GLOBAL::GetInstance();

#ifdef WIN32
	// Get LPN from spare
	UINT32	nLPNOffset = LPN_OFFSET_FROM_VPPN(GetVPPN());
	SetLPN(((INT32*)GetBuffer()->m_pSpareBuf)[nLPNOffset]);

	DEBUG_ASSERT(GetLPN() < pstGlobal->GetLPNCount());		// Check Spare LPN

	#if defined(SUPPORT_DATA_VERIFICATION)
	{
		UINT32	nSpareLPN = ((UINT32 *)m_pstBufEntry->m_pSpareBuf)[LPN_OFFSET_FROM_VPPN(m_nVPPN)];
		//DEBUG_ASSERT(((unsigned int *)pstRequest->pSpareBuf)[nLPNOffset] == pstRequest->nLPN);
		if (nSpareLPN != m_nLPN)
		{
			PRINTF("[FTL][GC] (1)LPN mismatch, request LPN: %d, SpareLPN: %d \n\r", m_nLPN, nSpareLPN);
			DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_MISCOMAPRE);
		}

		VNAND*	pstVNandMgr = DFTL_GLOBAL::GetVNandMgr();
		pstVNandMgr->ReadPageSimul(m_nVPPN, m_pstBufEntry->m_pMainBuf);
	}
	#endif	// #if defined(SUPPORT_DATA_VERIFICATION)

#else
	// on target
	m_nLPN = pstGlobal->GetVNandMgr()->GetV2L(m_nVPPN);
#endif

	REQUEST_MGR*	pstRequestMgr = pstGlobal->GetRequestMgr();
	GC_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetGCRequestInfo();

	pstRequestInfo->RemoveFromIssuedQ(this);	// remove from issued Q
	pstRequestInfo->AddToWaitQ(this);			// add to write wait Q

	GoToNextStatus();		// GC Read Issued -> Write Wait

#if (SUPPORT_GC_DEBUG == 1)
	_GetGCMgr()->m_stDebug.SetFlag(GetVPPN(), GC_DEBUG::GC_DEBUG_FLAG_READ_DONE);
#endif
}

FTL_REQUEST_ID
GC_REQUEST::_GetRquestID(VOID)
{
	FTL_REQUEST_ID stReqID;

	if (m_nStatus  == GC_REQUEST_READ_WAIT)
	{
		stReqID.stCommon.nType = FTL_REQUEST_ID_TYPE_GC_READ;
	}
	else if (m_nStatus == GC_REQUEST_WRITE_WAIT)
	{
		stReqID.stCommon.nType = FTL_REQUEST_ID_TYPE_WRITE;	// this type will be stored request id of PROGRAM_UNIT
	}
	else
	{
		ASSERT(0);	// Not implemented yet
	}

	stReqID.stGC.nRequestIndex = m_nRequestIndex;
	DEBUG_ASSERT(stReqID.stGC.nRequestIndex < GC_REQUEST_COUNT);

	return stReqID;
}

BOOL
GC_REQUEST::_AllocateBuf(VOID)
{
	// Allocate Buffer
	BUFFER_MGR*		pstBufferMgr = DFTL_GLOBAL::GetBufferMgr();
	m_pstBufEntry = pstBufferMgr->Allocate();

	return (m_pstBufEntry != NULL) ? TRUE : FALSE;
}

VOID
GC_REQUEST::_ReleaseBuf(VOID)
{
	BUFFER_MGR*		pstBufferMgr = DFTL_GLOBAL::GetBufferMgr();
	pstBufferMgr->Release(m_pstBufEntry);
}

BOOL
GC_REQUEST::_ProcessRead_Wait(VOID)
{
	if (_AllocateBuf() == FALSE)
	{
		return FALSE;
	}

	FTL_REQUEST_ID stReqID = _GetRquestID();

	VNAND*	pstVNAND = DFTL_GLOBAL::GetVNandMgr();

	// VNNAD Read
	pstVNAND->ReadPage(stReqID, m_nVPPN, m_pstBufEntry->m_pMainBuf, m_pstBufEntry->m_pSpareBuf);
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_BGC_READ);

	GoToNextStatus();

	REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
	GC_REQUEST_INFO*	pstGCRequestInfo = pstRequestMgr->GetGCRequestInfo();

	pstGCRequestInfo->RemoveFromWaitQ(this);
	pstGCRequestInfo->AddToIssuedQ(this);	// Issued request will be done by FIL call back

#if (SUPPORT_GC_DEBUG == 1)
	_GetGCMgr()->m_stDebug.SetFlag(GetVPPN(), GC_DEBUG::GC_DEBUG_FLAG_READ);
#endif

//	xil_printf("GC READ DONE\r\n");
	return TRUE;
}

BOOL
GC_REQUEST::_ProcessWrite_Wait(VOID)
{
	// Get ActiveBlock
	ACTIVE_BLOCK*	pstActiveBlock;
	BOOL			bSuccess;

	if (_IsWritable() == FALSE)
	{
		return FALSE;
	}

	pstActiveBlock = DFTL_GLOBAL::GetActiveBlockMgr()->GetActiveBlock(m_eIOType);
	bSuccess = pstActiveBlock->Write(this, IOTYPE_GC);
	if (bSuccess == TRUE)
	{
		GoToNextStatus();

		//Remove from WaitQ
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		GC_REQUEST_INFO*	pstGCRequestInfo = pstRequestMgr->GetGCRequestInfo();
		pstGCRequestInfo->RemoveFromWaitQ(this);
		pstGCRequestInfo->AddToDoneQ(this);

		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_BGC_WRITE);

#if (SUPPORT_GC_DEBUG == 1)
		_GetGCMgr()->m_stDebug.SetFlag(GetVPPN(), GC_DEBUG::GC_DEBUG_FLAG_WRITE_ISSUE);
#endif
	}

//	xil_printf("GC Write Wait DONE\r\n");
	return bSuccess;
}

/*
@brief		check L2P metadata is available for start and end LPN
a meta page(4KB) is enough to have mapping data 
for the maximun size(REQUEST_LPN_COUNT_MAX) of request
*/
BOOL
GC_REQUEST::_CheckAndLoadMeta(VOID)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	META_MGR*	pstMetaMgr = DFTL_GLOBAL::GetMetaMgr();

	BOOL	bAvailable = TRUE;

	if (pstMetaMgr->IsMetaAvailable(m_nLPN) == FALSE)
	{
		pstMetaMgr->LoadMeta(m_nLPN);
		bAvailable = FALSE;
	}

//	xil_printf("GC Check and Load Meta DONE\r\n");
	return bAvailable;
#else
	return TRUE;
#endif
}

BOOL
GC_REQUEST::_IsWritable(VOID)
{
	// check buffer available
	if (DFTL_GLOBAL::GetBufferMgr()->GetFreeCount() == 0)
	{
//		xil_printf("buffer unavailable\r\n");
		return FALSE;
	}


#if (SUPPORT_META_DEMAND_LOADING == 1)
	if ((m_eIOType == IOTYPE_GC) && (DFTL_GLOBAL::GetMetaGCMgr()->IsGCRunning() == TRUE))
	{
		return FALSE;
	}

	if (_CheckAndLoadMeta() == FALSE)
	{
//		xil_printf("GC Check and Load Meta ERROR\r\n");
		// load metadata
		return FALSE;
	}
#endif


//	xil_printf("GC Writeable\r\n");
	return TRUE;
}

GC_MGR*
GC_REQUEST::_GetGCMgr(VOID)
{
	if (m_eIOType == IOTYPE_GC)
	{
		return DFTL_GLOBAL::GetGCMgr();
	}

#if (SUPPORT_META_DEMAND_LOADING == 1)
	return DFTL_GLOBAL::GetMetaGCMgr();
#else
	ASSERT(0);
	return NULL;
#endif
}

BOOL
GC_REQUEST::_ProcessWrite_Done(VOID)
{
	// Release Request
	_ReleaseBuf();

	REQUEST_MGR*		pstRequestMgr		= DFTL_GLOBAL::GetRequestMgr();
	GC_REQUEST_INFO*	pstGCRequestInfo	= pstRequestMgr->GetGCRequestInfo();

	//Remove from DoneQ & Release Request
	pstGCRequestInfo->RemoveFromDoneQ(this);
	pstGCRequestInfo->ReleaseRequest(this);

	GC_MGR* pstGCMgr = _GetGCMgr();

	pstGCMgr->IncreaseWriteCount();

#if (SUPPORT_GC_DEBUG == 1)
	pstGCMgr->m_stDebug.SetFlag(GetVPPN(), GC_DEBUG::GC_DEBUG_FLAG_WRITE_DONE);
#endif

//	xil_printf("GC Write DONE\r\n");
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////
//
//	GC REQUEST INFORMATION
//
///////////////////////////////////////////////////////////////////////////////////

VOID
GC_REQUEST_INFO::Initialize(VOID)
{
	INIT_LIST_HEAD(&m_dlFree);
	INIT_LIST_HEAD(&m_dlWait);
	INIT_LIST_HEAD(&m_dlIssued);
	INIT_LIST_HEAD(&m_dlDone);

	m_nFreeCount = 0;
	m_nWaitCount = 0;
	m_nIssuedCount = 0;
	m_nDoneCount = 0;

	for (int i = 0; i < GC_REQUEST_COUNT; i++)
	{
		INIT_LIST_HEAD(&m_astRequestPool[i].m_dlList);
		m_astRequestPool[i].SetRequestIndex(i);

		ReleaseRequest(&m_astRequestPool[i]);
	}
}

GC_REQUEST*
GC_REQUEST_INFO::AllocateRequest(VOID)
{
	if (m_nFreeCount == 0)
	{
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlFree) == FALSE);

	GC_REQUEST*	pstRequest;

	pstRequest = list_first_entry(&m_dlFree, GC_REQUEST, m_dlList);

	list_del(&pstRequest->m_dlList);

	m_nFreeCount--;

	DEBUG_ASSERT(pstRequest->GetStatus() == GC_REQUEST_FREE);

	return pstRequest;
}

VOID
GC_REQUEST_INFO::AddToWaitQ(GC_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlWait);
	m_nWaitCount++;

	DEBUG_ASSERT(m_nWaitCount <= GC_REQUEST_COUNT);
}

VOID
GC_REQUEST_INFO::RemoveFromWaitQ(GC_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nWaitCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nWaitCount--;
}

VOID
GC_REQUEST_INFO::AddToIssuedQ(GC_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlIssued);
	m_nIssuedCount++;

	DEBUG_ASSERT(m_nIssuedCount <= (MAX(GC_REQUEST_COUNT, GC_REQUEST_COUNT)));
}

VOID
GC_REQUEST_INFO::RemoveFromIssuedQ(GC_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nIssuedCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nIssuedCount--;
}

VOID
GC_REQUEST_INFO::AddToDoneQ(GC_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlDone);
	m_nDoneCount++;

	DEBUG_ASSERT(m_nDoneCount <= GC_REQUEST_COUNT);
}

VOID
GC_REQUEST_INFO::RemoveFromDoneQ(GC_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nDoneCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nDoneCount--;
}

GC_REQUEST*
GC_REQUEST_INFO::GetWaitRequest(VOID)
{
	if (m_nWaitCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlWait) == TRUE);
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlWait) == FALSE);

	GC_REQUEST*	pstRequest = NULL;
	BOOL		bFound = FALSE;

#if (SUPPORT_META_DEMAND_LOADING == 1)
	if (DFTL_GLOBAL::GetMetaGCMgr()->IsGCRunning() == TRUE)
	{
		// lookup meta GC request
		list_for_each_entry(GC_REQUEST, pstRequest, &m_dlWait, m_dlList)
		{
			if (pstRequest->GetIOType() == IOTYPE_META)
			{
				bFound = TRUE;
				break;
			}
		}
	}
#endif

	if (bFound == FALSE)
	{
		pstRequest = list_first_entry(&m_dlWait, GC_REQUEST, m_dlList);
	}

	DEBUG_ASSERT(pstRequest != NULL);

	return pstRequest;
}

GC_REQUEST*
GC_REQUEST_INFO::GetDoneRequest(VOID)
{
	if (m_nDoneCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlDone) == TRUE);
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlDone) == FALSE);

	GC_REQUEST*	pstRequest;
	pstRequest = list_first_entry(&m_dlDone, GC_REQUEST, m_dlList);

	return pstRequest;
}

VOID
GC_REQUEST_INFO::ReleaseRequest(GC_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlFree);
	m_nFreeCount++;
	DEBUG_ASSERT(m_nFreeCount <= GC_REQUEST_COUNT);

	pstRequest->SetStatus(GC_REQUEST_FREE);
	DEBUG_ASSERT(m_nFreeCount <= GC_REQUEST_COUNT);
	DEBUG_ASSERT(_GetRequestIndex(pstRequest) < GC_REQUEST_COUNT);
}

INT32
GC_REQUEST_INFO::_GetRequestIndex(GC_REQUEST* pstRequest)
{
	return pstRequest - &m_astRequestPool[0];
}


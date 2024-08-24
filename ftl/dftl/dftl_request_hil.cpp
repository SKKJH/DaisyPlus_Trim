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
#include "../../hil/nvme/nvme_main.h"

///////////////////////////////////////////////////////////////////////////////
//
//	HIL_REQUEST_INFO
//
///////////////////////////////////////////////////////////////////////////////

VOID
HIL_REQUEST_INFO::Initialize(VOID)
{

	INIT_LIST_HEAD(&m_dlFree);
	INIT_LIST_HEAD(&m_dlWait);
	INIT_LIST_HEAD(&m_dlIssued);
	INIT_LIST_HEAD(&m_dlHDMA);
	INIT_LIST_HEAD(&m_dlDone);

	m_nFreeCount = 0;
	m_nWaitCount = 0;
	m_nIssuedCount = 0;
	m_nHDMACount = 0;
	m_nDoneCount = 0;

	for (int i = 0; i < HIL_REQUEST_COUNT; i++)
	{
		INIT_LIST_HEAD(&m_astRequestPool[i].m_dlList);
		m_astRequestPool[i].SetRequestIndex(i);

		ReleaseRequest(&m_astRequestPool[i]);
	}
}

HIL_REQUEST*
HIL_REQUEST_INFO::AllocateRequest(VOID)
{
	if (m_nFreeCount == 0)
	{
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlFree) == FALSE);

	HIL_REQUEST*	pstRequest;

	pstRequest = list_first_entry(&m_dlFree, HIL_REQUEST, m_dlList);

	list_del(&pstRequest->m_dlList);

	m_nFreeCount--;

	DEBUG_ASSERT(pstRequest->GetStatus() == HIL_REQUEST_FREE);

	return pstRequest;
}

VOID
HIL_REQUEST_INFO::AddToWaitQ(HIL_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlWait);
	m_nWaitCount++;

	DEBUG_ASSERT(m_nWaitCount <= HIL_REQUEST_COUNT);
}

VOID
HIL_REQUEST_INFO::RemoveFromWaitQ(HIL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nWaitCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nWaitCount--;
}

VOID
HIL_REQUEST_INFO::AddToIssuedQ(HIL_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlIssued);
	m_nIssuedCount++;

	DEBUG_ASSERT(m_nIssuedCount <= (MAX(HIL_REQUEST_COUNT, GC_REQUEST_COUNT)));
}

VOID
HIL_REQUEST_INFO::RemoveFromIssuedQ(HIL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nIssuedCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nIssuedCount--;
}

VOID
HIL_REQUEST_INFO::AddToDoneQ(HIL_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlDone);
	m_nDoneCount++;

	DEBUG_ASSERT(m_nDoneCount <= HIL_REQUEST_COUNT);
}

VOID
HIL_REQUEST_INFO::RemoveFromDoneQ(HIL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nDoneCount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nDoneCount--;
}

VOID
HIL_REQUEST_INFO::AddToHDMAIssuedQ(HIL_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlHDMA);
	m_nHDMACount++;

	DEBUG_ASSERT(m_nHDMACount <= HIL_REQUEST_COUNT);
}

VOID
HIL_REQUEST_INFO::RemoveFromHDMAIssuedQ(HIL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(m_nHDMACount > 0);

	list_del_init(&pstRequest->m_dlList);
	m_nHDMACount--;
}

HIL_REQUEST*
HIL_REQUEST_INFO::GetWaitRequest(VOID)
{
	if (m_nWaitCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlWait) == TRUE);
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlWait) == FALSE);

	HIL_REQUEST*	pstRequest;
	pstRequest = list_first_entry(&m_dlWait, HIL_REQUEST, m_dlList);

	return pstRequest;
}

HIL_REQUEST*
HIL_REQUEST_INFO::GetDoneRequest(VOID)
{
	if (m_nDoneCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlDone) == TRUE);
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlDone) == FALSE);

	HIL_REQUEST*	pstRequest;
	pstRequest = list_first_entry(&m_dlDone, HIL_REQUEST, m_dlList);

	return pstRequest;
}

HIL_REQUEST*
HIL_REQUEST_INFO::GetHDMARequest(VOID)
{
	if (m_nHDMACount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlHDMA) == TRUE);
		return NULL;
	}

	DEBUG_ASSERT(list_empty(&m_dlHDMA) == FALSE);

	HIL_REQUEST*	pstRequest;
	pstRequest = list_first_entry(&m_dlHDMA, HIL_REQUEST, m_dlList);

	return pstRequest;
}

VOID
HIL_REQUEST_INFO::ReleaseRequest(HIL_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->m_dlList, &m_dlFree);
	m_nFreeCount++;
	DEBUG_ASSERT(m_nFreeCount <= HIL_REQUEST_COUNT);

	pstRequest->SetStatus(HIL_REQUEST_FREE);
	DEBUG_ASSERT(m_nFreeCount <= HIL_REQUEST_COUNT);
	DEBUG_ASSERT(_GetRequestIndex(pstRequest) < HIL_REQUEST_COUNT);
}

INT32
HIL_REQUEST_INFO::_GetRequestIndex(HIL_REQUEST* pstRequest)
{
	return pstRequest - &m_astRequestPool[0];
}

///////////////////////////////////////////////////////////////////////////////
//
//	HIL REQUEST
//
///////////////////////////////////////////////////////////////////////////////

VOID
HIL_REQUEST::Initialize(HIL_REQUEST_STATUS nStatus, NVME_CMD_OPCODE nCmd, 
	UINT32 nLPN, UINT32 nHostCmdSlotTag, INT32 nLPNCount)
{

	m_nStatus = nStatus;
	m_nCmd = nCmd;
	m_nLPN = nLPN;
	m_nHostCmdSlotTag = nHostCmdSlotTag;
	m_nLPNCount = nLPNCount;
	m_nDoneLPNCount = 0;

	DEBUG_ASSERT(DFTL_GLOBAL::GetInstance()->IsValidLPN(m_nLPN) == TRUE);
	DEBUG_ASSERT(m_nLPNCount <= MAX_NUM_OF_NLB);
}

BOOL
HIL_REQUEST::Run(VOID)
{	
	BOOL bSuccess;
	switch (m_nCmd)
	{
	case NVME_CMD_OPCODE_READ:
		bSuccess = _ProcessRead();
		break;

	case NVME_CMD_OPCODE_WRITE:
		bSuccess = _ProcessWrite();
		break;

	default:
		ASSERT(0);
		bSuccess = FALSE;
		break;
	}

	return bSuccess;
}

FTL_REQUEST_ID
HIL_REQUEST::_GetRquestID(VOID)
{
	FTL_REQUEST_ID stReqID;

	if (m_nCmd == NVME_CMD_OPCODE_READ)
	{
		stReqID.stCommon.nType = FTL_REQUEST_ID_TYPE_HIL_READ;
	}
	else if (m_nCmd == NVME_CMD_OPCODE_WRITE)
	{
		stReqID.stCommon.nType = FTL_REQUEST_ID_TYPE_WRITE;	// this type will be stored request id of PROGRAM_UNIT
	}
	else
	{
		ASSERT(0);	// Not implemented yet
	}

	stReqID.stHIL.nRequestIndex = m_nRequestIndex;
	DEBUG_ASSERT(stReqID.stHIL.nRequestIndex < HIL_REQUEST_COUNT);

	return stReqID;
}

BOOL
HIL_REQUEST::_ProcessRead(VOID)
{
	BOOL	bSuccess;

	switch (m_nStatus)
	{
		case HIL_REQUEST_READ_WAIT:
			bSuccess = _ProcessRead_Wait();
			break;

		case HIL_REQUEST_READ_NAND_DONE:
			bSuccess = _ProcessRead_NANDReadDone();
			break;

		case HIL_REQUEST_READ_HDMA_ISSUE:
			bSuccess = _ProcessRead_HDMAIssue();
			break;

		case HIL_REQUEST_READ_DONE:
			bSuccess = _ProcessRead_Done();
			break;

		default:
			ASSERT(0);
			break;
	};

	return bSuccess;
}

/*
	@brief		check L2P metadata is available for start and end LPN
				a meta page(4KB) is enough to have mapping data 
				for the maximun size(REQUEST_LPN_COUNT_MAX) of request
*/
BOOL
HIL_REQUEST::_CheckAndLoadMeta(VOID)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	META_MGR*	pstMetaMgr = DFTL_GLOBAL::GetMetaMgr();

	BOOL	bAvailable = TRUE;

	if (pstMetaMgr->IsMetaAvailable(m_nLPN) == FALSE)
	{
		pstMetaMgr->LoadMeta(m_nLPN);
		bAvailable = FALSE;
		cmt_miss1 += 1;
	}
	else if (pstMetaMgr->IsMetaAvailable(m_nLPN + m_nLPNCount - 1) == FALSE)
	{
		pstMetaMgr->LoadMeta(m_nLPN + m_nLPNCount - 1);
		bAvailable = FALSE;
		cmt_miss1 += 2;
	}

	if (bAvailable == TRUE)
	{
		cmt_hit += 1;
	}

	return bAvailable;
#else
	return TRUE;
#endif
}

BOOL
HIL_REQUEST::_IsReadable(VOID)
{
	BUFFER_MGR*		pstBufferMgr = DFTL_GLOBAL::GetBufferMgr();

	// check is there enough free buffers
	if (m_nLPNCount > pstBufferMgr->GetFreeCount())
	{
		// not enough free buffer
		return FALSE;
	}

#if (SUPPORT_META_DEMAND_LOADING == 1)
	if (DFTL_GLOBAL::GetMetaGCMgr()->IsGCRunning() == TRUE)
	{
		return FALSE;
	}

	if (_CheckAndLoadMeta() == FALSE)
	{
		// load metadata
		return FALSE;
	}
#endif

	return TRUE;
}

BOOL
HIL_REQUEST::_ProcessRead_Wait(VOID)
{
	ASSERT(m_nStatus == HIL_REQUEST_READ_WAIT);
	ASSERT(m_nLPNCount <= REQUEST_LPN_COUNT_MAX);

	REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
	BUFFER_MGR*		pstBufferMgr = DFTL_GLOBAL::GetBufferMgr();

	if (_IsReadable() == FALSE)
	{
		return FALSE;
	}

	INIT_LIST_HEAD(&m_dlBuffer);

	BUFFER_ENTRY	*pBufEntry;

	UINT32			nVPPN;
	UINT32			nBufferingVPPN;
	UINT32			nLPN;

	FTL_REQUEST_ID stReqID = _GetRquestID();

	// allocate buffer
	for (int i = 0; i < m_nLPNCount; i++)
	{
		pBufEntry = pstBufferMgr->Allocate();
		list_add_tail(&pBufEntry->m_dlList, &m_dlBuffer);

		nLPN = GetStartLPN() + i;

		// GetPPN
		nVPPN = DFTL_GLOBAL::GetMetaMgr()->GetL2V(nLPN);
		if (nVPPN == INVALID_PPN)	// unmap read
		{
			FTL_DEBUG_PRINTF("Unmap Read LPN: %d\n\r", nLPN);
			m_anLPNOffsetOfBuffer[i] = 0;
			IncreaseDoneCount();

			DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_UNMAP_READ);

#if defined(WIN32) && defined(SUPPORT_DATA_VERIFICATION)
			// Set LPN on read buffer
			((unsigned int*)pBufEntry->m_pSpareBuf)[0] = DATA_VERIFICATION_INVALID_LPN;
#endif
		}
		else
		{
			m_anLPNOffsetOfBuffer[i] = LPN_OFFSET_FROM_VPPN(nVPPN);

			// Check buffering 
			BUFFERING_LPN* pstBufferingLPN = DFTL_GLOBAL::GetActiveBlockBufferingLPN();
			nBufferingVPPN = pstBufferingLPN->ReadLPN(nLPN, pBufEntry);
			if (nBufferingVPPN != INVALID_VPPN)
			{
				DEBUG_ASSERT(nVPPN == nBufferingVPPN);
				IncreaseDoneCount();
			}
			else
			{
				// VNNAD Read
				DFTL_GLOBAL::GetVNandMgr()->ReadPage(stReqID, nVPPN, pBufEntry->m_pMainBuf, pBufEntry->m_pSpareBuf);
			}

			DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_READ);
		}
		DEBUG_ASSERT(i < REQUEST_LPN_COUNT_MAX);
		DEBUG_ASSERT(m_anLPNOffsetOfBuffer[i] < LPN_PER_PHYSICAL_PAGE);
	}

	FTL_DEBUG_PRINTF("[FTL][WAITQ][Read] LPN:%d, Count: %d \r\n", pstRequest->nLPN, pstRequest->nLPNCount);

	GoToNextStatus();

	HIL_REQUEST_INFO*	pstHILRequestInfo = pstRequestMgr->GetHILRequestInfo();

	pstHILRequestInfo->RemoveFromWaitQ(this);
	if (m_nLPNCount == m_nDoneLPNCount)
	{
		GoToNextStatus();
		pstHILRequestInfo->AddToDoneQ(this);	// Issued request will be done by FIL call back
	}
	else
	{
		pstHILRequestInfo->AddToIssuedQ(this);	// Issued request will be done by FIL call back
	}

	return TRUE;
}

BOOL
HIL_REQUEST::_ProcessRead_NANDReadDone(VOID)
{
	DEBUG_ASSERT(m_nStatus == HIL_REQUEST_READ_NAND_DONE);
	DEBUG_ASSERT(m_nLPNCount == m_nDoneLPNCount);

	INT32				nDMAIndex = 0;
	BUFFER_ENTRY*		pstBufferEntry;
	UINT32				nBufAddr;

	list_for_each_entry(BUFFER_ENTRY, pstBufferEntry, &m_dlBuffer, m_dlList)
	{
		nBufAddr = (UINT32)pstBufferEntry->m_pMainBuf + (LOGICAL_PAGE_SIZE * m_anLPNOffsetOfBuffer[nDMAIndex]);
		DFTL_GLOBAL::GetHDMAMgr()->IssueTxDMA(m_nHostCmdSlotTag, nDMAIndex, nBufAddr);

#if defined(SUPPORT_DATA_VERIFICATION) || defined(WIN32)
		// Set LPN on main buffer to data verification
		if (*((UINT32 *)pstBufferEntry->m_pSpareBuf) != DATA_VERIFICATION_INVALID_LPN)		// check written page
		{
			//DEBUG_ASSERT(((unsigned int *)pstRequest->pSpareBuf)[nLPNOffset] == pstRequest->nLPN);
			if (((UINT32 *)pstBufferEntry->m_pSpareBuf)[m_anLPNOffsetOfBuffer[nDMAIndex]] != (m_nLPN + nDMAIndex))
			{
				PRINTF("[FTL] (1)LPN mismatch, request LPN: %d, SpareLPN: %d \n\r",
					(m_nLPN + nDMAIndex), ((UINT32 *)pstBufferEntry->m_pSpareBuf)[m_anLPNOffsetOfBuffer[nDMAIndex]]);
				DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_MISCOMAPRE);
			}

			if (*(UINT32 *)nBufAddr != (m_nLPN + nDMAIndex))
			{
				PRINTF("[FTL] (2)LPN mismatch, request LPN: %d, SpareLPN: %d \n\r",
					(m_nLPN + nDMAIndex), ((UINT32 *)pstBufferEntry->m_pMainBuf)[m_anLPNOffsetOfBuffer[nDMAIndex]]);
				DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_MISCOMAPRE);
			}
		}
#endif
		nDMAIndex++;
	}

	DEBUG_ASSERT(nDMAIndex == m_nDoneLPNCount);

	HDMA*	pstHDMA = DFTL_GLOBAL::GetHDMAMgr();

	m_nDMAReqTail		= pstHDMA->GetTxDMAIndex();
	m_nDMAOverFlowCount	= pstHDMA->GetTxDMAOverFlowCount();

	GoToNextStatus();

	HIL_REQUEST_INFO*	pstRequestInfo = DFTL_GLOBAL::GetRequestMgr()->GetHILRequestInfo();
	// Release Request
	pstRequestInfo->RemoveFromDoneQ(this);
	pstRequestInfo->AddToHDMAIssuedQ(this);

	return TRUE;
}

BOOL
HIL_REQUEST::_ProcessRead_HDMAIssue(VOID)
{
	BOOL			bDMADone;

	DEBUG_ASSERT(m_nStatus == HIL_REQUEST_READ_HDMA_ISSUE);
	bDMADone = DFTL_GLOBAL::GetHDMAMgr()->CheckTxDMADone(m_nDMAReqTail, m_nDMAOverFlowCount);
	if (bDMADone == TRUE)
	{
		HIL_REQUEST_INFO*	pstRequestInfo = DFTL_GLOBAL::GetRequestMgr()->GetHILRequestInfo();
		pstRequestInfo->RemoveFromHDMAIssuedQ(this);
		pstRequestInfo->AddToDoneQ(this);

		GoToNextStatus();
	}

	return bDMADone;
}

BOOL
HIL_REQUEST::_ProcessRead_Done(VOID)
{
	REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
	BUFFER_MGR*		pstBufferMgr = DFTL_GLOBAL::GetBufferMgr();

	BUFFER_ENTRY*	pstCurEntry;
	BUFFER_ENTRY*	pstNextEntry;

	INT32			nCount = 0;

	// release buffer
	list_for_each_entry_safe(BUFFER_ENTRY, pstCurEntry, pstNextEntry, &m_dlBuffer, m_dlList)
	{
		pstBufferMgr->Release(pstCurEntry);
		nCount++;
	}

	DEBUG_ASSERT(nCount == m_nLPNCount);

	pstRequestMgr->GetHILRequestInfo()->RemoveFromDoneQ(this);
	pstRequestMgr->GetHILRequestInfo()->ReleaseRequest(this);

	return TRUE;
}

BOOL
HIL_REQUEST::_ProcessWrite(VOID)
{
	BOOL	bSuccess;

	switch (m_nStatus)
	{
	case HIL_REQUEST_WRITE_WAIT:
		bSuccess = _ProcessWrite_Wait();
		break;

	case HIL_REQUEST_WRITE_DONE:
		bSuccess = _ProcessWrite_Done();
		break;

	default:
		ASSERT(0);
		break;
	};

	return bSuccess;
}

BOOL
HIL_REQUEST::_IsWritable(VOID)
{
	if (DFTL_GLOBAL::GetGCMgr()->IsGCRunning() == TRUE)
	{
		return FALSE;
	}

#if (SUPPORT_META_DEMAND_LOADING == 1)
	if (DFTL_GLOBAL::GetMetaGCMgr()->IsGCRunning() == TRUE)
	{
		return FALSE;
	}
#endif

	// check buffer available
	if (DFTL_GLOBAL::GetBufferMgr()->GetFreeCount() == 0)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL
HIL_REQUEST::_ProcessWrite_Wait(VOID)
{
	// Get ActiveBlock
	ACTIVE_BLOCK*	pstActiveBlock;
	BOOL			bSuccess;
	do
	{
		if (_IsWritable() == FALSE)
		{
			bSuccess = FALSE;
			break;
		}

#if (SUPPORT_META_DEMAND_LOADING == 1)
		if (_CheckAndLoadMeta() == FALSE)
		{
			bSuccess = FALSE;
			break;
		}
#endif

		pstActiveBlock = DFTL_GLOBAL::GetActiveBlockMgr()->GetActiveBlock(IOTYPE_HOST);
		bSuccess = pstActiveBlock->Write(this, IOTYPE_HOST);

	} while ((m_nDoneLPNCount < m_nLPNCount) && (bSuccess ==TRUE));

	if (bSuccess == TRUE)
	{
		DEBUG_ASSERT(m_nLPNCount == m_nDoneLPNCount);
		GoToNextStatus();

		//Remove from WaitQ
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		HIL_REQUEST_INFO*	pstHILRequestInfo = pstRequestMgr->GetHILRequestInfo();
		pstHILRequestInfo->RemoveFromWaitQ(this);
		pstHILRequestInfo->AddToDoneQ(this);
	}

	return bSuccess;
}

BOOL
HIL_REQUEST::_ProcessWrite_Done(VOID)
{
	// Release Request
	//Remove from DoneQ
	REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
	HIL_REQUEST_INFO*	pstHILRequestInfo = pstRequestMgr->GetHILRequestInfo();
	pstHILRequestInfo->RemoveFromDoneQ(this);
	pstHILRequestInfo->ReleaseRequest(this);

	return TRUE;
}

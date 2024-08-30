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
//	PROGRAM UNIT
//
///////////////////////////////////////////////////////////////////////////////

PROGRAM_UNIT::PROGRAM_UNIT(VOID)
	:
	m_bFree(TRUE),
	m_bProgramming(FALSE),
	m_nLPNCount(0),
	m_pstBufferEntry(NULL)
{
}

VOID
PROGRAM_UNIT::Add(VOID* pstRequest, UINT32 nVPPN, IOTYPE eBlockType, IOTYPE eRequestType)
{
	if(m_nLPNCount == 0)
	{
		m_nLPNCount = 0;
	}
	if (m_nLPNCount == 0)
	{
		DEBUG_ASSERT(m_bProgramming == FALSE);
		DEBUG_ASSERT(m_bFree == TRUE);

		// allocate buffer entry on the first add
		DEBUG_ASSERT(m_pstBufferEntry == NULL);
		m_pstBufferEntry = DFTL_GLOBAL::GetBufferMgr()->Allocate();

		m_bFree = FALSE;
		m_nVPPN = nVPPN;

		for (INT32 i = 0; i < LPN_PER_PHYSICAL_PAGE; i++)
		{
			m_anLPN[i] = INVALID_LPN;
		}
	}

	void* pBufferingAddr = m_pstBufferEntry->m_pMainBuf;
	pBufferingAddr = (void*)((UINT32)pBufferingAddr + (LOGICAL_PAGE_SIZE * m_nLPNCount));

	UINT32	nLPN;

	if (eBlockType == IOTYPE_HOST)
	{
		HIL_REQUEST* pstHILRequest = static_cast<HIL_REQUEST*>(pstRequest);

		// host write request, receive data from host 
		HDMA* pstHDMA = DFTL_GLOBAL::GetHDMAMgr();
		pstHDMA->IssueRxDMA(pstHILRequest->GetHostCmdSlotTag(),  pstHILRequest->GetHDMAOffset(), (unsigned int)pBufferingAddr);
		pstHILRequest->SetHDMAIssueInfo(pstHDMA->GetRxDMAIndex(), pstHDMA->GetRxDMAOverFlowCount());
		nLPN = pstHILRequest->GetCurLPN();
	}
	else if (eBlockType == IOTYPE_GC)
	{
		GC_REQUEST* pstGCRequest = static_cast<GC_REQUEST*>(pstRequest);
		UINT32	nLPNOffset = LPN_OFFSET_FROM_VPPN(pstGCRequest->GetVPPN());
		void* pSrcAddr = (char*)pstGCRequest->GetBuffer()->m_pMainBuf + (LOGICAL_PAGE_SIZE * nLPNOffset);
		OSAL_MEMCPY(pBufferingAddr, pSrcAddr, LOGICAL_PAGE_SIZE);
		nLPN = pstGCRequest->GetLPN();
	}
#if (SUPPORT_META_DEMAND_LOADING == 1)
	else if (eBlockType == IOTYPE_META)
	{
		if (eRequestType == IOTYPE_META)
		{
			META_REQUEST* pstMetaRequest = static_cast<META_REQUEST*>(pstRequest);
			void* pSrcAddr = pstMetaRequest->GetBuffer()->m_pMainBuf;
			OSAL_MEMCPY(pBufferingAddr, pSrcAddr, LOGICAL_PAGE_SIZE);
			nLPN = pstMetaRequest->GetLPN();
		}
		else
		{
			DEBUG_ASSERT(eRequestType == IOTYPE_GC);
			GC_REQUEST* pstGCRequest = static_cast<GC_REQUEST*>(pstRequest);
			UINT32	nLPNOffset = LPN_OFFSET_FROM_VPPN(pstGCRequest->GetVPPN());
			void* pSrcAddr = (char*)pstGCRequest->GetBuffer()->m_pMainBuf + (LOGICAL_PAGE_SIZE * nLPNOffset);
			OSAL_MEMCPY(pBufferingAddr, pSrcAddr, LOGICAL_PAGE_SIZE);
			nLPN = pstGCRequest->GetLPN();
		}
	}
#endif
	else
	{
		ASSERT(0);
	}

	m_anLPN[m_nLPNCount] = nLPN;

#if defined(SUPPORT_DATA_VERIFICATION) || defined(WIN32)
	UINT32 nSpareAddr = (UINT32)m_pstBufferEntry->m_pSpareBuf;
	nSpareAddr = nSpareAddr + (m_nLPNCount * sizeof(UINT32));
	*((UINT32*)nSpareAddr) = m_anLPN[m_nLPNCount];

	if (eBlockType != IOTYPE_META)
	{
		UINT32 nMainAddr = (UINT32)m_pstBufferEntry->m_pMainBuf;
		nMainAddr = nMainAddr + (m_nLPNCount * HOST_BLOCK_SIZE);
		*((UINT32*)nMainAddr) = m_anLPN[m_nLPNCount];
	}
#endif

	DFTL_GLOBAL::GetVNandMgr()->ProgramPageSimul(nLPN, nVPPN);

	m_nLPNCount++;

	DEBUG_ASSERT(m_nLPNCount <= LPN_PER_PHYSICAL_PAGE);
}


///////////////////////////////////////////////////////////////////////////////
//
//	ACTIVE_BLOCK
//
///////////////////////////////////////////////////////////////////////////////

ACTIVE_BLOCK::ACTIVE_BLOCK(VOID)
	:
	m_nCurProgramBuffering(0),
	m_nCurVPPN(INVALID_VPPN),
	m_nVBN(INVALID_VBN)
{
}

VOID
ACTIVE_BLOCK::_IssueProgram(VOID)
{
	PROGRAM_UNIT* pstProgramUnit = &m_astBuffering[m_nCurProgramBuffering];

	DEBUG_ASSERT(pstProgramUnit->m_bFree == FALSE);
	DEBUG_ASSERT(pstProgramUnit->m_bProgramming == FALSE);

	FTL_REQUEST_ID	stProgramReqID;
	stProgramReqID.stProgram.nType = FTL_REQUEST_ID_TYPE_WRITE;
	stProgramReqID.stProgram.nActiveBlockIndex = DFTL_GLOBAL::GetActiveBlockMgr()->GetActiveBlockIndex(m_eBlockType);
	stProgramReqID.stProgram.nBufferingIndex = m_nCurProgramBuffering;
	stProgramReqID.stProgram.nIOType = m_eBlockType;

	// do write
	DFTL_GLOBAL::GetVNandMgr()->ProgramPage(stProgramReqID, pstProgramUnit);

	pstProgramUnit->m_bProgramming = TRUE;

	// increase buffering index
	m_nCurProgramBuffering = INCREASE_IN_RANGE(m_nCurProgramBuffering, ACTIVE_BLOCK_BUFFERING_COUNT);

	if (m_eBlockType == IOTYPE_HOST)
	{
		// Wait Rx DMA Done
		DFTL_GLOBAL::GetHDMAMgr()->WaitRxDMADone(pstProgramUnit->m_nDMAReqTail, pstProgramUnit->m_nDMAOverFlowCount);
	}
}

/*
	@param	eIOType: type of pstRequest
*/
BOOL
ACTIVE_BLOCK::Write(VOID* pstRequest, IOTYPE eRequestType)
{
	DEBUG_ASSERT(m_nCurVPPN != INVALID_VPPN);
	DEBUG_ASSERT(VBN_FROM_VPPN(m_nCurVPPN) == m_nVBN);

	PROGRAM_UNIT*	pstProgramUnit = &m_astBuffering[m_nCurProgramBuffering];

	if (pstProgramUnit->m_bProgramming == TRUE)
	{
		DEBUG_ASSERT(pstProgramUnit->m_bFree == FALSE);
		DEBUG_ASSERT(pstProgramUnit->m_nLPNCount == LPN_PER_PHYSICAL_PAGE);

		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_ACTIVEBLOCK_PROGRAM_UNIT_FULL);
		// Not enough free request
		return FALSE;
	}

	// Add to buffering
	pstProgramUnit->Add(pstRequest, m_nCurVPPN, m_eBlockType, eRequestType);

	if (m_eBlockType == IOTYPE_HOST)
	{
		HIL_REQUEST*	pstHILRequest = static_cast<HIL_REQUEST*>(pstRequest);
		// Update Mapping
		DFTL_GLOBAL::GetMetaMgr()->SetL2V(pstHILRequest->GetCurLPN(), m_nCurVPPN);

		// host request only
		BUFFERING_LPN* pstBufferingLPN = DFTL_GLOBAL::GetActiveBlockBufferingLPN();
		pstBufferingLPN->Add(pstHILRequest->GetCurLPN());

		pstHILRequest->IncreaseDoneCount();
	}
	else if (m_eBlockType == IOTYPE_GC)
	{
		GC_REQUEST*	pstGCRequest = static_cast<GC_REQUEST*>(pstRequest);

		// Update Mapping
		DFTL_GLOBAL::GetMetaMgr()->SetL2V(pstGCRequest->GetLPN(), m_nCurVPPN);
	}
#if (SUPPORT_META_DEMAND_LOADING == 1)
	else if (m_eBlockType == IOTYPE_META)
	{
		UINT32	nLPN;
		if (eRequestType == IOTYPE_META)
		{
			META_REQUEST*	pstMetaRequest = static_cast<META_REQUEST*>(pstRequest);
			nLPN = pstMetaRequest->GetLPN();
		}
		else
		{
			DEBUG_ASSERT(eRequestType == IOTYPE_GC);
			GC_REQUEST*	pstGCRequest = static_cast<GC_REQUEST*>(pstRequest);
			nLPN = pstGCRequest->GetLPN();
		}

		DFTL_GLOBAL::GetMetaL2VMgr()->SetL2V(nLPN, m_nCurVPPN);

		BUFFERING_LPN* pstBufferingLPN = DFTL_GLOBAL::GetMetaActiveBlockBufferingLPN();
		pstBufferingLPN->Add(nLPN);
	}
#endif
	else
	{
		ASSERT(0);
	}

	// Add to active block
	if (pstProgramUnit->IsFull() == TRUE)
	{
		// Issue
		_IssueProgram();
	}

	m_nCurVPPN++;

	// check boundary of VBlock
	if (VPN_FROM_VPPN(m_nCurVPPN) == 0)
	{
		// end of vblock
		m_nCurVPPN = INVALID_VPPN;		// to go a new active block at the next write
	}

	return TRUE;
}

VOID
ACTIVE_BLOCK::IncreaseVPPN(VOID)
{
	UINT32	nCurLPageOffset = LPN_OFFSET_FROM_VPPN(m_nCurVPPN);

	if (nCurLPageOffset == DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock())
	{
		// end of block, a new vblock will be allocated at the next write
		m_nCurVPPN = INVALID_VPPN;
	}
	else
	{
		m_nCurVPPN++;
	}
}

VOID
ACTIVE_BLOCK::ProgramDone(INT32 nBufferingIndex)
{
	DEBUG_ASSERT(nBufferingIndex < ACTIVE_BLOCK_BUFFERING_COUNT);
	PROGRAM_UNIT*	pstProgramUnit = &m_astBuffering[nBufferingIndex];

	DEBUG_ASSERT(pstProgramUnit->m_bProgramming == TRUE);
	DEBUG_ASSERT(pstProgramUnit->m_bFree == FALSE);
	DEBUG_ASSERT(pstProgramUnit->m_nLPNCount == LPN_PER_PHYSICAL_PAGE);

	if ((m_eBlockType == IOTYPE_HOST) || (m_eBlockType == IOTYPE_META))
	{
		BUFFERING_LPN*	pstBufferingLPN;

		if (m_eBlockType == IOTYPE_HOST)
		{
			pstBufferingLPN = DFTL_GLOBAL::GetActiveBlockBufferingLPN();
		}
#if (SUPPORT_META_DEMAND_LOADING == 1)
		else
		{
			pstBufferingLPN = DFTL_GLOBAL::GetMetaActiveBlockBufferingLPN();
		}
#else
		else
		{
			ASSERT(0);
		}
#endif

		// update buffering LPN Hash
		for (UINT32 i = 0; i < pstProgramUnit->m_nLPNCount; i++)
		{
			pstBufferingLPN->Remove(pstProgramUnit->GetLPN(i));
		}
	}

	// release buffer
	DFTL_GLOBAL::GetBufferMgr()->Release(pstProgramUnit->GetBufferEntry());
	pstProgramUnit->SetBufferEntry(NULL);

	// update active block buffering state
	pstProgramUnit->m_bFree = TRUE;
	pstProgramUnit->m_bProgramming = FALSE;
	pstProgramUnit->m_nLPNCount = 0;
}

BOOL
ACTIVE_BLOCK::CheckAllProgramUnitIsFree(VOID)
{
	for (INT32 i = 0; i < ACTIVE_BLOCK_BUFFERING_COUNT; i++)
	{
		if ((m_astBuffering[i].m_bFree != TRUE) ||
			(m_astBuffering[i].m_bProgramming != FALSE))
		{
			DEBUG_ASSERT(0);
			return FALSE;
		}
	}

	return TRUE;
}

UINT32
ACTIVE_BLOCK::ReadBufferingLPN(UINT32 nLPN, BUFFER_ENTRY* pstBufferEntry)
{
	UINT32	nVPPN = INVALID_VPPN;

	UINT32	nBufferingIndex = m_nCurProgramBuffering;

	// lookup from the latest to oldest
	for (INT32 j = 0; j < ACTIVE_BLOCK_BUFFERING_COUNT; j++)
	{
		if (m_astBuffering[nBufferingIndex].m_bFree == TRUE)
		{
			continue;
		}

		for (INT32 nLPNIndex = (m_astBuffering[nBufferingIndex].m_nLPNCount - 1); nLPNIndex >= 0; nLPNIndex--)
		{
			if (m_astBuffering[nBufferingIndex].GetLPN(nLPNIndex) == nLPN)
			{
				// HIT
				//PRINTF("Read from buffering LPN: %d\n", nLPN);

				nVPPN = m_astBuffering[nBufferingIndex].GetVPPN() + nLPNIndex;

				INT32 nLPNOffset = LPN_OFFSET_FROM_VPPN(nVPPN);

				VOID* pDataAddr = pstBufferEntry->m_pMainBuf;
				pDataAddr = (VOID*)((UINT32)pDataAddr + (LOGICAL_PAGE_SIZE * nLPNOffset));

				VOID* pBufferingAddr =  m_astBuffering[nBufferingIndex].GetBufferEntry()->m_pMainBuf;
				pBufferingAddr = (VOID*)((UINT32)pBufferingAddr + (LOGICAL_PAGE_SIZE * nLPNIndex));

				OSAL_MEMCPY(pDataAddr, pBufferingAddr, LOGICAL_PAGE_SIZE);

				((UINT32*)pstBufferEntry->m_pSpareBuf)[nLPNOffset] = ((UINT32*)m_astBuffering[nBufferingIndex].GetBufferEntry()->m_pSpareBuf)[nLPNIndex];
#if defined(SUPPORT_DATA_VERIFICATION) || defined(WIN32)
				DEBUG_ASSERT(((UINT32*)pstBufferEntry->m_pSpareBuf)[nLPNOffset] == nLPN);
#endif
				return nVPPN;
			}
		}
		if (nBufferingIndex == 0)
		{
			nBufferingIndex = (ACTIVE_BLOCK_BUFFERING_COUNT - 1);
		}
		else
		{
			nBufferingIndex--;
		}
	}

	return nVPPN;
}

///////////////////////////////////////////////////////////////////////////////
//
//	BUFFERING_LPN
//

VOID
BUFFERING_LPN::Initialize(IOTYPE eIOType)
{
	OSAL_MEMSET(&m_anBufferingLPNCount[0], 0x00, sizeof(m_anBufferingLPNCount));
	m_eIOType = eIOType;
}

VOID
BUFFERING_LPN::Add(UINT32 nLPN)
{
	UINT16	nIndex = BUFFERING_LPN::GET_BUCKET_INDEX(nLPN);

	m_anBufferingLPNCount[nIndex]++;
}

VOID
BUFFERING_LPN::Remove(UINT32 nLPN)
{
	UINT16	nIndex = BUFFERING_LPN::GET_BUCKET_INDEX(nLPN);
	DEBUG_ASSERT(m_anBufferingLPNCount[nIndex] > 0);

	m_anBufferingLPNCount[nIndex]--;
}

/*
	@Brief read LPn from buffering active block
*/
UINT32
BUFFERING_LPN::ReadLPN(UINT32 nLPN, BUFFER_ENTRY* pstBufferEntry)
{
	UINT16	nIndex = BUFFERING_LPN::GET_BUCKET_INDEX(nLPN);

	if (m_anBufferingLPNCount[nIndex] == 0)
	{
		return INVALID_PPN;
	}

	// Get Active Block
	ACTIVE_BLOCK_MGR*	pstActiveBlockMgr = DFTL_GLOBAL::GetActiveBlockMgr();
	return pstActiveBlockMgr->ReadBufferingLPN(nLPN, pstBufferEntry, m_eIOType);
}

///////////////////////////////////////////////////////////////////////////////
//
//	ACTIVE_BLOCK_MGR
//
///////////////////////////////////////////////////////////////////////////////
VOID
ACTIVE_BLOCK_MGR::Initialize(VOID)
{
	m_nCurActiveBlockHost	= 0;
	m_nCurActiveBlockGC		= 0;
	m_nCurActiveBlockMeta	= 0;

	for (int i = 0; i < ACTIVE_BLOCK_COUNT_PER_STREAM; i++)
	{
		m_astActiveBlockHost[i].SetIOType(IOTYPE_HOST);
		m_astActiveBlockGC[i].SetIOType(IOTYPE_GC);
		m_astActiveBlockMeta[i].SetIOType(IOTYPE_META);
	}

	m_pstCurActiveBlockHost = &m_astActiveBlockHost[m_nCurActiveBlockHost];
	m_pstCurActiveBlockGC = &m_astActiveBlockGC[m_nCurActiveBlockGC];
	m_pstCurActiveBlockMeta = &m_astActiveBlockMeta[m_nCurActiveBlockMeta];

	m_stBufferingMgr.Initialize(IOTYPE_HOST);

#if (SUPPORT_META_DEMAND_LOADING == 1)
	m_stMetaBufferingMgr.Initialize(IOTYPE_META);
#endif
}

ACTIVE_BLOCK*
ACTIVE_BLOCK_MGR::_GetCurActiveBlock(IOTYPE eIOType)
{
	ACTIVE_BLOCK*		pstActiveBlock;

	switch (eIOType)
	{
	case IOTYPE_HOST:
		pstActiveBlock = m_pstCurActiveBlockHost;
		break;

	case IOTYPE_GC:
		pstActiveBlock = m_pstCurActiveBlockGC;
		break;

	case IOTYPE_META:
		pstActiveBlock = m_pstCurActiveBlockMeta;
		break;

	default:
		ASSERT(0);		// unknown type
		break;
	}

	return pstActiveBlock;
}

ACTIVE_BLOCK*
ACTIVE_BLOCK_MGR::_GoToNextActiveBlock(IOTYPE eIOType)
{
	ACTIVE_BLOCK*		pstActiveBlock;
	BOOL				bUser = FALSE;
	BOOL				bGC = FALSE;
	BOOL				bMeta = FALSE;

	BLOCK_MGR* pstBlockMgr;

	switch (eIOType)
	{
	case IOTYPE_HOST:
		m_nCurActiveBlockHost = INCREASE_IN_RANGE(m_nCurActiveBlockHost, ACTIVE_BLOCK_COUNT_PER_STREAM);
		m_pstCurActiveBlockHost = &m_astActiveBlockHost[m_nCurActiveBlockHost];
		pstActiveBlock = m_pstCurActiveBlockHost;
		bUser = TRUE;

		pstBlockMgr = DFTL_GLOBAL::GetInstance()->GetUserBlockMgr();
		break;

	case IOTYPE_GC:
		m_nCurActiveBlockGC = INCREASE_IN_RANGE(m_nCurActiveBlockGC, ACTIVE_BLOCK_COUNT_PER_STREAM);
		m_pstCurActiveBlockGC = &m_astActiveBlockGC[m_nCurActiveBlockGC];
		pstActiveBlock = m_pstCurActiveBlockGC;
		bGC = TRUE;

		pstBlockMgr = DFTL_GLOBAL::GetInstance()->GetUserBlockMgr();
		break;

#if (SUPPORT_META_DEMAND_LOADING == 1)
	case IOTYPE_META:
		m_nCurActiveBlockMeta = INCREASE_IN_RANGE(m_nCurActiveBlockMeta, ACTIVE_BLOCK_COUNT_PER_STREAM);
		m_pstCurActiveBlockMeta = &m_astActiveBlockMeta[m_nCurActiveBlockMeta];
		pstActiveBlock = m_pstCurActiveBlockMeta;
		bMeta = TRUE;

		pstBlockMgr = DFTL_GLOBAL::GetInstance()->GetMetaBlockMgr();
		break;
#endif

	default:
		ASSERT(0);
		break;
	}
	DEBUG_ASSERT(pstActiveBlock->m_nCurVPPN == INVALID_PPN);

	// Clear current active status
	if (pstActiveBlock->m_nVBN != INVALID_VBN)
	{
		VBINFO* pstVBInfo = DFTL_GLOBAL::GetVBInfoMgr()->GetVBInfo(pstActiveBlock->m_nVBN);
		DEBUG_ASSERT(pstVBInfo->IsActive() == TRUE);

		pstVBInfo->ClearActive();
#if (SUPPORT_BLOCK_DEBUG == 1)
		pstBlockMgr->CheckVPC(pstActiveBlock->m_nVBN);
#endif
	}

	// Allocate New Block
	UINT32 nVBN = pstBlockMgr->Allocate(bUser, bGC, bMeta);

	pstActiveBlock->m_nVBN = nVBN;
	pstActiveBlock->m_nCurVPPN = GET_VPPN_FROM_VPN_VBN(0, nVBN);
	pstActiveBlock->SetIOType(eIOType);

	DEBUG_ASSERT(pstActiveBlock->CheckAllProgramUnitIsFree() == TRUE);

	switch (eIOType)
	{
	case IOTYPE_HOST:
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_IO_ERASE, DFTL_GLOBAL::GetVNandMgr()->GetPBlocksPerVBlock());
		break;

	case IOTYPE_GC:
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_GC_ERASE, DFTL_GLOBAL::GetVNandMgr()->GetPBlocksPerVBlock());
		break;

	case IOTYPE_META:
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_META_ERASE, DFTL_GLOBAL::GetVNandMgr()->GetPBlocksPerVBlock());
		break;

	default:
		ASSERT(0);
		break;
	}

	return pstActiveBlock;
}

/*
	@brief get active block for writes
*/
ACTIVE_BLOCK* 
ACTIVE_BLOCK_MGR::GetActiveBlock(IOTYPE eIOType)
{
	BOOL	bNeedNewBlock = FALSE;

	ACTIVE_BLOCK*		pstActiveBlock;
	pstActiveBlock = _GetCurActiveBlock(eIOType);

	if (pstActiveBlock->m_nCurVPPN == INVALID_PPN)
	{
		pstActiveBlock = _GoToNextActiveBlock(eIOType);
	}

	return pstActiveBlock;
}

/*
	@brief get active block data structure
*/
ACTIVE_BLOCK*
ACTIVE_BLOCK_MGR::GetActiveBlock(INT32 nIndex, IOTYPE eIOType)
{
	DEBUG_ASSERT(nIndex < ACTIVE_BLOCK_COUNT_PER_STREAM);

	ACTIVE_BLOCK* pstActiveBlock;
	switch (eIOType)
	{
	case IOTYPE_HOST:
		pstActiveBlock = &m_astActiveBlockHost[nIndex];
		break;

	case IOTYPE_GC:
		pstActiveBlock = &m_astActiveBlockGC[nIndex];
		break;

	case IOTYPE_META:
		pstActiveBlock = &m_astActiveBlockMeta[nIndex];
		break;

	default:
		ASSERT(0);
		pstActiveBlock = NULL;
		break;
	}

	return pstActiveBlock;
}

UINT32
ACTIVE_BLOCK_MGR::ReadBufferingLPN(UINT32 nLPN, BUFFER_ENTRY* pstBufferEntry, IOTYPE eIOType)
{
	ACTIVE_BLOCK*	pstActiveBlock;
	UINT32			nVPPN;

	for (int i = 0; i < ACTIVE_BLOCK_COUNT_PER_STREAM; i++)
	{
		pstActiveBlock = GetActiveBlock(i, eIOType);
		nVPPN = pstActiveBlock->ReadBufferingLPN(nLPN, pstBufferEntry);
		if (nVPPN != INVALID_VPPN)
		{
			break;
		}
	}

	return nVPPN;
}

UINT32
ACTIVE_BLOCK_MGR::GetActiveBlockIndex(IOTYPE eIOType)
{
	switch (eIOType)
	{
	case IOTYPE_HOST:
		return m_nCurActiveBlockHost;

	case IOTYPE_GC:
		return m_nCurActiveBlockGC;

	case IOTYPE_META:
		return m_nCurActiveBlockMeta;

	default:
		ASSERT(0);
		break;
	}

	return INVALID_INDEX;
}


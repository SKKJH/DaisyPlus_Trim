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

VOID 
GC_MGR::Initialize(UINT32 nGCTh, IOTYPE eIOType)
{
	m_nThreshold	= nGCTh;
	m_eIOType		= eIOType;

	gc_cnt = 1;
	meta_cnt = 1;
	GC_POLICY_GREEDY::Initialize(eIOType);
}

VOID
GC_MGR::Run(VOID)
{
	if (IsGCRunning() == FALSE)
	{
		// nothing to do
		return;
	}

#if (SUPPORT_META_DEMAND_LOADING == 1)
	if (m_eIOType == IOTYPE_GC)
	{
		// Check MetaGC
		if (DFTL_GLOBAL::GetMetaGCMgr()->IsGCRunning() == TRUE)
		{
			return;
		}
	}
#endif

	_Read();
}

BOOL
GC_MGR::IsGCRunning(VOID)
{
	if (m_nVictimVBN == INVALID_VBN)
	{
		return FALSE;
	}

	return TRUE;
}

VOID
GC_MGR::CheckAndStartGC(const char* callerName)
{
	// check is there anys running GC
	if (IsGCRunning() == TRUE)
	{
//		xil_printf("GC Running\r\n");
		return;
	}

#if (SUPPORT_META_DEMAND_LOADING == 1)
	if (m_eIOType == IOTYPE_GC)
	{
		// Check MetaGC
		if (DFTL_GLOBAL::GetMetaGCMgr()->IsGCRunning() == TRUE)
		{
//			xil_printf("meta GC Running\r\n");
			return;
		}
	}
#endif

	UINT32 nFreeBlock;

	DFTL_GLOBAL*	pstGlobal = DFTL_GLOBAL::GetInstance();
	nFreeBlock = m_pstBlockMgr->GetFreeBlockCount();

	if (nFreeBlock > m_nThreshold)
	{
//		xil_printf("enough free block\r\n");
		// enough free block
		return;
	}
	if (strcmp(callerName, "GetMetaGCMgr") != 0)
	{
//		xil_printf("This function is executed in the class: %s, %d\r\n", callerName, gc_cnt);
		gc_cnt += 1;
	}
	else
	{
		meta_cnt += 1;
	}
	m_nVictimVBN = GetVictimVBN();

	m_nVPC = pstGlobal->GetVPagePerVBlock() - DFTL_GLOBAL::GetVBInfoMgr()->GetVBInfo(m_nVictimVBN)->GetInvalidLPNCount();
	m_nCurReadVPageOffset = 0;
	m_nWriteCount = 0;

#if (SUPPORT_GC_DEBUG == 1)
	m_stDebug.Initialize();
#endif

#if (SUPPORT_BLOCK_DEBUG == 1)
	m_pstBlockMgr->CheckVPC(m_nVictimVBN);
#endif
}

VOID
GC_MGR::IncreaseWriteCount(VOID)
{
	m_nWriteCount++;

	DEBUG_ASSERT(m_nWriteCount <= m_nVPC);

	if (m_nWriteCount == m_nVPC)
	{
		// GC Done for current victim
		m_nVictimVBN = INVALID_VBN;
	}
}

VOID
GC_MGR::_Read(VOID)
{
	DFTL_GLOBAL*	pstGlobal = DFTL_GLOBAL::GetInstance();
	VNAND*			pstVNand = pstGlobal->GetVNandMgr();
//	xil_printf("GC Start\r\n");
	do
	{
		if (m_nCurReadVPageOffset >= pstGlobal->GetVPagePerVBlock())
		{
//			xil_printf("end of block, GC Read Done\r\n");
			// end of block, GC Read Done
			return;
		}

		BOOL bValid = pstVNand->IsValid(m_nVictimVBN, m_nCurReadVPageOffset);
		if (bValid == TRUE)
		{
//			xil_printf("bValid == TRUE\r\n");
			break;
		}

		m_nCurReadVPageOffset++;
		DEBUG_ASSERT(m_nCurReadVPageOffset <= pstGlobal->GetVPagePerVBlock());
	} while (1);

	REQUEST_MGR*	pstReqMgr = pstGlobal->GetRequestMgr();

	// Issue Read
	GC_REQUEST*	pstRequest;

	pstRequest = pstReqMgr->AllocateGCRequest();
	if (pstRequest == NULL)
	{
//		xil_printf("no more free request\r\n");
		// no more free request
		return;
	}

	UINT32	nVPPN = VPPN_FROM_VBN_VPN(m_nVictimVBN, m_nCurReadVPageOffset);

	pstRequest->Initialize(GC_REQUEST_READ_WAIT, nVPPN, m_eIOType);

	pstReqMgr->AddToGCRequestWaitQ(pstRequest);

	m_nCurReadVPageOffset++;
//	xil_printf("GC End\r\n");

	return;
}

///////////////////////////////////////////////////////////////////////////////
//
//	GREEDY VICTIM SELECTION
//
///////////////////////////////////////////////////////////////////////////////

VOID 
GC_POLICY_GREEDY::Initialize(IOTYPE eIOType)
{
	switch (eIOType)
	{
	case IOTYPE_GC:
		m_pstBlockMgr = DFTL_GLOBAL::GetUserBlockMgr();
		break;

#if (SUPPORT_META_DEMAND_LOADING == 1)
	case IOTYPE_META:
		m_pstBlockMgr = DFTL_GLOBAL::GetMetaBlockMgr();
		break;
#endif

	default:
		ASSERT(0);
		break;
	}
}

/*
	@brief	choose a maximum invalid page block
			this function can be imrpoved an InvalidPageCount hash data structure
				to decrease MaxInvalidPageBlock lookup time
*/
UINT32
GC_POLICY_GREEDY::GetVictimVBN(VOID)
{
	VBINFO*	pstVBInfo;
	UINT32	nVictimVBN = INVALID_VBN;
	UINT32	nMaxInvalid = 0;

	// lookup all used blocks
	// CHECKME, TBD, 성능 개선 필요.
	list_for_each_entry(VBINFO, pstVBInfo, &m_pstBlockMgr->m_dlUsedBlocks, m_dlList)
	{
		DEBUG_ASSERT(pstVBInfo->IsFree() == FALSE);

		if (pstVBInfo->IsActive() == TRUE)
		{
			continue;
		}

		if (pstVBInfo->GetInvalidLPNCount() > nMaxInvalid)
		{
			nMaxInvalid	= pstVBInfo->GetInvalidLPNCount();
			nVictimVBN	= pstVBInfo->m_nVBN;
		}
	}

	DEBUG_ASSERT(nVictimVBN != INVALID_VBN);

	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_BGC);

	return nVictimVBN;
}


///////////////////////////////////////////////////////////////////////////////
//
//	DEBUGGING
//
///////////////////////////////////////////////////////////////////////////////

VOID
GC_DEBUG::Initialize(VOID)
{
	INT32 nSize;
	nSize = DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock() * sizeof(GC_DEBUG_FLAG);

	if (m_apstVPNStatus == NULL)
	{
		m_apstVPNStatus = static_cast<GC_DEBUG_FLAG*>(OSAL_MemAlloc(MEM_TYPE_FW_DATA, nSize, OSAL_MEMALLOC_FW_ALIGNMENT));
	}

	OSAL_MEMSET(m_apstVPNStatus, 0x00, nSize);
	m_nRead = 0;
	m_nWrite = 0;
}

VOID
GC_DEBUG::SetFlag(UINT32 nVPPN, GC_DEBUG_FLAG eFlag)
{
	UINT32 nVPageOffset = VPN_FROM_VPPN(nVPPN);

	DEBUG_ASSERT((m_apstVPNStatus[nVPageOffset] & eFlag) == 0);

	m_apstVPNStatus[nVPageOffset] = static_cast<GC_DEBUG_FLAG>((UINT32)m_apstVPNStatus[nVPageOffset] | (UINT32)eFlag);

	switch (eFlag)
	{
	case GC_DEBUG_FLAG_READ:
		m_nRead++;
		break;

	case GC_DEBUG_FLAG_WRITE_ISSUE:
		m_nWrite++;
		break;

	case GC_DEBUG_FLAG_WRITE_DONE:
		DEBUG_ASSERT(m_apstVPNStatus[nVPageOffset] == GC_DEBUG_FLAG_MASK);
		break;
	}
}

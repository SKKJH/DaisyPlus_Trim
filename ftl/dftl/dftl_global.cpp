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

#include "hil.h"

#include "dftl_internal.h"

DFTL_GLOBAL* DFTL_GLOBAL::m_pstInstance;

VIRTUAL VOID 
DFTL_GLOBAL::Initialize(VOID)
{
	m_pstInstance = this;
	_Initialize();

	GetVNandMgr()->Initialize();
	GetMetaMgr()->Initialize();
	GetMetaL2VMgr()->Initialize();
	GetVBInfoMgr()->Initialize();
	GetUserBlockMgr()->Initialize(USER_BLOCK_MGR);		// must be formantted before meta block mgr
#if (SUPPORT_META_DEMAND_LOADING == 1)
	GetMetaBlockMgr()->Initialize(META_BLOCK_MGR);
#endif
	GetRequestMgr()->Initialize();
	GetBufferMgr()->Initialize();
	GetActiveBlockMgr()->Initialize();
	GetGCMgr()->Initialize(GetGCTh(), IOTYPE_GC);
#if (SUPPORT_META_DEMAND_LOADING == 1)
	GetMetaGCMgr()->Initialize(META_GC_THRESHOLD, IOTYPE_META);
#endif

	_PrintInfo();
}

VIRTUAL BOOL
DFTL_GLOBAL::Format(VOID)
{
	BOOL	bRet;

	GetUserBlockMgr()->Format();
//	xil_printf("GetUserBlockMgr done\r\n");

#if (SUPPORT_META_DEMAND_LOADING == 1)
	GetMetaBlockMgr()->Format();
//	xil_printf("GetMetaBlockMgr done\r\n");
	bRet = GetMetaL2VMgr()->Format();
//	xil_printf("GetMetaL2VMgr done\r\n");
#endif

	bRet = GetMetaMgr()->Format();			// THIS MUST BE AT THE END OF FORMAT ROUTINE
//	xil_printf("GetMetaMgr done\r\n");

	return bRet;
}

VIRTUAL VOID 
DFTL_GLOBAL::Run(VOID)
{
	FIL_Run();
	GetRequestMgr()->Run();

#if (SUPPORT_META_DEMAND_LOADING == 1)
	GetMetaGCMgr()->CheckAndStartGC("GetMetaGCMgr");
#endif

	GetGCMgr()->CheckAndStartGC("GetGCMgr");

#if (SUPPORT_META_DEMAND_LOADING == 1)
	GetMetaGCMgr()->Run();
#endif

	GetGCMgr()->Run();
}

/*
	@brief	Add HIL read request to waitQ
*/
VIRTUAL VOID
DFTL_GLOBAL::ReadPage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount)
{
	HIL_REQUEST*	pstRequest;

	do
	{
		// allocate request
		pstRequest = m_stRequestMgr.AllocateHILRequest();
		if (pstRequest == NULL)
		{
			Run();
		}
	} while (pstRequest == NULL);

	pstRequest->Initialize(HIL_REQUEST_READ_WAIT, NVME_CMD_OPCODE_READ, 
						nLPN, nCmdSlotTag, nCount);

	// add to waitQ
	m_stRequestMgr.AddToHILRequestWaitQ(pstRequest);

	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_READ, pstRequest->GetLPNCount());
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_READ_REQ);
}

VIRTUAL VOID
DFTL_GLOBAL::WritePage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount)
{
	HIL_REQUEST*	pstRequest;

	do
	{
		// allocate request
		pstRequest = m_stRequestMgr.AllocateHILRequest();
		if (pstRequest == NULL)
		{
			Run();
		}
	} while (pstRequest == NULL);

	pstRequest->Initialize(HIL_REQUEST_WRITE_WAIT, NVME_CMD_OPCODE_WRITE, 
		nLPN, nCmdSlotTag, nCount);
//	xil_printf("nCount : %d\r\n",nCount);

	// add to waitQ
	m_stRequestMgr.AddToHILRequestWaitQ(pstRequest);

	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_WRITE, pstRequest->GetLPNCount());
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_WRITE_REQ);
}

VIRTUAL VOID
DFTL_GLOBAL::CallBack(FTL_REQUEST_ID stReqID)
{
#if (UNIT_TEST_FIL_PERF == 1)
	return;
#endif

	switch (stReqID.stCommon.nType)
	{
	case FTL_REQUEST_ID_TYPE_HIL_READ:
	{
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		HIL_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetHILRequestInfo();
		HIL_REQUEST * pstRequest = pstRequestInfo->GetRequest(stReqID.stHIL.nRequestIndex);

		pstRequest->IncreaseDoneCount();

		if (pstRequest->GetDoneCount() == pstRequest->GetLPNCount())
		{
			// all read done  
			// remove from issued Q
			pstRequestInfo->RemoveFromIssuedQ(pstRequest);

			// add to done Q
			pstRequestInfo->AddToDoneQ(pstRequest);		// wait for HDMA Issue

			pstRequest->GoToNextStatus();		// NAND Issued -> NAND_DONE
		}

		break;
	}
	case FTL_REQUEST_ID_TYPE_WRITE:
	{
		// Pysical page program done
		INT32	nIndex = stReqID.stProgram.nActiveBlockIndex;
		IOTYPE	eIOType = static_cast<IOTYPE>(stReqID.stProgram.nIOType);

		ACTIVE_BLOCK* pstActiveBlock = DFTL_GLOBAL::GetActiveBlockMgr()->GetActiveBlock(nIndex, eIOType);
		pstActiveBlock->ProgramDone(stReqID.stProgram.nBufferingIndex);
		break;
	}
	case FTL_REQUEST_ID_TYPE_GC_READ:
	{
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		GC_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetGCRequestInfo();
		GC_REQUEST * pstRequest = pstRequestInfo->GetRequest(stReqID.stGC.nRequestIndex);
		pstRequest->GCReadDone();
		break;
	}
#if (SUPPORT_META_DEMAND_LOADING == 1)
	case FTL_REQUEST_ID_TYPE_META_READ:
	{
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		META_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetMetaRequestInfo();
		META_REQUEST * pstRequest = pstRequestInfo->GetRequest(stReqID.stMeta.nRequestIndex);

		pstRequestInfo->RemoveFromIssuedQ(pstRequest);
		pstRequestInfo->AddToDoneQ(pstRequest);
		pstRequest->GoToNextStatus();					// NAND Issued -> NAND_DONE

		break;
	}
#endif
	default:
		ASSERT(0);
		break;
	}
}

VIRTUAL VOID 
DFTL_GLOBAL::IOCtl(IOCTL_TYPE eType)
{
	switch (eType)
	{
	case IOCTL_INIT_PROFILE_COUNT:
		m_stProfile.Initialize();
		break;

	case IOCTL_PRINT_PROFILE_COUNT:
		m_stProfile.Print();
		break;

	default:
		ASSERT(0);		// unknown type
		break;
	}

	return;
}

VOID
DFTL_GLOBAL::SetStatus(DFTL_STATUS eStatus)
{
	m_eStatus = static_cast<DFTL_STATUS>(m_eStatus | eStatus);
}

BOOL
DFTL_GLOBAL::CheckStatus(DFTL_STATUS eStatus)
{
	return (m_eStatus & eStatus) ? TRUE : FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
//	static function
//

VOID 
DFTL_GLOBAL::_Initialize(VOID)
{
	UINT32 nPPagesPerVBlock = m_stVNand.GetPPagesPerVBlock();
	m_nPhysicalFlashSizeKB = m_stVNand.GetVBlockCount() * nPPagesPerVBlock * (PHYSICAL_PAGE_SIZE / KB);

	m_nVBlockSizeKB			= nPPagesPerVBlock * PHYSICAL_PAGE_SIZE;
	m_nVPagesPerVBlock		= m_stVNand.GetVPagesPerVBlock();
	m_nLPagesPerVBlockBits	= UTIL_GetBitCount(m_nVPagesPerVBlock);
	m_nLPagesPerVBlockMask	= (1 << m_nLPagesPerVBlockBits) - 1;

	m_fOverProvisionRatio = (float)OVERPROVISION_RATIO_DEFAULT;
	m_nOverprovisionSizeKB = (INT64)(m_nPhysicalFlashSizeKB * m_fOverProvisionRatio);
	m_nLogicalFlashSizeKB = m_nPhysicalFlashSizeKB - m_nOverprovisionSizeKB;

#if (SUPPORT_STATIC_DENSITY != 0)
	UINT32 nLogicalFlashSizeKB = SUPPORT_STATIC_DENSITY * (GB / KB);

	ASSERT(m_nLogicalFlashSizeKB >= nLogicalFlashSizeKB);

	m_nLogicalFlashSizeKB = nLogicalFlashSizeKB;		// update logical flash size

	ASSERT(m_nLogicalFlashSizeKB >= nLogicalFlashSizeKB);
#endif

	m_nLPNCount			= m_nLogicalFlashSizeKB / LOGICAL_PAGE_SIZE_KB;

	m_nVBlockCount		= m_stVNand.GetVBlockCount();

#if (SUPPORT_META_BLOCK == 1)
	m_bEnableMetaBlock = TRUE;
#else
	m_bEnableMetaBlock = FALSE;
#endif

	m_nGCTh = FREE_BLOCK_GC_THRESHOLD_DEFAULT;

	HIL_SetStorageBlocks(m_nLPNCount);

	m_stProfile.Initialize();
}

VOID 
DFTL_GLOBAL::_PrintInfo(VOID)
{
#if defined(FPM_FTL)
	char	psFTL[] = "FPMFTL";
#elif defined(DFTL)
	char	psFTL[] = "DFTL";
#else
#error check config
#endif

	PRINTF("[%s] Physical Density: %d MB \n\r", psFTL, m_nPhysicalFlashSizeKB / KB);
	PRINTF("[%s] Logical Density: %d MB \n\r", psFTL, m_nLogicalFlashSizeKB / KB);
}

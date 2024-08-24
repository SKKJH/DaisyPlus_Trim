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
#include "host_lld.h"

VOID 
HDMA::IssueTxDMA(UINT32 nCmdSlotTag, UINT32 nCmd4KOffset, UINT32 nBufAddr)
{
	// host read done, send data to host 
	set_auto_tx_dma(nCmdSlotTag, nCmd4KOffset, nBufAddr, NVME_COMMAND_AUTO_COMPLETION_ON);
}

VOID 
HDMA::IssueRxDMA(UINT32 nCmdSlotTag, UINT32 nCmd4KOffset, UINT32 nBufAddr)
{
	// host write request, receive data from host 
	set_auto_rx_dma(nCmdSlotTag, nCmd4KOffset, nBufAddr, NVME_COMMAND_AUTO_COMPLETION_ON);
}

/*
	@brief current DMA Rx(Host->SSD) index
*/
UINT8
HDMA::GetRxDMAIndex(VOID)
{
	return g_hostDmaStatus.fifoTail.autoDmaRx;
}

UINT8
HDMA::GetTxDMAIndex(VOID)
{
	return g_hostDmaStatus.fifoTail.autoDmaTx;
}

/*
	@brief current DMA Rx(Host->SSD) Overflow Count
*/
UINT32
HDMA::GetRxDMAOverFlowCount(VOID)
{
	return g_hostDmaAssistStatus.autoDmaRxOverFlowCnt;
}

UINT32
HDMA::GetTxDMAOverFlowCount(VOID)
{
	return g_hostDmaAssistStatus.autoDmaTxOverFlowCnt;
}

VOID
HDMA::WaitRxDMADone(UINT32 nDMaReqTail, UINT32 nOverflowCount)
{
	// Wait DMA Done
	BOOL bDMADone;
	do
	{
		bDMADone = check_auto_rx_dma_partial_done(nDMaReqTail, nOverflowCount);

#if defined(WIN32) || (NVME_UNIT_TEST == 1)
		// process virtual DMA done by force
		SYSTEM_Run();
#endif

	} while (bDMADone == FALSE);

	return;
}

BOOL
HDMA::CheckTxDMADone(UINT32 nDMaReqTail, UINT32 nOverflowCount)
{
	return check_auto_tx_dma_partial_done(nDMaReqTail, nOverflowCount);
}


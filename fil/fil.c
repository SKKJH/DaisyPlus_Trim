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
#include "assert.h"
#include "xil_types.h"
#include "debug.h"

#include "cosmos_plus_system.h"
#include "cosmos_plus_memory_map.h"

#ifdef GREEDY_FTL
#include "request_schedule.h"
#endif

#include "cosmos_plus_system.h"

#include "fil.h"
#include "fil_nand.h"
#include "../Target/osal.h"
#include "fil_request.h"

static void _AddToWaitQ(FTL_REQUEST_TYPE nReqType, FTL_REQUEST_ID stReqID, NAND_ADDR stPhyAddr, void* pMainBuf, void* pSpareBuf);

void FIL_Initialize(void)
{
	NAND_Initialize();
	FIL_InitRequest();
}

void FIL_Run(void)
{
	if (FIL_IsIdle() == TRUE)
	{
		return;
	}

	// not implemented yet!
	FIL_ProcessDoneQ();
	FIL_ProcessIssuedQ();
	FIL_ProcessWaitQ();
}

/*
	@brief check a block is bad or not.
			channel and way does not considered in current implementation.
			the reason is FTL uses VBlock, which is super block.
*/
BOOL FIL_IsBad(INT32 nPBN)
{
	// Bad block of cosmos+
	static int anBadPBN[] = 
	{
		1108, 1125, 1130, 1160, 1173,
		1000, 1051, 1076, 1077, 1082, 1094, 1095,
		932, 933, 946, 964, 965, 987,
		808,
		700, 701, 717, 767, 768, 769, 772, 780, 793,
		600, 607, 642, 653, 655, 687, 692, 696,
		562,
		330, 320,
		256,
//		527, 517, 513, 505,
//		481, 415,
//		294, 292, 286, 256,
//		5, 3, 1, 0,
	};

	for (int i = 0; i < sizeof(anBadPBN) / sizeof(int); i++)
	{
		if (anBadPBN[i] == nPBN)
		{
			return TRUE;
		}
	}

	return FALSE;
}

/*
	@brief	COSMOS+ does not support partial page read 
*/
void FIL_ReadPage(FTL_REQUEST_ID stReqID, NAND_ADDR stPhyAddr, void* pMainBuf, void* pSpareBuf)
{
#ifdef SUPPORT_DATA_VERIFICATION
	#ifdef WIN32
		OSAL_MEMSET(pMainBuf, 0xFF, (sizeof(INT32) * 4/*LPN_PER_PHYSICAL_PAGE*/));
	#endif
		OSAL_MEMSET(pSpareBuf, 0xFF, (sizeof(INT32) * 4 /*LPN_PER_PHYSICAL_PAGE*/));
#endif
	
	_AddToWaitQ(FTL_REQUEST_READ, stReqID, stPhyAddr, pMainBuf, pSpareBuf);
}

void FIL_ProgramPage(FTL_REQUEST_ID	stReqID, NAND_ADDR stPhyAddr, void* pMainBuf, void* pSpareBuf)
{
	_AddToWaitQ(FTL_REQUEST_PROGRAM, stReqID, stPhyAddr, pMainBuf, pSpareBuf);
}

void FIL_EraseBlock(FTL_REQUEST_ID	stReqID, NAND_ADDR stPhyAddr)
{
	_AddToWaitQ(FTL_REQUEST_ERASE, stReqID, stPhyAddr, NULL, NULL);
}

int FIL_GetPagesPerBlock(void)
{
	int nPagesPerBlock;

	if (BITS_PER_CELL == 1)
	{
		nPagesPerBlock = PAGES_PER_SLC_BLOCK;
	}
	else if (BITS_PER_CELL == 2)
	{
		nPagesPerBlock = PAGES_PER_MLC_BLOCK;
	}
	else
	{
		ASSERT(0);	// not supported
		nPagesPerBlock = 0;
	}

	return nPagesPerBlock;
}

static void _AddToWaitQ(FTL_REQUEST_TYPE nReqType, FTL_REQUEST_ID stReqID, NAND_ADDR stPhyAddr, void* pMainBuf, void* pSpareBuf)
{
#if defined(_DEBUG) && defined(STREAM_FTL)
	DEBUG_ASSERT(stReqID.stCommon.nType < FTL_REQUEST_ID_TYPE_COUNT);
	switch (stReqID.stCommon.nType)
	{
		case FTL_REQUEST_ID_TYPE_PROGRAM:
			// In this context FIL does not know what sub-type of write, (sub type must be one of HIL Write, GC Write, StreamMerge Write)
			break;

		case FTL_REQUEST_ID_TYPE_HIL_READ:
		case FTL_REQUEST_ID_TYPE_STREAM_MERGE_READ:
		case FTL_REQUEST_ID_TYPE_BLOCK_GC_READ:
			break;

		default:
			ASSERT(0);		// Invalid type for NAND IO
			break;
	}

	DEBUG_ASSERT(pMainBuf);
	DEBUG_ASSERT(pSpareBuf);
#endif

	// Allocate Request
	FTL_REQUEST*	pstRequest;
	pstRequest = FIL_AllocateRequest();
	DEBUG_ASSERT(pstRequest->nStatus == FTL_REQUEST_FREE);

	pstRequest->stRequestID = stReqID;
	pstRequest->nType = nReqType;
	pstRequest->stAddr = stPhyAddr;
	pstRequest->stBuf.pMainBuf = pMainBuf;
	pstRequest->stBuf.pSpareBuf = pSpareBuf;

	switch (nReqType)
	{
	case FTL_REQUEST_READ:
		pstRequest->nStatus = FTL_REQUEST_READ_WAIT;
		break;

	case FTL_REQUEST_PROGRAM:
		if (stPhyAddr.nPPage == 0)
		{
			pstRequest->nStatus = FTL_REQUEST_ERASE_FOR_PROGRAM_WAIT;
		}
		else
		{
			pstRequest->nStatus = FTL_REQUEST_PROGRAM_WAIT;
		}
		break;

	case FTL_REQUEST_ERASE:
		pstRequest->nStatus = FTL_REQUEST_ERASE_WAIT;
		break;
	}

	// Add to waitQ
	FIL_AddToRequestWaitQ(pstRequest, FALSE);
}

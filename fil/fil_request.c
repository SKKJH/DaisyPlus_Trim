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

#include "cosmos_plus_system.h"
#include "debug.h"

#include "fil_request.h"
#include "osal.h"
#include "fil.h"
#include "fil_nand.h"
#include "ftl.h"

typedef struct
{

	struct list_head		dlFree;									// free NVMeRequest
	unsigned int			nFreeCount;								// count of free request

	// [TODO] lets add wait way bitmap for fast issue
	// [TODO] lest add Issued way bitmap for fast status check

	struct list_head		adlWait[USER_CHANNELS][USER_WAYS];		// wait queue, adl: array double linked list
	int						anWaitCount[USER_CHANNELS][USER_WAYS];

	struct list_head		adlIssued[USER_CHANNELS][USER_WAYS];		// issued queue
	int						anIssuedCount[USER_CHANNELS][USER_WAYS];	// issued count is always 1 or 0

	struct list_head		dlDoneQ;									// request will be callbeck to FTL
	int						nDoneCount;								// do i need to check count?
} FTL_REQUEST_INFO;

static void _InitRequestInfo(void);
static void _InitRequestPool(void);
static void _ReleaseRequest(FTL_REQUEST* pstRequest);
static void _ProcessWaitQ(int nCh, int nWay);
static void _ProcessIssuedQ(int nCh, int nWay);
static void _ProcessDoneQ(void);

static void _RemoveFromRequestWaitQ(FTL_REQUEST* pstRequest);
static void _AddToRequestIssuedQ(FTL_REQUEST* pstRequest);
static void _RemoveFromRequestIssuedQ(FTL_REQUEST* pstRequest);
static void _AddToRequestDoneQ(FTL_REQUEST* pstRequest);
static void _RemoveFromRequestDoneQ(FTL_REQUEST* pstRequest);

static int _IsValidAddress(NAND_ADDR* pstAddr);

static FTL_REQUEST_POOL*		g_pstFTLRequestPool;
static FTL_REQUEST_INFO*		g_pstFTLRequestInfo;

#ifdef __cplusplus
FTL_REQUEST_STATUS& operator++(FTL_REQUEST_STATUS& nVal, int)
{
	nVal = static_cast<FTL_REQUEST_STATUS>(static_cast<int>(nVal) + 1);
	return nVal;
}
#endif

void FIL_InitRequest(void)
{
	_InitRequestInfo();
	_InitRequestPool();
}

/*
	@brief	
*/
FTL_REQUEST* FIL_AllocateRequest(void)
{
	while (g_pstFTLRequestInfo->nFreeCount == 0)
	{
		// call FIL_Run to finish operation and get some free request
		FIL_Run();
	}

	DEBUG_ASSERT(list_empty(&g_pstFTLRequestInfo->dlFree) == FALSE);

	FTL_REQUEST*	pRequest;

	pRequest = list_first_entry(&g_pstFTLRequestInfo->dlFree, FTL_REQUEST, dlList);

	list_del(&pRequest->dlList);

	g_pstFTLRequestInfo->nFreeCount--;
	return pRequest;
}

void FIL_AddToRequestWaitQ(FTL_REQUEST* pstRequest, int bHead)
{
	_IsValidAddress(&pstRequest->stAddr);

	if (bHead == FALSE)		// default
	{
		list_add_tail(&pstRequest->dlList, &g_pstFTLRequestInfo->adlWait[pstRequest->stAddr.nCh][pstRequest->stAddr.nWay]);
	}
	else
	{
		list_add_head(&pstRequest->dlList, &g_pstFTLRequestInfo->adlWait[pstRequest->stAddr.nCh][pstRequest->stAddr.nWay]);
	}
	g_pstFTLRequestInfo->anWaitCount[pstRequest->stAddr.nCh][pstRequest->stAddr.nWay]++;
}

void FIL_ProcessWaitQ(void)
{
	// loop all ch/way
	for (int nWay = 0; nWay < USER_WAYS; nWay++)
	{
		for (int nCh = 0; nCh < USER_CHANNELS; nCh++)
		{
			if ((g_pstFTLRequestInfo->anIssuedCount[nCh][nWay] == 0) && (g_pstFTLRequestInfo->anWaitCount[nCh][nWay] > 0))
			{
				_ProcessWaitQ(nCh, nWay);
			}
		}
	}
}

void FIL_ProcessIssuedQ(void)
{
	// loop all ch/way
	for (int nWay = 0; nWay < USER_WAYS; nWay++)	
	{
		for (int nCh = 0; nCh < USER_CHANNELS; nCh++)
		{
			if (g_pstFTLRequestInfo->anIssuedCount[nCh][nWay] > 0)
			{
				_ProcessIssuedQ(nCh, nWay);
			}
		}
	}
}

void FIL_ProcessDoneQ(void)
{
	if (g_pstFTLRequestInfo->nDoneCount > 0)
	{
		_ProcessDoneQ();
	}
}

/*
	@breif	check FIL is idle or not
*/
int FIL_IsIdle(void)
{
	if (g_pstFTLRequestInfo->nFreeCount == FTL_REQUEST_COUNT)
	{
		DEBUG_ASSERT(g_pstFTLRequestInfo->nFreeCount <= FTL_REQUEST_COUNT);
		return TRUE;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////
//
//	static functions
//

static void _InitRequestPool(void)
{
	g_pstFTLRequestPool = (FTL_REQUEST_POOL*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, sizeof(FTL_REQUEST_POOL), 0);
	ASSERT(g_pstFTLRequestPool);

	for (int i = 0; i < FTL_REQUEST_COUNT; i++)
	{
		INIT_LIST_HEAD(&g_pstFTLRequestPool->astRequest[i].dlList);
		_ReleaseRequest(&g_pstFTLRequestPool->astRequest[i]);
	}

	return;
}

static void _InitRequestInfo(void)
{
	g_pstFTLRequestInfo = (FTL_REQUEST_INFO*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, sizeof(FTL_REQUEST_INFO), 0);
	ASSERT(g_pstFTLRequestInfo);

	INIT_LIST_HEAD(&g_pstFTLRequestInfo->dlFree);
	g_pstFTLRequestInfo->nFreeCount = 0;

	for (int nCh = 0; nCh < USER_CHANNELS; nCh++)
	{
		for (int nWay = 0; nWay < USER_WAYS; nWay++)
		{
			INIT_LIST_HEAD(&g_pstFTLRequestInfo->adlWait[nCh][nWay]);
			g_pstFTLRequestInfo->anWaitCount[nCh][nWay] = 0;

			INIT_LIST_HEAD(&g_pstFTLRequestInfo->adlIssued[nCh][nWay]);
			g_pstFTLRequestInfo->anIssuedCount[nCh][nWay] = 0;
			
		}
	}

	INIT_LIST_HEAD(&g_pstFTLRequestInfo->dlDoneQ);
	g_pstFTLRequestInfo->nDoneCount = 0;
}

static void _ReleaseRequest(FTL_REQUEST* pstRequest)
{
	list_add_tail(&pstRequest->dlList, &g_pstFTLRequestInfo->dlFree);
	pstRequest->nStatus = FTL_REQUEST_FREE;
	g_pstFTLRequestInfo->nFreeCount++;
	DEBUG_ASSERT(g_pstFTLRequestInfo->nFreeCount <= FTL_REQUEST_COUNT);
}


static void _RemoveFromRequestWaitQ(FTL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(_IsValidAddress(&pstRequest->stAddr) == TRUE);

	list_del_init(&pstRequest->dlList);
	g_pstFTLRequestInfo->anWaitCount[pstRequest->stAddr.nCh][pstRequest->stAddr.nWay]--;
}

static void _AddToRequestIssuedQ(FTL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(_IsValidAddress(&pstRequest->stAddr) == TRUE);

	list_add_tail(&pstRequest->dlList, &g_pstFTLRequestInfo->adlIssued[pstRequest->stAddr.nCh][pstRequest->stAddr.nWay]);
	g_pstFTLRequestInfo->anIssuedCount[pstRequest->stAddr.nCh][pstRequest->stAddr.nWay]++;
}

static void _RemoveFromRequestIssuedQ(FTL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(_IsValidAddress(&pstRequest->stAddr) == TRUE);

	list_del_init(&pstRequest->dlList);
	g_pstFTLRequestInfo->anIssuedCount[pstRequest->stAddr.nCh][pstRequest->stAddr.nWay]--;
}

static void _AddToRequestDoneQ(FTL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(_IsValidAddress(&pstRequest->stAddr) == TRUE);

	list_add_tail(&pstRequest->dlList, &g_pstFTLRequestInfo->dlDoneQ);
	g_pstFTLRequestInfo->nDoneCount++;
}

static void _RemoveFromRequestDoneQ(FTL_REQUEST* pstRequest)
{
	DEBUG_ASSERT(_IsValidAddress(&pstRequest->stAddr) == TRUE);

	list_del_init(&pstRequest->dlList);
	DEBUG_ASSERT(g_pstFTLRequestInfo->nDoneCount > 0);
	g_pstFTLRequestInfo->nDoneCount--;
}

static void _ProcessWaitQ(int nCh, int nWay)
{
	struct list_head*	pdlWaitHead = &g_pstFTLRequestInfo->adlWait[nCh][nWay];

	DEBUG_ASSERT(list_empty(pdlWaitHead) == FALSE);

	FTL_REQUEST* pstRequest;
	pstRequest = list_first_entry(pdlWaitHead, FTL_REQUEST, dlList);

	NAND_RESULT		nResult = NAND_RESULT_DONE;

	switch (pstRequest->nStatus)
	{
		case FTL_REQUEST_READ_WAIT:
			nResult = NAND_IssueRead(pstRequest->stAddr.nCh, pstRequest->stAddr.nWay, pstRequest->stAddr.nBlock, pstRequest->stAddr.nPPage,
							pstRequest->stBuf.pMainBuf, pstRequest->stBuf.pSpareBuf, TRUE, FALSE);
			break;

		case FTL_REQUEST_PROGRAM_WAIT:
			nResult = NAND_IssueProgram(pstRequest->stAddr.nCh, pstRequest->stAddr.nWay, pstRequest->stAddr.nBlock, pstRequest->stAddr.nPPage,
							pstRequest->stBuf.pMainBuf, pstRequest->stBuf.pSpareBuf, FALSE);
			break;

		case FTL_REQUEST_ERASE_FOR_PROGRAM_WAIT:
		case FTL_REQUEST_ERASE_WAIT:
			nResult = NAND_IssueErase(pstRequest->stAddr.nCh, pstRequest->stAddr.nWay, pstRequest->stAddr.nBlock, FALSE);
			break;

		default:
			ASSERT(0);
			break;
	}

	pstRequest->nStatus++;

	_RemoveFromRequestWaitQ(pstRequest);
	if (nResult == NAND_RESULT_OP_RUNNING)
	{
		_AddToRequestIssuedQ(pstRequest);		// Issued request will be done by FIL call back
	}
	else
	{
		DEBUG_ASSERT(nResult == NAND_RESULT_DONE);
		pstRequest->nStatus++;					// go to next steop
		_AddToRequestDoneQ(pstRequest);			// Issued request will be done by FIL call back
	}

	return;
}

static void _ProcessIssuedQ(int nCh, int nWay)
{
	struct list_head*	pdlIssuedHead = &g_pstFTLRequestInfo->adlIssued[nCh][nWay];

	DEBUG_ASSERT(list_empty(pdlIssuedHead) == FALSE);

	FTL_REQUEST* pstRequest;
	NAND_RESULT	nResult = NAND_RESULT_DONE;

	pstRequest = list_first_entry(pdlIssuedHead, FTL_REQUEST, dlList);

	switch (pstRequest->nStatus)
	{
		case FTL_REQUEST_READ_ISSUED:
			nResult = NAND_ProcessRead(pstRequest->stAddr.nCh, pstRequest->stAddr.nWay, pstRequest->stAddr.nBlock, pstRequest->stAddr.nPPage,
				pstRequest->stBuf.pMainBuf, pstRequest->stBuf.pSpareBuf, TRUE, FALSE);
			break;

		case FTL_REQUEST_PROGRAM_ISSUED:
			nResult = NAND_ProcessProgram(pstRequest->stAddr.nCh, pstRequest->stAddr.nWay, pstRequest->stAddr.nBlock, pstRequest->stAddr.nPPage,
				pstRequest->stBuf.pMainBuf, pstRequest->stBuf.pSpareBuf, FALSE);
			break;

		case FTL_REQUEST_ERASE_FOR_PROGRAM_ISSUED:
		case FTL_REQUEST_ERASE_ISSUED:
			nResult = NAND_ProcessErase(pstRequest->stAddr.nCh, pstRequest->stAddr.nWay, pstRequest->stAddr.nBlock, FALSE);
			break;

		default:
			ASSERT(0);
			break;
	}

	if (nResult == NAND_RESULT_DONE)
	{
		_RemoveFromRequestIssuedQ(pstRequest);
		pstRequest->nStatus++;
		_AddToRequestDoneQ(pstRequest);
	}

	return;
}

static void _ProcessDoneQ(void)
{
	struct list_head*	pdlDoneQHead = &g_pstFTLRequestInfo->dlDoneQ;

	do
	{
		DEBUG_ASSERT(list_empty(pdlDoneQHead) == FALSE);

		FTL_REQUEST* pstRequest;
		pstRequest = list_first_entry(pdlDoneQHead, FTL_REQUEST, dlList);

		_RemoveFromRequestDoneQ(pstRequest);

		if (pstRequest->nStatus != FTL_REQUEST_ERASE_FOR_PROGRAM_DONE)
		{
			FTL_CallBack(pstRequest->stRequestID);
			_ReleaseRequest(pstRequest);
		}
		else
		{
			// add to waitQ for program after erase
			pstRequest->nStatus++;
			FIL_AddToRequestWaitQ(pstRequest, TRUE);
		}
	} while (g_pstFTLRequestInfo->nDoneCount > 0);

	return;
}

static int _IsValidAddress(NAND_ADDR* pstAddr)
{
	if ((pstAddr->nCh >= USER_CHANNELS) ||
		(pstAddr->nWay >= USER_WAYS) ||
		(pstAddr->nBlock >= TOTAL_BLOCKS_PER_DIE) ||
		(pstAddr->nPPage >= PAGES_PER_MLC_BLOCK))
	{
		return FALSE;
	}

	return TRUE;
}

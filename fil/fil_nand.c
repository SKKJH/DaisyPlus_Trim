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

#include "cosmos_plus_system.h"

#include "fil.h"
#include "../Target/osal.h"
#include "fil_request.h"
#include "fil_nand.h"

#define OPERATION_STEP_START		(0)

typedef enum
{
	NAND_READ_STEP_NONE = OPERATION_STEP_START,					// must be 0
	NAND_READ_STEP_CHECK_READ_ISSUABLE,
	NAND_READ_STEP_ISSUE_READ,
	NAND_READ_STEP_WAIT_READ_DONE,
	NAND_READ_STEP_ISSUE_GET_STATUS_READ,
	NAND_READ_STEP_WAIT_GET_STATUS_READ_DONE,
	NAND_READ_STEP_CHECK_READ_RESULT,
	NAND_READ_STEP_CHECK_TRANSFER_ISSUABLE,
	NAND_READ_STEP_ISSUE_TRANSFER,
	NAND_READ_STEP_WAIT_TRASFER_DONE,
	NAND_READ_STEP_CHECK_TRANSFER_RESULT,
	NAND_READ_STEP_DONE,
	NAND_READ_STEP_COUNT,
} NAND_READ_STEP;

typedef enum
{
	NAND_PGM_STEP_NONE = OPERATION_STEP_START,						// must be 0
	NAND_PGM_STEP_CHECK_PROGRAM_ISSUABLE,
	NAND_PGM_STEP_ISSUE_PROGRAM,
	NAND_PGM_STEP_WAIT_PROGRAM_DONE,
	NAND_PGM_STEP_ISSUE_GET_STATUS,
	NAND_PGM_STEP_WAIT_GET_STATUS_DONE,
	NAND_PGM_STEP_CHECK_STATUS,
	NAND_PGM_STEP_DONE,
	NAND_PGM_STEP_COUNT,
} NAND_PROGRAM_STEP;

typedef enum
{
	NAND_ERASE_STEP_NONE = OPERATION_STEP_START,					// must be 0
	NAND_ERASE_STEP_CHECK_ERASE_ISSUABLE,
	NAND_ERASE_STEP_ISSUE_ERASE,
	NAND_ERASE_STEP_WAIT_ERASE_DONE,			// issue GetStatus just right after checking way idle
	NAND_ERASE_STEP_ISSUE_GET_STATUS,
	NAND_ERASE_STEP_WAIT_GET_STATUS_DONE,
	NAND_ERASE_STEP_CHECK_STATUS,				//	--> NAND_ERASE_STATUS_NON
	NAND_ERASE_STEP_DONE,
	NAND_ERASE_STEP_COUNT,
} NAND_ERASE_STEP;

typedef struct
{
	union
	{
		NAND_READ_STEP		nRead;
		NAND_PROGRAM_STEP	nProgram;
		NAND_ERASE_STEP		nErase;
		unsigned int		nCommon;
	};

	unsigned int			nRetry;
} WAY_STATUS;

typedef struct
{
	unsigned int anData[USER_CHANNELS][USER_WAYS];
} NAND_COMPLETION;

typedef struct
{
	unsigned int anData[USER_CHANNELS][USER_WAYS];
} NAND_STATUS;

typedef struct
{
	unsigned int anData[USER_CHANNELS][USER_WAYS][ERROR_INFO_WORD_COUNT];
} NAND_ERROR;

typedef struct
{
	unsigned int anVerificationData[USER_CHANNELS][USER_WAYS][TOTAL_BLOCKS_PER_DIE][ROWS_PER_MLC_BLOCK][LPN_PER_PHYSICAL_PAGE];			// 4 BYTE VERIFICATION DATA
} NAND_DEBUG;

typedef struct
{
	WAY_STATUS		stWayStatus[USER_CHANNELS][USER_WAYS];

	NAND_COMPLETION *pstNAND_Completion;
	NAND_STATUS		*pstNAND_Status;
	NAND_ERROR		*pstNAND_Error;

#if defined(SUPPORT_DATA_VERIFICATION) || defined(WIN32)
	NAND_DEBUG		*pstDebug;
#endif
} NAND_GLOBAL;

V2FMCRegisters* chCtlReg[USER_CHANNELS];
NAND_GLOBAL	*g_pstNAND;

#ifdef WIN32
static V2FMCRegisters	g_stV2FMC[USER_CHANNELS];
#endif

#if (NAND_IO_PRINT == 1)
	#define NAND_DEBUG_PRINTF	PRINTF
#else
	#define NAND_DEBUG_PRINTF(...)		((void)0)
#endif

static int _IsValidAddress(int nCh, int nWay, int nBlock, int nPage);
static int _GetTransferResult(unsigned int *panErrorFlag);
static unsigned int _GetLUNBaseAddr(int nLUN);
static int _IsWayIdle(int nPCh, int nPWay);
static int _IsCMDIssuable(int nPCh, int nPWay);
static unsigned int _GetRowAddress(int nPCh, int nPWay, int nBlock, int nPage);
static int _IsRequestComplete(unsigned int nResult);
static int _IsRequestFail(unsigned int nResult);
static int _IsCmdDone(volatile unsigned int nStatusResult);
static int _IsCmdSuccess(volatile unsigned int nStatusResult);

///////////////////////////////////////////////////////////////////////////////
//
//	NAND functions
//

void NAND_Initialize(void)
{
	NAND_InitChCtlReg();
	NAND_InitNandArray();

	g_pstNAND = (NAND_GLOBAL*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, sizeof(NAND_GLOBAL), 0);
	g_pstNAND->pstNAND_Completion = (NAND_COMPLETION*)OSAL_MemAlloc(MEM_TYPE_HW, sizeof(NAND_COMPLETION), 0);
	g_pstNAND->pstNAND_Status = (NAND_STATUS*)OSAL_MemAlloc(MEM_TYPE_HW, sizeof(NAND_STATUS), 0);
	g_pstNAND->pstNAND_Error = (NAND_ERROR*)OSAL_MemAlloc(MEM_TYPE_HW, sizeof(NAND_ERROR), 0);

	for (int nCh = 0; nCh < USER_CHANNELS; nCh++)
	{
		for (int nWay = 0; nWay < USER_WAYS; nWay++)
		{
			g_pstNAND->stWayStatus[nCh][nWay].nCommon = OPERATION_STEP_START;
		}
	}

#if defined(SUPPORT_DATA_VERIFICATION) || defined(WIN32)
	g_pstNAND->pstDebug		= (NAND_DEBUG*)OSAL_MemAlloc(MEM_TYPE_WIN32_DEBUG, sizeof(NAND_DEBUG), 0);
	OSAL_MEMSET(g_pstNAND->pstDebug, DATA_VERIFICATION_INVALID_LPN, sizeof(NAND_DEBUG));
#endif
}

/*
	@brief	The first step of erase
*/
NAND_RESULT NAND_IssueErase(int nPCh, int nPWay, int nBlock, int bSync)
{
	NAND_DEBUG_PRINTF("[NAND] Erase - Ch/Way/Block:%d/%d/%d\r\n", nPCh, nPWay, nBlock);

	DEBUG_ASSERT(g_pstNAND->stWayStatus[nPCh][nPWay].nErase == NAND_ERASE_STEP_NONE);
	return NAND_ProcessErase(nPCh, nPWay, nBlock, bSync);
}

/*
	@brief process erase operation
*/
NAND_RESULT NAND_ProcessErase(int nPCh, int nPWay, int nBlock, int bSync)
{
	DEBUG_ASSERT(_IsValidAddress(nPCh, nPWay, nBlock, 0));
	unsigned int nRowAddr;

	NAND_ERASE_STEP		*pnCurStatus = &g_pstNAND->stWayStatus[nPCh][nPWay].nErase;
	DEBUG_ASSERT(*pnCurStatus < NAND_ERASE_STEP_COUNT);

	int bIdle;
	volatile unsigned int *pnStatusResult;

	static long nSleepTimeEraseUS = 7000;			// mandatory
	static long nSleepTimeGetStatusUS = 1;			// mandatory
	static long nSleepTimeCheckStatusUS = 1;		// mandatory

	NAND_RESULT nResult = NAND_RESULT_OP_RUNNING;

	do
	{
		switch (*pnCurStatus)
		{
		case NAND_ERASE_STEP_NONE:
			*pnCurStatus = NAND_ERASE_STEP_CHECK_ERASE_ISSUABLE;
			// fall down

		case NAND_ERASE_STEP_CHECK_ERASE_ISSUABLE:
			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				if (bSync == TRUE)
				{
					continue;
				}

				break;		// try at next time
			}

			*pnCurStatus = NAND_ERASE_STEP_ISSUE_ERASE;
			// Fall through

		case NAND_ERASE_STEP_ISSUE_ERASE:
			nRowAddr = _GetRowAddress(nPCh, nPWay, nBlock, 0 /*Don't Care */);

			// Issue NAND Command
			V2FEraseBlockAsync(chCtlReg[nPCh], nPWay, nRowAddr);
			*pnCurStatus = NAND_ERASE_STEP_WAIT_ERASE_DONE;
			break;

			//OSAL_SleepUS(nSleepTimeUS);		// no need to sleep
		case NAND_ERASE_STEP_WAIT_ERASE_DONE:
			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				break;
			}

			*pnCurStatus = NAND_ERASE_STEP_ISSUE_GET_STATUS;
			// fall through

			if (bSync)
			{
				OSAL_SleepUS(nSleepTimeEraseUS);		// mandatory sleep
			}
		case NAND_ERASE_STEP_ISSUE_GET_STATUS:
			pnStatusResult = (volatile unsigned int*)&g_pstNAND->pstNAND_Status->anData[nPCh][nPWay];
			*pnStatusResult = 0;
			V2FStatusCheckAsync(chCtlReg[nPCh], nPWay, (unsigned int*)pnStatusResult);

			*pnCurStatus = NAND_ERASE_STEP_WAIT_GET_STATUS_DONE;
			break;

		case NAND_ERASE_STEP_WAIT_GET_STATUS_DONE:
			if (bSync)
			{
				OSAL_SleepUS(nSleepTimeGetStatusUS);		// mandatory sleep
			}

			bIdle = _IsWayIdle(nPCh, nPWay);
			if (bIdle == FALSE)
			{
				break;
			}

			*pnCurStatus = NAND_ERASE_STEP_CHECK_STATUS;
			// fall through

		case NAND_ERASE_STEP_CHECK_STATUS:
			pnStatusResult = (volatile unsigned int*)&g_pstNAND->pstNAND_Status->anData[nPCh][nPWay];
			if (_IsCmdDone(*pnStatusResult))
			{
				if (_IsCmdSuccess(*pnStatusResult) == FALSE)
				{
					// Fail
					xil_printf("NAND Erase Fail - Ch/Way/Block:%d/%d/%d\r\n", nPCh, nPWay, nBlock);
				}
				else
				{
					// Success
				}

				*pnCurStatus = NAND_ERASE_STEP_DONE;
			}
			else
			{
				//xil_printf("NAND Erase Status - 0x%X\r\n", *pnStatusResult);

				if (_IsRequestComplete(*pnStatusResult) == FALSE)
				{
					*pnCurStatus = NAND_ERASE_STEP_WAIT_ERASE_DONE;
				}

				if (bSync)
				{
					OSAL_SleepUS(nSleepTimeCheckStatusUS);	// mandatory
				}
			}

			break;

		default:
			assert(0);
			break;
		}

		if (*pnCurStatus == NAND_ERASE_STEP_DONE)
		{
			// Erase Done
			*pnCurStatus = NAND_ERASE_STEP_NONE;
			nResult = NAND_RESULT_DONE;
			break;
		}
	} while (bSync == TRUE);

#if defined(SUPPORT_DATA_VERIFICATION) || defined(WIN32)
	if (nResult == NAND_RESULT_DONE)
	{
		for (int i = 0; i < ROWS_PER_MLC_BLOCK; i++)
		{
			OSAL_MEMSET(&g_pstNAND->pstDebug->anVerificationData[nPCh][nPWay][nBlock], 0xFF, sizeof(unsigned int) * ROWS_PER_MLC_BLOCK * LPN_PER_PHYSICAL_PAGE);
#if 0
			for (int j = 0; j < LPN_PER_PHYSICAL_PAGE; j++)
			{
				g_pstNAND->pstDebug->anVerificationData[nPCh][nPWay][nBlock][i][j] = DATA_VERIFICATION_INVALID_LPN;
			}
#endif
		}
	}
#endif

	return nResult;
}

NAND_RESULT NAND_IssueRead(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bECCOn, int bSync)
{
	NAND_DEBUG_PRINTF("[NAND] Read - Ch/Way/Block/Page/bECCOn:%d/%d/%d/%d/%d\r\n", nPCh, nPWay, nBlock, nPage, bECCOn);

	g_pstNAND->stWayStatus[nPCh][nPWay].nRetry = 0;
	DEBUG_ASSERT(g_pstNAND->stWayStatus[nPCh][nPWay].nRead == NAND_READ_STEP_NONE);
	return NAND_ProcessRead(nPCh, nPWay, nBlock, nPage, pMainBuf, pSpareBuf, bECCOn, bSync);
}

/*
@param	pMainBuf: 16KB main data buffer
@param pSpareBuf: 256Byte spare buffer
*/
NAND_RESULT NAND_ProcessRead(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bECCOn, int bSync)
{
	DEBUG_ASSERT(_IsValidAddress(nPCh, nPWay, nBlock, nPage));

	unsigned int nRowAddr;

	NAND_READ_STEP		*pnCurStatus = &g_pstNAND->stWayStatus[nPCh][nPWay].nRead;
	DEBUG_ASSERT(*pnCurStatus < NAND_READ_STEP_COUNT);

	volatile unsigned int *pnStatusResult;

	unsigned int *panErrorInfo = (unsigned int*)(&g_pstNAND->pstNAND_Error->anData[nPCh][nPWay]);
	unsigned int *pnCompletion = (unsigned int*)(&g_pstNAND->pstNAND_Completion->anData[nPCh][nPWay]);

	static long nSleepTimeReadUS = 110;			// mandatory, LSB PGM time
	static long nSleepTimeGetStatusUS = 2;			// mandatory
	static long nSleepTimeCheckStatusUS = 2;		// mandatory
	static long nSleepTimeTransferUS = 110;		// for DMA 16KB

	NAND_RESULT nResult = NAND_RESULT_OP_RUNNING;

	do
	{
		switch (*pnCurStatus)
		{
		case NAND_READ_STEP_NONE:
			*pnCurStatus = NAND_READ_STEP_CHECK_READ_ISSUABLE;
			// fall down

		case NAND_READ_STEP_CHECK_READ_ISSUABLE:
			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				break;
			}

			*pnCurStatus = NAND_READ_STEP_ISSUE_READ;
			// Fall through

		case NAND_READ_STEP_ISSUE_READ:
			nRowAddr = _GetRowAddress(nPCh, nPWay, nBlock, nPage);

			// Issue NAND Command
			V2FReadPageTriggerAsync(chCtlReg[nPCh], nPWay, nRowAddr);
			*pnCurStatus = NAND_READ_STEP_WAIT_READ_DONE;
			break;

		case NAND_READ_STEP_WAIT_READ_DONE:
			if (bSync == TRUE)
			{
				OSAL_SleepUS(nSleepTimeReadUS);		// mandatory sleep
			}

			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				continue;
			}

			*pnCurStatus = NAND_READ_STEP_ISSUE_GET_STATUS_READ;
			// fall through

		case NAND_READ_STEP_ISSUE_GET_STATUS_READ:
			pnStatusResult = (volatile unsigned int*)&g_pstNAND->pstNAND_Status->anData[nPCh][nPWay];
			*pnStatusResult = 0;
			V2FStatusCheckAsync(chCtlReg[nPCh], nPWay, (unsigned int*)pnStatusResult);

			*pnCurStatus = NAND_READ_STEP_WAIT_GET_STATUS_READ_DONE;
			break;

		case NAND_READ_STEP_WAIT_GET_STATUS_READ_DONE:
			if (bSync == TRUE)
			{
				OSAL_SleepUS(nSleepTimeGetStatusUS);		// mandatory sleep
			}

			if (_IsWayIdle(nPCh, nPWay) == FALSE)
			{
				continue;
			}

			*pnCurStatus = NAND_READ_STEP_CHECK_READ_RESULT;
			// fall through

		case NAND_READ_STEP_CHECK_READ_RESULT:
			pnStatusResult = (volatile unsigned int*)&g_pstNAND->pstNAND_Status->anData[nPCh][nPWay];
			if (_IsCmdDone(*pnStatusResult))
			{
				if (_IsCmdSuccess(*pnStatusResult) == FALSE)
				{
					// Fail
					PRINTF("NAND Read Fail - Ch/Way/Block/Page:%d/%d/%d/%d, Try:%d\r\n", nPCh, nPWay, nBlock, nPage, g_pstNAND->stWayStatus[nPCh][nPWay].nRetry);

					g_pstNAND->stWayStatus[nPCh][nPWay].nRetry++;
					if (g_pstNAND->stWayStatus[nPCh][nPWay].nRetry > FIL_READ_RETRY)
					{
						*pnCurStatus = NAND_READ_STEP_DONE;
					}
					else
					{
						*pnCurStatus = NAND_READ_STEP_NONE;
					}
				}
				else
				{
					// Success
					*pnCurStatus = NAND_READ_STEP_CHECK_TRANSFER_ISSUABLE;
				}
			}
			else
			{
				//xil_printf("NAND Read tR Status - 0x%X\r\n", *pnStatusResult);
				*pnCurStatus = NAND_READ_STEP_WAIT_READ_DONE;
			}

			break;

		case NAND_READ_STEP_CHECK_TRANSFER_ISSUABLE:
			*pnCurStatus = NAND_READ_STEP_ISSUE_TRANSFER;
			//break;		// fall through

		case NAND_READ_STEP_ISSUE_TRANSFER:
			if (bSync == TRUE)
			{
				OSAL_SleepUS(nSleepTimeGetStatusUS);		// mandatory sleep
			}

			if (bECCOn == TRUE)
			{
				nRowAddr = _GetRowAddress(nPCh, nPWay, nBlock, nPage);
				V2FReadPageTransferAsync(chCtlReg[nPCh], nPWay, pMainBuf, pSpareBuf, panErrorInfo, pnCompletion, nRowAddr);
			}
			else
			{
				V2FReadPageTransferRawAsync(chCtlReg[nPCh], nPWay, pMainBuf, pnCompletion);
			}

			*pnCurStatus = NAND_READ_STEP_WAIT_TRASFER_DONE;
			break;

		case NAND_READ_STEP_WAIT_TRASFER_DONE:
			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				break;;
			}

			*pnCurStatus = NAND_READ_STEP_CHECK_TRANSFER_RESULT;
			break;

		case NAND_READ_STEP_CHECK_TRANSFER_RESULT:
			if (bSync == TRUE)
			{
				OSAL_SleepUS(nSleepTimeTransferUS);		// mandatory sleep
			}

			if (V2FTransferComplete(*pnCompletion))
			{
				if ((bECCOn == TRUE) && (_GetTransferResult(panErrorInfo) == ERROR_INFO_FAIL))
				{
					PRINTF("NAND Read Fail UECC - Ch/Way/Block/Page:%d/%d/%d/%d, Error0/Error1:0x%X/0x%X, Try:%d\r\n", 
									nPCh, nPWay, nBlock, nPage, panErrorInfo[0], panErrorInfo[1], g_pstNAND->stWayStatus[nPCh][nPWay].nRetry);
					g_pstNAND->stWayStatus[nPCh][nPWay].nRetry++;
					if (g_pstNAND->stWayStatus[nPCh][nPWay].nRetry > FIL_READ_RETRY)
					{
						*pnCurStatus = NAND_READ_STEP_DONE;
					}
					else
					{
						*pnCurStatus = NAND_READ_STEP_NONE;
					}
				}
				else
				{
					*pnCurStatus = NAND_READ_STEP_DONE;
				}
			}
			else
			{
				//xil_printf("NAND Read Transfer Status - Completion: 0x%X\r\n", *pnCompletion);
				OSAL_SleepUS(nSleepTimeCheckStatusUS);	// mandatory

				*pnCurStatus = NAND_READ_STEP_WAIT_TRASFER_DONE;
			}

			break;

		case NAND_READ_STEP_DONE:
			// done, do nothing just break
			break;

		default:
			assert(0);
		}

		if (*pnCurStatus == NAND_READ_STEP_DONE)
		{

			*pnCurStatus = NAND_READ_STEP_NONE;
			nResult = NAND_RESULT_DONE;

#if defined(WIN32)
			for (int i = 0; i < LPN_PER_PHYSICAL_PAGE; i++)
			{
				((unsigned int*)pSpareBuf)[i]	= g_pstNAND->pstDebug->anVerificationData[nPCh][nPWay][nBlock][nPage][i];

	#if defined(SUPPORT_DATA_VERIFICATION)
				((unsigned int*)pMainBuf)[i * (LOGICAL_PAGE_SIZE / sizeof(unsigned int))]	= g_pstNAND->pstDebug->anVerificationData[nPCh][nPWay][nBlock][nPage][i];
	#endif
			}
#endif
			break;
		}

	} while (bSync == TRUE);

	return nResult;
}

NAND_RESULT NAND_IssueProgram(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bSync)
{
	NAND_DEBUG_PRINTF("[NAND] Program - Ch/Way/Block/Page:%d/%d/%d/%d\r\n", nPCh, nPWay, nBlock, nPage);

	DEBUG_ASSERT(g_pstNAND->stWayStatus[nPCh][nPWay].nProgram == NAND_PGM_STEP_NONE);
	return NAND_ProcessProgram(nPCh, nPWay, nBlock, nPage, pMainBuf, pSpareBuf, bSync);
}

/*
@param	pMainBuf: 16KB main data buffer
@param pSpareBuf: 256Byte spare buffer
*/
NAND_RESULT NAND_ProcessProgram(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bSync)
{
	DEBUG_ASSERT(_IsValidAddress(nPCh, nPWay, nBlock, nPage));

	unsigned int nRowAddr;

	NAND_PROGRAM_STEP	*pnCurStatus = &g_pstNAND->stWayStatus[nPCh][nPWay].nProgram;
	DEBUG_ASSERT(*pnCurStatus < NAND_PGM_STEP_COUNT);

	volatile unsigned int *pnStatusResult;

	static long nSleepTimeProgramUS = 110;			// mandatory, LSB PGM time
	static long nSleepTimeGetStatusUS = 1;			// mandatory
	static long nSleepTimeCheckStatusUS = 1;		// mandatory

	NAND_RESULT	nResult = NAND_RESULT_OP_RUNNING;

	do
	{
		switch (*pnCurStatus)
		{
		case NAND_PGM_STEP_NONE:
			*pnCurStatus = NAND_PGM_STEP_CHECK_PROGRAM_ISSUABLE;
			// fall down

		case NAND_PGM_STEP_CHECK_PROGRAM_ISSUABLE:
			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				NAND_DEBUG_PRINTF("Ch(%d)/Way(%d)/Block(%d) Busy - Program Issue waiting\r\n", nPCh, nPWay, nBlock);
				break;
			}

			*pnCurStatus = NAND_PGM_STEP_ISSUE_PROGRAM;
			// Fall through

		case NAND_PGM_STEP_ISSUE_PROGRAM:
			nRowAddr = _GetRowAddress(nPCh, nPWay, nBlock, nPage);

			// Issue NAND Command
			V2FProgramPageAsync(chCtlReg[nPCh], nPWay, nRowAddr, pMainBuf, pSpareBuf);
			*pnCurStatus = NAND_PGM_STEP_WAIT_PROGRAM_DONE;
			break;

		case NAND_PGM_STEP_WAIT_PROGRAM_DONE:
			if (bSync == TRUE)
			{
				OSAL_SleepUS(nSleepTimeProgramUS);		// mandatory sleep
			}

			if (_IsCMDIssuable(nPCh, nPWay) == FALSE)
			{
				NAND_DEBUG_PRINTF("Ch(%d)/Way(%d)/Block(%d) Busy - Program Done waiting\r\n", nPCh, nPWay, nBlock);
				break;
			}

			*pnCurStatus = NAND_PGM_STEP_ISSUE_GET_STATUS;
			// fall through

		case NAND_PGM_STEP_ISSUE_GET_STATUS:
			pnStatusResult = (volatile unsigned int*)&g_pstNAND->pstNAND_Status->anData[nPCh][nPWay];
			*pnStatusResult = 0;
			V2FStatusCheckAsync(chCtlReg[nPCh], nPWay, (unsigned int*)pnStatusResult);

			*pnCurStatus = NAND_PGM_STEP_WAIT_GET_STATUS_DONE;
			break;

		case NAND_PGM_STEP_WAIT_GET_STATUS_DONE:
			if (bSync == TRUE)
			{
				OSAL_SleepUS(nSleepTimeGetStatusUS);		// mandatory sleep
			}

			if (_IsWayIdle(nPCh, nPWay) == FALSE)
			{
				NAND_DEBUG_PRINTF("Ch(%d)/Way(%d)/Block(%d) Busy - Program Get Status waiting\r\n", nPCh, nPWay, nBlock);
				break;
			}

			*pnCurStatus = NAND_PGM_STEP_CHECK_STATUS;
			// fall through

		case NAND_PGM_STEP_CHECK_STATUS:
			pnStatusResult = (volatile unsigned int*)&g_pstNAND->pstNAND_Status->anData[nPCh][nPWay];
			if (_IsCmdDone(*pnStatusResult))
			{
				if (_IsCmdSuccess(*pnStatusResult) == FALSE)
				{
					// Fail
					PRINTF("NAND Program Fail - Ch/Way/Block/Page:%d/%d/%d/%d\r\n", nPCh, nPWay, nBlock, nPage);
				}
				else
				{
					// Success
				}

				*pnCurStatus = NAND_PGM_STEP_DONE;
			}
			else
			{
				//xil_printf("NAND Program Status - 0x%X\r\n", *pnStatusResult);
				if (_IsRequestComplete(*pnStatusResult) == FALSE)
				{
					*pnCurStatus = NAND_PGM_STEP_WAIT_PROGRAM_DONE;
				}

				if (bSync == TRUE)
				{
					OSAL_SleepUS(nSleepTimeCheckStatusUS);	// mandatory
				}
			}
			break;

		default:
			assert(0);
		}

		if (*pnCurStatus == NAND_PGM_STEP_DONE)
		{
			// Program Done
			*pnCurStatus = NAND_PGM_STEP_NONE;
			nResult = NAND_RESULT_DONE;

#if defined(SUPPORT_DATA_VERIFICATION) || defined(WIN32)
			for (int i = 0; i < LPN_PER_PHYSICAL_PAGE; i++)
			{
				DEBUG_ASSERT(g_pstNAND->pstDebug->anVerificationData[nPCh][nPWay][nBlock][nPage][i] == DATA_VERIFICATION_INVALID_LPN);		// INVALID_LPN, Check overwrite or not erased
				g_pstNAND->pstDebug->anVerificationData[nPCh][nPWay][nBlock][nPage][i] = ((unsigned int*)pSpareBuf)[i];
			}
			
#endif

			break;
		}
	} while (bSync == TRUE);

	return nResult;
}

void NAND_InitChCtlReg(void)
{
	if (USER_CHANNELS < 1)
		assert(!"[WARNING] Configuration Error: Channel [WARNING]");

	chCtlReg[0] = (V2FMCRegisters*)NSC_0_BASEADDR;

	if (USER_CHANNELS > 1)
		chCtlReg[1] = (V2FMCRegisters*)NSC_1_BASEADDR;

	if (USER_CHANNELS > 2)
		chCtlReg[2] = (V2FMCRegisters*)NSC_2_BASEADDR;

	if (USER_CHANNELS > 3)
		chCtlReg[3] = (V2FMCRegisters*)NSC_3_BASEADDR;

	if (USER_CHANNELS > 4)
		chCtlReg[4] = (V2FMCRegisters*)NSC_4_BASEADDR;

	if (USER_CHANNELS > 5)
		chCtlReg[5] = (V2FMCRegisters*)NSC_5_BASEADDR;

	if (USER_CHANNELS > 6)
		chCtlReg[6] = (V2FMCRegisters*)NSC_6_BASEADDR;

	if (USER_CHANNELS > 7)
		chCtlReg[7] = (V2FMCRegisters*)NSC_7_BASEADDR;

#ifdef WIN32
	for (int i = 0; i < USER_CHANNELS; i++)
	{
		chCtlReg[i] = &g_stV2FMC[i];
		chCtlReg[i]->readyBusy = 0xFFFFFFFF;		// controller always ready for simulator
	}
#endif
}

void NAND_InitNandArray(void)
{
	for (int nCh = 0; nCh < USER_CHANNELS; nCh++)
	{
		for (int nWay = 0; nWay < USER_WAYS; nWay++)
		{
			V2FResetSync(chCtlReg[nCh], nWay);
			OSAL_SleepUS(1000);
#ifndef WIN32
			V2FEnterToggleMode(chCtlReg[nCh], nWay, (unsigned int)FW_DATA_BUFFER_START_ADDR);
#endif
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//	static functions
//

static int _IsValidAddress(int nCh, int nWay, int nBlock, int nPage)
{
	if ((nCh >= USER_CHANNELS) ||
		(nWay >= USER_WAYS) ||
		(nBlock >= TOTAL_BLOCKS_PER_DIE) ||
		(nPage >= PAGES_PER_MLC_BLOCK))
	{
		return FALSE;
	}

	return TRUE;
}

static int _GetTransferResult(unsigned int *panErrorFlag)
{
	assert(TRUE == ERROR_INFO_PASS);
	assert(FALSE == ERROR_INFO_FAIL);

	unsigned int nErrorInfo0 = panErrorFlag[0];
	unsigned int nErrorInfo1 = panErrorFlag[1];

	if (V2FCrcValid(nErrorInfo0))
	{
		if (V2FSpareChunkValid(nErrorInfo0))
		{
			if (V2FPageChunkValid(nErrorInfo1))
			{
				if (V2FWorstChunkErrorCount(nErrorInfo0) > BIT_ERROR_THRESHOLD_PER_CHUNK)
				{
					return ERROR_INFO_WARNING;
				}

				return ERROR_INFO_PASS;
			}
		}
	}

	return ERROR_INFO_FAIL;
}

static unsigned int _GetLUNBaseAddr(int nLUN)
{
	if (nLUN == 0)
	{
		return LUN_0_BASE_ADDR;
	}
	else
	{
		assert(nLUN == 1);
	}

	return LUN_1_BASE_ADDR;
}

static int _IsWayIdle(int nPCh, int nPWay)
{
	// 2. Erase Done Check
	unsigned int nReadyBusy;
	nReadyBusy = V2FReadyBusyAsync(chCtlReg[nPCh]);
	if (V2FWayReady(nReadyBusy, nPWay))
	{
		return TRUE;
	}

	return FALSE;
}

static int _IsCMDIssuable(int nPCh, int nPWay)
{
	if (V2FIsControllerBusy(chCtlReg[nPCh]))
	{
		return FALSE;
	}

	if (_IsWayIdle(nPCh, nPWay) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

static unsigned int _GetRowAddress(int nPCh, int nPWay, int nBlock, int nPage)
{
	unsigned int nDieNo;
	unsigned int nLUN;				// Logical Unit Number,		Die in a chip
	unsigned int nLocalBlockNo;		//	BlockNo in a Logical Unit
	unsigned int nRowAddr;

	nDieNo = PhyChWway2VDie(nPCh, nPWay);
	nLUN = nBlock / TOTAL_BLOCKS_PER_LUN;
	nLocalBlockNo = (nLUN * TOTAL_BLOCKS_PER_LUN) + (nBlock % TOTAL_BLOCKS_PER_LUN);

	nRowAddr = _GetLUNBaseAddr(nLUN) + nLocalBlockNo * PAGES_PER_MLC_BLOCK + nPage;

	return nRowAddr;
}

static int _IsRequestComplete(unsigned int nResult)
{
	const unsigned int REQUEST_FAIL_MASK = 0xC0;
	if (nResult & REQUEST_FAIL_MASK)
	{
		return TRUE;
	}

	return FALSE;
}

static int _IsRequestFail(unsigned int nResult)
{
	const unsigned int REQUEST_FAIL_MASK = 0x6;
	if ((nResult & REQUEST_FAIL_MASK) == REQUEST_FAIL_MASK)
	{
		return TRUE;
	}

	return FALSE;
}

/*
@return TRUE: Done
@return FALSE: wait more
*/
static int _IsCmdDone(volatile unsigned int nStatusResult)
{
	volatile unsigned int nStatus;

	if (V2FRequestReportDone(nStatusResult))
	{
		nStatus = V2FEliminateReportDoneFlag(nStatusResult);
		if (V2FRequestComplete(nStatus))
		{
			return TRUE;
		}
	}

	return FALSE;
}

/*
@return	TRUE: Success
@return FALSE: Fail
*/
static int _IsCmdSuccess(volatile unsigned int nStatusResult)
{
	unsigned int nStatus;
	nStatus = V2FEliminateReportDoneFlag(nStatusResult);
	assert(V2FRequestComplete(nStatus));
	if (V2FRequestFail(nStatus))
	{
		return FALSE;
	}

	return TRUE;
}


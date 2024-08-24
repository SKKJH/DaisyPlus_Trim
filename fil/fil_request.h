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

#ifndef __FIL_REQUEST_H__
#define __FIL_REQUEST_H__

#include "cosmos_types.h"
#include "fil_config.h"
#include "fil.h"

#include "list.h"

typedef enum
{
	FTL_REQUEST_READ,
	FTL_REQUEST_PROGRAM,
	FTL_REQUEST_ERASE
} FTL_REQUEST_TYPE;

typedef enum
{
	FTL_REQUEST_FREE,

	// Read status
	FTL_REQUEST_READ_WAIT,			// At the wait list
	FTL_REQUEST_READ_ISSUED,
	FTL_REQUEST_READ_DONE,
	FTL_REQUEST_DMA_ISSUE,
	FTL_REQUEST_DMA_DONE,

	// Program status
	FTL_REQUEST_ERASE_FOR_PROGRAM_WAIT,
	FTL_REQUEST_ERASE_FOR_PROGRAM_ISSUED,		// FOR PAGE NUMBER 0, auto erase
	FTL_REQUEST_ERASE_FOR_PROGRAM_DONE,
	FTL_REQUEST_PROGRAM_WAIT,
	FTL_REQUEST_PROGRAM_ISSUED,
	FTL_REQUEST_PROGRAM_DONE,

	// Erase status
	FTL_REQUEST_ERASE_WAIT,
	FTL_REQUEST_ERASE_ISSUED,
	FTL_REQUEST_ERASE_DONE,
} FTL_REQUEST_STATUS;

typedef struct
{
	void*		pMainBuf;				// size must be BYTES_PER_DATA_REGION_OF_NAND_ROW
	void*		pSpareBuf;			// size must be one of belows
										//	BYTES_PER_SPARE_REGION_OF_NAND_ROW(1664)	: raw NAND read for bad block info
										//	 BYTES_PER_SPARE_REGION_OF_SLICE(256)		: FTL data read and last 8 byte is reserved for CRC
										// (BYTES_PER_SPARE_REGION_OF_NAND_ROW - BYTES_PER_SPARE_REGION_OF_PAGE) bytes are used by ECC engine (Parity data)
} NAND_IO_BUF;

typedef struct
{
	FTL_REQUEST_TYPE	nType;
	FTL_REQUEST_STATUS	nStatus;

	FTL_REQUEST_ID		stRequestID;			// Request ID from HIl
	NAND_ADDR			stAddr;
	NAND_IO_BUF			stBuf;

	struct list_head	dlList;
} FTL_REQUEST;

typedef struct
{
	FTL_REQUEST			astRequest[FTL_REQUEST_COUNT];
} FTL_REQUEST_POOL;

void			FIL_InitRequest(void);
FTL_REQUEST*	FIL_AllocateRequest(void);
void			FIL_AddToRequestWaitQ(FTL_REQUEST* pstRequest, int bHead);

void			FIL_ProcessWaitQ(void);
void			FIL_ProcessIssuedQ(void);
void			FIL_ProcessDoneQ(void);
int				FIL_IsIdle(void);

#endif		// end of #ifndef __FIL_REQUEST_H__


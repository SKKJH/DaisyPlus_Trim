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

#ifndef __FIL_H__
#define __FIL_H__

#include "cosmos_plus_global_config.h"
#include "cosmos_plus_system.h"
#include "cosmos_types.h"

#define PhyChWway2VDie(chNo, wayNo)			((chNo) + (wayNo) * (USER_CHANNELS))

#if (USER_CHANNELS == 8)
	#define CHANNEL_BITS					(3)
#elif (USER_CHANNELS == 4)
	#define CHANNEL_BITS					(2)
#elif (USER_CHANNELS == 2)
	#define CHANNEL_BITS					(1)
#elif (USER_CHANNELS == 1)
	#define CHANNEL_BITS					(0)
#else
	#error	"Check config!!"
#endif

#if (USER_WAYS == 8)
	#define NUM_BIT_WAY					(3)
#elif (USER_WAYS == 4)
	#define NUM_BIT_WAY					(2)
#elif (USER_WAYS == 2)
	#define NUM_BIT_WAY					(1)
#elif (USER_WAYS == 1)
	#define NUM_BIT_WAY					(0)
#else
	#error	"Check config!!"
#endif
#define NUM_BIT_BLOCK				(14)		// Main 12bit + SpareBlock 1bit + LUN 1bit
#define NUM_BIT_LPN_PER_PAGE		(2)			// 16KB / 4KB
#define NUM_BIT_PPAGE				PAGES_PER_BLOCK_BITS
#define NUM_BIT_VPAGE				(CHANNEL_BITS + NUM_BIT_WAY + NUM_BIT_LPN_PER_PAGE + NUM_BIT_PPAGE)

// Refer to ftl_config.h to get layout
#define NAND_ADDR_CHANNEL_SHIFT		(NUM_BIT_LPN_PER_PAGE)
#define NAND_ADDR_WAY_SHIFT			(NUM_BIT_LPN_PER_PAGE + CHANNEL_BITS)
#define NAND_ADDR_PPAGE_SHIFT		(NUM_BIT_LPN_PER_PAGE + CHANNEL_BITS + NUM_BIT_WAY)
#define NAND_ADDR_BLOCK_SHIFT		(NUM_BIT_VPAGE)
#define NAND_ADDR_VBN_SHIFT			(NAND_ADDR_BLOCK_SHIFT)
#define NAND_ADDR_VPAGE_SHIFT		(0)
#define NAND_ADDR_LPN_OFFSET_SHIFT	(0)

#define NAND_ADDR_CHANNEL_MASK		((1 << CHANNEL_BITS) - 1)
#define NAND_ADDR_WAY_MASK			((1 << NUM_BIT_WAY) - 1)
#define NAND_ADDR_BLOCK_MASK		((1 << NUM_BIT_BLOCK) - 1)
#define NAND_ADDR_PPAGE_MASK		((1 << NUM_BIT_PPAGE) - 1)
#define NAND_ADDR_LPN_PER_PAGE_MASK	((1 << NUM_BIT_LPN_PER_PAGE) - 1)
#define NAND_ADDR_VPAGE_MASK		((1 << NUM_BIT_VPAGE) - 1)

#ifndef GREEDY_FTL
	#define ERROR_INFO_FAIL		0
	#define ERROR_INFO_PASS		1
	#define ERROR_INFO_WARNING	2
#endif

typedef enum
{
	NAND_RESULT_SUCCESS,
	//NAND_RESULT_CMD_FAIL,		// COMMAND FAIL
	NAND_RESULT_UECC,			// Uncorrectable ECC
	NAND_RESULT_OP_RUNNING,	// operation running status
	NAND_RESULT_DONE,		// operation done
} NAND_RESULT;

/*
	Address for NAND I/o
*/
typedef struct
{
	unsigned int nCh		: CHANNEL_BITS;					// LSB position, bit 0
	unsigned int nWay		: NUM_BIT_WAY;
	unsigned int nPPage		: ROTS_PER_MLC_BLOCK_BITS;		// physical page number
	unsigned int nBlock		: NUM_BIT_BLOCK;
} NAND_ADDR;

#ifdef __cplusplus
extern "C" {
#endif

	void FIL_Initialize(void);
	void FIL_Run(void);

	BOOL FIL_IsBad(INT32 nPBN);

	void FIL_ReadPage(FTL_REQUEST_ID stReqID, NAND_ADDR stPhyAddr, void* pMainBuf, void* pSpareBuf);
	void FIL_ProgramPage(FTL_REQUEST_ID	stReqID, NAND_ADDR stPhyAddr, void* pMainBuf, void* pSpareBuf);
	void FIL_EraseBlock(FTL_REQUEST_ID	stReqID, NAND_ADDR stPhyAddr);
	int FIL_GetPagesPerBlock(void);

#ifdef __cplusplus
}
#endif

#endif	// end of #ifndef __FIL_H__

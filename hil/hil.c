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

#include "xil_types.h"

#include "debug.h"

#include "host_lld.h"
#include "hil.h"

#include "list.h"

#include "cosmos_types.h"
#include "cosmos_plus_system.h"

#include "osal.h"

#include "hil.h"

#include "ftl.h"

#ifndef GREEDY_FTL
	#include "fil.h"
#endif

unsigned int storageCapacity_L;

void HIL_Initialize(void)
{
	DEBUG_ASSERT(LOGICAL_PAGE_SIZE == HOST_BLOCK_SIZE);	// if this is not same, HIL must convert block number to LPN

#ifndef RAM_FTL
	FIL_Initialize();
#endif

	FTL_Initialize();
}
void HIL_Format(void)
{
	BOOL	bRet;

	do
	{
		bRet = FTL_Format();
		if (bRet == FALSE)
		{
//			xil_printf("Format done\r\n");
			FTL_Run();
		}
	} while (bRet == FALSE);
}

static int bInitProfileCount = 0;
static int bPrintProfileCount = 0;

void HIL_Run(void)
{
	FTL_Run();			// FTL side run
	SYSTEM_Run();

	if (bInitProfileCount == 1)
	{
		FTL_IOCtl(IOCTL_INIT_PROFILE_COUNT);
		bInitProfileCount = 0;
	}

	if (bPrintProfileCount == 1)
	{
		FTL_IOCtl(IOCTL_PRINT_PROFILE_COUNT);
		bPrintProfileCount = 0;
	}
}

void HIL_SetStorageBlocks(unsigned int nStorageBlocks)
{
#if (STATIC_USER_DENSITY_MB > 0)
	nStorageBlocks = STATIC_USER_DENSITY_MB * KB / (HOST_BLOCK_SIZE / KB);
#endif
	storageCapacity_L = nStorageBlocks;

	ASSERT(storageCapacity_L > 0);
}

unsigned int HIL_GetStorageBlocks(void)
{
	return storageCapacity_L;
}

void HIL_ReadBlock(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount)
{
	FTL_ReadPage(nCmdSlotTag, nStartLBA, nLBACount);
}

void HIL_WriteBlock(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount)
{
	FTL_WritePage(nCmdSlotTag, nStartLBA, nLBACount);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	STATIC FUNCTIONS
//


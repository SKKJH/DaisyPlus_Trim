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

/*
	Terminology

	VBN:	Virtual Block Number
	VPage:	Pages in a VBN
	VPN:	Virtual Page Number - Device Global Logical Page number (4K Unit)
	VPPN:	FTL Mapping information, VBLOCK & VPageNo
	interleaving policy
	1. Channel
	2. Way
	Layout (same as NAND_ADDR)
	31                                     0   (bitoffset)
	--------------------------------------
	|  Block | Page | Way | Ch | MapUnit |
	--------------------------------------
*/

#ifndef __FTL_H__
#define __FTL_H__

/*
	This file is reference by all kind of FTLs
	FTL Common configruation will be configured at this file
*/

#include "cosmos_types.h"

#define OVERPROVISION_RATIO_DEFAULT			(0.1)			// percent of over-provision ratio, default 10%
#define SUPPORT_STATIC_DENSITY				(32)			// user density, GB
#define SUPPORT_META_BLOCK					(0)				// consume meta data block on initializing

#define SUPPORT_AUTO_ERASE					(1)				// Erase block on 1st page is programming, no explicit erase operation

#define FREE_BLOCK_GC_THRESHOLD_DEFAULT		20
//#define FREE_BLOCK_GC_THRESHOLD_DEFAULT		1024

#define SUPPORT_FTL_DEBUG			(0)

#if (SUPPORT_FTL_DEBUG == 0)
	#define FTL_DEBUG_PRINTF(...)	((void)0)
#else
	#define FTL_DEBUG_PRINTF	PRINTF
#endif

#define InitFTL()			FTL_Initialize()

#ifdef __cplusplus
extern "C" {
#endif

	// ftl interface
	VOID FTL_Initialize(VOID);
	BOOL FTL_Format(VOID);

	VOID FTL_Run(VOID);

	VOID FTL_ReadPage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount);
	VOID FTL_WritePage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount);
	VOID FTL_CallBack(FTL_REQUEST_ID stReqID);

	VOID FTL_IOCtl(IOCTL_TYPE eType);

	#define SyncAllLowLevelReqDone()				// do nothing, just for GREEDY_FTL_3_0_0

#ifdef __cplusplus
}
#endif

#endif

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

#ifdef __cplusplus
extern "C" {
#endif

	// ftl interface
	VOID FTL_Initialize(VOID);
	VOID FTL_Run(VOID);
	BOOL FTL_Format(VOID);

	VOID FTL_ReadPage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount);
	VOID FTL_WritePage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount);
	VOID FTL_CallBack(FTL_REQUEST_ID stReqID);
	VOID FTL_IOCtl(IOCTL_TYPE eType);

#define SyncAllLowLevelReqDone()				// do nothing, just for GREEDY_FTL_3_0_0

#ifdef __cplusplus
}
#endif

DFTL_GLOBAL	*g_pstDFTL;

VOID FTL_Initialize(VOID)
{
	void* p = OSAL_MemAlloc(MEM_TYPE_FW_DATA, sizeof(DFTL_GLOBAL), OSAL_MEMALLOC_ALIGNMENT);
	g_pstDFTL = new(p) DFTL_GLOBAL();

	g_pstDFTL->Initialize();

	return;
}

BOOL FTL_Format(VOID)
{
	return g_pstDFTL->Format();
}

VOID FTL_ReadPage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount)
{
	g_pstDFTL->ReadPage(nCmdSlotTag, nLPN, nCount);
}

VOID FTL_WritePage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount)
{
	g_pstDFTL->WritePage(nCmdSlotTag, nLPN, nCount);
}

VOID FTL_Run(VOID)
{
	g_pstDFTL->Run();
}

VOID FTL_CallBack(FTL_REQUEST_ID stReqID)
{
	g_pstDFTL->CallBack(stReqID);
}

VOID FTL_IOCtl(IOCTL_TYPE eType)
{
	g_pstDFTL->IOCtl(eType);
}


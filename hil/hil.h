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

#ifndef __HIL_H__
#define __HIL_H__

#include "list.h"
#include "nvme.h"
#include "cosmos_types.h"

// TODO, HIL BUFFER HIT 판단 필요

#ifdef __cplusplus
extern "C" {
#endif

	extern unsigned int storageCapacity_L;

	void HIL_Initialize(void);
	void HIL_Format(void);

	void HIL_ReadBlock(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount);
	void HIL_WriteBlock(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount);
	void HIL_Run(void);

	void HIL_SetStorageBlocks(unsigned int nStorageBlocks);
	unsigned int HIL_GetStorageBlocks(void);

#ifdef __cplusplus
}
#endif

#endif	// end of #ifndef __HIL_H__
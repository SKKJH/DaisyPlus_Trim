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

#ifndef __FIL_NAND_H__
#define __FIL_NAND_H__

void NAND_Initialize(void);
void NAND_InitChCtlReg(void);
void NAND_InitNandArray(void);

NAND_RESULT NAND_IssueRead(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bECCOn, int bSync);
NAND_RESULT NAND_ProcessRead(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bECCOn, int bSync);
NAND_RESULT NAND_IssueProgram(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bSync);
NAND_RESULT NAND_ProcessProgram(int nPCh, int nPWay, int nBlock, int nPage, void* pMainBuf, void* pSpareBuf, int bSync);
NAND_RESULT NAND_IssueErase(int nPCh, int nPWay, int nBlock, int bSync);
NAND_RESULT NAND_ProcessErase(int nPCh, int nPWay, int nBlock, int bSync);

#endif	// end of #ifndef __FIL_H__

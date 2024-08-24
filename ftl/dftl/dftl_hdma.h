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

#ifndef __DFTL_HDMA_H__
#define __DFTL_HDMA_H__

class HDMA
{
public:
	VOID IssueRxDMA(UINT32 nCmdSlotTag, UINT32 nCmd4KOffset, UINT32 nBufAddr);
	VOID IssueTxDMA(UINT32 nCmdSlotTag, UINT32 nCmd4KOffset, UINT32 nBufAddr);

	VOID WaitRxDMADone(UINT32 nDMaReqTail, UINT32 nOverflowCount);

	BOOL CheckTxDMADone(UINT32 nDMaReqTail, UINT32 nOverflowCount);

	UINT8 GetRxDMAIndex(VOID);
	UINT8 GetTxDMAIndex(VOID);

	UINT32 GetRxDMAOverFlowCount(VOID);
	UINT32 GetTxDMAOverFlowCount(VOID);

private:
};

#endif

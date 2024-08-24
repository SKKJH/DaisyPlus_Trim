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

UINT32
VNAND::GetPBlocksPerVBlock(VOID)
{
	return (USER_CHANNELS * USER_WAYS);
}

UINT32 
VNAND::GetPPagesPerVBlock(VOID)
{
	return (GetPBlocksPerVBlock() * FIL_GetPagesPerBlock());
}

UINT32 
VNAND::GetVPagesPerVBlock(VOID)
{
	return GetPPagesPerVBlock() * LPN_PER_PHYSICAL_PAGE;
}

UINT32
VNAND::GetVBlockCount(void)
{
	return (TOTAL_BLOCKS_PER_DIE);
}

VOID 
VNAND::Initialize(VOID)
{
	// create the PB map
	UINT32	nVBlockCount = GetVBlockCount();

	INT32	nSize = sizeof(VIRTUAL_BLOCK) * nVBlockCount;
	m_pstVB = (VIRTUAL_BLOCK *)OSAL_MemAlloc(MEM_TYPE_FW_DATA, nSize, OSAL_MEMALLOC_FW_ALIGNMENT);
	OSAL_MEMSET(m_pstVB, 0x00, nSize);

	INT32	nValidBitmapBytePerBlock;
	INT32	nLPagePerVBlock = DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock();
	nValidBitmapBytePerBlock = CEIL(nLPagePerVBlock, NBBY);
	nSize = nValidBitmapBytePerBlock * nVBlockCount;			// Byte count

	UINT8* pstPBValidBitmap = (UINT8*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, nSize, OSAL_MEMALLOC_FW_ALIGNMENT);
	OSAL_MEMSET(pstPBValidBitmap, 0x00, nSize);			// clear all bitmap

	nSize = sizeof(INT32) * nLPagePerVBlock * nVBlockCount;
	UINT32* pstP2L = (UINT32*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, nSize, OSAL_MEMALLOC_FW_ALIGNMENT);
	ASSERT(pstP2L);

	for (UINT32 i = 0; i < nVBlockCount; i++)
	{
		m_pstVB[i].m_pnV2L		= &pstP2L[i * nLPagePerVBlock];
		m_pstVB[i].m_pbValid	= &pstPBValidBitmap[i * nValidBitmapBytePerBlock];

		for (int j = 0; j < nLPagePerVBlock; j++)
		{
			m_pstVB[i].m_pnV2L[j] = INVALID_LPN;
		}
	}
}

UINT32
VNAND::GetV2L(UINT32 nVPPN)
{
	UINT32 nVBN = VBN_FROM_VPPN(nVPPN);
	UINT32 nVPAGE = VPN_FROM_VPPN(nVPPN);

	DEBUG_ASSERT(nVBN < GetVBlockCount());

	return m_pstVB[nVBN].m_pnV2L[nVPAGE];
}

/*
	@brief Invalidate an Logical Page of VBLock
			Invalidated page will be be moved while GC operation
*/
VOID
VNAND::Invalidate(UINT32 nVPPN)
{
	INT32	nVBN = VBN_FROM_VPPN(nVPPN);
	INT32	nVPageOffset = VPN_FROM_VPPN(nVPPN);

	DEBUG_ASSERT(ISSET(m_pstVB[nVBN].m_pbValid, nVPageOffset) == TRUE);

	CLEARBIT(m_pstVB[nVBN].m_pbValid, nVPageOffset);
}

BOOL
VNAND::IsValid(UINT32 nVBN, UINT32 nVPageNo)
{
	DEBUG_ASSERT(nVBN < GetVBlockCount());
	DEBUG_ASSERT(nVPageNo < DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock());

	return ISSET(m_pstVB[nVBN].m_pbValid, nVPageNo);
}

INT32
VNAND::ReadPage(FTL_REQUEST_ID stReqID, UINT32 nVPPN, void * pMainBuf, void * pSpareBuf)
{
	NAND_ADDR	stNandAddr = _GetNandAddrFromVPPN(nVPPN);
	if (stReqID.stCommon.nType != FTL_REQUEST_ID_TYPE_DEBUG)
	{
		FIL_ReadPage(stReqID, stNandAddr, pMainBuf, pSpareBuf);

		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_NAND_READ);
	}

	return GetV2L(nVPPN);
}

VOID
VNAND::ReadPageSimul(UINT32 nVPPN, void * pMainBuf)
{
#if defined(WIN32) && (SUPPORT_META_DEMAND_LOADING == 1)
	UINT32	nVBN = VBN_FROM_VPPN(nVPPN);

	// check is this meta block
	VBINFO_MGR*	pstVBInfoMgr = DFTL_GLOBAL::GetVBInfoMgr();
	VBINFO*	pstVBInfo = pstVBInfoMgr->GetVBInfo(nVBN);

	if (pstVBInfo->IsMeta() == TRUE)
	{
		UINT32	nVPN = VPN_FROM_VPPN(nVPPN);
		nVPN = nVPN & (~NAND_ADDR_LPN_PER_PAGE_MASK);	// remove LPN offset

		VOID*	pSrc;
		pSrc = (VOID*)((UINT32)m_pstVB[nVBN].m_pData + (META_VPAGE_SIZE * nVPN));

		OSAL_MEMCPY(pMainBuf, pSrc, PHYSICAL_PAGE_SIZE);
	}
#endif
}

VOID
VNAND::ProgramPage(FTL_REQUEST_ID stReqID, PROGRAM_UNIT *pstProgram)
{
	NAND_ADDR	stNandAddr = _GetNandAddrFromVPPN(pstProgram->GetVPPN());

	FIL_ProgramPage(stReqID, stNandAddr, pstProgram->GetBufferEntry()->m_pMainBuf, pstProgram->GetBufferEntry()->m_pSpareBuf);

	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_NAND_WRITE);

#if defined(WIN32) && (SUPPORT_META_DEMAND_LOADING == 1)

	UINT32	nVBN = VBN_FROM_VPPN(pstProgram->GetVPPN());

	// check is this meta block
	VBINFO_MGR*	pstVBInfoMgr = DFTL_GLOBAL::GetVBInfoMgr();
	VBINFO*	pstVBInfo = pstVBInfoMgr->GetVBInfo(nVBN);

	if (pstVBInfo->IsMeta() == TRUE)
	{
		UINT32	nVPN = VPN_FROM_VPPN(pstProgram->GetVPPN());
		if ((nVPN == 0) && (m_pstVB[nVBN].m_pData == NULL))
		{
			m_pstVB[nVBN].m_pData = OSAL_MemAlloc(MEM_TYPE_WIN32_DEBUG, pstVBInfoMgr->GetVBSize(), MEM_TYPE_FW_DATA);
		}

		DEBUG_ASSERT(LPN_OFFSET_FROM_VPPN(pstProgram->GetVPPN()) == 0);

		VOID*	pDest;
		pDest = (VOID*)((UINT32)m_pstVB[nVBN].m_pData + (META_VPAGE_SIZE * nVPN));

		OSAL_MEMCPY(pDest, pstProgram->GetBufferEntry()->m_pMainBuf, PHYSICAL_PAGE_SIZE);
	}
#endif
}

VOID
VNAND::ProgramPageSimul(UINT32 nLPN, UINT32 nVPPN)
{
	UINT32 nVPageNo		= VPN_FROM_VPPN(nVPPN);
	UINT32 nVBN			= VBN_FROM_VPPN(nVPPN);

	DEBUG_ASSERT(nLPN < DFTL_GLOBAL::GetInstance()->GetLPNCount());
	DEBUG_ASSERT(nVBN < GetVBlockCount());
	DEBUG_ASSERT(nVPageNo < DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock());

#if (SUPPORT_AUTO_ERASE == 1)
	if (nVPageNo == 0)
	{
		_EraseSimul(nVBN);
	}
#endif

	// this page must be free page
	DEBUG_ASSERT(m_pstVB[nVBN].m_pnV2L[nVPageNo] == INVALID_LPN);
	DEBUG_ASSERT(ISSET(m_pstVB[nVBN].m_pbValid, nVPageNo) == FALSE);

	SETBIT(m_pstVB[nVBN].m_pbValid, nVPageNo);
	m_pstVB[nVBN].m_pnV2L[nVPageNo] = nLPN;
}

UINT32
VNAND::GetValidVPNCount(UINT32 nVBN)
{
	UINT32 nVPagePerVBN = DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock();

	UINT32	nVPC = 0;

	for (UINT32 i = 0; i < nVPagePerVBN; i++)
	{
		if (IsValid(nVBN, i) == TRUE)
		{
			nVPC++;
		}
	}

	return nVPC;
}

/*
@berief	update erase information
*/
VOID
VNAND::_EraseSimul(INT32 nVBN)
{
	INT32	nLPagePerVBlock = DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock();

	/* Init PB structure*/
	for (int i = 0; i < nLPagePerVBlock; i++)
	{
		m_pstVB[nVBN].m_pnV2L[i] = INVALID_LPN;
		DEBUG_ASSERT(ISCLEAR(m_pstVB[nVBN].m_pbValid, i) == TRUE);
	}

	INT32	nValidBitmapBytePerBlock;
	nValidBitmapBytePerBlock = CEIL(nLPagePerVBlock, NBBY);
	OSAL_MEMSET(m_pstVB[nVBN].m_pbValid, 0x00, nValidBitmapBytePerBlock);

	m_pstVB[nVBN].m_nEC++;		// Increase EC

	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_NAND_ERASE);
}

NAND_ADDR
VNAND::_GetNandAddrFromVPPN(UINT32 nVPPN)
{
	NAND_ADDR	stNandAddr;

	stNandAddr.nCh		= CHANNEL_FROM_VPPN(nVPPN);
	stNandAddr.nWay		= WAY_FROM_VPPN(nVPPN);
	stNandAddr.nBlock	= PBN_FROM_VPPN(nVPPN);
	stNandAddr.nPPage	= PPAGE_FROM_VPPN(nVPPN);

#if (BITS_PER_CELL == 1)
#define Vpage2PlsbPageTranslation(pageNo) ((pageNo) > (0) ? (2 * (pageNo) - 1): (0))
	INT32	nPPage = stNandAddr.nPPage;

	stNandAddr.nPPage = Vpage2PlsbPageTranslation(nPPage);		// covert to LSB page
#endif

	return stNandAddr;
}


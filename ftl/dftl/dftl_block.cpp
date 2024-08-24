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

///////////////////////////////////////////////////////////////////////////////
//
//	VBlock Information 
//
///////////////////////////////////////////////////////////////////////////////
VOID
VBINFO::IncreaseInvalidate(VOID)
{
	m_nInvalidLPN++;
	DEBUG_ASSERT(m_nInvalidLPN <= DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock());
}

BOOL
VBINFO::IsFullInvalid(VOID)
{
	return (m_nInvalidLPN < DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock()) ? FALSE : TRUE;
}

VOID
VBINFO::SetFullInvalid(VOID)
{
	m_nInvalidLPN = DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock();
}

///////////////////////////////////////////////////////////////////////////////
//
//	VBlock Information Manager
//
///////////////////////////////////////////////////////////////////////////////
VOID
VBINFO_MGR::Initialize(VOID)
{
	INT32 nSize = sizeof(VBINFO) * DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount();
	m_pastVBInfo = (VBINFO*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, nSize, OSAL_MEMALLOC_FW_ALIGNMENT);

	ASSERT(GetVBSize() == (DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock() * LOGICAL_PAGE_SIZE));
}

VOID
VBINFO_MGR::Format(VOID)
{
	UINT32 nVBlockCount = DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount();
	INT32 nSize = sizeof(VBINFO) * nVBlockCount;
	OSAL_MEMSET(m_pastVBInfo, 0x00, nSize);

	VBINFO*		pstVBInfo;
	BOOL		bBad;
	for (UINT32 i = 0; i < nVBlockCount; i++)
	{
		bBad = DFTL_GLOBAL::GetVNandMgr()->IsBadBlock(i);
		if (bBad == FALSE)
		{
			pstVBInfo = GetVBInfo(i);
			INIT_LIST_HEAD(&pstVBInfo->m_dlList);
			pstVBInfo->m_nVBN = i;

			pstVBInfo->SetFullInvalid();	// to avoid debug assert
		}
	}
}

VBINFO*
VBINFO_MGR::GetVBInfo(UINT32 nVBN)
{
	DEBUG_ASSERT(nVBN < DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount());
	return &m_pastVBInfo[nVBN];
}

/*
	@brief return Size of VirtualBlock (byte)
*/
UINT32
VBINFO_MGR::GetVBSize(VOID)
{
	return ((1 << NAND_ADDR_VBN_SHIFT) * LOGICAL_PAGE_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
//
//	Block Manager
//
///////////////////////////////////////////////////////////////////////////////
VOID
BLOCK_MGR::Initialize(BLOCK_MGR_TYPE eType)
{
	// Nothing to do
	INIT_LIST_HEAD(&m_dlFreeBlocks);
	m_nFreeBlocks = 0;

	INIT_LIST_HEAD(&m_dlUsedBlocks);
	m_nUsedBlocks = 0;

	m_eType = eType;

	m_bFormatted = FALSE;
}

VOID
BLOCK_MGR::_FormatUser(VOID)
{
	UINT32 nVBlockCount = DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount();

	VNAND*		pstVNandMgr = DFTL_GLOBAL::GetVNandMgr();
	VBINFO_MGR* pstVBInfoMgr = DFTL_GLOBAL::GetVBInfoMgr();

	VBINFO*		pstVBInfo;
	BOOL		bBad;
	for (UINT32 i = 0; i < nVBlockCount; i++)
	{
		pstVBInfo = pstVBInfoMgr->GetVBInfo(i);

		bBad = pstVNandMgr->IsBadBlock(i);
		if (bBad == FALSE)
		{
			INIT_LIST_HEAD(&pstVBInfo->m_dlList);
			pstVBInfo->m_nVBN = i;

			pstVBInfo->SetFullInvalid();	// to avoid debug assert

			m_nUsedBlocks++;				// to avoid underflow by release without allocation
			Release(i);
		}
		else
		{
			pstVBInfo->SetBad();
			pstVBInfo->ClearUser();
			pstVBInfo->ClearGC();
		}
	}
}

VOID
BLOCK_MGR::_FormatMeta(VOID)
{
	META_L2V_MGR*	pstMetaL2VMgr = DFTL_GLOBAL::GetMetaL2VMgr();
	UINT32			nMetaVPageCount = pstMetaL2VMgr->GetMetaLPNCount();
	UINT32			nVPagePerVBN	= DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock();
	UINT32			nMetaVBlockCount = CEIL((INT32)nMetaVPageCount, (INT32)nVPagePerVBN) * (1 + META_OP_RATIO);

	nMetaVBlockCount = MAX(nMetaVBlockCount, META_VBLOCK_COUNT_MIN); 

	VBINFO_MGR* pstVBInfoMgr = DFTL_GLOBAL::GetVBInfoMgr();

	// Get blocks from user block
	BLOCK_MGR*	pstUserBlockMgr = DFTL_GLOBAL::GetUserBlockMgr();
	UINT32		nVBN;
	VBINFO*		pstVBInfo;

	for (UINT32 i = 0; i < nMetaVBlockCount; i++)
	{
		nVBN = pstUserBlockMgr->Allocate(TRUE, FALSE, FALSE);

		DEBUG_ASSERT(DFTL_GLOBAL::GetVNandMgr()->IsBadBlock(nVBN) == FALSE);

		pstVBInfo = pstVBInfoMgr->GetVBInfo(nVBN);
		list_del_init(&pstVBInfo->m_dlList);

		pstVBInfo->SetFullInvalid();	// to avoid debug assert

		m_nUsedBlocks++;				// to avoid underflow by release without allocation
		Release(nVBN);
	}
}

VOID
BLOCK_MGR::Format(VOID)
{
	if (m_bFormatted == TRUE)
	{
		return;
	}

	if (m_eType == META_BLOCK_MGR)
	{
		_FormatMeta();
	}
	else
	{
		DEBUG_ASSERT(m_eType == USER_BLOCK_MGR);
		_FormatUser();
	}

	m_bFormatted = TRUE;
}

UINT32
BLOCK_MGR::Allocate(BOOL bUser, BOOL bGC, BOOL bMeta)
{
	DEBUG_ASSERT(m_nFreeBlocks > 0);
	DEBUG_ASSERT(m_nUsedBlocks < DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount());
	DEBUG_ASSERT((bMeta == TRUE) ? (m_eType == META_BLOCK_MGR) : TRUE);

	VBINFO* pstVBInfo = list_first_entry(&m_dlFreeBlocks, VBINFO, m_dlList);

	list_del_init(&pstVBInfo->m_dlList);
	list_add_tail(&pstVBInfo->m_dlList, &m_dlUsedBlocks);

	pstVBInfo->ClearFree();
	pstVBInfo->ClearUser();
	pstVBInfo->ClearGC();
	pstVBInfo->ClearMeta();

	if (bUser == TRUE)
	{
		pstVBInfo->SetUser();
	}
	else if (bGC == TRUE)
	{
		pstVBInfo->SetGC();
	}
	else if (bMeta == TRUE)
	{
		pstVBInfo->SetMeta();
	}
	else
	{
		ASSERT(0);
	}

	pstVBInfo->SetActive();
	pstVBInfo->SetInvalidPageCount(0);

	m_nFreeBlocks--;
	m_nUsedBlocks++;

	return pstVBInfo->m_nVBN;
}

VOID
BLOCK_MGR::Release(UINT32 nVBN)
{
	DEBUG_ASSERT(nVBN < DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount());
	DEBUG_ASSERT(m_nUsedBlocks > 0);
	DEBUG_ASSERT(m_nFreeBlocks < DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount());

	VBINFO*		pstVBInfo;

	pstVBInfo = DFTL_GLOBAL::GetVBInfoMgr()->GetVBInfo(nVBN);

	DEBUG_ASSERT(pstVBInfo->IsFullInvalid() == TRUE);

	pstVBInfo->SetFree();

	list_del_init(&pstVBInfo->m_dlList);
	m_nUsedBlocks--;

	// Add to free block list
	list_add_tail(&pstVBInfo->m_dlList, &m_dlFreeBlocks);
	m_nFreeBlocks++;
}

VOID
BLOCK_MGR::Invalidate(UINT32 nVPPN)
{
	UINT32 nVBN = VBN_FROM_VPPN(nVPPN);

	DEBUG_ASSERT(nVBN < DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount());

	VBINFO*		pstVBInfo;
	pstVBInfo = DFTL_GLOBAL::GetVBInfoMgr()->GetVBInfo(nVBN);

	pstVBInfo->IncreaseInvalidate();

	if (pstVBInfo->IsFullInvalid() == TRUE)
	{
		// Release block
		Release(nVBN);

		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_FULL_INVALID_BLOCK, DFTL_GLOBAL::GetVNandMgr()->GetPBlocksPerVBlock());
	}
}

VOID
BLOCK_MGR::CheckVPC(UINT32 nVBN)
{
	DEBUG_ASSERT(nVBN < DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount());

	VBINFO*		pstVBInfo;
	pstVBInfo = DFTL_GLOBAL::GetVBInfoMgr()->GetVBInfo(nVBN);

	UINT32 nValidVPN_VBInfo;

	nValidVPN_VBInfo = DFTL_GLOBAL::GetInstance()->GetVPagePerVBlock() - pstVBInfo->GetInvalidLPNCount();

	VNAND* pstVNandMgr = DFTL_GLOBAL::GetVNandMgr();
	UINT32 nValidVPN_VNand = pstVNandMgr->GetValidVPNCount(nVBN);

	DEBUG_ASSERT(nValidVPN_VBInfo == nValidVPN_VNand);
}


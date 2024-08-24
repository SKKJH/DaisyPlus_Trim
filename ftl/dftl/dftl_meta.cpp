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

VOID
META_CACHE_ENTRY::Initialize(VOID)
{
	m_nMetaLPN = INVALID_LPN;
	INIT_LIST_HEAD(&m_dlList);

	m_bValid = FALSE;
	m_bDirty = FALSE;
	m_bIORunning = FALSE;
}

VOID
META_CACHE::Initialize(VOID)
{
	INIT_LIST_HEAD(&m_dlLRU);
	INIT_LIST_HEAD(&m_dlFree);
	m_nFreeCount = 0;

	m_nHit = 0;
	m_nMiss = 0;

	for (INT32 i = 0; i < META_CACHE_ENTRY_COUNT; i++)
	{
		m_astCacheEntry[i].Initialize();

		_Release(&m_astCacheEntry[i]);
	}

	m_nFormatMetaLPN = INVALID_LPN;
}

BOOL
META_CACHE::Format(VOID)
{
	BOOL bRet;

	do
	{
		if (m_nFormatMetaLPN == INVALID_LPN)
		{
			m_nFormatMetaLPN = 0;
		}

		META_L2V_MGR*	pstMetaL2VMgr = DFTL_GLOBAL::GetMetaL2VMgr();
		if (m_nFormatMetaLPN >= pstMetaL2VMgr->GetMetaLPNCount())
		{
			// Format done
			bRet = TRUE;
			break;
		}

		META_CACHE_ENTRY*	pstEntry = _Allocate();
		if (pstEntry == NULL)
		{
			bRet = FALSE;
			break;
		}

		DEBUG_ASSERT(pstEntry->m_bValid == FALSE);
		DEBUG_ASSERT(pstEntry->m_bDirty == FALSE);
		DEBUG_ASSERT(pstEntry->m_bIORunning == FALSE);

		for (INT32 i = 0; i < L2V_PER_META_PAGE; i++)
		{
			pstEntry->m_anL2V[i] = INVALID_LPN;
		}

		pstEntry->m_bDirty = TRUE;
		pstEntry->m_bValid = TRUE;

		pstEntry->m_nMetaLPN = m_nFormatMetaLPN;

		m_nFormatMetaLPN++;

		bRet = FALSE;

	} while(0);

	return bRet;
}

META_CACHE_ENTRY*
META_CACHE::GetMetaEntry(UINT32 nLPN)
{
	static INT32	nPrevLPN;

	META_CACHE_ENTRY*		pstEntry;
	UINT32	nMetaLPN = _GetMetaLPN(nLPN);

	list_for_each_entry(META_CACHE_ENTRY, pstEntry, &m_dlLRU, m_dlList)
	{
		if (pstEntry->m_nMetaLPN == nMetaLPN)
		{
			// hit, move to head
			list_move_head(&pstEntry->m_dlList, &m_dlLRU);

			DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_L2PCACHE_HIT);
			nPrevLPN = nLPN;
			return pstEntry;
		}
	}

	if (nPrevLPN != nLPN)
	{
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_L2PCACHE_MISS);
		nPrevLPN = nLPN;
	}

	return NULL;
}

BOOL
META_CACHE::IsMetaAvailable(UINT32 nLPN)
{
	META_CACHE_ENTRY*	pstEntry = GetMetaEntry(nLPN);

	if (pstEntry == NULL)
	{
		return FALSE;
	}

	return (pstEntry->m_bValid == TRUE) ? TRUE : FALSE;
}

VOID
META_CACHE::LoadMeta(UINT32 nLPN)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	META_CACHE_ENTRY*	pstEntry = GetMetaEntry(nLPN);
	if (pstEntry != NULL)
	{
		DEBUG_ASSERT(pstEntry->m_bIORunning == TRUE);
		DEBUG_ASSERT(pstEntry->m_bValid == FALSE);
		return;
	}

	pstEntry = _Allocate();
	if (pstEntry == NULL)
	{
		// busy
		return;
	}

	REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
	META_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetMetaRequestInfo();
	META_REQUEST*	pstRequest = pstRequestInfo->AllocateRequest();

	if (pstRequest == NULL)
	{
		// there is no free request
		_Release(pstEntry);			// Release Meta entry
		return;
	}

	pstEntry->m_nMetaLPN = _GetMetaLPN(nLPN);

	pstRequest->Initialize(META_REQUEST_READ_WAIT, pstEntry->m_nMetaLPN, pstEntry, IOTYPE_META);
	pstRequestInfo->AddToWaitQ(pstRequest);

	pstEntry->m_bIORunning = TRUE;

	META_DEBUG_PRINTF("[META] Load MetaLPN: %d \n\r", pstEntry->m_nMetaLPN);
#else
	ASSERT(0);
#endif
}

UINT32
META_CACHE::GetL2V(UINT32 nLPN)
{
	META_CACHE_ENTRY*		pstEntry;

	pstEntry = GetMetaEntry(nLPN);
	ASSERT(pstEntry != NULL);

	DEBUG_ASSERT(pstEntry->m_bValid == TRUE);

	UINT32 nOffset = nLPN % L2V_PER_META_PAGE;

	return pstEntry->m_anL2V[nOffset];
}

/*
	@brief	set a new VPPN,
	@return	OldVPPN
*/
UINT32
META_CACHE::SetL2V(UINT32 nLPN, UINT32 nVPPN)
{
	META_CACHE_ENTRY*		pstEntry;

	pstEntry = GetMetaEntry(nLPN);
	ASSERT(pstEntry != NULL);

	DEBUG_ASSERT(pstEntry->m_bValid == TRUE);

	UINT32 nOffset = nLPN % L2V_PER_META_PAGE;

	UINT32 nOldVPPN = pstEntry->m_anL2V[nOffset];

	pstEntry->m_anL2V[nOffset] = nVPPN;

	pstEntry->m_bDirty = TRUE;

	return nOldVPPN;
}

UINT32
META_CACHE::_GetMetaLPN(UINT32 nLPN)
{
	return nLPN / L2V_PER_META_PAGE;
}

VOID
META_CACHE::_Release(META_CACHE_ENTRY* pstEntry)
{
	list_add_head(&pstEntry->m_dlList, &m_dlFree);
	m_nFreeCount++;
}

/*
	@brief	allocate a cache entry
*/

META_CACHE_ENTRY*
META_CACHE::_Allocate(VOID)
{
	META_CACHE_ENTRY*	pstEntry;
	if (m_nFreeCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlFree) == TRUE);
		pstEntry = list_last_entry(&m_dlLRU, META_CACHE_ENTRY, m_dlList);

		if (pstEntry->m_bIORunning == TRUE)
		{
			// Programing on going
			return NULL;
		}

		DEBUG_ASSERT(pstEntry->m_bValid == TRUE);

		if (pstEntry->m_bDirty == TRUE)
		{
			// need to write this entry to NAND flash memory
			// Add To Meta Write Queue
			REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
			META_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetMetaRequestInfo();
			META_REQUEST*	pstRequest = pstRequestInfo->AllocateRequest();
			if (pstRequest == NULL)
			{
				// there is no free reuqest
				return NULL;
			}

			pstRequest->Initialize(META_REQUEST_WRITE_WAIT, pstEntry->m_nMetaLPN, pstEntry, IOTYPE_META);

			// copy meta to buffer entry
			OSAL_MEMCPY(pstRequest->GetBuffer()->m_pMainBuf, &pstEntry->m_anL2V[0], META_VPAGE_SIZE);

			pstRequestInfo->AddToWaitQ(pstRequest);

			pstEntry->m_bIORunning = TRUE;

			META_DEBUG_PRINTF("[META] Flush MetaLPN: %d \r\n", pstEntry->m_nMetaLPN);

			return NULL;
		}

		META_DEBUG_PRINTF("[META] Evicted MetaLPN: %d \r\n", pstEntry->m_nMetaLPN);

		ASSERT(pstEntry->m_bValid == TRUE);
	}
	else
	{
		pstEntry = list_first_entry(&m_dlFree, META_CACHE_ENTRY, m_dlList);
		m_nFreeCount--;
	}

	list_move_head(&pstEntry->m_dlList, &m_dlLRU);

	pstEntry->m_bValid = FALSE;
	ASSERT(pstEntry->m_bDirty == FALSE);
	ASSERT(pstEntry->m_bIORunning == FALSE);

	return pstEntry;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	Meta Manager
//
///////////////////////////////////////////////////////////////////////////////////

VOID
META_MGR::Initialize(VOID)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	m_stMetaCache.Initialize();
#else
	m_panL2V = (UINT32*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, _GetL2PSize(), OSAL_MEMALLOC_FW_ALIGNMENT);
#endif

	m_bFormatted = FALSE;
}

BOOL
META_MGR::Format(VOID)
{
	if (m_bFormatted == TRUE)
	{
		return TRUE;
	}

	BOOL	bRet;

#if (SUPPORT_META_DEMAND_LOADING == 1)
	bRet = m_stMetaCache.Format();
#else
	OSAL_MEMSET(m_panL2V, 0xFF, _GetL2PSize());		// set invalid LPN
	bRet = TRUE;
#endif

	if (bRet == TRUE)
	{
		m_nVPC = 0;
		m_bFormatted = TRUE;
	}

	return bRet;
}

BOOL
META_MGR::IsMetaAvailable(UINT32 nLPN)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	return m_stMetaCache.IsMetaAvailable(nLPN);
#else
	return TRUE;
#endif
}

VOID
META_MGR::LoadMeta(UINT32 nLPN)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	return m_stMetaCache.LoadMeta(nLPN);
#else
	return;
#endif
}

VOID
META_MGR::LoadDone(META_CACHE_ENTRY* pstMetaEntry, VOID* pBuf)
{
	DEBUG_ASSERT(FALSE == pstMetaEntry->m_bValid);
	DEBUG_ASSERT(FALSE == pstMetaEntry->m_bDirty);
	DEBUG_ASSERT(TRUE == pstMetaEntry->m_bIORunning);

	pstMetaEntry->m_bIORunning = FALSE;
	pstMetaEntry->m_bDirty = FALSE;
	pstMetaEntry->m_bValid = TRUE;

	OSAL_MEMCPY(&pstMetaEntry->m_anL2V[0], pBuf, META_VPAGE_SIZE);
}

VOID
META_MGR::StoreDone(META_CACHE_ENTRY* pstMetaEntry)
{
	DEBUG_ASSERT(TRUE == pstMetaEntry->m_bValid);
	DEBUG_ASSERT(TRUE == pstMetaEntry->m_bDirty);
	DEBUG_ASSERT(TRUE == pstMetaEntry->m_bIORunning);

	pstMetaEntry->m_bIORunning = FALSE;
	pstMetaEntry->m_bDirty = FALSE;
}

UINT32
META_MGR::GetL2V(UINT32 nLPN)
{
	DEBUG_ASSERT(nLPN < DFTL_GLOBAL::GetInstance()->GetLPNCount());

#if (SUPPORT_META_DEMAND_LOADING == 1)
	return m_stMetaCache.GetL2V(nLPN);
#else
	return	m_panL2V[nLPN];
#endif
}

VOID
META_MGR::SetL2V(UINT32 nLPN, UINT32 nVPPN)
{
	DEBUG_ASSERT(nLPN < DFTL_GLOBAL::GetInstance()->GetLPNCount());

	UINT32	nOldVPPN;

#if (SUPPORT_META_DEMAND_LOADING == 1)
	nOldVPPN = m_stMetaCache.SetL2V(nLPN, nVPPN);
#else
	nOldVPPN = m_panL2V[nLPN];

	m_panL2V[nLPN] = nVPPN;
#endif

	if (nOldVPPN == INVALID_PPN)
	{
		m_nVPC++;
	}
	else
	{
		// Invalidate OLD PPN
		VNAND*	pstVNand = DFTL_GLOBAL::GetVNandMgr();
		DEBUG_ASSERT(pstVNand->GetV2L(nOldVPPN) == nLPN);
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_OVERWRITE);

		pstVNand->Invalidate(nOldVPPN);
		DFTL_GLOBAL::GetInstance()->GetUserBlockMgr()->Invalidate(nOldVPPN);
	}
}

/*
@brief	return L2p SIZE IN BYTE
*/
INT32
META_MGR::_GetL2PSize(VOID)
{
	return sizeof(UINT32) * DFTL_GLOBAL::GetInstance()->GetLPNCount();
}

///////////////////////////////////////////////////////////////////////////////////
//
//	Meta L2V Manager
//
///////////////////////////////////////////////////////////////////////////////////

VOID
META_L2V_MGR::Initialize(VOID)
{
	m_panL2V = (UINT32*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, _GetL2PSize(), OSAL_MEMALLOC_FW_ALIGNMENT);

	m_bFormatted = FALSE;
}

BOOL
META_L2V_MGR::Format(VOID)
{
	if (m_bFormatted == TRUE)
	{
		return TRUE;
	}

	OSAL_MEMSET(m_panL2V, 0xFF, _GetL2PSize());		// set invalid LPN

	m_nVPC = 0;
	m_bFormatted = TRUE;

	return TRUE;
}

UINT32
META_L2V_MGR::GetMetaLPNCount(VOID)
{
	UINT32	nLPNCount	= DFTL_GLOBAL::GetInstance()->GetLPNCount();
	return CEIL(((INT32)nLPNCount), (INT32)L2V_PER_META_PAGE);
}

UINT32
META_L2V_MGR::GetL2V(UINT32 nLPN)
{
	DEBUG_ASSERT(nLPN < GetMetaLPNCount());
	return	m_panL2V[nLPN];
}

VOID
META_L2V_MGR::SetL2V(UINT32 nLPN, UINT32 nVPPN)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	DEBUG_ASSERT(nLPN < GetMetaLPNCount());

	BOOL	bOverWrite;

	if (m_panL2V[nLPN] != INVALID_PPN)
	{
		// Invalidate OLD PPN
		UINT32 nOldVPPN = m_panL2V[nLPN];
		VNAND*	pstVNand = DFTL_GLOBAL::GetVNandMgr();
		DEBUG_ASSERT(pstVNand->GetV2L(nOldVPPN) == nLPN);

		pstVNand->Invalidate(nOldVPPN);

		DFTL_GLOBAL::GetInstance()->GetMetaBlockMgr()->Invalidate(nOldVPPN);

		bOverWrite = TRUE;
	}
	else
	{
		bOverWrite = FALSE;
	}

	m_panL2V[nLPN] = nVPPN;

	if (bOverWrite == FALSE)
	{
		m_nVPC++;
	}
#else
	ASSERT(0);		// check option
#endif
}

/*
@brief	return L2p SIZE IN BYTE
*/
INT32
META_L2V_MGR::_GetL2PSize(VOID)
{
	return sizeof(UINT32) * GetMetaLPNCount();
}

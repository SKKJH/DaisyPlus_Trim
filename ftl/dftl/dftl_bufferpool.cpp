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
BUFFER_MGR::Initialize(VOID)
{
	INT32	nChannelWayInterleaving = USER_CHANNELS * USER_WAYS * 2;			// 1 Programming and 1 buffering at each way

	m_nTotalCount = MAX(nChannelWayInterleaving, HOST_REQUEST_BUF_COUNT);

	// allocate buffer pool
	m_pastEntry = (BUFFER_ENTRY*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, (sizeof(BUFFER_ENTRY) * m_nTotalCount), OSAL_MEMALLOC_FW_ALIGNMENT);

	INIT_LIST_HEAD(&m_dlFree);
	m_nFreeCount = 0;

	BUFFER_ENTRY*		pstEntry;

	// allocate main buffer
	for (int i = 0; i < m_nTotalCount; i++)
	{
		pstEntry = GetEntry(i);
		pstEntry->m_pMainBuf = (void*)OSAL_MemAlloc(MEM_TYPE_BUF, BYTES_PER_DATA_REGION_OF_PAGE, BYTES_PER_DATA_REGION_OF_PAGE);
	}

	// allocate spare buffer
	for (int i = 0; i < m_nTotalCount; i++)
	{
		pstEntry = GetEntry(i);
		pstEntry->m_pSpareBuf= (void*)OSAL_MemAlloc(MEM_TYPE_BUF, BYTES_PER_SPARE_REGION_OF_PAGE, BYTES_PER_SPARE_REGION_OF_PAGE);
	}

	// add to free list
	for (int i = 0; i < m_nTotalCount; i++)
	{
		pstEntry = GetEntry(i);
		Release(pstEntry);
	}

	return;
}

BUFFER_ENTRY*
BUFFER_MGR::GetEntry(INT32 nIndex)
{
	DEBUG_ASSERT(nIndex < m_nTotalCount);
	return &m_pastEntry[nIndex];
}

BUFFER_ENTRY*
BUFFER_MGR::Allocate(VOID)
{
	if (m_nFreeCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlFree) == TRUE);
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_OUT_OF_BUFFER);
		return NULL;
	}

	BUFFER_ENTRY*	pstEntry;
	pstEntry = list_first_entry(&m_dlFree, BUFFER_ENTRY, m_dlList);
	list_del(&pstEntry->m_dlList);

	m_nFreeCount--;

	DEBUG_ASSERT(m_nFreeCount <= m_nTotalCount);
	DEBUG_ASSERT(m_nFreeCount >= 0);

	return pstEntry;
}

VOID
BUFFER_MGR::Release(BUFFER_ENTRY* pstEntry)
{
	list_add_tail(&pstEntry->m_dlList, &m_dlFree);
	m_nFreeCount++;

	DEBUG_ASSERT(m_nFreeCount <= m_nTotalCount);
}


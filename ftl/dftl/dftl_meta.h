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

#ifndef __DFTL_META_H__
#define __DFTL_META_H__

#define SUPPORT_META_DEBUG		0

#if (SUPPORT_META_DEBUG == 0)
	#define META_DEBUG_PRINTF(...)	((void)0)
#else
	#define META_DEBUG_PRINTF	PRINTF
#endif

#define SIZE_OF_L2V							(sizeof(UINT32))		// 4 byte
#define L2V_PER_META_PAGE					(META_VPAGE_SIZE / SIZE_OF_L2V)
#define META_CACHE_ENTRY_COUNT				(META_CACHE_SIZE / META_VPAGE_SIZE)

class META_CACHE_ENTRY
{
public:
	VOID	Initialize(VOID);

	UINT32				m_nMetaLPN;		// Meta Pate Number

	unsigned int		m_bValid : 1;		// a Valid Entry
	unsigned int		m_bDirty : 1;		// this cache entry is updated, need to stored to meta block
	unsigned int		m_bIORunning : 1;	// on Read/programming entry

	struct list_head	m_dlList;

	UINT32				m_anL2V[L2V_PER_META_PAGE];
};

class META_CACHE
{
public:
	VOID				Initialize(VOID);
	BOOL				Format(VOID);
	BOOL				IsMetaAvailable(UINT32 nLPN);
	META_CACHE_ENTRY*	GetMetaEntry(UINT32 nLPN);
	VOID				LoadMeta(UINT32 nLPN);

	UINT32	GetL2V(UINT32 nLPN);
	UINT32	SetL2V(UINT32 nLPN, UINT32 nVPPN);

private:
	META_CACHE_ENTRY*	_Allocate(VOID);
	VOID				_Release(META_CACHE_ENTRY* pstEntry);
	UINT32				_GetMetaLPN(UINT32 nLPN);

	META_CACHE_ENTRY	m_astCacheEntry[META_CACHE_ENTRY_COUNT];

	struct list_head	m_dlLRU;		// LRU for victim Meta cache entry selection, MRU:@Head, LRU:@tail
	struct list_head	m_dlFree;		// Free cache entry list
	INT32				m_nFreeCount;

	UINT32				m_nHit;			// L2P Hit Count
	UINT32				m_nMiss;		// L2P Miss Count

	// For metadata format
	UINT32				m_nFormatMetaLPN;
};

class META_MGR
{
public:
	VOID	Initialize(VOID);
	BOOL	Format(VOID);
	UINT32	GetL2V(UINT32 nLPN);
	VOID	SetL2V(UINT32 nLPN, UINT32 nVPPN);

	BOOL	IsMetaAvailable(UINT32 nLPN);
	VOID	LoadMeta(UINT32 nLPN);

	VOID	LoadDone(META_CACHE_ENTRY* pstMetaEntry, VOID* pBuf);
	VOID	StoreDone(META_CACHE_ENTRY* pstMetaEntry);		// NAND IO Done

protected:
	INT32	_GetL2PSize(VOID);

private:
#if (SUPPORT_META_DEMAND_LOADING == 1)
	META_CACHE	m_stMetaCache;
#else
	UINT32		*m_panL2V;			// L2VPpn Array
#endif

protected:
	UINT32		m_nVPC;				// FTL Valid Page Count
	unsigned int	m_bFormatted : 1;	// format status
};

class META_L2V_MGR : public META_MGR
{
public:
	VOID	Initialize(VOID);
	BOOL	Format(VOID);
	UINT32	GetL2V(UINT32 nLPN);
	VOID	SetL2V(UINT32 nLPN, UINT32 nVPPN);

	UINT32	GetMetaLPNCount(VOID);

private:
	INT32	_GetL2PSize(VOID);

	UINT32		*m_panL2V;
};

#endif

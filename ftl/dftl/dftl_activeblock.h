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

#ifndef __DFTL_ACTIVEBLOCK_H__
#define __DFTL_ACTIVEBLOCK_H__

class HIL_REQUEST;
class GC_REQUEST;

class PROGRAM_UNIT
{
public:
	PROGRAM_UNIT(VOID);
	VOID Add(VOID* pstRequest, UINT32 nVPPN, IOTYPE eBlockType, IOTYPE eRequestType);

	BOOL IsFull(VOID)
	{
		return (m_nLPNCount == LPN_PER_PHYSICAL_PAGE) ? TRUE : FALSE;
	}

	UINT32	GetVPPN(VOID) {return m_nVPPN;}
	BUFFER_ENTRY*	GetBufferEntry(VOID) {return m_pstBufferEntry;}
	VOID SetBufferEntry(BUFFER_ENTRY* pstEntry) {m_pstBufferEntry = pstEntry;}
	UINT32	GetLPN(INT32 nIndex) {return m_anLPN[nIndex];}

	unsigned int		m_bFree : 1;			// this entry is free
	unsigned int		m_bProgramming : 1;		// on programming
	unsigned int		m_nLPNCount : 3;		// max 4 LPNs
	unsigned int		m_nDMAReqTail : 8;		// DMA tail request, to check HDMA done, Host Write only

	UINT32				m_nDMAOverFlowCount;		// count of DMA index overflow, to check HDMA done

private:
	FTL_REQUEST_ID		m_astRequestID[LPN_PER_PHYSICAL_PAGE];	// request ID for current request = Request ID for each LPN
	INT32				m_anLPN[LPN_PER_PHYSICAL_PAGE];			// Buffered LPN

	UINT32				m_nVPPN;					// Program VPPN, 1st page of requests
	BUFFER_ENTRY*		m_pstBufferEntry;			// NAND IO Buffer
} ;

/*
	ActiveBlock
*/
class ACTIVE_BLOCK
{
public:
	ACTIVE_BLOCK(VOID);

	BOOL Write(VOID* pstRequest, IOTYPE eRequestType);

	VOID IncreaseVPPN(VOID);
	VOID SetIOType(IOTYPE eIOType) { m_eBlockType = eIOType; }
	VOID ProgramDone(INT32 nBufferingIndex);

	BOOL CheckAllProgramUnitIsFree(VOID);			// debugging

	UINT32 ReadBufferingLPN(UINT32 nLPN, BUFFER_ENTRY* pstBufferEntry);

	UINT32			m_nVBN;
	UINT32			m_nCurVPPN;				// PPN recently written

private:
	VOID			_IssueProgram(VOID);

	IOTYPE			m_eBlockType;				// type of active block

	unsigned int	m_nCurProgramBuffering : ACTIVE_BLOCK_BUFFERING_INDEX_BITS;
												// current index of astBuffering 
	PROGRAM_UNIT	m_astBuffering[ACTIVE_BLOCK_BUFFERING_COUNT];		// program buffering information
} ;

/*
	@brief Buffering LPNs for host write
*/

class BUFFERING_LPN
{
public:

	static const UINT32 HASH_BUCKET_COUNT = 4096;
	static const UINT32 HASH_BUCKET_MASK = (HASH_BUCKET_COUNT - 1);

	VOID Initialize(IOTYPE eIOType);
	UINT32 ReadLPN(UINT32 nLPN, BUFFER_ENTRY* pstBufferEntry);

	static inline UINT32 GET_BUCKET_INDEX(UINT32 nLPN)
	{
		return nLPN & HASH_BUCKET_MASK;
	}

	VOID Add(UINT32 nLPN);
	VOID Remove(UINT32 nLPN);

private:
	IOTYPE		m_eIOType;
	UINT16		m_anBufferingLPNCount[HASH_BUCKET_COUNT];
};

class ACTIVE_BLOCK_MGR
{
public:
	VOID Initialize(VOID);
	ACTIVE_BLOCK* GetActiveBlock(IOTYPE eIOType);
	ACTIVE_BLOCK* GetActiveBlock(INT32 nIndex, IOTYPE eIOType);
	BUFFERING_LPN* GetBufferingMgr(VOID) { return &m_stBufferingMgr; }

	BUFFERING_LPN* GetMetaBufferingMgr(VOID) { return &m_stMetaBufferingMgr; }

	UINT32 ReadBufferingLPN(UINT32 nLPN, BUFFER_ENTRY* pstBufferEntry, IOTYPE eIOType);

	UINT32 GetActiveBlockIndex(IOTYPE eIOType);

private:
	BUFFERING_LPN	m_stBufferingMgr;

	BUFFERING_LPN	m_stMetaBufferingMgr;

	ACTIVE_BLOCK*	_GetCurActiveBlock(IOTYPE eIOType);
	ACTIVE_BLOCK*	_GoToNextActiveBlock(IOTYPE eIOType);

	INT8			m_nCurActiveBlockHost;
	INT8			m_nCurActiveBlockGC;
	INT8			m_nCurActiveBlockMeta;

	ACTIVE_BLOCK*	m_pstCurActiveBlockHost;
	ACTIVE_BLOCK*	m_pstCurActiveBlockGC;
	ACTIVE_BLOCK*	m_pstCurActiveBlockMeta;

	ACTIVE_BLOCK	m_astActiveBlockHost[ACTIVE_BLOCK_COUNT_PER_STREAM];
	ACTIVE_BLOCK	m_astActiveBlockGC[ACTIVE_BLOCK_COUNT_PER_STREAM];
	ACTIVE_BLOCK	m_astActiveBlockMeta[ACTIVE_BLOCK_COUNT_PER_STREAM];
};

#endif
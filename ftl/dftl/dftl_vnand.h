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

#ifndef __DFTL_VNAND_H__
#define __DFTL_VNAND_H__

// Refer to ftl_config.h for the PPN layout 
#define VBN_FROM_VPPN(_nVPPN)				(((_nVPPN) >> NAND_ADDR_BLOCK_SHIFT) & NAND_ADDR_BLOCK_MASK)
#define VPN_FROM_VPPN(_nVPPN)				(_nVPPN & NAND_ADDR_VPAGE_MASK)
#define VPPN_FROM_VBN_VPN(_nVBN, _nVPN)		(_nVPN + (_nVBN << NUM_BIT_VPAGE))

#define CHANNEL_FROM_VPPN(_nVPPN)			((_nVPPN >> NAND_ADDR_CHANNEL_SHIFT) & NAND_ADDR_CHANNEL_MASK)
#define WAY_FROM_VPPN(_nVPPN)				((_nVPPN >> NAND_ADDR_WAY_SHIFT) & NAND_ADDR_WAY_MASK)
#define PBN_FROM_VPPN(_nVPPN)				(VBN_FROM_VPPN(_nVPPN))
#define PPAGE_FROM_VPPN(_nVPPN)				((_nVPPN >> (NAND_ADDR_PPAGE_SHIFT)) & NAND_ADDR_PPAGE_MASK)
#define LPN_OFFSET_FROM_VPPN(_nVPPN)		((_nVPPN >> (NAND_ADDR_LPN_OFFSET_SHIFT)) & NAND_ADDR_LPN_PER_PAGE_MASK)

#define GET_VPPN_FROM_VPN_VBN(_nVPN, _nVBN)	(_nVPN + (_nVBN << NAND_ADDR_VBN_SHIFT))

// physical block
class VIRTUAL_BLOCK
{
public:
	VIRTUAL_BLOCK() :
#if defined(WIN32) && (SUPPORT_META_DEMAND_LOADING == 1)
		m_pData(NULL),
#endif
		m_nEC(0) {}

	UINT32*	m_pnV2L;			// store PPN 2 LPN
	UINT8*	m_pbValid;			// Logical Page Valid Bitmap
	INT32	m_nEC;				// Erase Count of block

#if defined(WIN32) && (SUPPORT_META_DEMAND_LOADING == 1)
	VOID*	m_pData;		// Main Area of NAND for store FTL metadata
#endif
};

class PROGRAM_UNIT;

/*
	Virtual NAND 
*/
class VNAND
{
public:
	VOID	Initialize(VOID);

	UINT32	GetPBlocksPerVBlock(VOID);
	UINT32	GetPPagesPerVBlock(VOID);
	UINT32	GetVPagesPerVBlock(VOID);
	UINT32	GetVBlockCount(void);
	UINT32	GetV2L(UINT32 nVPPN);
	VOID	Invalidate(UINT32 nVPPN);

	BOOL	IsValid(UINT32 nVBN, UINT32 nVPageNo);

	BOOL	IsBadBlock(UINT32 nVBN) { return FIL_IsBad(nVBN); }

	INT32	ReadPage(FTL_REQUEST_ID stReqID, UINT32 nVPPN, void * pMainBuf, void * pSpareBuf);
	VOID	ProgramPage(FTL_REQUEST_ID stReqID, PROGRAM_UNIT *pstProgram);
	VOID	ReadPageSimul(UINT32 nVPPN, void * pMainBuf);		// for metadata read (only for demand loading)
	VOID	ProgramPageSimul(UINT32 nLPN, UINT32 nVPPN);

	UINT32	GetValidVPNCount(UINT32 nVBN);

private:
	NAND_ADDR	_GetNandAddrFromVPPN(UINT32 nVPPN);
	VOID		_EraseSimul(INT32 nVBN);

	VIRTUAL_BLOCK*		m_pstVB;
};

#endif		// end of #ifndef __DFTL_VNAND_H__
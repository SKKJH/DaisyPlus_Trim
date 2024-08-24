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

#ifndef __DFTL_GLOBAL_H__
#define __DFTL_GLOBAL_H__

class FTL_INTERFACE;
class VNAND;
class META_MGR;

class PROFILE
{
public:
	VOID Initialize(VOID)
	{
		for (INT32 i = 0; i < PROFILE_COUNT; i++)
		{
			m_astStatistics[i].nType = static_cast<PROFILE_TYPE>(i);
			m_astStatistics[i].nCount = 0;
		}
	}

	VOID IncreaseCount(PROFILE_TYPE eType)
	{
		IncreaseCount(eType, 1);
	}

	VOID IncreaseCount(PROFILE_TYPE eType, UINT32 nCount)
	{
		m_astStatistics[eType].nCount += nCount;
	}

	UINT32 GetCount(PROFILE_TYPE eType)
	{
		return m_astStatistics[eType].nCount;
	}

	VOID Print(VOID)
	{
		static const char *apsProfileString[] =
		{
			FOREACH_PROFILE(GENERATE_STRING)
		};

		for (int i = 0; i < PROFILE_COUNT; i++)
		{
			PRINTF("%s, %d\n\r", apsProfileString[i], GetCount((PROFILE_TYPE)i));
		}
	}


private:
	PROFILE_ENTRY	m_astStatistics[PROFILE_COUNT];
};

typedef enum
{
	DFTL_STATUS_NONE		= 0x00,
	DFTL_STATUS_META_IO		= 0x01,			// meta loading / storing
	DFTL_STATUS_FORMATTING	= 0x02,			// now formatting
} DFTL_STATUS;

class DFTL_GLOBAL : public FTL_INTERFACE
{
public:
	//DFTL_GLOBAL() {};

	VIRTUAL VOID Initialize(VOID);
	VIRTUAL BOOL Format(VOID);
	VIRTUAL VOID Run(VOID);
	VIRTUAL VOID ReadPage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount);
	VIRTUAL VOID WritePage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount);
	VIRTUAL VOID CallBack(FTL_REQUEST_ID stReqID);
	VIRTUAL VOID IOCtl(IOCTL_TYPE eType);

	static DFTL_GLOBAL*		GetInstance(VOID) { return m_pstInstance; }
	static VNAND*			GetVNandMgr(VOID) { return &m_pstInstance->m_stVNand; }
	static BLOCK_MGR*		GetUserBlockMgr(VOID) { return &m_pstInstance->m_stUserBlockMgr; }
#if (SUPPORT_META_DEMAND_LOADING == 1)
	static BLOCK_MGR*		GetMetaBlockMgr(VOID) { return &m_pstInstance->m_stMetaBlockMgr; }
#endif
	static VBINFO_MGR*		GetVBInfoMgr(VOID) { return &m_pstInstance->m_stVBInfoMgr; }
	static REQUEST_MGR*		GetRequestMgr(VOID)	{return &m_pstInstance->m_stRequestMgr;}
	static GC_MGR*			GetGCMgr(VOID) { return &m_pstInstance->m_stGCMgr; }
#if (SUPPORT_META_DEMAND_LOADING == 1)
	static GC_MGR*			GetMetaGCMgr(VOID) { return &m_pstInstance->m_stMetaGCMgr; }
#endif
	static ACTIVE_BLOCK_MGR* GetActiveBlockMgr(VOID) { return &m_pstInstance->m_stActiveBlockMgr; }
	static BUFFERING_LPN*	GetActiveBlockBufferingLPN(VOID) { return m_pstInstance->m_stActiveBlockMgr.GetBufferingMgr(); }

	static BUFFERING_LPN* GetMetaActiveBlockBufferingLPN(VOID) { return m_pstInstance->m_stActiveBlockMgr.GetMetaBufferingMgr(); }

	static META_MGR*		GetMetaMgr(VOID) { return &m_pstInstance->m_stMetaMgr; }
	static META_L2V_MGR*	GetMetaL2VMgr(VOID) { return &m_pstInstance->m_stMetaL2VMgr; }
	static BUFFER_MGR*		GetBufferMgr(VOID)	{return &m_pstInstance->m_stBufferMgr;}
	static HDMA*			GetHDMAMgr(VOID)	{return &m_pstInstance->m_stHostDMA;}

	UINT32 GetVPagePerVBlock(VOID) {return m_nVPagesPerVBlock;}

	UINT32 GetLPNCount(VOID) {return m_nLPNCount;}
	BOOL	IsValidLPN(UINT32 nLPN)
	{
		return (nLPN < m_nLPNCount) ? TRUE : FALSE;
	}

	VOID IncreaseProfileCount(PROFILE_TYPE eType)
	{
		m_stProfile.IncreaseCount(eType);
	}

	VOID IncreaseProfileCount(PROFILE_TYPE eType, UINT32 nCount)
	{
		m_stProfile.IncreaseCount(eType, nCount);
	}

	UINT32 GetProfileCount(PROFILE_TYPE eType)
	{
		return m_stProfile.GetCount(eType);
	}

	UINT32 GetGCTh(VOID) { return m_nGCTh; }

	VOID SetStatus(DFTL_STATUS eStatus);
	BOOL CheckStatus(DFTL_STATUS eStatus);

private:		// fuctions
	VOID _Initialize(VOID);

private:
	VOID _PrintInfo(VOID);

	static DFTL_GLOBAL*	m_pstInstance;

	VNAND				m_stVNand;				// virtual nand module
	META_MGR			m_stMetaMgr;			// meta moudle
	META_L2V_MGR		m_stMetaL2VMgr;			// met data location
	BLOCK_MGR			m_stUserBlockMgr;		// block module
#if (SUPPORT_META_DEMAND_LOADING == 1)
	BLOCK_MGR			m_stMetaBlockMgr;		// block module
#endif
	VBINFO_MGR			m_stVBInfoMgr;			// Virtual Information Manager
	REQUEST_MGR			m_stRequestMgr;			// read/write request manager
	GC_MGR				m_stGCMgr;				// garbage collector
#if (SUPPORT_META_DEMAND_LOADING == 1)
	GC_MGR				m_stMetaGCMgr;			// meta garbage collector
#endif
	ACTIVE_BLOCK_MGR	m_stActiveBlockMgr;
	BUFFER_MGR			m_stBufferMgr;
	HDMA				m_stHostDMA;

	DFTL_STATUS	m_eStatus;

	PROFILE		m_stProfile;

	UINT32		m_nPhysicalFlashSizeKB;
	UINT32		m_nVBlockSizeKB;
	UINT32		m_nVPagesPerVBlock;
	UINT32		m_nLPagesPerVBlockBits;
	UINT32		m_nLPagesPerVBlockMask;

	float		m_fOverProvisionRatio;
	UINT32		m_nOverprovisionSizeKB;
	UINT32		m_nLogicalFlashSizeKB;

	UINT32		m_nLPNCount;
	UINT32		m_nVBlockCount;

	UINT32		m_nGCTh;				// free block count for GC trigger

	BOOL		m_bEnableMetaBlock;

	UINT32		CheckGC;
};

#endif		// end of #ifndef __DFTL_H__

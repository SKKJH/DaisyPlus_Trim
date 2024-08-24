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

#ifndef __DFTL_REQUEST_H__
#define __DFTL_REQUEST_H__

/*
	@brief	request manager
*/
class REQUEST_MGR
{
public:
	VOID Initialize(VOID);
	VOID Run(VOID);

	HIL_REQUEST* AllocateHILRequest(VOID)				{	return m_stHILRequestInfo.AllocateRequest();	}
	VOID AddToHILRequestWaitQ(HIL_REQUEST* pstRequest)	{	m_stHILRequestInfo.AddToWaitQ(pstRequest);	}
	HIL_REQUEST_INFO* GetHILRequestInfo(VOID)			{	return &m_stHILRequestInfo;	}

	GC_REQUEST* AllocateGCRequest(VOID)						{	return m_stGCRequestInfo.AllocateRequest();	}
	VOID		AddToGCRequestWaitQ(GC_REQUEST* pstRequest)	{	m_stGCRequestInfo.AddToWaitQ(pstRequest);	}
	GC_REQUEST_INFO* GetGCRequestInfo(VOID)					{	return &m_stGCRequestInfo;	}

	META_REQUEST_INFO* GetMetaRequestInfo(VOID)				{	return &m_stMetaRequestInfo;	}
	META_REQUEST* AllocateMetaRequest(VOID)					{	return m_stMetaRequestInfo.AllocateRequest();	}
	VOID AddToGCRequestWaitQ(META_REQUEST* pstRequest)		{	m_stMetaRequestInfo.AddToWaitQ(pstRequest);	}

	VOID _ProcessWaitQ(VOID);
	VOID _ProcessDoneQ(VOID);

	VOID _ProcessHDMARequestIssuedQ(VOID);

	VOID _ProcessGCRequestDoneQ(VOID);
	VOID _ProcessGCRequestWaitQ(VOID);

	VOID _ProcessHILRequestWaitQ(VOID);
	VOID _ProcessHILRequestDoneQ(VOID);

	VOID _ProcessMetaRequestWaitQ(VOID);
	VOID _ProcessMetaRequestDoneQ(VOID);

private:
	HIL_REQUEST_INFO	m_stHILRequestInfo;
	GC_REQUEST_INFO		m_stGCRequestInfo;			// USER & META GC
	META_REQUEST_INFO	m_stMetaRequestInfo;
};

#endif
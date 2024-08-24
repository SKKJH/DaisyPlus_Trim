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
//	REQUEST MANAGER
//
///////////////////////////////////////////////////////////////////////////////

VOID
REQUEST_MGR::Initialize(VOID)
{
	m_stHILRequestInfo.Initialize();
	m_stGCRequestInfo.Initialize();

#if (SUPPORT_META_DEMAND_LOADING == 1)
	m_stMetaRequestInfo.Initialize();
#endif
}

VOID
REQUEST_MGR::Run(VOID)
{
	_ProcessDoneQ();
	_ProcessWaitQ();
}

VOID
REQUEST_MGR::_ProcessDoneQ(VOID)
{
	_ProcessHILRequestDoneQ();
	_ProcessHDMARequestIssuedQ();	// Let's check when there is not enough free buffer
	_ProcessGCRequestDoneQ();

#if (SUPPORT_META_DEMAND_LOADING == 1)
	_ProcessMetaRequestDoneQ();
#endif
}

VOID
REQUEST_MGR::_ProcessWaitQ(VOID)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	_ProcessMetaRequestWaitQ();
#endif
	_ProcessGCRequestWaitQ();
	_ProcessHILRequestWaitQ();
}

VOID
REQUEST_MGR::_ProcessHILRequestWaitQ(VOID)
{
	HIL_REQUEST*	pstRequest;
	BOOL bSuccess;

	do
	{
		pstRequest = m_stHILRequestInfo.GetWaitRequest();
		if (pstRequest == NULL)
		{
			return;
		}

		bSuccess = pstRequest->Run();
	} while (bSuccess == TRUE);

	return;
}

VOID
REQUEST_MGR::_ProcessHILRequestDoneQ(VOID)
{
	HIL_REQUEST*	pstRequest;
	BOOL bSuccess;

	do
	{
		pstRequest = m_stHILRequestInfo.GetDoneRequest();
		if (pstRequest == NULL)
		{
			// Nothing to do
			return;
		}

		bSuccess = pstRequest->Run();
	} while (bSuccess == TRUE);
}

VOID
REQUEST_MGR::_ProcessGCRequestWaitQ(VOID)
{
	GC_REQUEST*	pstRequest;
	BOOL bSuccess;

	do
	{
		pstRequest = m_stGCRequestInfo.GetWaitRequest();
		if (pstRequest == NULL)
		{
			return;
		}

		bSuccess = pstRequest->Run();
	} while (bSuccess == TRUE);

	return;
}

VOID
REQUEST_MGR::_ProcessHDMARequestIssuedQ(VOID)
{
	HIL_REQUEST*	pstRequest;
	BOOL bSuccess;

	do
	{
		pstRequest = m_stHILRequestInfo.GetHDMARequest();
		if (pstRequest == NULL)
		{
			// Nothing to do
			return;
		}

		bSuccess = pstRequest->Run();
	} while (bSuccess == TRUE);
}

VOID
REQUEST_MGR::_ProcessGCRequestDoneQ(VOID)
{
	GC_REQUEST*	pstRequest;
	BOOL bSuccess;

	do
	{
		pstRequest = m_stGCRequestInfo.GetDoneRequest();
		if (pstRequest == NULL)
		{
			return;
		}

		bSuccess = pstRequest->Run();
	} while (bSuccess == TRUE);

	return;
}

VOID
REQUEST_MGR::_ProcessMetaRequestWaitQ(VOID)
{
	META_REQUEST*	pstRequest;
	BOOL bSuccess;

	do
	{
		pstRequest = m_stMetaRequestInfo.GetWaitRequest();
		if (pstRequest == NULL)
		{
			return;
		}

		bSuccess = pstRequest->Run();
	} while (bSuccess == TRUE);

	return;
}

VOID
REQUEST_MGR::_ProcessMetaRequestDoneQ(VOID)
{
	META_REQUEST*	pstRequest;
	BOOL bSuccess;

	do
	{
		pstRequest = m_stMetaRequestInfo.GetDoneRequest();
		if (pstRequest == NULL)
		{
			return;
		}

		bSuccess = pstRequest->Run();
	} while (bSuccess == TRUE);

	return;
}


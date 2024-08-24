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

#ifndef __OSAL_H__
#define __OSAL_H__

#ifdef COSMOS_PLUS
	#include "xil_printf.h"
#endif

#define OSAL_MEMALLOC_ALIGNMENT			(4)		// WORD alignment
#define OSAL_MEMALLOC_FW_ALIGNMENT		(4)		// FW DATA alignment
#define OSAL_MEMALLOC_BUF_ALIGNMENT		(16)	// I'm not sure the DMA alignement size.
#define OSAL_MEMALLOC_DMA_ALIGNMENT		(16)

typedef enum
{
	MEM_TYPE_BUF,					// NAND IO BUFFER, must be at region of uncached & nonbuffered
	MEM_TYPE_HW = MEM_TYPE_BUF,		// data for HW access, uncached & nonbuffered
	MEM_TYPE_FW_DATA,
#ifdef WIN32
	MEM_TYPE_WIN32_DEBUG,			// Just for debugging on simulator
#endif
	MEM_TYPE_COUNT,
} MEM_TYPE;

#ifdef __cplusplus
extern "C" {
#endif

	void	OSAL_Initialize(void);
	void	OSAL_SleepUS(long nMicroSecond);
	void*	OSAL_MemAlloc(MEM_TYPE nMemType, int nSize, int nAlign);
	unsigned long long OSAL_GetCurrentTickUS(void);

#ifdef __cplusplus
}
#endif

#define	OSAL_FREE		free
#define	OSAL_MEMSET		memset
#define	OSAL_MEMCPY		memcpy

#ifdef WIN32
	#define		PRINTF			printf
#else
	#define		PRINTF			xil_printf
#endif

// log print
#ifdef COSMOS_PLUS
	#define FPRINTF
#else
	#define FPRINTF		fprintf
#endif

#ifdef WIN32
	#define LOG_PRINTF(_fmt, ...)		do { PRINTF(_fmt, __VA_ARGS__);	FPRINTF(g_fpTestLog, _fmt, __VA_ARGS__);	} while(0)
	#define LOG_PRINTFILE(_fmt, ...)	do { FPRINTF(g_fpTestLog, _fmt, __VA_ARGS__);	} while(0)
	#define LOG_PRINTFILE_ANALYSIS(_fmt, ...)	do { FPRINTF(pFP, _fmt, __VA_ARGS__);	} while(0)
#else
	#define LOG_PRINTF				PRINTF
	#define LOG_PRINTF				PRINTF
	#define LOG_PRINTF				PRINTF
#endif

#endif	// end of #ifndef __OSAL_H__

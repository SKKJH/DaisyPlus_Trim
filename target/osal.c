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

#ifdef WIN32
	#include <windows.h>
	#include <stdio.h>
#else
	#include "Xtime_l.h"
	#include "sleep.h"
#endif

#include "util.h"
#include "osal.h"
#include "cosmos_plus_memory_map.h"

#include "debug.h"


typedef struct
{
	unsigned int		m_nAllocByte;

	unsigned int		m_nBaseAddr;
	unsigned int		m_nMemSize;			// size of DRAM
	unsigned int		m_nCurPointer;
} MEMORY;

MEMORY	g_stMemory[MEM_TYPE_COUNT];

void OSAL_Initialize(void)
{
	// BUFFER MEMORY
	g_stMemory[MEM_TYPE_BUF].m_nAllocByte = 0;
	g_stMemory[MEM_TYPE_BUF].m_nBaseAddr = FW_DATA_BUFFER_START_ADDR;
	g_stMemory[MEM_TYPE_BUF].m_nMemSize = FW_DATA_BUFFER_SIZE;

#ifdef WIN32
	g_stMemory[MEM_TYPE_BUF].m_nBaseAddr = (unsigned int)malloc(g_stMemory[MEM_TYPE_BUF].m_nMemSize);
	ASSERT(g_stMemory[MEM_TYPE_BUF].m_nBaseAddr);
#endif

	g_stMemory[MEM_TYPE_BUF].m_nCurPointer = g_stMemory[MEM_TYPE_BUF].m_nBaseAddr;

	// MEM_TYPE_FW_DATA
	g_stMemory[MEM_TYPE_FW_DATA].m_nAllocByte = 0;
	g_stMemory[MEM_TYPE_FW_DATA].m_nBaseAddr = FW_DRAM_START_ADDR;
	g_stMemory[MEM_TYPE_FW_DATA].m_nMemSize = FW_DRAM_SIZE;

#ifdef WIN32
	g_stMemory[MEM_TYPE_FW_DATA].m_nBaseAddr = (unsigned int)malloc(g_stMemory[MEM_TYPE_FW_DATA].m_nMemSize);
	ASSERT(g_stMemory[MEM_TYPE_FW_DATA].m_nBaseAddr);
#endif

	g_stMemory[MEM_TYPE_FW_DATA].m_nCurPointer = g_stMemory[MEM_TYPE_FW_DATA].m_nBaseAddr;
}

void OSAL_SleepUS(long nMicroSecond)
{
	if (nMicroSecond == 0)
	{
		return;
	}

#ifdef WIN32
	long nMS = CEIL(nMicroSecond, 1000);
	//Sleep(nMS);			// ignore sleep
#else
	usleep(nMicroSecond);
#endif
}

void* OSAL_MemAlloc(MEM_TYPE nMemType, int nSize, int nAlign)
{
#ifdef WIN32
	if (nMemType == MEM_TYPE_WIN32_DEBUG)
	{
		return malloc(nSize);
	}
#endif

	ASSERT(nMemType < MEM_TYPE_COUNT);

	g_stMemory[nMemType].m_nAllocByte += nSize;

	unsigned int	nCurPtr;

	if (nAlign == 0)
	{
		nAlign = OSAL_MEMALLOC_ALIGNMENT;
	}

	g_stMemory[nMemType].m_nCurPointer = ROUND_UP(g_stMemory[nMemType].m_nCurPointer, nAlign);
	nCurPtr = g_stMemory[nMemType].m_nCurPointer;

	g_stMemory[nMemType].m_nCurPointer += nSize;

	ASSERT(g_stMemory[nMemType].m_nCurPointer < (g_stMemory[nMemType].m_nBaseAddr + g_stMemory[nMemType].m_nMemSize));

#ifdef MEM_ALLOC_TRACE
	static int nCount;
	printf("[%d] MemAlloc: %d byte, total allocated: %d\n", nCount, nSize, g_stMemory[nMemType].m_nAllocByte);
#endif

	return (void*)nCurPtr;
}

unsigned long long OSAL_GetCurrentTickUS(void)
{
#ifdef WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	unsigned long long tt = ft.dwHighDateTime;
	tt <<=32;
	tt |= ft.dwLowDateTime;
	tt /=10;
	tt -= 11644473600000000ULL;
	return tt;
#else
	XTime tCur;
	XTime_GetTime(&tCur);		// return microsocond tick 
	return (unsigned long long)tCur / (COUNTS_PER_SECOND / 1000000);
#endif
}



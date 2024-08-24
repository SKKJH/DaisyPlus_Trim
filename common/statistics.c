//#include "cosmos_types.h"
//#include "osal.h"
//#include "debug.h"
//#include "statistics.h"
//
//PROFILE_ENTRY	stStatistics[PROFILE_COUNT];	// counter
//
//void STAT_Initialize(VOID)
//{
//	OSAL_MEMSET(&stStatistics, 0x00, sizeof(stStatistics));
//
//	for (int i = 0; i < PROFILE_COUNT; i++)
//	{
//		stStatistics[i].nType = i;
//	}
//}
//
//unsigned int* _GetProfilePointer(PROFILE_TYPE	eType)
//{
//	DEBUG_ASSERT(eType < PROFILE_COUNT);
//	return &stStatistics[eType].nCount;
//}
//
//void STAT_IncreaseCount(PROFILE_TYPE eType, unsigned int nCnt)
//{
//	INT32 *pnCnt;
//	pnCnt = _GetProfilePointer(eType);
//	*pnCnt += nCnt;
//}
//
//unsigned int STAT_GetCount(PROFILE_TYPE eType)
//{
//	int *pnCnt;
//	pnCnt = _GetProfilePointer(eType);
//	return *pnCnt;
//}
//
//void STAT_Print(void)
//{
//	static const char *apsProfileString[] =
//	{
//		FOREACH_PROFILE(GENERATE_STRING)
//	};
//
//	for (int i = 0; i < PROFILE_COUNT; i++)
//	{
//		PRINTF("%s, %d\n\r", apsProfileString[i], STAT_GetCount(i));
//	}
//}

#include "debug.h"
#include "cosmos_types.h"
#include "util.h"

UINT32 UTIL_Pow(UINT32 a, UINT32 n)
{
	UINT32 r = 1;

	while (n > 0)
	{
		if (n & 1)
		{
			r *= a;
		}

		a *= a;

		n >>= 1;
	}

	return r;
}

int UTIL_GetBitCount(unsigned int nValue)
{
	DEBUG_ASSERT(nValue > 0);
	
	INT32 nRet = 0;
	UINT32 nCur = 0;
	do
	{
		nRet++;
		nCur = 1 << nRet;
	} while (nCur < nValue);

	return nRet;
}

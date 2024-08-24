
#ifndef __UTIL_H__
#define __UTIL_H__

#define MIN(X,Y)			((X) < (Y) ? (X) : (Y))
#define MAX(X,Y)			((X) > (Y) ? (X) : (Y))

#define ROUND_UP(N, S)		((((N) + (S) - 1) / (S)) * (S))			// N: value, S: Alignment
#define ROUND_DOWN(N, S)	((N / S) * S)

#define CEIL(n, d)			(((n) < 0) ? (-((-(n))/(d))) : (n)/(d) + ((n)%(d) != 0))
#define FLOOR(n, d)			(((n) < 0) ? (-((-(n))/(d))) - ((n)%(d) != 0) : (n)/(d))

#define	NBBY			(8)		/* 8 bits per byte */
#define	SETBIT(a, i)	(((UINT8 *)a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	CLEARBIT(a, i)	(((UINT8 *)a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	ISSET(a, i)		((((const UINT8 *)a)[(i)/NBBY] & (1<<((i)%NBBY))) ? TRUE : FALSE)
#define	ISCLEAR(a, i)	((((const UINT8 *)a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

#define INCREASE_IN_RANGE(_Value, _Range)		((_Value + 1) % _Range)

#if defined(__cplusplus) && defined(WIN32)
extern "C" {
#endif

	unsigned int UTIL_Pow(unsigned int a, unsigned int n);
	int UTIL_GetBitCount(unsigned int nValue);

#if defined(__cplusplus) && defined(WIN32)
}
#endif

#endif	// end of #ifndef __UTIL_H__

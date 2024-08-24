#include "random.h"

//typedef unsigned int uint32;
//typedef unsigned long long uint64;

uint32 x[5];

uint32 BRandom() {
	uint64 sum;
	sum = (uint64)2111111111 * (uint64)x[3] +
		(uint64)1492 * (uint64)(x[2]) +
		(uint64)1776 * (uint64)(x[1]) +
		(uint64)5115 * (uint64)(x[0]) +
		(uint64)x[4];
	x[3] = x[2];  x[2] = x[1];  x[1] = x[0];
	x[4] = (uint32)(sum >> 32);            // Carry
	x[0] = (uint32)(sum);                  // Low 32 bits of sum
	return x[0];
}

void RandomInit(uint32 seed)
{
	int i;
	uint32 s = seed;

	// make random numbers and put them into the buffer
	for (i = 0; i < 5; i++)
	{
		s = s * 29943829 - 1;
		x[i] = s;
	}
	// randomize some more
	for (i = 0; i<19; i++) BRandom();
}

// returns a random number between 0 and 1:
double Random()
{
	return (double)BRandom() * (1. / (65536.*65536.));
}

int IRandom(int min, int max)
{
	int r;
	// Output random integer in the interval min <= x <= max
	// Relative error on frequencies < 2^-32
	if (max <= min)
	{
		if (max == min)
		{
			return min;
		}
		else
		{
			return 0x80000000;
		}
	}
	// Multiply interval with random and truncate
	r = (int)((max - min + 1) * Random()) + min;
	if (r > max)
	{
		r = max;
	}
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////
/* Reentrant random function from POSIX.1c.
Copyright (C) 1996, 1999, 2009 Free Software Foundation, Inc.
This file is part of the GNU C Library.
Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with the GNU C Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA.  */


/* This algorithm is mentioned in the ISO C standard, here extended
for 32 bits.  */
int
rand_r(unsigned int *seed)
{
	unsigned int next = *seed;
	int result;

	next *= 1103515245;
	next += 12345;
	result = (unsigned int)(next / 65536) % 2048;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int)(next / 65536) % 1024;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int)(next / 65536) % 1024;

	*seed = next;

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////

unsigned int nSeed;

void rand_s(unsigned int nNewSeed)
{
	nSeed = nNewSeed;
}

int UTIL_Random(void)
{
	return rand_r(&nSeed);
}


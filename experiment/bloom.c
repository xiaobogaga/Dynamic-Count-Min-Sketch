/*-------------------------------------------------------------------------
*
* bloom.c
*	  Bloom Filter implementation.
*
* Copyright (c) 2013-2015, PipelineDB
*
* src/backend/pipeline/bloom.c
*
*-------------------------------------------------------------------------
*/

#include <math.h>
#include "bloom.h"

#define DEFAULT_P 0.02
#define DEFAULT_N (2 << 17) /* 16384 */
#define BUCKET_IDX(bf, i) (((idx) / 64) % (bf)->blen)
#define BIT_MASK(i) (1 << ((i) % 64))

#define MURMUR_SEED 0x99496f1ddc863e6fL

/*
* Murmur is faster than an SHA-based approach and provides as-good collision
* resistance.  The combinatorial generation approach described in
* https://www.eecs.harvard.edu/~michaelm/postscripts/tr-02-05.pdf
* does prove to work in actual tests, and is obviously faster
* than performing multiple iterations of Murmur.
*/

BloomFilter *
BloomFilterCreateWithMAndK(uint32_t m, uint16_t k, long seed)
{
	BloomFilter *bf;
	uint32_t blen = ceil(m / 64.0); /* round m up to nearest uint64_t limit */

	bf = (BloomFilter*) malloc(sizeof(BloomFilter));
	bf->b = malloc(sizeof(uint64_t) * blen);
	memset(bf->b, 0, sizeof(uint64_t) * blen);
	bf->m = m;
	bf->k = k;
	bf->blen = blen;
	bf->seed = seed;

	return bf;
}

BloomFilter *
BloomFilterCreateWithPAndN(float8 p, uint32_t n, long seed)
{
	/* Determine m and k from p and n */
	uint32_t m = -1 * ceil(n * log(p) / (pow(log(2), 2)));
	uint16_t k = round(log(2.0) * m / n);
	return BloomFilterCreateWithMAndK(m, k, seed);
}

BloomFilter *
BloomFilterCreate(void)
{
	return BloomFilterCreateWithPAndN(DEFAULT_P, DEFAULT_N, MURMUR_SEED);
}

BloomFilter * BloomFilterCreateWithSeed(long seed) {
	return BloomFilterCreateWithPAndN(DEFAULT_P, DEFAULT_N, seed);
}

void
BloomFilterDestroy(BloomFilter *bf)
{
	free(bf->b);
	free(bf);
}

void
BloomFilterAdd(BloomFilter *bf, void *key, Size size)
{
	uint32_t i;
	uint64_t hash[2];
	MurmurHash3_128(key, size, bf->seed, &hash);

	for (i = 0; i < bf->k; i++)
	{
		uint64_t h = hash[0] + (i * hash[1]);
		uint32_t idx = h % bf->m;
		bf->b[BUCKET_IDX(bf, idx)] |= BIT_MASK(idx);
	}
}

bool
BloomFilterContains(BloomFilter *bf, void *key, Size size)
{
	uint32_t i;
	uint64_t hash[2];
	MurmurHash3_128(key, size, bf->seed, &hash);

	for (i = 0; i < bf->k; i++)
	{
		uint64_t h = hash[0] + (i * hash[1]);
		uint32_t idx = h % bf->m;
		if (!(bf->b[BUCKET_IDX(bf, idx)] & BIT_MASK(idx)))
			return false;
	}

	return true;
}

BloomFilter *
BloomFilterUnion(BloomFilter *result, BloomFilter *incoming)
{
	uint32_t i;

	for (i = 0; i < result->blen; i++)
		result->b[i] |= incoming->b[i];

	return result;
}

BloomFilter *
BloomFilterIntersection(BloomFilter *result, BloomFilter *incoming)
{
	uint32_t i;

	for (i = 0; i < result->blen; i++)
		result->b[i] &= incoming->b[i];

	return result;
}

Size
BloomFilterSize(BloomFilter *bf)
{
	return (sizeof(uint64_t) * bf->blen);
}


#pragma once

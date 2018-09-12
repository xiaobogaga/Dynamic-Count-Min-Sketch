#pragma once
/*-------------------------------------------------------------------------
*
* bloom.h
*	  Interface for Bloom Filter support
*
* Copyright (c) 2013-2015, PipelineDB
*
* src/include/pipeline/bloom.h
*
*-------------------------------------------------------------------------
*/
#ifndef PIPELINE_BLOOM_H
#define PIPELINE_BLOOM_H

#include "mytypes.h"
#include "hash.h"
#include "mytime.h"

typedef struct BloomFilter
{
	uint32	vl_len_;
	uint32_t m;
	uint16_t k;
	uint32_t blen;
	uint64_t* b;
	long seed;
} BloomFilter;

extern BloomFilter *BloomFilterCreateWithMAndK(uint32_t m, uint16_t k, long seed);
extern BloomFilter *BloomFilterCreateWithPAndN(float8 p, uint32_t n, long seed);
extern BloomFilter *BloomFilterCreate(void);
extern BloomFilter *BloomFilterCreateWithSeed(long seed);
extern void BloomFilterDestroy(BloomFilter *bf);

extern void BloomFilterAdd(BloomFilter *bf, void *key, Size size);
extern bool BloomFilterContains(BloomFilter *bf, void *key, Size size);
extern BloomFilter *BloomFilterUnion(BloomFilter *result, BloomFilter *incoming);
extern BloomFilter *BloomFilterIntersection(BloomFilter *result, BloomFilter *incoming);
extern Size BloomFilterSize(BloomFilter *bf);

#endif

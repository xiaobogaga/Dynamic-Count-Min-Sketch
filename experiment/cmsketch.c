/*-------------------------------------------------------------------------
*
* cmsketch.c
*	  Count-Min Sketch implementation.
*
* Copyright (c) 2013-2015, PipelineDB
*
* src/backend/pipeline/cmsketch.c
*
*-------------------------------------------------------------------------
*/
#include "cmsketch.h"
#include <float.h>

/*
* These values give us an error bound of 0.2% with a confidence of 99.5% and the size
* of the resulting Count-Min Sketch structure is ~31k
*/
#ifndef DCOUNT_PARTIAL
#define DCOUNT_PARTIAL 0.2
#endif

#ifndef DEFAULT_P
#define DEFAULT_P 0.995
#endif

#ifndef DEFAULT_EPS
#define DEFAULT_EPS 0.002
#endif

#ifndef MURMUR_SEED
#define MURMUR_SEED 0x99496f1ddc863e6fL
#endif

CountMinSketch *
CountMinSketchCreateWithDAndW(uint32_t d, uint32_t w, long seed)
{
	CountMinSketch *cms = (CountMinSketch*) malloc(sizeof(CountMinSketch));
	cms->table = (ulong*) malloc((sizeof(ulong) * d * w));
	memset(cms->table, 0, sizeof(ulong) * d * w);
	cms->d = d;
	cms->w = w;
	cms->seed = seed;
	return cms;
}

CountMinSketch *
CountMinSketchCreateWithEpsAndP(float8 epsilon, float8 p, long seed)
{
	uint32_t w = (uint32_t)ceil(exp(1) / epsilon);
	uint32_t d = (uint32_t)ceil(log(1 / (1 - p)));
	return CountMinSketchCreateWithDAndW(d, w, seed);
}

CountMinSketch *
CountMinSketchCreate(long seed)
{
	return CountMinSketchCreateWithEpsAndP(DEFAULT_EPS, DEFAULT_P, seed);
}

void
CountMinSketchDestroy(CountMinSketch *cms)
{
	free(cms->table);
	free(cms);
}

void
CountMinSketchAdd(CountMinSketch *cms, void *key, Size size, ulong count)
{
	/*
	* Since we only have positive increments, we're using the conservative update
	* variant which apparently has better accuracy--albeit moderately slower.
	*
	* http://dimacs.rutgers.edu/~graham/pubs/papers/cmencyc.pdf
	*/
	uint32_t min = UINT_MAX;
	uint32_t i;
	uint64_t hash[2];
	MurmurHash3_128(key, size, cms->seed, &hash);

	for (i = 0; i < cms->d; i++) {
		uint32_t start = i * cms->w;
		uint32_t j = (hash[0] + (i * hash[1])) % cms->w;
		if (min > cms->table[start + j])
			min = cms->table[start + j];
	}

	for (i = 0; i < cms->d; i++) {
		uint32_t start = i * cms->w;
		uint32_t j = (hash[0] + (i * hash[1])) % cms->w;
		if ((min + count) > cms->table[start + j])
			cms->table[start + j] = min + count;
	}
	

	cms->count += count;
}

ulong
CountMinSketchEstimateFrequency(CountMinSketch *cms, void *key, Size size)
{
	ulong count = ULLONG_MAX;
	uint32_t i;
	uint64_t hash[2];
	MurmurHash3_128(key, size, cms->seed, &hash);

	for (i = 0; i < cms->d; i++)
	{
		uint32_t start = i * cms->w;
		uint32_t j = (hash[0] + (i * hash[1])) % cms->w;
		if (count > cms->table[start + j]) {
			count = cms->table[start + j];
		}
	}

	return count;
}

uint64_t
CountMinSketchTotal(CountMinSketch *cms)
{
	return cms->count;
}

float8
CountMinSketchEstimateNormFrequency(CountMinSketch *cms, void *key, Size size)
{
	return 1.0 * CountMinSketchEstimateFrequency(cms, key, size) / cms->count;
}

CountMinSketch *
CountMinSketchMerge(CountMinSketch *result, CountMinSketch* incoming)
{
	uint32_t i;

	if (result->d != incoming->d || result->w != incoming->w)
		printf("cannot merge count-min sketches of different sizes\n");

	for (i = 0; i < result->d * result->w; i++)
		result->table[i] += incoming->table[i];

	result->count += incoming->count;

	return result;
}

Size
CountMinSketchSize(CountMinSketch *cms)
{
	return cms->w * cms->d;
}

ulong RCountMinSketchEstimateCount(RCountMinSketch* sketch, uint32_t depth,
	uint32_t item, Size size) {
	// return an estimate of item at level 
	int j;
	int offset;
	ulong estimate;
	uint64_t hash[2];
	MurmurHash3_128(&item, size, sketch->seed, &hash);

	if (depth >= sketch->levels) return (sketch->count);
	if (depth >= sketch->freelim) {
		// use an exact count if there is one
		return (sketch->table[depth][item]);
	}
	// else, use the appropriate sketch to make an estimate
	offset = 0;
	estimate = ULLONG_MAX;
	for (j = 0; j < sketch->d; j++) {
		offset += sketch->w;
		if (estimate > sketch->table[depth][(hash[0] + (j * hash[1])) % sketch->w + offset]) {
			estimate = sketch->table[depth][(hash[0] + (j * hash[1])) % sketch->w + offset];
		}
	}
	return (estimate);

}

uint64_t RCountMinSketchEstimateRangeSum(RCountMinSketch* sketch, uint32_t start,
	uint32_t end, Size size) {
	int leftend, rightend, i, depth, result, topend;

	topend = 1 << sketch->U;
	if (topend < end) {
		end = topend;
	}
	if ((end > topend) && (start == 0))
		return sketch->count;

	end += 1; // adjust for end effects
	result = 0;
	for (depth = 0; depth <= sketch->levels; depth++) {
		if (start == end) break;
		if ((end - start + 1) < (1 << sketch->gran)) {
			// at the highest level, avoid overcounting	
			for (i = start; i < end; i++)
				result += RCountMinSketchEstimateCount(sketch, depth, i, size);
			break;
		}
		else {
			// figure out what needs to be done at each end
			leftend = (((start >> sketch->gran) + 1) << sketch->gran) - start;
			rightend = (end)-((end >> sketch->gran) << sketch->gran);
			if ((leftend > 0) && (start < end))
				for (i = 0; i < leftend; i++) {
					result += RCountMinSketchEstimateCount(sketch, depth, start + i, size);
				}
			if ((rightend > 0) && (start < end))
				for (i = 0; i < rightend; i++) {
					result += RCountMinSketchEstimateCount(sketch, depth, end - i - 1, size);
				}
			start = start >> sketch->gran;
			if (leftend > 0) start++;
			end = end >> sketch->gran;
		}
	}
	return result;
}

RCountMinSketch* RCountMinSketchCreateWithEpsAndP(float8 epslon, float8 p,
	uint32_t U, uint32_t Gran, long seed) {
	uint32_t w = (uint32_t)ceil(exp(1) / epslon) * (DCOUNT_PARTIAL);
	uint32_t d = (uint32_t)ceil(log(1 / (1 - p)));
	return RCountMinSketchCreateWithDAndW(d, w, U, Gran, seed);
}

RCountMinSketch* RCountMinSketchCreateWithDAndW(uint32_t d, uint32_t w, uint32_t U, uint32_t Gran, long seed) {
	RCountMinSketch* sketch = NULL;
	int i, j;
	uint64_t size = 0;
	if (U <= 0 || U > 32) return (NULL);
	// U is the log the size of the universe in bits

	if (Gran > U || Gran < 1) return (NULL);
	// gran is the granularity to look at the universe in 
	// check that the parameters make sense...
	sketch = (RCountMinSketch*) malloc(sizeof(RCountMinSketch));
	sketch->gran = Gran;
	sketch->U = U;
	sketch->d = d;
	sketch->w = w;
	sketch->count = 0;
	sketch->seed = seed;
	sketch->levels = (int)ceil(((float)U) / ((float)Gran));
	sketch->table = (ulong**) malloc(sizeof(ulong*) * sketch->levels);

	for (j = 0; j < sketch->levels; j++)
		if (1 << (sketch->gran * j) <= sketch->d * sketch->w)
			sketch->freelim = j;

	//find the level up to which it is cheaper to keep exact counts
	sketch->freelim = sketch->levels - sketch->freelim;
	j = 1;
	for (i = sketch->levels - 1; i >= 0; i--) {
		if (i >= sketch->freelim) { // allocate space for representing things exactly at high levels
			sketch->table[i] = (ulong*) malloc(1 << (sketch->gran * j) * sizeof(ulong));
			memset(sketch->table[i], 0, 1 << (sketch->gran * j) * sizeof(ulong));
			size += 1 << (sketch->gran * j);
			j++;
		}
		else { // allocate space for a sketch
			sketch->table[i] = (ulong*) malloc(sizeof(ulong) * sketch->d * sketch->w);
			memset(sketch->table[i], 0, sizeof(ulong) * sketch->d * sketch->w);
			size += sketch->d * sketch->w;
		}
	}

	sketch->size = sizeof(RCountMinSketch) + size + sizeof(ulong*) * sketch->levels;

	return sketch;

}

/*
* for range query. note that the key are always a integer.
*/
RCountMinSketch* RCountMinSketchAdd(RCountMinSketch* sketch, uint32_t* key, Size size,
	ulong count) {
	int i, j, offset;
	uint64_t hash[2];
	uint32_t item = *key;
	MurmurHash3_128(key, size, sketch->seed, &hash);
	sketch->count++;
	for (i = 0; i < sketch->levels; i++) {
		offset = 0;
		if (i >= sketch->freelim) {
			sketch->table[i][item] += count;
			// keep exact counts at high levels in the hierarchy  
		}
		else {
			for (j = 0; j < sketch->d; j++) {
				sketch->table[i][(hash[0] + (j * hash[1])) % sketch->w + offset] += count;
				offset += sketch->d;
			}
		}
		item >>= sketch->gran;
	}
	return sketch;
}

/*
* self-join size implementation for count-min sketch.
*/

/*
double CountMinSketchEstimateSelfJoinSize(CountMinSketch* cms) {
	uint32_t start = 0, j = 0;
	double minimal = LDBL_MAX;
	double* sumArr = (double*) malloc(sizeof(double) * cms->d);
	memset(sumArr, 0, sizeof(double) * cms->d);
	for (j = 0; j < cms->d; j++) {
		start = j * cms->w;
		while (start < ((j + 1) * cms->w)) {
			sumArr[j] += (double) cms->table[start] * (double) cms->table[start];
			start++;
		}
	}
	j = 0;
	while (j < cms->d) {
		if (minimal > sumArr[j]) {
			minimal = sumArr[j];
		}
		j++;
	}
	free(sumArr);
	return minimal;
}
*/

double CountMinSketchEstimateSelfJoinSize(CountMinSketch* cms) {
	uint32_t start = 0, j = 0;
	/*
	ulong minimal = ULLONG_MAX;
	ulong* sumArr = (ulong*)malloc(sizeof(ulong) * cms->d);
	memset(sumArr, 0, sizeof(ulong) * cms->d);
	*/
	double minimal = DBL_MAX;
	double* sumArr = (double*)malloc(sizeof(double) * cms->d);
	memset(sumArr, 0, sizeof(double) * cms->d);

	for (j = 0; j < cms->d; j++) {
		start = j * cms->w;
		while (start < ((j + 1) * cms->w)) {
			// printf("[%d,%d] : %lld \n", j, start, cms->table[start]);
			sumArr[j] += cms->table[start] * cms->table[start];
			start++;
		}
	}
	j = 0;
	while (j < cms->d) {
		// printf("sumArr[%d] : %lld \n", j, sumArr[j]);
		if (minimal > sumArr[j]) {
			minimal = sumArr[j];
		}
		j++;
	}
	free(sumArr);
	return minimal;
}

void RCountMinSketchDestroy(RCountMinSketch* sketch) {
	int i = 0;
	while (i < sketch->levels) {
		free(sketch->table[i++]);
	}
	free(sketch->table);
	free(sketch);
}

Size RCountMinSketchSize(RCountMinSketch* sketch) {
	return sketch->levels * sketch->w * sketch->d;
}

#pragma once

#pragma/*-------------------------------------------------------------------------
 *
 * cmsketch.h
 *	  Interface for Count-Min Sketch support
 *
 * Copyright (c) 2013-2015, PipelineDB
 *
 * src/include/pipeline/cmsketch.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PIPELINE_CMSKETCH_H
#define PIPELINE_CMSKETCH_H

#include "mytypes.h"
#include "hash.h"

 /*
 * Unfortunately we can't do top-K with Count-Min Sketch for continuous
 * queries because it will require us to store O(n) values. This isn't a
 * problem in the case when Count-Min Sketch structures are not being
 * merged in which case only O(1) values need to be stored.
 */

typedef struct CountMinSketch
{
	uint32	vl_len_;
	ulong count;
	uint32_t d;
	uint32_t w;
	ulong* table;
	long seed;
	int dynamic;
} CountMinSketch;

/*
* count-min sketch structure modification for range sum query.
*/
typedef struct RCountMinSketch {
	uint32_t U;
	uint32_t gran;
	uint32_t levels;
	uint32_t freelim;
	uint32_t d;
	uint32_t w;
	ulong count;
	ulong** table;
	uint32_t size;
	long seed;
	int dynamic;
} RCountMinSketch;

extern CountMinSketch *CountMinSketchCreateWithDAndW(uint32_t d, uint32_t w, long seed);
extern CountMinSketch *CountMinSketchCreateWithEpsAndP(float8 epsilon, float8 p, long seed);
extern CountMinSketch *CountMinSketchCreate(long seed);
extern void CountMinSketchDestroy(CountMinSketch *cms);

extern void CountMinSketchAdd(CountMinSketch *cms, void *key, Size size, ulong count);
extern ulong CountMinSketchEstimateFrequency(CountMinSketch *cms, void *key, Size size);
extern float8 CountMinSketchEstimateNormFrequency(CountMinSketch *cms, void *key, Size size);
extern uint64_t CountMinSketchTotal(CountMinSketch *cms);
extern CountMinSketch *CountMinSketchMerge(CountMinSketch *result, CountMinSketch* incoming);
extern Size CountMinSketchSize(CountMinSketch *cms);
// extern double CountMinSketchEstimateSelfJoinSize(CountMinSketch* cms);
extern double CountMinSketchEstimateSelfJoinSize(CountMinSketch* cms);


extern uint64_t RCountMinSketchEstimateRangeSum(RCountMinSketch* sketch, uint32_t start, uint32_t end, Size size);
extern ulong RCountMinSketchEstimateCount(RCountMinSketch* sketch, uint32_t dpeth, uint32_t item, Size size);
extern RCountMinSketch* RCountMinSketchCreateWithEpsAndP(float8 epslon, float8 p,
	uint32_t U, uint32_t Gran, long seed);
extern RCountMinSketch* RCountMinSketchCreateWithDAndW(uint32_t d, uint32_t w,
	uint32_t U, uint32_t Gran, long seed);
extern RCountMinSketch* RCountMinSketchAdd(RCountMinSketch* sketch, uint32_t* key, Size size,
	ulong count);

extern void RCountMinSketchDestroy(RCountMinSketch* sketch);

extern Size RCountMinSketchSize(RCountMinSketch* sketch);


#endif

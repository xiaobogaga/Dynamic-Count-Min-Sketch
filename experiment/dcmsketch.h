/*
* a simple dynamic count-min sketch implementation
*
*/

#ifndef PIPELINEDB_DCMSKETCH_H_
#define PIPELINEDB_DCMSKETCH_H_

#include "cmsketch.h"
#include "hll.h"
//#include "myhll.h"
#include "bloom.h"
#include "hash.h"

/*
* define the dynamic count-min sketch here.
* since the dynamic count-min sketch is chained, we declare it as a linked list
*/

typedef struct CountMinSketchNode {
	CountMinSketch* sketch;
	struct CountMinSketchNode* prev;
} CountMinSketchNode;

typedef struct BloomFilterNode {
	BloomFilter* bf;
	struct BloomFilterNode* prev;
} BloomFilterNode;

/*
	used for point query procedure and self-join size query.
*/
typedef struct DBCountMinSketchChain {
	CountMinSketchNode* sketch;
	BloomFilterNode* bf;
	uint64_t GIC;
	uint64_t GCC;
	uint64_t PrevGCC;
	int32_t Length;
	struct HLL hyperloglog;
	// MyHLL* hyperloglog;
	// MapHLL* hyperloglog;
	int dynamic;
	int isFixedLength;
	int MaxLength;
	int round;
	int queryType;
	int initSize;
	int currentSize;
	int currentCardinality;
	int skewed;
	int increaseType;
	int k;
	int initCardinality;
	float8 epsilon;
} DBCountMinSketchChain;

/*
	create a DBCountMinSketchChain for specific query:
	1 --> point query
	2 --> self join size query
	when the chain is for point query, then the initSize is needed, the increase Type is the increase type for data size or data cardinality
	, of course, the initCardinaty is to sepcific the initial cardinality for the self join size query.
	increase type :
	1 --> a_j = j * a_1;
	2 --> a_j = j * k * a_1;
	3 --> a_j = 2^j * a_1

	when the "skewed" parameter is 1, means we want to test point query under data is skewed
*/
extern DBCountMinSketchChain* DBCountMinSketchCreateWithDAndW(float8 epsilon, uint32_t d, uint32_t w, int dynamic, int isFixedLength,
	int maxLength, int queryType, int initSize, int currentSize, int skewed, int increaseType, int k, int initCaridinaty, int currentCardinality);
extern DBCountMinSketchChain* DBCountMinSketchCreateWithEpsAndP(float8 epsilon, uint32_t d, int dynamic,
	int isFixedLength, int maxLength, int queryType, int initSize, int skewed, int increaseType, int k, int initCaridinaty);
extern void DBCountMinSketchAdd(DBCountMinSketchChain* dbcms, void* key, Size size, uint32_t count);

extern ulong DBCountMinSketchEstimateFrequency(DBCountMinSketchChain* dbcms, void *key, Size size);
// extern double DBCountMinSketchEstimateSelfJoinSize(DBCountMinSketchChain* dbcms);
extern double DBCountMinSketchEstimateSelfJoinSize(DBCountMinSketchChain* dbcms);

extern Size DBCountMinSketchChainSize(DBCountMinSketchChain* chain);
extern void DBCountMinSketchChainDestroy(DBCountMinSketchChain* chain);

#endif
#pragma once

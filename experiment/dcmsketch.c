/*
* a simple implementation for dynamic count-min sketch
*/

#include <limits.h>
#include <math.h>
#include "dcmsketch.h"
#include "mytime.h"
#include <float.h>

#ifndef DCOUNT_PARTIAL
#define DCOUNT_PARTIAL 0.2
#endif

#ifndef MURMUR_SEED
#define MURMUR_SEED 0x99496f1ddc863e6fL
#endif

DBCountMinSketchChain*
DBCountMinSketchCreateWithDAndW(float8 epsilon, uint32_t d, uint32_t w, int dynamic, int isFixedLength, 
	int maxLength, int queryType, int initSize, int currentSize, int skewed, int increaseType, int k, int initCaridinaty, int currentCardinality) {
	DBCountMinSketchChain *dbcms = (DBCountMinSketchChain*) malloc(sizeof(DBCountMinSketchChain));
	dbcms->epsilon = epsilon;
	dbcms->GCC = 0;
	dbcms->GIC = 0;
	dbcms->PrevGCC = 0;
	dbcms->Length = 1;
	dbcms->isFixedLength = isFixedLength;
	dbcms->MaxLength = maxLength;
	dbcms->dynamic = dynamic;
	dbcms->round = 0;
	dbcms->queryType = queryType;
	dbcms->initSize = initSize;
	dbcms->currentSize = currentSize;
	dbcms->skewed = skewed;
	dbcms->increaseType = increaseType;
	dbcms->k = k;
	dbcms->initCardinality = initCaridinaty;
	dbcms->currentCardinality = currentCardinality;
	dbcms->sketch = (CountMinSketchNode*)malloc(sizeof(CountMinSketchNode));
	dbcms->sketch->prev = NULL;
	if (dynamic) {
		dbcms->sketch->sketch = CountMinSketchCreateWithDAndW(d, w, getSystemTime());
	}
	else {
		dbcms->sketch->sketch = CountMinSketchCreateWithDAndW(d, w, MURMUR_SEED);
	}
	if (queryType == 2) {
		// dbcms->hyperloglog = MapHLLCreate(dbcms->initCardinality);
		hll_init(&dbcms->hyperloglog, 14);
		dbcms->bf = (BloomFilterNode*)malloc(sizeof(BloomFilterNode));
		dbcms->bf->prev = NULL;
		if (dynamic) {
			dbcms->bf->bf = BloomFilterCreate(getSystemTime());
		}
		else {
			dbcms->bf->bf = BloomFilterCreate();
		}
	}
	else {
		if (dbcms->skewed) {
			// dbcms->hyperloglog = MapHLLCreate(dbcms->initCardinality);
			hll_init(&dbcms->hyperloglog, 14);
		}
	}
	return dbcms;
}

DBCountMinSketchChain*
DBCountMinSketchCreateWithEpsAndP(float8 epsilon, uint32_t d, int dynamic, 
	int isFixedLength, int maxLength, int queryType, int initSize, int skewed, 
	int increaseType, int k, int initCaridinaty) {
	int currentSize = 0;
	int currentCardinality = 0;
	uint32_t w;
	if (queryType == 1) {
		if (skewed == 0) {

			if (increaseType == 1) {
				w = ceil(exp(1) / (epsilon * (1 / (float) 1)));
				currentSize = initSize * 1;
			}
			else if (increaseType == 2) {
				w = ceil(exp(1) / (epsilon * (1 / (float)(k * 1))));
				currentSize = initSize * 1 * k;
			}
			else {
				w = ceil(exp(1) / (epsilon * (1 / pow(2, 1))));
				currentSize = initSize * pow(2, 1);
			}
			
		}
		else if (skewed == 1) {
			
			if (increaseType == 1) {
				currentCardinality = initCaridinaty * 1;
				w = (uint32_t)ceil(exp(1) / (epsilon));
			}
			else if (increaseType == 2) {
				currentCardinality = 1 * initCaridinaty * k;
				w = (uint32_t)ceil(exp(1) / (epsilon * (1 / (float)((float)1 * (float)k))));
			}
			else {
				currentCardinality = pow(2, 1) * initCaridinaty;
				w = (uint32_t)ceil(exp(1) / (epsilon * (1 / pow(2, 1))));
			}

		}
	}
	else {
		w = ceil(exp(1) / epsilon);
		// printf("w : %d \n", w);
	}

	return DBCountMinSketchCreateWithDAndW(epsilon, d, w, dynamic, isFixedLength, maxLength, queryType, initSize, currentSize, skewed, increaseType, k, initCaridinaty, currentCardinality);
}

void DBCountMinSketchAdd(DBCountMinSketchChain* dbcms, void* key, Size size, uint32_t count) {
	int result;
	BloomFilterNode* bfNode;
	CountMinSketchNode* sketchNode;
	CountMinSketchNode* headSketchNode = NULL;
	BloomFilterNode* headBfNode = NULL;
	dbcms->GIC++;
	int i = 0;

	if (dbcms->queryType == 1) {
		if (dbcms->skewed == 0) {

			if (dbcms->GIC <= dbcms->currentSize) {
				CountMinSketchAdd(dbcms->sketch->sketch, key, size, count);
			}
			else {
				// need to create new CountMinSketch.
				dbcms->PrevGCC = dbcms->GCC;
				dbcms->GCC = dbcms->GIC = 1;
				dbcms->Length++;
				uint32_t w;
				if (dbcms->increaseType == 1) {
					dbcms->currentSize= dbcms->Length * dbcms->initSize;
					w = (uint32_t)ceil(exp(1) / (dbcms->epsilon * (1 / (float)dbcms->Length)));
				}
				else if (dbcms->increaseType == 2) {
					dbcms->currentSize = dbcms->Length * dbcms->initSize * dbcms->k;
					w = (uint32_t)ceil(exp(1) / (dbcms->epsilon * (1 / (float)((float)dbcms->Length * (float)dbcms->k))));
				}
				else {
					dbcms->currentSize = pow(2, dbcms->Length) * dbcms->initSize;
					w = (uint32_t)ceil(exp(1) / (dbcms->epsilon * (1 / pow(2, dbcms->Length))));
				}
				sketchNode = dbcms->sketch;
				dbcms->sketch = (CountMinSketchNode*)malloc(sizeof(CountMinSketchNode));
				dbcms->sketch->prev = sketchNode;
				if (dbcms->dynamic) {
					dbcms->sketch->sketch = CountMinSketchCreateWithDAndW(sketchNode->sketch->d, w, getSystemTime() * dbcms->Length * 13);
				}
				else {
					dbcms->sketch->sketch = CountMinSketchCreateWithDAndW(sketchNode->sketch->d,
						w, MURMUR_SEED);
				}
				CountMinSketchAdd(dbcms->sketch->sketch, key, size, count);
			}
		}
		else {
			// for skewed data.
			if (dbcms->GCC < dbcms->currentCardinality) {
				CountMinSketchAdd(dbcms->sketch->sketch, key, size, count);
				// MapHLLAdd(dbcms->hyperloglog, key, size);
				// dbcms->GCC = MapHLLCardinality(dbcms->hyperloglog);
				hll_add(&dbcms->hyperloglog, key, size);
				dbcms->GCC = (uint64_t)hll_count(&dbcms->hyperloglog);
			}
			else {
				// need to create new CountMinSketch.
				dbcms->PrevGCC = dbcms->GCC;
				dbcms->GCC = dbcms->GIC = 1;
				dbcms->Length++;
				sketchNode = dbcms->sketch;
				dbcms->sketch = (CountMinSketchNode*)malloc(sizeof(CountMinSketchNode));
				dbcms->sketch->prev = sketchNode;

				uint32_t w;
				if (dbcms->increaseType == 1) {
					dbcms->currentCardinality = dbcms->initCardinality * dbcms->Length;
					w = (uint32_t)ceil(exp(1) / (dbcms->epsilon * (1 / (float)dbcms->Length)));
				}
				else if (dbcms->increaseType == 2) {
					dbcms->currentCardinality = dbcms->Length * dbcms->initCardinality * dbcms->k;
					w = (uint32_t)ceil(exp(1) / (dbcms->epsilon * (1 / (float)((float)dbcms->Length * (float)dbcms->k))));
				}
				else {
					dbcms->currentCardinality = pow(2, dbcms->Length) * dbcms->initCardinality;
					w = (uint32_t)ceil(exp(1) / (dbcms->epsilon * (1 / pow(2, dbcms->Length))));
				}

				if (dbcms->dynamic) {
					dbcms->sketch->sketch = CountMinSketchCreateWithDAndW(sketchNode->sketch->d, w, getSystemTime() * dbcms->Length * 13);
				}
				else {
					dbcms->sketch->sketch = CountMinSketchCreateWithDAndW(sketchNode->sketch->d,
						w, MURMUR_SEED);
				}
				CountMinSketchAdd(dbcms->sketch->sketch, key, size, count);

				// free previous hyperLog, and add the new element.
				// MapHllDestroy(dbcms->hyperloglog);
				// dbcms->hyperloglog = MapHLLCreate(dbcms->initCardinality);
				// MapHLLAdd(dbcms->hyperloglog, key, size);
				hll_destroy(&dbcms->hyperloglog);
				hll_init(&dbcms->hyperloglog, 14);
				hll_add(&dbcms->hyperloglog, key, size);
			}
		}
	}
	else {
		// search for the specific element before inserting this element.
		bfNode = dbcms->bf;
		sketchNode = dbcms->sketch;
		while (bfNode != NULL) {
			// since contains this element, add this element.
			// otherwise forward to search
			if (BloomFilterContains(bfNode->bf, key, size)) {
				CountMinSketchAdd(sketchNode->sketch, key, size, count);
				BloomFilterAdd(bfNode->bf, key, size);
				return;
			}
			else {
				bfNode = bfNode->prev;
				sketchNode = sketchNode->prev;
			}
		}

		if (dbcms->GCC >= 9999) {
//			printf("catch it");
		}


		if (dbcms->GCC <= dbcms->initCardinality) {
			CountMinSketchAdd(dbcms->sketch->sketch, key, size, count);
			BloomFilterAdd(dbcms->bf->bf, key, size);
			// MapHLLAdd(dbcms->hyperloglog, key, size);
			// dbcms->GCC = MapHLLCardinality(dbcms->hyperloglog);
			hll_add(&dbcms->hyperloglog, key, size);
			dbcms->GCC = (uint64_t) hll_count(&dbcms->hyperloglog);
		}
		else {
			// need to create new CountMinSketch.
			dbcms->PrevGCC = dbcms->GCC;
			dbcms->GCC = dbcms->GIC = 1;
			dbcms->Length++;
			sketchNode = dbcms->sketch;
			uint32_t w = sketchNode->sketch->w;
			dbcms->sketch = (CountMinSketchNode*)malloc(sizeof(CountMinSketchNode));
			dbcms->sketch->prev = sketchNode;
			

			if (dbcms->dynamic) {
				dbcms->sketch->sketch = CountMinSketchCreateWithDAndW(sketchNode->sketch->d, w, getSystemTime() * dbcms->Length * 13);
			}
			else {
				dbcms->sketch->sketch = CountMinSketchCreateWithDAndW(sketchNode->sketch->d,
					w, MURMUR_SEED);
			}
			bfNode = dbcms->bf;
			dbcms->bf = (BloomFilterNode*)malloc(sizeof(BloomFilterNode));
			dbcms->bf->prev = bfNode;
			if (dbcms->dynamic) {
				dbcms->bf->bf = BloomFilterCreateWithSeed(getSystemTime() * dbcms->Length * 13);
			}
			else {
				dbcms->bf->bf = BloomFilterCreate();
			}

			CountMinSketchAdd(dbcms->sketch->sketch, key, size, count);
			BloomFilterAdd(dbcms->bf->bf, key, size);

			// free previous hyperLog, and add the new element.
			// MapHllDestroy(dbcms->hyperloglog);
			// dbcms->hyperloglog = MapHLLCreate(dbcms->initCardinality);
			// MapHLLAdd(dbcms->hyperloglog, key, size);
			hll_destroy(&dbcms->hyperloglog);
			hll_init(&dbcms->hyperloglog, 14);
			hll_add(&dbcms->hyperloglog, key, size);
		}
	}
}


Size DBCountMinSketchChainSize(DBCountMinSketchChain* chain) {
	CountMinSketchNode* node = chain->sketch;
	int total = 0;
	while (node != NULL) {
		total += CountMinSketchSize(node->sketch);
		node = node->prev;
	}
	return total;
}

/*
* estimate the frequency namely, point query implementation
*/
ulong DBCountMinSketchEstimateFrequency(DBCountMinSketchChain* dbcms, void *key, Size size) {
	CountMinSketchNode* sketchNode = dbcms->sketch;
	ulong totalFre = 0;
	while (sketchNode != NULL) {
		totalFre += CountMinSketchEstimateFrequency(sketchNode->sketch, key, size);
		sketchNode = sketchNode->prev;
	}
	return totalFre;
}

double DBCountMinSketchEstimateSelfJoinSize(DBCountMinSketchChain* dbcms) {
	CountMinSketchNode* sketchNode = dbcms->sketch;
	CountMinSketch* sketch = NULL;
	int j = 0;
	uint32_t start = 0;

	/*
	ulong minimal = ULLONG_MAX;
	ulong* sumArr = (ulong*)malloc(sizeof(ulong) * dbcms->sketch->sketch->d);
	memset(sumArr, 0, sizeof(ulong) * dbcms->sketch->sketch->d);
	*/
	double minimal = DBL_MAX;
	// double* sumArr = (double*)malloc(sizeof(double) * dbcms->sketch->sketch->d);
	// memset(sumArr, 0, sizeof(double) * dbcms->sketch->sketch->d);
	double sumArr[3];
	sumArr[0] = 0, sumArr[1] = 0, sumArr[2] = 0;
	while (sketchNode != NULL) {
		sketch = sketchNode->sketch;
		for (j = 0; j < sketch->d; j++) {
			start = j * sketch->w;
			while (start < ((j + 1) * sketch->w)) {
				// printf("[%d,%d] : %lld \n", j, start, sketch->table[start]);
				sumArr[j] += sketch->table[start] * sketch->table[start];
				start++;
			}
		}
		sketchNode = sketchNode->prev;
	}
	j = 0;
	while (j < dbcms->sketch->sketch->d) {
		if (minimal > sumArr[j]) {
			minimal = sumArr[j];
		}
		j++;
	}

//	free(sumArr);
	return minimal;
}

void DBCountMinSketchChainDestroy(DBCountMinSketchChain* chain) {
	CountMinSketchNode* node = chain->sketch;
	BloomFilterNode* bfnode = chain->bf;
	CountMinSketchNode* temp = NULL;
	BloomFilterNode* bfTemp;
	if (chain->queryType == 1) {
		while (node != NULL) {
			temp = node;
			node = node->prev;
			CountMinSketchDestroy(temp->sketch);
			free(temp);
		}
		if (chain->skewed == 1)
			// MapHllDestroy(chain->hyperloglog);
			hll_destroy(&chain->hyperloglog);
	}
	else {
		while (node != NULL) {
			temp = node;
			bfTemp = bfnode;
			node = node->prev;
			bfnode = bfnode->prev;
			CountMinSketchDestroy(temp->sketch);
			BloomFilterDestroy(bfTemp->bf);
			free(temp);
			free(bfTemp);
		}
		hll_destroy(&chain->hyperloglog);
		// MapHllDestroy(chain->hyperloglog);
	}
	free(chain);
}

#pragma once

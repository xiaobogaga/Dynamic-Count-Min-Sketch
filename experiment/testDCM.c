#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "prng.h"
#include "dcmsketch.h"
#include "pbl.h"
#include "mytime.h"
#include "uniform.h"
#include "normal.h"

/*
	this file is used mainly for experiment1 which is used to  test the impact of p , N , r on memory space.
	By generating settable skew data, we can verify the impact of these variables under different data size and different parameter setting. 
*/

int * CreateStream(int length, int n, float zipfpar, int* stream, int* exact, int seed)
{
	long a, b;
	float zet;
	int i;
	long value;
	int netpos;
	prng_type * prng;
	

	// n = 1048575;

	prng = prng_Init(seed, 2);
	a = (long long)(prng_int(prng) % MOD);
	b = (long long)(prng_int(prng) % MOD);

	netpos = 0;

	zet = zeta(length, zipfpar);

	for (i = 1; i <= length; i++) {
		value = (hash31(a, b, ((int)floor(fastzipf(zipfpar, n, zet, prng)))) & 1048575);
		exact[value]++;
		netpos++;
		stream[i] = value;
		// printf("Stream %d is %d \n",i,value);
	}

	prng_Destroy(prng);

	return (stream);

}


void saveDataToFile(char* absoluteFileName, int* data, int size) {
	FILE* f = fopen(absoluteFileName, "w");
	if (f == NULL) {
		printf("open file failed\n");
		return;
	}
	else {
		int i = 0;
		while (i < size) {
			fprintf(f, "%d,%d\n", i + 1, data[i]);
			i++;
		}
		fflush(f);
	}
	fclose(f);
}

/*
	test the self join size query error change with the skew of data
*/
void experiment11(int pointQueryTestTime, int range, float8 zipfparInit, int dynamic) {
	DBCountMinSketchChain* selfJoinSizeQueryDcm;
	CountMinSketch* selfJoinSizeQueryCM;

	int* stream;
	int* exact;
	int seed;
	int n = 1048575;
	int* times;
	int* timeValue;
	int timeValueLength;
	int distinctIndicator = 0;
	PblMap* datatimes;
	PblIterator* iterator;
	PblMapEntry* keyValue;
	int* data;
	int* time;
	uint32_t realtime = 0;

	int start = 0;
	int specificKey, specificKeyTime = 0;
	int i = 0;
	uint32_t dcmD = 3;
	uint32_t cmD = 3, cmW = 0;
	datatimes = pblMapNewHashMap();
	int dcmMemory = 0;
	int* uniformData;
	int* normalData;
	int uniformDataSeed, normalDataSeed;
	int low = 0, high = 20000;
	ulong dcmselfJoinSize;
	double dcmselfJoinSizeError;
	ulong dcmrealSelfJoinSize;
	double dcmselfJoinError;
	ulong cmselfJoinSize;
	double cmselfJoinSizeError;
	ulong cmrealSelfJoinSize;
	double cmselfJoinError;

	float8 zipfpar = zipfparInit, zipfparTill = 5.1, zipfparStep = 0.1;
	int N = range;
	int skewed = 1;
	int queryType = 2;
	int increaseType = 1;
	float8 epsilon = 0.01;
	float part1 = 0.2;
	float k = 10;
	float r = 2;
	float alpha = 0.1;
	int c1 = (1 - alpha) * N * part1;
	float e = 2.7182;
	int d = 3;
	cmW = ceil(exp(1) / epsilon);
	int cmMemory = cmD * cmW;
	float reallypointqueryEpsilon;
	float reallySelfjoinSizeEpsilon;
	int initCardinaty = c1;
	float totalCardinality;
	int initCMMemory = 0, initDCMMemory = 0;
	reallySelfjoinSizeEpsilon = epsilon * (1 / part1);
	printf("self join size query settings : c : %d, epsilon : %f \n", c1, reallySelfjoinSizeEpsilon);

	/*
	first test point query.
	*/

	while (zipfpar <= zipfparTill) {

			totalCardinality = 0;
			start = 0;
			dcmMemory = 0;
			dcmselfJoinError = 0;
			cmselfJoinError = 0;
			while (start < pointQueryTestTime) {
				dcmrealSelfJoinSize = 0;
				cmrealSelfJoinSize = 0;
				distinctIndicator = 0;
				times = (int*)malloc(sizeof(int) * range);
				for (i = 0; i < range; i++)
					times[i] = 1;
				i = 0;
				exact = (int *)calloc(n + 1, sizeof(int));
				stream = (int *)calloc(range + 1, sizeof(int));
				seed = getSystemTime();
				if (seed < 0) {
					seed = -1 * seed;
				}
				stream = CreateStream(range, n, zipfpar, stream, exact, seed);
				specificKey = stream[88];
				specificKeyTime = 0;
				if (dynamic) {
					selfJoinSizeQueryCM = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					selfJoinSizeQueryCM = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}	
				selfJoinSizeQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallySelfjoinSizeEpsilon, dcmD, dynamic, 0, 0, 2, 0, 0, increaseType, k, initCardinaty);
				initCMMemory = CountMinSketchSize(selfJoinSizeQueryCM);
				initDCMMemory = DBCountMinSketchChainSize(selfJoinSizeQueryDcm);
				i = 0;
				while (i < range) {
					if (stream[i] < 0) {
						printf("opps, value < 0");
					}
					CountMinSketchAdd(selfJoinSizeQueryCM, &stream[i], 4, 1);
					DBCountMinSketchAdd(selfJoinSizeQueryDcm, &stream[i], 4, 1);
					if (pblMapContainsKey(datatimes, &stream[i], 4)) {
						timeValue = (int*)pblMapGet(datatimes, &stream[i], 4, &timeValueLength);
						(*timeValue) = (*timeValue) + 1;
						pblMapAdd(datatimes, &stream[i], 4, timeValue, 4);
					}
					else {
						pblMapAdd(datatimes, &stream[i], 4, &times[distinctIndicator], 4);
						distinctIndicator++;
					}
					if (specificKey == stream[i]) {
						specificKeyTime++;
					}
					i++;
				}

				// check whether the map is correct.
				realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
				if (specificKeyTime != realtime) {
					printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
					return;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					data = (int*)pblMapEntryKey(keyValue);
					time = (int*)pblMapEntryValue(keyValue);
					cmrealSelfJoinSize += (*time) * (*time);
					dcmrealSelfJoinSize += (*time) * (*time);
					i++;
				}

				dcmselfJoinSize = DBCountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryDcm);
				if (dcmrealSelfJoinSize >= dcmselfJoinSize) {
					dcmselfJoinError += (double)(dcmrealSelfJoinSize - dcmselfJoinSize);
				}
				else {
					dcmselfJoinError += (double)(dcmselfJoinSize - dcmrealSelfJoinSize);
				}


				cmselfJoinSize = CountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryCM);
				if (cmrealSelfJoinSize >= cmselfJoinSize) {
					cmselfJoinError += (double)(cmrealSelfJoinSize - cmselfJoinSize);
				}
				else {
					cmselfJoinError += (double)(cmselfJoinSize - cmrealSelfJoinSize);
				}

				totalCardinality += i;
				dcmMemory += DBCountMinSketchChainSize(selfJoinSizeQueryDcm);
				pblIteratorFree(iterator);
				pblMapClear(datatimes);
				free(times);
				free(stream);
				free(exact);
				DBCountMinSketchChainDestroy(selfJoinSizeQueryDcm);
				CountMinSketchDestroy(selfJoinSizeQueryCM);
				start++;
			}

			totalCardinality = totalCardinality / pointQueryTestTime;
			dcmMemory = dcmMemory / pointQueryTestTime;
			cmselfJoinError = cmselfJoinError / pointQueryTestTime;
			dcmselfJoinError = dcmselfJoinError / pointQueryTestTime;
			printf("[zipfian-self join size query] : with [skew : %f] , cmerror: %f, dcmerror : %f, cmMemory : %d, dcmMemory: %d, initCMMemory : %d, initDCMMemory : %d, cardinality : %f \n",
				zipfpar, cmselfJoinError, dcmselfJoinError, cmMemory, dcmMemory, initCMMemory, initDCMMemory,totalCardinality);
			zipfpar += zipfparStep;
		}
		
	

}

/*
	test the point query error change with the skew of data.
*/
void experiment12(int pointQueryTestTime, int range, float8 zipfparInit, int dynamic) {
	DBCountMinSketchChain* pointQueryDcm;
	CountMinSketch* pointQueryCM;

	int* stream;
	int* exact;
	int seed;
	int n = 1048575;
	int* times;
	int* timeValue;
	int timeValueLength;
	int distinctIndicator = 0;
	PblMap* datatimes;
	PblIterator* iterator;
	PblMapEntry* keyValue;
	int* data;
	int* time;
	uint32_t realtime = 0;

	int start = 0;
	int specificKey, specificKeyTime = 0;
	int i = 0;
	uint32_t dcmD = 3;
	uint32_t cmD = 3, cmW = 0;
	float8 dcmerror = 0, cmerror = 0;
	float8 totalDCMerror = 0, totalCMerror = 0;
	datatimes = pblMapNewHashMap();
	int dcmMemory = 0;
	int* uniformData;
	int* normalData;
	int uniformDataSeed, normalDataSeed;
	int low = 0, high = 300;

	float8 zipfpar = zipfparInit, zipfparTill = 5.1, zipfparStep = 0.1;
	int N = range;
	int skewed = 1;
	int queryType = 2;
	int increaseType = 1;
	float8 epsilon = 0.01;
	float part1 = 0.2;
	float k = 10;
	float r = 2;
	float alpha = 0.1;
	int c1 = N * part1 * (1-alpha), c2 = N * (1-alpha) * part1, c3 = N * (1-alpha) * part1;
	int length1 = ceil(sqrt((N * 2 * (1-alpha) / (float) c1) + 0.25) - 0.5), 
		length2 = ceil(sqrt((N * 2 * (1-alpha) / (float) (k * c2)) + 0.25) - 0.5),
		length3 = ceil(log((r - 1) * (1-alpha) * N / (float) c3 + r) / log(r) - 1);
	float epsilon1 = epsilon * N * 2 * length1 * (1 - alpha) / (float) (length1 * (length1 - 1) * c1 + 2 * (1 - alpha) * N),
		epsilon2 = epsilon * k * length2 * N / (float) (N + k * length2 * (length2 - 1) * c2 / (float) (2 * (1 - alpha))),
		epsilon3 = epsilon * N * pow(r, length3) / (float) (N + c3 / (float) (1 - alpha) * (length3 * pow(r, length3) - r * (pow(r, length3) - 1) / (float) (r - 1)));
	float e = 2.7182;
	int d = 3;
	cmW = ceil(exp(1) / epsilon);
	int cmMemory = cmD * cmW;
	float reallypointqueryEpsilon;
	int initCardinaty;
	float totalCardinality;
	printf("point query settings : length1 : %d, length2 : %d, length3 : %d, c1 : %d, c2 : %d, c3 : %d, epsilon1 : %f, epsilon2 : %f, epsilon3 : %f \n", 
		length1, length2, length3, c1, c2, c3, epsilon1, epsilon2, epsilon3);
	int initCMMemory, initDCMMemory;


	/*
		first test point query.
	*/
	
	while (zipfpar <= zipfparTill) {

		increaseType = 1;
		while (increaseType <= 3) {

			if (increaseType == 1) {
				reallypointqueryEpsilon = epsilon1;
				initCardinaty = c1;
				increaseType++;
				continue;
			}
			else if (increaseType == 2) {
				reallypointqueryEpsilon = epsilon2;
				initCardinaty = c2;
				increaseType++;
				continue;
			}
			else {
				reallypointqueryEpsilon = epsilon3;
				initCardinaty = c3;
			}
			totalCardinality = 0;
			start = 0;
			dcmMemory = 0;
			totalDCMerror = 0, totalCMerror = 0;
			dcmerror = 0, cmerror = 0;
			while (start < pointQueryTestTime) {
				distinctIndicator = 0;
				times = (int*)malloc(sizeof(int) * range);
				memset(times, 0, sizeof(int) * range);
				i = 0;
				exact = (int *)calloc(n + 1, sizeof(int));
				stream = (int *)calloc(range + 1, sizeof(int));
				seed = getSystemTime();
				if (seed < 0) {
					seed = -1 * seed;
				}
				stream = CreateStream(range, n, zipfpar, stream, exact, seed);
				// char fileName[100];
				// snprintf(fileName, 99, "C://Users/andrew/Desktop/mywork/paper/experiments/code/tempData/zipfian-%f.txt", zipfpar);
				// saveDataToFile(fileName, stream, range);
				specificKey = stream[88];
				specificKeyTime = 0;
				if (dynamic) {
					pointQueryCM = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					pointQueryCM = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}
				pointQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallypointqueryEpsilon, dcmD, dynamic, 0, 0, 1, 0, 1, increaseType, k, initCardinaty);

				initCMMemory = CountMinSketchSize(pointQueryCM);
				initDCMMemory = DBCountMinSketchChainSize(pointQueryDcm);


				i = 0;
				while (i < range) {
					if (stream[i] < 0) {
						printf("opps, value < 0");
					}
					CountMinSketchAdd(pointQueryCM, &stream[i], 4, 1);
					DBCountMinSketchAdd(pointQueryDcm, &stream[i], 4, 1);
					if (pblMapContainsKey(datatimes, &stream[i], 4)) {
						timeValue = (int*)pblMapGet(datatimes, &stream[i], 4, &timeValueLength);
						(*timeValue) = (*timeValue) + 1;
						pblMapAdd(datatimes, &stream[i], 4, timeValue, 4);
					}
					else {
						times[distinctIndicator] = 1;
						pblMapAdd(datatimes, &stream[i], 4, &times[distinctIndicator], 4);
						distinctIndicator++;
					}
					if (specificKey == stream[i]) {
						specificKeyTime++;
					}
					i++;
				}

				// check whether the map is correct.
				realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
				if (specificKeyTime != realtime) {
					printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
					return;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				dcmerror = 0;
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					data = (int*)pblMapEntryKey(keyValue);
					time = (int*)pblMapEntryValue(keyValue);
					realtime = CountMinSketchEstimateFrequency(pointQueryCM, data, 4);
					if (realtime > (float8)(*time)) {
						cmerror += (realtime - (float8)(*time));
					}
					else {
						cmerror += (float8)(*time) - realtime;
					}
					realtime = DBCountMinSketchEstimateFrequency(pointQueryDcm, data, 4);
					if (realtime > (float8)(*time)) {
						dcmerror += (realtime - (float8)(*time));
					}
					else {
						dcmerror += (float8)(*time) - realtime;
					}
					i++;
				}
				totalCardinality += i;
				cmerror = cmerror / i;
				totalCMerror += cmerror;
				dcmerror = dcmerror / i;
				totalDCMerror += dcmerror;
				dcmMemory += DBCountMinSketchChainSize(pointQueryDcm);
				pblIteratorFree(iterator);
				pblMapClear(datatimes);
				free(times);
				free(stream);
				free(exact);
				DBCountMinSketchChainDestroy(pointQueryDcm);
				CountMinSketchDestroy(pointQueryCM);
				start++;
			}

			totalCMerror = totalCMerror / pointQueryTestTime;
			totalDCMerror = totalDCMerror / pointQueryTestTime;
			dcmMemory = dcmMemory / pointQueryTestTime;
			totalCardinality = totalCardinality / (float) pointQueryTestTime;
			printf("[zipfian-point query-%d] : with [skew : %f] , cminitMemory : [%d], dcmInitMemory : [%d], cmMemory : [%d], dcmMemory : [%d], cmerror : [%f], dcmerror : [%f], cardinality : %f\n",
				increaseType, zipfpar, initCMMemory, initDCMMemory ,cmMemory, dcmMemory, totalCMerror, totalDCMerror, totalCardinality);
			increaseType++;
		}
		zipfpar += zipfparStep;
	}

}

/*
	used for testing point query with the impact of dynamic data set.
	for a dynamic dataset, obviously that they have unknown data set size.
*/
void experiment13(int pointQueryTestTime, int N, float zipfpar, int dynamic) {
	DBCountMinSketchChain* pointQueryDcm;
	CountMinSketch* pointQueryCm;
	int* stream;
	int* exact;
	int seed;
	int n = 1048575;
	int* times;
	int* timeValue;
	int timeValueLength;
	int distinctIndicator = 0;
	PblMap* datatimes;
	PblIterator* iterator;
	PblMapEntry* keyValue;
	int* data;
	int* time;
	uint32_t realtime = 0;

	int start = 0;
	int specificKey, specificKeyTime = 0;
	int i = 0;
	uint32_t dcmD = 3;
	uint32_t cmD = 3, cmW = 0;
	float8 dcmerror = 0, cmerror = 0;
	float8 totalDCMerror = 0, totalCMerror = 0;
	datatimes = pblMapNewHashMap();
	int* uniformData;
	int* normalData;
	int uniformDataSeed, normalDataSeed;
	int low = 0, high = 20000;
	int range = 5000, rangeStep = 5000;
	int plSize;
	int queryType;
	int increaseType = 1;
	float8 epsilon = 0.01;
	float part1 = 0.2;
	float k = 10;
	float r = 2;
	int a1 = N * part1, a2 = N * part1, a3 = N * part1;
	int length1 = ceil(sqrt((N * 2 / (float) a1) + 0.25) - 0.5), length2 = ceil(sqrt((N * 2 / (float) (k * a2)) + 0.25) - 0.5),
		length3 = ceil(log((r - 1) * (float) (N / (float) a3) + r) / log(r) - 1);

	float epsilon1 = epsilon * N * (length1) / (float) (N + length1 * (length1 - 1) / (float) 2 * a1),
		epsilon2 = epsilon * k * length2 * N / (float) (N + k * length2 * (length2 - 1) / (float) 2 * a2),
		epsilon3 = pow(r, length3) / (float) (N + a3*(length3 * pow(r, length3) - (r * (pow(r, length3) - 1) / (float) (r - 1)))) *epsilon * N;


	float e = 2.7182;
	int d = 3;
	cmW = ceil(exp(1) / epsilon);
	int rangeTill = N;
	float reallypointqueryEpsilon;
	int initSize;
	float totalCardinality;
	int pointquerydcmMemory = 0, cmMemory = cmD * cmW, selfjoindcmMemory = 0;
	printf("point query settings : length1 : %d, length2 : %d, length3 : %d, a1 : %d, a2 : %d, a3 : %d,  epsilon1 : %f, epsilon2 : %f, epsilon3 : %f \n", length1 , 
		length2, length3, a1 , a2, a3, epsilon1, epsilon2, epsilon3);

	/*
		first test point query.
	*/
	range = 5000;
	while (range <= rangeTill) {
		increaseType = 1;
		while (increaseType <= 3) {
			start = 0;
			pointquerydcmMemory = 0;
			selfjoindcmMemory = 0;
			totalDCMerror = 0, totalCMerror = 0;
			dcmerror = 0, cmerror = 0;
			totalCardinality = 0;
			if (increaseType == 1) {
				reallypointqueryEpsilon = epsilon1;
				initSize = a1;
				increaseType++;
				continue;
			}
			else if (increaseType == 2) {
				reallypointqueryEpsilon = epsilon2;
				initSize = a2;
				increaseType++;
				continue;
			}
			else {
				reallypointqueryEpsilon = epsilon3;
				initSize = a3;
			}

			while (start < pointQueryTestTime) {
				distinctIndicator = 0;
				times = (int*)malloc(sizeof(int) * range);
				for (i = 0; i < range; i++)
					times[i] = 1;
				i = 0;
				exact = (int *)calloc(n + 1, sizeof(int));
				stream = (int *)calloc(range + 1, sizeof(int));
				seed = getSystemTime();
				if (seed < 0) {
					seed = -1 * seed;
				}
				stream = CreateStream(range, n, zipfpar, stream, exact, seed);
				specificKey = stream[88];
				specificKeyTime = 0;
				if (dynamic) {
					pointQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					pointQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}
				pointQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallypointqueryEpsilon, dcmD, dynamic, 0, 0, 1, initSize, 0, increaseType, k, 0);
				i = 0;
				while (i < range) {
					if (stream[i] < 0) {
						printf("opps, value < 0");
					}
					CountMinSketchAdd(pointQueryCm, &stream[i], 4, 1);
					DBCountMinSketchAdd(pointQueryDcm, &stream[i], 4, 1);
					if (pblMapContainsKey(datatimes, &stream[i], 4)) {
						timeValue = (int*)pblMapGet(datatimes, &stream[i], 4, &timeValueLength);
						(*timeValue) = (*timeValue) + 1;
						pblMapAdd(datatimes, &stream[i], 4, timeValue, 4);
					}
					else {
						pblMapAdd(datatimes, &stream[i], 4, &times[distinctIndicator], 4);
						distinctIndicator++;
					}
					if (specificKey == stream[i]) {
						specificKeyTime++;
					}
					i++;
				}

				// check whether the map is correct.
				realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
				if (specificKeyTime != realtime) {
					printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
					return;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				dcmerror = 0;
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					data = (int*)pblMapEntryKey(keyValue);
					time = (int*)pblMapEntryValue(keyValue);
					realtime = CountMinSketchEstimateFrequency(pointQueryCm, data, 4);
					if (realtime >= (*time)) {
						cmerror += realtime - (float8)(*time);
					}
					else {
						cmerror += (float8) (*time) - realtime;
					}
					realtime = DBCountMinSketchEstimateFrequency(pointQueryDcm, data, 4);
					if (realtime >= (*time)) {
						dcmerror += realtime - (float8)(*time);
					}
					else {
						dcmerror += (float8)(*time) - realtime;
					}
					i++;
				}
				totalCardinality += i;
				cmerror = cmerror / i;
				totalCMerror += cmerror;
				dcmerror = dcmerror / i;
				totalDCMerror += dcmerror;
				pointquerydcmMemory += DBCountMinSketchChainSize(pointQueryDcm);
				pblIteratorFree(iterator);
				pblMapClear(datatimes);
				free(times);
				free(stream);
				free(exact);
				DBCountMinSketchChainDestroy(pointQueryDcm);
				CountMinSketchDestroy(pointQueryCm);
				start++;
			}
			totalCardinality = totalCardinality / pointQueryTestTime;
			totalCMerror = totalCMerror / pointQueryTestTime;
			totalDCMerror = totalDCMerror / pointQueryTestTime;
			pointquerydcmMemory = pointquerydcmMemory / pointQueryTestTime;
			printf("[zipfian-point query : %d] : with [range : %d] , cmMemory : [%d], dcmMemory : [%d], cmerror : [%f], dcmerror : [%f] , cardinality : %f \n",
				increaseType, range, cmMemory, pointquerydcmMemory, totalCMerror, totalDCMerror, totalCardinality);
			increaseType++;
		}	
		range += rangeStep;
	}


	/*
	increaseType = 1;
	range = 5000;
	while (range <= rangeTill) {
		increaseType = 1;
		while (increaseType <= 3) {
			start = 0;
			pointquerydcmMemory = 0;
			selfjoindcmMemory = 0;
			totalDCMerror = 0, totalCMerror = 0;
			dcmerror = 0, cmerror = 0;
			totalCardinality = 0;
			while (start < pointQueryTestTime) {
				distinctIndicator = 0;
				times = (int*)malloc(sizeof(int) * range);
				for (i = 0; i < range; i++)
					times[i] = 1;
				i = 0;
				uniformDataSeed = getSystemTime();
				uniformData = generateUniformData(range, 1, 2000, &uniformDataSeed);
				specificKey = uniformData[88];
				specificKeyTime = 0;
				if (dynamic) {
					pointQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					pointQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}
				pointQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallypointqueryEpsilon, dcmD, dynamic, 0, 0, 1, initSize, 0, increaseType, k, initCardinaty);
				i = 0;
				while (i < range) {
					if (uniformData[i] < 0) {
						printf("opps, value < 0");
					}
					CountMinSketchAdd(pointQueryCm, &uniformData[i], 4, 1);
					DBCountMinSketchAdd(pointQueryDcm, &uniformData[i], 4, 1);
					if (pblMapContainsKey(datatimes, &uniformData[i], 4)) {
						timeValue = (int*)pblMapGet(datatimes, &uniformData[i], 4, &timeValueLength);
						(*timeValue) = (*timeValue) + 1;
						pblMapAdd(datatimes, &uniformData[i], 4, timeValue, 4);
					}
					else {
						pblMapAdd(datatimes, &uniformData[i], 4, &times[distinctIndicator], 4);
						distinctIndicator++;
					}
					if (specificKey == uniformData[i]) {
						specificKeyTime++;
					}
					i++;
				}

				// check whether the map is correct.
				realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
				if (specificKeyTime != realtime) {
					printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
					return;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				dcmerror = 0;
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					data = (int*)pblMapEntryKey(keyValue);
					time = (int*)pblMapEntryValue(keyValue);
					realtime = CountMinSketchEstimateFrequency(pointQueryCm, data, 4);
					if (realtime >= (*time)) {
						cmerror += realtime - (float8)(*time);
					}
					else {
						cmerror += (float8)(*time) - realtime;
					}
					realtime = DBCountMinSketchEstimateFrequency(pointQueryDcm, data, 4);
					if (realtime >= (*time)) {
						dcmerror += realtime - (float8)(*time);
					}
					else {
						dcmerror += (float8)(*time) - realtime;
					}
					i++;
				}
				totalCardinality += i;
				cmerror = cmerror / i;
				totalCMerror += cmerror;
				dcmerror = dcmerror / i;
				totalDCMerror += dcmerror;
				pointquerydcmMemory += DBCountMinSketchChainSize(pointQueryDcm);
				pblIteratorFree(iterator);
				pblMapClear(datatimes);
				freeUniformData(uniformData);
				free(times);
				DBCountMinSketchChainDestroy(pointQueryDcm);
				CountMinSketchDestroy(pointQueryCm);
				start++;
			}
			totalCardinality = totalCardinality / pointQueryTestTime;
			totalCMerror = totalCMerror / pointQueryTestTime;
			totalDCMerror = totalDCMerror / pointQueryTestTime;
			pointquerydcmMemory = pointquerydcmMemory / pointQueryTestTime;
			printf("[uniform-point query-%d] : with [range : %d] , cmMemory : [%d], dcmMemory : [%d], cmerror : [%f], dcmerror : [%f], cardinality : %f \n",
				increaseType, range, cmMemory, pointquerydcmMemory, totalCMerror, totalDCMerror, totalCardinality);
			increaseType++;
		}
		range += rangeStep;
	}
	*/

	range = 5000;
	increaseType = 1;
	while (range <= rangeTill) {
		increaseType = 1;
		while (increaseType <= 3) {
			start = 0;
			pointquerydcmMemory = 0;
			selfjoindcmMemory = 0;
			totalDCMerror = 0, totalCMerror = 0;
			dcmerror = 0, cmerror = 0;
			totalCardinality = 0;
			if (increaseType == 1) {
				reallypointqueryEpsilon = epsilon1;
				initSize = a1;
				increaseType++;
				continue;
			}
			else if (increaseType == 2) {
				reallypointqueryEpsilon = epsilon2;
				initSize = a2;
				increaseType++;
				continue;
			}
			else {
				reallypointqueryEpsilon = epsilon3;
				initSize = a3;
			}
			while (start < pointQueryTestTime) {
				distinctIndicator = 0;
				times = (int*)malloc(sizeof(int) * range);
				for (i = 0; i < range; i++)
					times[i] = 1;
				i = 0;
				normalDataSeed = getSystemTime();
				normalData = generateIntNormalData(low, high, range);
				specificKey = normalData[88];
				specificKeyTime = 0;
				if (dynamic) {
					pointQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					pointQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}
				pointQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallypointqueryEpsilon, dcmD, dynamic, 0, 0, 1, initSize, 0, increaseType, k, 0);
				i = 0;
				while (i < range) {
					if (normalData[i] < 0) {
						printf("opps, value < 0");
					}
					CountMinSketchAdd(pointQueryCm, &normalData[i], 4, 1);
					DBCountMinSketchAdd(pointQueryDcm, &normalData[i], 4, 1);
					if (pblMapContainsKey(datatimes, &normalData[i], 4)) {
						timeValue = (int*)pblMapGet(datatimes, &normalData[i], 4, &timeValueLength);
						(*timeValue) = (*timeValue) + 1;
						pblMapAdd(datatimes, &normalData[i], 4, timeValue, 4);
					}
					else {
						pblMapAdd(datatimes, &normalData[i], 4, &times[distinctIndicator], 4);
						distinctIndicator++;
					}
					if (specificKey == normalData[i]) {
						specificKeyTime++;
					}
					i++;
				}

				// check whether the map is correct.
				realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
				if (specificKeyTime != realtime) {
					printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
					return;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				dcmerror = 0;
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					data = (int*)pblMapEntryKey(keyValue);
					time = (int*)pblMapEntryValue(keyValue);
					realtime = CountMinSketchEstimateFrequency(pointQueryCm, data, 4);
					if (realtime >= (*time)) {
						cmerror += realtime - (float8)(*time);
					}
					else {
						cmerror += (float8)(*time) - realtime;
					}
					realtime = DBCountMinSketchEstimateFrequency(pointQueryDcm, data, 4);
					if (realtime >= (*time)) {
						dcmerror += realtime - (float8)(*time);
					}
					else {
						dcmerror += (float8)(*time) - realtime;
					}
					i++;
				}

				plSize = pblMapSize(datatimes);
				totalCardinality += i;
				cmerror = cmerror / i;
				totalCMerror += cmerror;
				dcmerror = dcmerror / i;
				totalDCMerror += dcmerror;
				pointquerydcmMemory += DBCountMinSketchChainSize(pointQueryDcm);
				pblIteratorFree(iterator);
				pblMapClear(datatimes);
				free(times);
				freeIntNormalData(normalData);
				DBCountMinSketchChainDestroy(pointQueryDcm);
				CountMinSketchDestroy(pointQueryCm);
				start++;
			}
			totalCardinality = totalCardinality / pointQueryTestTime;
			totalCMerror = totalCMerror / pointQueryTestTime;
			totalDCMerror = totalDCMerror / pointQueryTestTime;
			pointquerydcmMemory = pointquerydcmMemory / pointQueryTestTime;		
			printf("[normal-point query-%d] : with [range : %d] , cmMemory : [%d], dcmMemory : [%d], cmerror : [%f], dcmerror : [%f], cardinality : %f \n",
				increaseType, range, cmMemory, pointquerydcmMemory, totalCMerror, totalDCMerror, totalCardinality);
			increaseType++;
		}
		range += rangeStep;
	}

}

/*
	used for testing self join size query under dynamic data set.
*/
void experiment14(int pointQueryTestTime, int N, float zipfpar, int dynamic) {
	DBCountMinSketchChain* selfJoinSizeQueryDcm;
	CountMinSketch* selfJoinSizeQueryCm;
	int* stream;
	int* exact;
	int seed;
	int n = 1048575;
	int* times;
	int* timeValue;
	int timeValueLength;
	int distinctIndicator = 0;
	PblMap* datatimes;
	PblIterator* iterator;
	PblMapEntry* keyValue;
	int* data;
	int* time;
	uint32_t realtime = 0;

	int start = 0;
	int specificKey, specificKeyTime = 0;
	int i = 0;
	uint32_t dcmD = 3;
	uint32_t cmD = 3, cmW = 0;
	datatimes = pblMapNewHashMap();
	int* uniformData;
	int* normalData;
	int uniformDataSeed, normalDataSeed;
	int low = 0, high = 20000;
	ulong dcmselfJoinSize;
	double dcmselfJoinSizeError;
	ulong dcmrealSelfJoinSize;
	double dcmselfJoinError;
	ulong cmselfJoinSize;
	double cmselfJoinSizeError;
	ulong cmrealSelfJoinSize;
	double cmselfJoinError;
	int range = 5000, rangeStep = 5000;
	int plSize;
	int queryType;
	int increaseType = 1;
	float8 epsilon = 0.01;
	float part1 = 0.2;
	float k = 10;
	float r = 2;
	float alpha = 0.1;
	int a1 = N * (1 - alpha) * part1;
	float e = 2.7182;
	int d = 3;
	cmW = ceil(exp(1) / epsilon);
	int rangeTill = N;
	float reallypointqueryEpsilon;
	float reallySelfjoinSizeEpsilon;
	int initSize;
	int initCardinaty = a1;
	float totalCardinality;
	reallySelfjoinSizeEpsilon = (1 / part1) * epsilon;
	int pointquerydcmMemory = 0, cmMemory = cmD * cmW, selfjoindcmMemory = 0;
	printf("self join size query settings : c : %d, epsilon : %f \n", initCardinaty, reallySelfjoinSizeEpsilon);
	/*
	first test point query.
	*/
	
	range = 5000;
	while (range <= rangeTill) {
		start = 0;
		pointquerydcmMemory = 0;
		selfjoindcmMemory = 0;
		dcmselfJoinError = 0;
		cmselfJoinError = 0;
		totalCardinality = 0;
		while (start < pointQueryTestTime) {
				dcmrealSelfJoinSize = 0;
				cmrealSelfJoinSize = 0;
				distinctIndicator = 0;
				times = (int*)malloc(sizeof(int) * range);
				for (i = 0; i < range; i++)
					times[i] = 1;
				i = 0;
				exact = (int *)calloc(n + 1, sizeof(int));
				stream = (int *)calloc(range + 1, sizeof(int));
				seed = getSystemTime();
				if (seed < 0) {
					seed = -1 * seed;
				}
				stream = CreateStream(range, n, zipfpar, stream, exact, seed);
				specificKey = stream[88];
				specificKeyTime = 0;
				if (dynamic) {
					selfJoinSizeQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					selfJoinSizeQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}
				
				selfJoinSizeQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallySelfjoinSizeEpsilon, dcmD, dynamic, 0, 0, 2, 0, 0, 0, k, initCardinaty);
				i = 0;
				while (i < range) {
					if (stream[i] < 0) {
						printf("opps, value < 0");
					}
					CountMinSketchAdd(selfJoinSizeQueryCm, &stream[i], 4, 1);
					DBCountMinSketchAdd(selfJoinSizeQueryDcm, &stream[i], 4, 1);
					if (pblMapContainsKey(datatimes, &stream[i], 4)) {
						timeValue = (int*)pblMapGet(datatimes, &stream[i], 4, &timeValueLength);
						(*timeValue) = (*timeValue) + 1;
						pblMapAdd(datatimes, &stream[i], 4, timeValue, 4);
					}
					else {
						pblMapAdd(datatimes, &stream[i], 4, &times[distinctIndicator], 4);
						distinctIndicator++;
					}
					if (specificKey == stream[i]) {
						specificKeyTime++;
					}
					i++;
				}

				// check whether the map is correct.
				realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
				if (specificKeyTime != realtime) {
					printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
					return;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					time = (int*)pblMapEntryValue(keyValue);
					i++;
					cmrealSelfJoinSize += (*time) * (*time);
					dcmrealSelfJoinSize += (*time) * (*time);
				}

				dcmselfJoinSize = DBCountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryDcm);
				if (dcmrealSelfJoinSize >= dcmselfJoinSize) {
					dcmselfJoinError += (double)(dcmrealSelfJoinSize - dcmselfJoinSize);
				}
				else {
					dcmselfJoinError += (double)(dcmselfJoinSize - dcmrealSelfJoinSize);
				}


				cmselfJoinSize = CountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryCm);
				if (cmrealSelfJoinSize >= cmselfJoinSize) {
					cmselfJoinError += (double)(cmrealSelfJoinSize - cmselfJoinSize);
				}
				else {
					cmselfJoinError += (double)(cmselfJoinSize - cmrealSelfJoinSize);
				}
				totalCardinality += i;
				selfjoindcmMemory += DBCountMinSketchChainSize(selfJoinSizeQueryDcm);
				pblIteratorFree(iterator);
				pblMapClear(datatimes);
				free(times);
				free(stream);
				free(exact);
				DBCountMinSketchChainDestroy(selfJoinSizeQueryDcm);
				CountMinSketchDestroy(selfJoinSizeQueryCm);
				start++;
			}
			totalCardinality = totalCardinality / pointQueryTestTime;
			selfjoindcmMemory = selfjoindcmMemory / pointQueryTestTime;
			dcmselfJoinError = dcmselfJoinError / pointQueryTestTime;
			cmselfJoinError = cmselfJoinError / pointQueryTestTime;
			printf("[zipfian-self join size query] : with [range : %d] , cmerror : %f, dcmerror: %f, cmMemory : %d, dcmMemory: %d, cardinality : %f \n",
				range, cmselfJoinError, dcmselfJoinError, cmMemory, selfjoindcmMemory, totalCardinality);
			range += rangeStep;
		}

	/*
	increaseType = 1;
	range = 5000;
	while (range <= rangeTill) {
			start = 0;
			pointquerydcmMemory = 0;
			selfjoindcmMemory = 0;
			dcmselfJoinError = 0;
			cmselfJoinError = 0;
			totalCardinality = 0;
			while (start < pointQueryTestTime) {
				distinctIndicator = 0;
				dcmrealSelfJoinSize = 0;
				cmrealSelfJoinSize = 0;
				times = (int*)malloc(sizeof(int) * range);
				for (i = 0; i < range; i++)
					times[i] = 1;
				i = 0;
				uniformDataSeed = getSystemTime();
				uniformData = generateUniformData(range, 1, 2000, &uniformDataSeed);
				specificKey = uniformData[88];
				specificKeyTime = 0;
				if (dynamic) {
					selfJoinSizeQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					selfJoinSizeQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}
				selfJoinSizeQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallySelfjoinSizeEpsilon, dcmD, dynamic, 0, 0, 2, 0, 0, 0, k, initCardinaty);
				i = 0;
				while (i < range) {
					if (uniformData[i] < 0) {
						printf("opps, value < 0");
					}
					CountMinSketchAdd(selfJoinSizeQueryCm, &uniformData[i], 4, 1);
					DBCountMinSketchAdd(selfJoinSizeQueryDcm, &uniformData[i], 4, 1);
					if (pblMapContainsKey(datatimes, &uniformData[i], 4)) {
						timeValue = (int*)pblMapGet(datatimes, &uniformData[i], 4, &timeValueLength);
						(*timeValue) = (*timeValue) + 1;
						pblMapAdd(datatimes, &uniformData[i], 4, timeValue, 4);
					}
					else {
						pblMapAdd(datatimes, &uniformData[i], 4, &times[distinctIndicator], 4);
						distinctIndicator++;
					}
					if (specificKey == uniformData[i]) {
						specificKeyTime++;
					}
					i++;
				}

				// check whether the map is correct.
				realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
				if (specificKeyTime != realtime) {
					printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
					return;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					time = (int*)pblMapEntryValue(keyValue);
					cmrealSelfJoinSize += (*time) * (*time);
					dcmrealSelfJoinSize += (*time) * (*time);
					i++;
				}

				dcmselfJoinSize = DBCountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryDcm);
				if (dcmrealSelfJoinSize >= dcmselfJoinSize) {
					dcmselfJoinError += (double)(dcmrealSelfJoinSize - dcmselfJoinSize);
				}
				else {
					dcmselfJoinError += (double)(dcmselfJoinSize - dcmrealSelfJoinSize);
				}


				cmselfJoinSize = CountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryCm);
				if (cmrealSelfJoinSize >= cmselfJoinSize) {
					cmselfJoinError += (double)(cmrealSelfJoinSize - cmselfJoinSize);
				}
				else {
					cmselfJoinError += (double)(cmselfJoinSize - cmrealSelfJoinSize);
				}
				totalCardinality += i;
				selfjoindcmMemory += DBCountMinSketchChainSize(selfJoinSizeQueryDcm);
				pblIteratorFree(iterator);
				pblMapClear(datatimes);
				freeUniformData(uniformData);
				free(times);
				DBCountMinSketchChainDestroy(selfJoinSizeQueryDcm);
				CountMinSketchDestroy(selfJoinSizeQueryCm);
				start++;
			}
			totalCardinality = totalCardinality / pointQueryTestTime;
			selfjoindcmMemory = selfjoindcmMemory / pointQueryTestTime;
			dcmselfJoinError = dcmselfJoinError / pointQueryTestTime;
			cmselfJoinError = cmselfJoinError / pointQueryTestTime;
			printf("[uniform-self join size query] : with [range : %d] , cmerror: %f, dcmerror : %f, cmMemory : %d, dcmMemory: %d, cardinality : %f \n",
				range, cmselfJoinError, dcmselfJoinError, cmMemory, selfjoindcmMemory, totalCardinality);

		range += rangeStep;
	}
	*/


	range = 5000;
	increaseType = 1;
	while (range <= rangeTill) {
			start = 0;
			pointquerydcmMemory = 0;
			selfjoindcmMemory = 0;
			dcmselfJoinError = 0;
			cmselfJoinError = 0;
			totalCardinality = 0;
			while (start < pointQueryTestTime) {
				distinctIndicator = 0;
				dcmrealSelfJoinSize = 0;
				cmrealSelfJoinSize = 0;
				times = (int*)malloc(sizeof(int) * range);
				for (i = 0; i < range; i++)
					times[i] = 1;
				i = 0;
				normalDataSeed = getSystemTime();
				normalData = generateIntNormalData(low, high, range);
				specificKey = normalData[88];
				specificKeyTime = 0;
				if (dynamic) {
					selfJoinSizeQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					selfJoinSizeQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}
				selfJoinSizeQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallySelfjoinSizeEpsilon, dcmD, dynamic, 0, 0, 2, 0, 0, 0, k, initCardinaty);
				i = 0;
				while (i < range) {
					if (normalData[i] < 0) {
						printf("opps, value < 0");
					}
					CountMinSketchAdd(selfJoinSizeQueryCm, &normalData[i], 4, 1);
					DBCountMinSketchAdd(selfJoinSizeQueryDcm, &normalData[i], 4, 1);
					if (pblMapContainsKey(datatimes, &normalData[i], 4)) {
						timeValue = (int*)pblMapGet(datatimes, &normalData[i], 4, &timeValueLength);
						(*timeValue) = (*timeValue) + 1;
						pblMapAdd(datatimes, &normalData[i], 4, timeValue, 4);
					}
					else {
						pblMapAdd(datatimes, &normalData[i], 4, &times[distinctIndicator], 4);
						distinctIndicator++;
					}
					if (specificKey == normalData[i]) {
						specificKeyTime++;
					}
					i++;
				}

				// check whether the map is correct.
				realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
				if (specificKeyTime != realtime) {
					printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
					return;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					time = (int*)pblMapEntryValue(keyValue);
					cmrealSelfJoinSize += (*time) * (*time);
					dcmrealSelfJoinSize += (*time) * (*time);
					i++;
				}

				plSize = pblMapSize(datatimes);


				dcmselfJoinSize = DBCountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryDcm);
				if (dcmrealSelfJoinSize >= dcmselfJoinSize) {
					dcmselfJoinError += (double)(dcmrealSelfJoinSize - dcmselfJoinSize);
				}
				else {
					dcmselfJoinError += (double)(dcmselfJoinSize - dcmrealSelfJoinSize);
				}


				cmselfJoinSize = CountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryCm);
				if (cmrealSelfJoinSize >= cmselfJoinSize) {
					cmselfJoinError += (double)(cmrealSelfJoinSize - cmselfJoinSize);
				}
				else {
					cmselfJoinError += (double)(cmselfJoinSize - cmrealSelfJoinSize);
				}
				totalCardinality += i;
				selfjoindcmMemory += DBCountMinSketchChainSize(selfJoinSizeQueryDcm);
				pblIteratorFree(iterator);
				pblMapClear(datatimes);
				free(times);
				freeIntNormalData(normalData);
				DBCountMinSketchChainDestroy(selfJoinSizeQueryDcm);
				CountMinSketchDestroy(selfJoinSizeQueryCm);
				start++;
			}
			totalCardinality = totalCardinality / pointQueryTestTime;
			selfjoindcmMemory = selfjoindcmMemory / pointQueryTestTime;
			dcmselfJoinError = dcmselfJoinError / pointQueryTestTime;
			cmselfJoinError = cmselfJoinError / pointQueryTestTime;
			printf("[normal-self join size query] : with [range : %d] , cmerror : %f, dcmerror: %f, cmMemory : %d, dcmMemory: %d, cardinality: %f \n",
				range, cmselfJoinError, dcmselfJoinError, cmMemory, selfjoindcmMemory, totalCardinality);
		range += rangeStep;
	}
}

int findChar(char* chars, char c) {
	int i = 0;
	while (chars != NULL) {
		if (*chars == c) {
			return i;
		}
		i++;
		chars++;
	}
	return -1;
}

void experiment15_selfjoin(int pointQueryTestTime, int range, char stream[1000001][65], int dynamic, PblMap* datatimes);
void experiment15_point_query(int pointQueryTestTime, int range, char stream[1000001][65], int dynamic, PblMap* datatimes);

/*
	test real data set
*/
void experiment15(int pointQueryTestTime, int dynamic, int queryType) {
	char* totalWordsName = "C://Users/andrew/Desktop/mywork/paper/experiments/code/dataset/sampleed/alltweets-total-hash.csv";
	char* dictFrequencyFileName = "C://Users/andrew/Desktop/mywork/paper/experiments/code/dataset/sampleed/alltweets-frequency-sorted-hash.csv";
	int i = 0, wordsFrequencyLength = 0, totalWordsLength = 0;
	int location = 0;
	char* wordFrequency = NULL;
	FILE* totalWordsFile = fopen(totalWordsName, "r");
	FILE* dictFrequencyFile = fopen(dictFrequencyFileName, "r");
	PblMap* hashFresMaps = pblMapNewHashMap();
	if (totalWordsFile == NULL || dictFrequencyFile == NULL) {
		printf("open [%s] || [%s] file failed \n", totalWordsName, dictFrequencyFileName);
		return;
	}
	else {
		// open file success

		// first read hash, frequency pair.
		i = 0;
		char wordsFrequency[110242][100];
		char hashs[110242][65];
		int times[110242];
		while (fscanf(dictFrequencyFile, "%[^\n]", wordsFrequency[i]) != EOF) {
			fgetc(dictFrequencyFile);
			wordFrequency = wordsFrequency[i];
			location = findChar(wordFrequency, '\t');
			strncpy(hashs[i], wordFrequency, location);
			times[i] = atoi(wordFrequency + location + 1);
			pblMapAdd(hashFresMaps, hashs[i], 64, &times[i], 4);
			i++;
			wordsFrequencyLength++;
		}

		i = 0;
		char totalData[1000001][100]; //
		char words[1000001][65];
		while (fscanf(totalWordsFile, "%[^\n]", totalData[i]) != EOF) {
			fgetc(totalWordsFile);
			location = findChar(totalData[i], '\t');
			strncpy(words[i], totalData[i] + location + 1, 100 - location - 1);
			i++;
			totalWordsLength++;
		}

		fclose(totalWordsFile);
		fclose(dictFrequencyFile);

		// now we first test self join size query
		if (queryType == 1)
			experiment15_selfjoin(pointQueryTestTime, 1000000, words, dynamic, hashFresMaps);
		if (queryType == 0)
			experiment15_point_query(pointQueryTestTime, 1000000, words, dynamic, hashFresMaps);
	}

	// this step take much time.
	pblMapFree(hashFresMaps);

}

/*
	used by real data set test. self join query
*/
void experiment15_selfjoin(int pointQueryTestTime, int range, char stream[1000001][65], int dynamic, PblMap* datatimes) {
	DBCountMinSketchChain* selfJoinSizeQueryDcm;
	CountMinSketch* selfJoinSizeQueryCM;
	int* timeValue;
	int timeValueLength;
	PblIterator* iterator;
	PblMapEntry* keyValue;
	int* time;
	uint32_t realtime = 0;

	int start = 0;
	int i = 0;
	uint32_t dcmD = 3;
	uint32_t cmD = 3, cmW = 0;
	int dcmMemory = 0;
	double dcmselfJoinSize;
	double dcmselfJoinSizeError;
	double dcmrealSelfJoinSize;
	double dcmselfJoinError;
	double cmselfJoinSize;
	double cmselfJoinSizeError;
	double cmrealSelfJoinSize;
	double cmselfJoinError;

	int N = range;
	int skewed = 1;
	int queryType = 2;
	int increaseType = 1;
	float8 epsilon = 0.01;
	float part1 = 0.2;
	float alpha = 0.1;

	while (alpha <= 0.99) {

		float k = 10;
		float r = 2;
		int c1 = N * (1-alpha) * part1, c2 = N * (1-alpha) * part1, c3 = N * (1-alpha) * part1;
		float e = 2.7182;
		int d = 3;
		cmW = ceil(exp(1) / epsilon);
		int cmMemory = cmD * cmW;
		float reallySelfjoinSizeEpsilon;
		int selfinitCardinaty = c1;
		float totalCardinality;
		int initCMMemory = 0, initDCMMemory = 0;
		reallySelfjoinSizeEpsilon = epsilon * (1 / part1);

		printf("self join size query settings : c : %d, epsilon : %f \n", selfinitCardinaty, reallySelfjoinSizeEpsilon);

		/*
		first test point query.
		*/

		totalCardinality = 0;
		start = 0;
		dcmMemory = 0;
		dcmselfJoinError = 0;
		cmselfJoinError = 0;
		while (start < pointQueryTestTime) {
			dcmrealSelfJoinSize = 0;
			cmrealSelfJoinSize = 0;

			// char fileName[100];
			// snprintf(fileName, 99, "C://Users/andrew/Desktop/mywork/paper/experiments/code/tempData/zipfian-%f.txt", zipfpar);
			// saveDataToFile(fileName, stream, range);
			if (dynamic) {
				selfJoinSizeQueryCM = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime() * 13 * (start + 1));
			}
			else {
				selfJoinSizeQueryCM = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
			}
			selfJoinSizeQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallySelfjoinSizeEpsilon, dcmD, dynamic, 0, 0, 2, 0, 0, increaseType, k, selfinitCardinaty);

			initCMMemory = CountMinSketchSize(selfJoinSizeQueryCM);
			initDCMMemory = DBCountMinSketchChainSize(selfJoinSizeQueryDcm);

			i = 0;
			while (i < range) {
				CountMinSketchAdd(selfJoinSizeQueryCM, stream[i], 64 * sizeof(char), 1);
				DBCountMinSketchAdd(selfJoinSizeQueryDcm, stream[i], 64 * sizeof(char), 1);
				i++;
			}

			i = 0;
			iterator = pblMapIteratorNew(datatimes);
			while (pblIteratorHasNext(iterator)) {
				keyValue = (PblMapEntry*)pblIteratorNext(iterator);
				time = (int*)pblMapEntryValue(keyValue);
				cmrealSelfJoinSize += (*time) * (*time);
				dcmrealSelfJoinSize += (*time) * (*time);
				i++;
			}

			dcmselfJoinSize = DBCountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryDcm);
			if (dcmrealSelfJoinSize >= dcmselfJoinSize) {
				dcmselfJoinError += (double)(dcmrealSelfJoinSize - dcmselfJoinSize);
			}
			else {
				dcmselfJoinError += (double)(dcmselfJoinSize - dcmrealSelfJoinSize);
			}


			cmselfJoinSize = CountMinSketchEstimateSelfJoinSize(selfJoinSizeQueryCM);
			if (cmrealSelfJoinSize >= cmselfJoinSize) {
				cmselfJoinError += (double)(cmrealSelfJoinSize - cmselfJoinSize);
			}
			else {
				cmselfJoinError += (double)(cmselfJoinSize - cmrealSelfJoinSize);
			}

			totalCardinality += i;
			dcmMemory += DBCountMinSketchChainSize(selfJoinSizeQueryDcm);
			pblIteratorFree(iterator);
			DBCountMinSketchChainDestroy(selfJoinSizeQueryDcm);
			CountMinSketchDestroy(selfJoinSizeQueryCM);
			// if (start == 0)
				// printf("[realdata-self join size query] : cmerror: %f, dcmerror : %f, initCMMemory : %d, initDCMMemory : %d, cmMemory : %d, dcmMemory: %d, cardinality : %f \n",
					// cmselfJoinError, dcmselfJoinError, initCMMemory, initDCMMemory,cmMemory, dcmMemory, totalCardinality);

			start++;
		}

		totalCardinality = totalCardinality / pointQueryTestTime;
		dcmMemory = dcmMemory / pointQueryTestTime;
		cmselfJoinError = cmselfJoinError / pointQueryTestTime;
		dcmselfJoinError = dcmselfJoinError / pointQueryTestTime;
		printf("[realdata-self join size query-%f] : cmerror: %f, dcmerror : %f, initCMMemory : %d, initDCMMemory : %d, cmMemory : %d, dcmMemory: %d, cardinality : %f \n",
			alpha, cmselfJoinError, dcmselfJoinError, initCMMemory, initDCMMemory, cmMemory, dcmMemory, totalCardinality);
		alpha += 0.1;
	}
	
}

/*
	used to test for point query.
*/
void experiment15_point_query(int pointQueryTestTime, int range, char stream[1000001][65], int dynamic, PblMap* datatimes) {
	DBCountMinSketchChain* pointQueryDcm;
	CountMinSketch* pointQueryCM;
	int* timeValue;
	int timeValueLength;
	PblIterator* iterator;
	PblMapEntry* keyValue;
	char* data;
	int* time;
	uint32_t realtime = 0;
	int start = 0;
	int i = 0;
	uint32_t dcmD = 3;
	uint32_t cmD = 3, cmW = 0;
	float8 dcmerror = 0, cmerror = 0;
	float8 totalDCMerror = 0, totalCMerror = 0;
	int dcmMemory = 0;
	int N = range;
	int skewed = 1;
	int queryType = 2;
	int increaseType = 1;
	float8 epsilon = 0.01;
	float part1 = 0.2;
	float alpha = 0.1;

	while (alpha <= 1) {
		float k = 10;
		float r = 2;
		int c1 = N * (1-alpha) * part1, c2 = N * (1-alpha) * part1, c3 = N * (1-alpha) * part1;
		int length1 = ceil(sqrt((N * 2 * (1 - alpha) / (float)c1) + 0.25) - 0.5),
			length2 = ceil(sqrt((N * 2 * (1 - alpha) / (float)(k * c2)) + 0.25) - 0.5),
			length3 = ceil(log((r - 1) * (1 - alpha) * N / (float)c3 + r) / log(r) - 1);
		float epsilon1 = epsilon * N * 2 * length1 * (1 - alpha) / (float)(length1 * (length1 - 1) * c1 + 2 * (1 - alpha) * N),
			epsilon2 = epsilon * k * length2 * N / (float)(N + k * length2 * (length2 - 1) * c2 / (float)(2 * (1 - alpha))),
			epsilon3 = epsilon * N * pow(r, length3) / (float)(N + c3 / (float)(1 - alpha) * (length3 * pow(r, length3) - r * (pow(r, length3) - 1) / (float)(r - 1)));
		float e = 2.7182;
		int d = 3;
		cmW = ceil(exp(1) / epsilon);
		int cmMemory = cmD * cmW;
		float reallypointqueryEpsilon;
		int initCardinaty;
		float totalCardinality;
		int initCMMemory = 0, initDCMMemory = 0;
		printf("point query settings : length1 : %d, length2 : %d, length3 : %d, c1 : %d, c2 : %d, c3 : %d, epsilon1 : %f, epsilon2 : %f, epsilon3 : %f \n",
			length1, length2, length3, c1, c2, c3, epsilon1, epsilon2, epsilon3);

		/*
			first test point query.
		*/
		increaseType = 1;
		while (increaseType <= 3) {
			if (increaseType == 1) {
				reallypointqueryEpsilon = epsilon1;
				initCardinaty = c1;
			}
			else if (increaseType == 2) {
				reallypointqueryEpsilon = epsilon2;
				initCardinaty = c2;
			}
			else {
				reallypointqueryEpsilon = epsilon3;
				initCardinaty = c3;
			}
			totalCardinality = 0;
			start = 0;
			dcmMemory = 0;
			totalDCMerror = 0, totalCMerror = 0;
			dcmerror = 0, cmerror = 0;
			while (start < pointQueryTestTime) {
				if (dynamic) {
					pointQueryCM = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
				}
				else {
					pointQueryCM = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
				}
				pointQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallypointqueryEpsilon, dcmD, dynamic, 0, 0, 1, 0, 1, increaseType, k, initCardinaty);
				initCMMemory = CountMinSketchSize(pointQueryCM);
				initDCMMemory = DBCountMinSketchChainSize(pointQueryDcm);
				i = 0;
				while (i < range) {
					CountMinSketchAdd(pointQueryCM, stream[i], 64 * sizeof(char), 1);
					DBCountMinSketchAdd(pointQueryDcm, stream[i], 64 * sizeof(char), 1);
					i++;
				}

				i = 0;
				iterator = pblMapIteratorNew(datatimes);
				dcmerror = 0;
				while (pblIteratorHasNext(iterator)) {
					keyValue = (PblMapEntry*)pblIteratorNext(iterator);
					data = (char*)pblMapEntryKey(keyValue);
					time = (int*)pblMapEntryValue(keyValue);
					realtime = CountMinSketchEstimateFrequency(pointQueryCM, data, 64 * sizeof(char));
					if (realtime > (float8)(*time)) {
						cmerror += (realtime - (float8)(*time));
					}
					else {
						cmerror += (float8)(*time) - realtime;
					}
					realtime = DBCountMinSketchEstimateFrequency(pointQueryDcm, data, 64 * sizeof(char));
					if (realtime > (float8)(*time)) {
						dcmerror += (realtime - (float8)(*time));
					}
					else {
						dcmerror += (float8)(*time) - realtime;
					}
					i++;
				}

				totalCardinality += i;
				cmerror = cmerror / i;
				totalCMerror += cmerror;
				dcmerror = dcmerror / i;
				totalDCMerror += dcmerror;
				dcmMemory += DBCountMinSketchChainSize(pointQueryDcm);
				pblIteratorFree(iterator);
				DBCountMinSketchChainDestroy(pointQueryDcm);
				CountMinSketchDestroy(pointQueryCM);

				// if (start == 0) 
					// printf("[realdata-point query-%d] : initCMMemory : [%d], initDCMMemory : [%d], cmMemory : [%d], dcmMemory : [%d], cmerror : [%f], dcmerror : [%f], cardinality : %f \n",
						// increaseType, initCMMemory, initDCMMemory, cmMemory, dcmMemory, totalCMerror, totalDCMerror, totalCardinality);
				start++;
			}

			totalCMerror = totalCMerror / pointQueryTestTime;
			totalDCMerror = totalDCMerror / pointQueryTestTime;
			dcmMemory = dcmMemory / pointQueryTestTime;
			totalCardinality = totalCardinality / (float)pointQueryTestTime;
			printf("[realdata-point query-%d-%f] : initCMMemory : [%d], initDCMMemory : [%d], cmMemory : [%d], dcmMemory : [%d], cmerror : [%f], dcmerror : [%f], cardinality : %f \n",
				increaseType, alpha, initCMMemory, initDCMMemory, cmMemory, dcmMemory, totalCMerror, totalDCMerror, totalCardinality);
			increaseType++;
		}
		alpha += 0.1;
	}
	
}


void testMurMurHash() {
	int hash1[2];
	int hash2[2];
	int key = 1;
	MurmurHash3_128(&key, 4, getSystemTime() + 13 * 100, &hash1);

	MurmurHash3_128(&key, 4, getSystemTime() + 13 * 1001, &hash2);

	printf("hash1 : %d,%d\n", hash1[0], hash1[1]);
	printf("hash2 : %d,%d\n", hash2[0], hash2[1]);

}

/*
	test the accuracy and memory overhead of dcm point query under mathematic analysis.
*/
void testDifferentExtention() {
	int N = 100000;
	int realN = 1;
	int i = 0;

	/*
		three different extensions:
		first : epsilon_j = epsilon_1 * (1/j), a_j = a_1 * j;
		second : epsilon_j = epsilon_1 (1 / (10 * j)), a_j = a_1 * (1 / (10 * j);
		third : epsilon_j = epsilon_1^k , a_j = a_1 ^ k;
		fourth : epsilon_j = (1/2^j) * epsilon_1, a_j = (2^j) * k;
	*/

	float epsilon = 0.01;
	float part1 = 0.01;
	float k = 10;
	float r = 2;
	int a1 = N * part1, a2 = N * part1, a3 = N * part1, a4 = N * part1;
	int length1 = ceil(sqrt((N * 2 / (float) a1) + 0.25) - 0.5), length2 = ceil(sqrt((N * 2 / (float) (k * a2)) + 0.25) - 0.5), 
		length3 = ceil(log(N) / log(a3)), length4 = ceil(log((r - 1) * (N / (float) a4) + r) / log(r) - 1);

	/*
	float epsilon1 = epsilon * N * (length1) / (N + length1 * (length1 - 1) / 2 * a1), 
		epsilon2 = epsilon * k * length2 * N / (N + k * length2 * (length2 - 1) / 2 * a2), 
		epsilon3 = pow((epsilon * N), 1 / (float)length3) / a3, 
		epsilon4 = pow(r, length4) / (N + a4*(length4 * pow(r, length4) - (r * (pow(r, length4)-1) / (r-1)))) *epsilon * N;
	*/

	float epsilon1 = epsilon * N * length1 / (float)(N + length1 * (length1 - 1) * a1 / (float)2),
		epsilon2 = epsilon * N * length2 * k / (float)(N + length2 * (length2 - 1) * k * a2 / (float)2),
		epsilon4 = epsilon * N * pow(r, length4) / (float)(N + a3 * (length4 * pow(r, length4) - r * (pow(r, length4) - 1) / (float) (r - 1)));

	float e = 2.7182;
	int d = 3;
	float memory1 = 0, memory2 = 0, memory3 = 0, memory4 = 0;
	float lastMemory1 = 0, lastMemory2 = 0, lastMemory3 = 0, lastMemory4 = 0;
	float cmMemory = ceil(e / epsilon) * d;
	float error1 = 0, error2 = 0, error3 = 0, error4 = 0;
	int size1 = 0, size2 = 0, size3 = 0, size4 = 0;
	int prevSize1 = 0, prevSize2 = 0 , prevSize3 = 0 , prevSize4 = 0;
	int sketchLength1 = 0, sketchLength2 = 0, sketchLength3 = 0, sketchLength4 = 0;

	printf("point query settings : length1 : %d, length2 : %d, length3 : %d, a1 : %d, a2 : %d, a3 : %d, epsilon1 : %f, epsilon2 : %f, epsilon3 : %f \n", 
		length1, length2, length4, a1, a2, a4, epsilon1, epsilon2, epsilon4);

	while (i < N) {
		if (i > size1) {
			sketchLength1++;
			if (sketchLength1 > 1) {
				error1 += epsilon1 * (1 / (float)(sketchLength1 - 1)) * (a1 * (sketchLength1 - 1));
			}
			prevSize1 = size1;
			size1 += a1 * sketchLength1;
			lastMemory1 = memory1;
			memory1 += ceil(e / epsilon1 * sketchLength1) * d;
			printf("[strategy one] : create a new sketch : newMemory : %f, lastMemory : %f, currentErr : %f when inserting %d items \n", memory1, lastMemory1, error1, i);
		}
		i++;
	}
	error1 += epsilon1 * (1 / (float)(sketchLength1)) * (N - prevSize1);
	printf("[strategy one] : after inserting %d elements, error : %f \n", N, error1);

	i = 0;
	while (i < N) {
		if (i > size2) {
			sketchLength2++;
			if (sketchLength2 > 1) {
				error2 += epsilon2 * (1 / (float)(sketchLength2 - 1)) * (1 / (float) k) * (a2 * (sketchLength2 - 1) * k);
			}
			prevSize2 = size2;
			size2 += a2 * sketchLength2 * k;
			lastMemory2 = memory2;
			memory2 += ceil(e / (epsilon2 * (1 / (float)(sketchLength2)) * (1 / (float)k))) * d;
			printf("[strategy two] : create a new sketch : newMemory : %f, lastMemory : %f, currentErr : %f when inserting %d items \n", memory2, lastMemory2, error2, i);
		}
		i++;
	}
	error2 += epsilon2 * (1 / (float)(sketchLength2)) * (1 / (float)k) * (N - prevSize2);
	printf("[strategy two] : after inserting %d elements, error : %f \n", N, error2);

	i = 0;
	while (i < N) {
		if (i > size4) {
			sketchLength4++;
			if (sketchLength4 > 1) {
				error4 += epsilon4 * (1 / pow(r, sketchLength4 - 1)) * a4 * pow(r, sketchLength4 - 1);
			}
			prevSize4 = size4;
			size4 += a4 * pow(r, sketchLength4);
			lastMemory4 = memory4;
			memory4 += ceil(e / (epsilon4 * (1 / pow(r, sketchLength4)))) * d;
			printf("[strategy forth] : create a new sketch : newMemory : %f, lastMemory : %f, currentErr : %f when inserting %d items \n", memory4, lastMemory4, error4, i);
		}
		i++;
	}
	error4 += epsilon4 * (1 / pow(r, sketchLength4)) * (N - prevSize4);
	printf("[strategy forth] : after inserting %d elements, error : %f \n", N, error4);

}

/*
	used for testing when data is characters. whether the count-min sketch can work
*/
void testChars() {
	CountMinSketch* cm = CountMinSketchCreateWithDAndW(3, 100, getSystemTime());
	char* hellos = "hello";
	CountMinSketchAdd(cm, hellos, sizeof(char) * 5, 2);
	char* hi = "hi";
	CountMinSketchAdd(cm, hi, sizeof(char) * 2, 2);
	printf("the frequency is %d \n" , CountMinSketchEstimateFrequency(cm, hellos, 5));
	printf("the frequency is %d \n", CountMinSketchEstimateFrequency(cm, hi, 2));
	printf("the self join size is %d \n", CountMinSketchEstimateSelfJoinSize(cm));
}

void tempTest(int pointQueryTestTime, int N, int C, float zipfpar, int dynamic) {
		DBCountMinSketchChain* pointQueryDcm;
		CountMinSketch* pointQueryCm;
		int* stream;
		int* exact;
		int seed;
		int n = 1048575;
		int* times;
		int* timeValue;
		int timeValueLength;
		int distinctIndicator = 0;
		PblMap* datatimes;
		PblIterator* iterator;
		PblMapEntry* keyValue;
		int* data;
		int* time;
		uint32_t realtime = 0;

		int start = 0;
		int specificKey, specificKeyTime = 0;
		int i = 0;
		uint32_t dcmD = 3;
		uint32_t cmD = 3, cmW = 0;
		float8 dcmerror = 0, cmerror = 0;
		float8 totalDCMerror = 0, totalCMerror = 0;
		datatimes = pblMapNewHashMap();
		int* uniformData;
		int* normalData;
		int uniformDataSeed, normalDataSeed;
		int low = 0, high = 300;
		int range = 5000, rangeStep = 5000;
		int plSize;
		int queryType;
		int increaseType = 0;
		float8 epsilon = 0.01;
		float part1 = 0.01;
		float k = 10;
		float r = 2;


		float e = 2.7182;
		int d = 3;
		cmW = ceil(exp(1) / epsilon);
		int rangeTill = N;
		float reallypointqueryEpsilon = epsilon * 2;
		int initSize = 50000;
		float cPart = 0.1;
		int initCardinaty = cPart * C;
		float totalCardinality;
		int pointquerydcmMemory = 0, cmMemory = cmD * cmW, selfjoindcmMemory = 0;

		/*
		first test point query.
		*/
		range = 100000;
		while (range <= rangeTill) {
			increaseType = 0;

				start = 0;
				pointquerydcmMemory = 0;
				selfjoindcmMemory = 0;
				totalDCMerror = 0, totalCMerror = 0;
				dcmerror = 0, cmerror = 0;
				totalCardinality = 0;


				while (start < pointQueryTestTime) {
					distinctIndicator = 0;
					times = (int*)malloc(sizeof(int) * range);
					for (i = 0; i < range; i++)
						times[i] = 1;
					i = 0;
					exact = (int *)calloc(n + 1, sizeof(int));
					stream = (int *)calloc(range + 1, sizeof(int));
					seed = getSystemTime();
					if (seed < 0) {
						seed = -1 * seed;
					}
					stream = CreateStream(range, n, zipfpar, stream, exact, seed);
					specificKey = stream[88];
					specificKeyTime = 0;
					if (dynamic) {
						pointQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, getSystemTime());
					}
					else {
						pointQueryCm = CountMinSketchCreateWithDAndW(cmD, cmW, 0x99496f1ddc863e6fL);
					}
					pointQueryDcm = DBCountMinSketchCreateWithEpsAndP(reallypointqueryEpsilon, dcmD, dynamic, 0, 0, 1, initSize, 0, increaseType, k, initCardinaty);
					i = 0;
					while (i < range) {
						if (stream[i] < 0) {
							printf("opps, value < 0");
						}
						CountMinSketchAdd(pointQueryCm, &stream[i], 4, 1);
						DBCountMinSketchAdd(pointQueryDcm, &stream[i], 4, 1);
						if (pblMapContainsKey(datatimes, &stream[i], 4)) {
							timeValue = (int*)pblMapGet(datatimes, &stream[i], 4, &timeValueLength);
							(*timeValue) = (*timeValue) + 1;
							pblMapAdd(datatimes, &stream[i], 4, timeValue, 4);
						}
						else {
							pblMapAdd(datatimes, &stream[i], 4, &times[distinctIndicator], 4);
							distinctIndicator++;
						}
						if (specificKey == stream[i]) {
							specificKeyTime++;
						}
						i++;
					}

					// check whether the map is correct.
					realtime = (*(int*)pblMapGet(datatimes, &specificKey, 4, &timeValueLength));
					if (specificKeyTime != realtime) {
						printf("specificKeyTime[%d] != realTime[%d]", specificKeyTime, realtime);
						return;
					}

					i = 0;
					iterator = pblMapIteratorNew(datatimes);
					dcmerror = 0;
					while (pblIteratorHasNext(iterator)) {
						keyValue = (PblMapEntry*)pblIteratorNext(iterator);
						data = (int*)pblMapEntryKey(keyValue);
						time = (int*)pblMapEntryValue(keyValue);
						realtime = CountMinSketchEstimateFrequency(pointQueryCm, data, 4);
						if (realtime >= (*time)) {
							cmerror += realtime - (float8)(*time);
						}
						else {
							cmerror += (float8)(*time) - realtime;
						}
						realtime = DBCountMinSketchEstimateFrequency(pointQueryDcm, data, 4);
						if (realtime >= (*time)) {
							dcmerror += realtime - (float8)(*time);
						}
						else {
							dcmerror += (float8)(*time) - realtime;
						}
						i++;
					}
					totalCardinality += i;
					cmerror = cmerror / i;
					totalCMerror += cmerror;
					dcmerror = dcmerror / i;
					totalDCMerror += dcmerror;
					pointquerydcmMemory += DBCountMinSketchChainSize(pointQueryDcm);
					pblIteratorFree(iterator);
					pblMapClear(datatimes);
					free(times);
					free(stream);
					free(exact);
					DBCountMinSketchChainDestroy(pointQueryDcm);
					CountMinSketchDestroy(pointQueryCm);
					start++;
				}
				totalCardinality = totalCardinality / pointQueryTestTime;
				totalCMerror = totalCMerror / pointQueryTestTime;
				totalDCMerror = totalDCMerror / pointQueryTestTime;
				pointquerydcmMemory = pointquerydcmMemory / pointQueryTestTime;
				printf("[zipfian-point query : %d] : with [range : %d] , cmMemory : [%d], dcmMemory : [%d], cmerror : [%f], dcmerror : [%f] , cardinality : %f \n",
					increaseType, range, cmMemory, pointquerydcmMemory, totalCMerror, totalDCMerror, totalCardinality);
				increaseType++;
			
			range += rangeStep;
		}
}


/*
	this test is used to test the impact of memory under different n.
*/
void MemoryTest() {
	float p = 0.05;
	int N = 100000;
	int initialMemory = 0;
	int lastMemory = 0;
	int k = 2;
	int length = 0;
	float epsilon = 0.01;
	float dcmEpsilon = 0;
	int cmMemory = ceil(exp(1) / 0.01);
	int i = 0;
	while (p <= 1) {
		// the initial memory and last memory.
		length = ceil(log((k - 1) * (float)(N / (float)(N * p)) + k) / log(k) - 1);
		dcmEpsilon = pow(k, length) / (float)(N + N * p *(length * pow(k, length) - (k * (pow(k, length) - 1) / (float)(k - 1)))) *epsilon * N;
		initialMemory = ceil(exp(1) / (0.5 * dcmEpsilon));
		lastMemory = 0;
		i = 1;
		while (i <= length) {
			lastMemory += ceil(exp(1) / (1 / pow(2, i) * dcmEpsilon));
			i++;
		}
		printf("[%f] : initialMemory / cmMemory = %f, lastMemory / cmMemory = %f \n", p, initialMemory / (float)cmMemory, lastMemory / (float) cmMemory);
		p += 0.05;
	}
}

void TestZipF(int range, float zipfparInit, int testTime) {

	int* stream;
	int* exact;
	int seed;
	int n = 1048575;
	int* times;
	int* timeValue;
	int timeValueLength;
	int distinctIndicator = 0;
	PblMap* datatimes;
	PblIterator* iterator;
	PblMapEntry* keyValue;
	int* data;
	int* time;
	uint32_t realtime = 0;

	int i = 0;
	datatimes = pblMapNewHashMap();

	float8 zipfpar = zipfparInit, zipfparTill = 5.1, zipfparStep = 0.1;
	int N = range;
	int start = 0;
	float result = 0;
	/*
	first test point query.
	*/

	while (zipfpar <= zipfparTill) {
		result = 0;
		start = 0;
		while (start < testTime) {
			distinctIndicator = 0;
			times = (int*)malloc(sizeof(int) * range);
			for (i = 0; i < range; i++)
				times[i] = 1;
			i = 0;
			exact = (int *)calloc(n + 1, sizeof(int));
			stream = (int *)calloc(range + 1, sizeof(int));
			seed = getSystemTime();
			if (seed < 0) {
				seed = -1 * seed;
			}
			stream = CreateStream(range, n, zipfpar, stream, exact, seed * (start + 1)* 13);

			i = 0;
			while (i < range) {

				if (pblMapContainsKey(datatimes, &stream[i], 4)) {
					timeValue = (int*)pblMapGet(datatimes, &stream[i], 4, &timeValueLength);
					(*timeValue) = (*timeValue) + 1;
					pblMapAdd(datatimes, &stream[i], 4, timeValue, 4);
				}
				else {
					pblMapAdd(datatimes, &stream[i], 4, &times[distinctIndicator], 4);
					distinctIndicator++;
				}
				i++;
			}
			result += distinctIndicator / (float)range;
			pblMapClear(datatimes);
			free(times);
			free(stream);
			free(exact);
			start++;
		}

			printf("[zifpar-%f] : c/n : %f \n", zipfpar, result / testTime);


		zipfpar += zipfparStep;
	}
}

/*
	test for fixed hash function.
*/
int main(int argc, char** argv) {
	int range = 10000;
	float zipfparInit = 0.1;
	int stopkey = 0;
	// TestZipF(10000, 0.1, 10);
	// testDifferentExtention();
	// testMurMurHash();
	// MemoryTest();
	/*
		test the self-join size query error change with the skew of data.
	*/
	experiment11(10, 10000, 0.1, 0);

	/*
		test the point query error change with the skew of data.	
	*/
	// experiment12(10, 10000, 0.1, 0);

	/*
		test the point query error under dynamic data set
	*/
	// experiment13(10, 100000, 0.8, 0);

	/*
		test the self-join size query error under dynamic data set.
	*/
	// experiment14(10, 100000, 0.8, 0);

	/*
		test point query under real data set
	*/
	// experiment15(1, 1, 0);

	/*
		test self join size query under dynamic data set
	*/
	// experiment15(1, 1, 1);

	/*
		test strings as the cms key
	*/
	// testChars();

	// int n = 16384;
	// double p = 0.02;

	// uint32_t m = -1 * ceil(n * log(p) / (pow(log(2), 2)));
	// uint16_t k = round(log(2.0) * m / n);

	// printf("m : %u, k : %u", m, k);

	scanf_s("%d", &stopkey);

	/*

	
	*/

}
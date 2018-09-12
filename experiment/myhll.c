/*
	implement myself a simple hll algorithm for cardinator estimation here.
*/

#include <stdio.h>
#include <string.h>
#include "myhll.h"
#include <stdlib.h>

MyHLL *MyHLLCreate(int size) {
	MyHLL* hll = (MyHLL*)malloc(sizeof(MyHLL));
	hll->cardinator = 0;
	hll->size = size;
	hll->ele = (int*)malloc(sizeof(int) * size);
	memset(hll->ele, 0, sizeof(int) * size);
	return hll;
}

int MyHLLAdd(MyHLL *hll, int ele) {
	int i = 0;
	int f = 0;

	// after exceed the limit, just return.
	if (hll->cardinator >= hll->size)
		return 0;

	while (i < hll->cardinator) {
		if (hll->ele[i] == ele)
			return -1;
		i++;
	}

	hll->ele[hll->cardinator] = ele;
	hll->cardinator++;
	return 1;
}

int MyHLLCardinality(MyHLL *hll) {
	return hll->cardinator;
}

void MyHllDestroy(MyHLL* hll) {
	free(hll->ele);
	free(hll);
}

MapHLL *MapHLLCreate(int size) {
	MapHLL* hll = (MapHLL*)malloc(sizeof(MapHLL));
	memset(hll, 0, sizeof(MapHLL));
	hll->map = pblMapNewHashMap();
	return hll;
}

int MapHLLAdd(MapHLL *hll, void* ele, int size) {
	int time = 1;
	if (pblMapContainsKey(hll->map, ele, size)) {
		return;
	}
	else {
		pblMapAdd(hll->map, ele, size, &time, 4);
		hll->size++;
	}
}

int MapHLLCardinality(MapHLL *hll) {
	return hll->size;
}

void MapHllDestroy(MapHLL* hll) {
	pblMapFree(hll->map);
	free(hll);
}


#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "cmsketch.h"

/*
	a simple test for cmsketch implementation.
*/

int main_testcm() {
	int key = 199, key2 = 199;
	int stopkey = 0;
	CountMinSketch* cm = CountMinSketchCreateWithDAndW(3, 100, 0x99496f1ddc863e6fL);

	CountMinSketchAdd(cm, &key, 4, 1);
	printf("%d time\n", CountMinSketchEstimateFrequency(cm, &key2, 4));

	scanf("%d", &stopkey);
	return 0;
}
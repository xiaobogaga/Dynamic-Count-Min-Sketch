#include <stdio.h>
#include "hash.h"

#define seed 0x99496f1ddc863e6fL

/*
	test hash function.
*/
int testhash() {
	int hash1, hash2;
	int key1 = 1, key2 = 2;
	int stopkey;
	hash1 = MurmurHash3_64(&key1, sizeof(int), seed);
	hash2 = MurmurHash3_64(&key2, sizeof(int), seed);

	printf("hash 1 : %d, hash2 is %d\n", hash1, hash2);
	scanf_s("%d", &stopkey);
	return 0;
}
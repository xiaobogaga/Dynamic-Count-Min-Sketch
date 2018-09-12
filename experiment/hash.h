
#ifndef EXPERIMENT_HASH_H
#define EXPERIMENT_HASH_H

#include "mytypes.h"

void MurmurHash3_128(const void *key, const Size len, const uint64_t seed, void *out);
uint64_t MurmurHash3_64(const void *key, const Size len, const uint64_t seed);
uint32_t MurmurHash3_x86_32(const void *key, uint32_t len, uint32_t seed);

#endif
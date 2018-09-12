#include "hash.h"

static inline uint64_t
rotl64(uint64_t x, int8_t r)
{
	return (x << r) | (x >> (64 - r));
}

static inline uint64_t
getblock64(const uint64_t *p, int i)
{
	return p[i];
}

static inline
uint64_t fmix64(uint64_t k)
{
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccd;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53;
	k ^= k >> 33;

	return k;
}

#define	ROTL32(x, r)	((x) << (r)) | ((x) >> (32 - (r)))

uint32_t MurmurHash3_x86_32(const void *key, uint32_t len, uint32_t seed) {
	const uint8_t *data = (const uint8_t *)key;
	const int32_t nblocks = (int32_t)len / 4;

	uint32_t h1 = seed;
	int i;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;
	const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);

	for (i = -nblocks; i; i++)
	{
		uint32_t k1 = blocks[i];

		k1 *= c1;
		k1 = ROTL32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = ROTL32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	const uint8_t * tail = (const uint8_t *)(data + nblocks * 4);

	uint32_t k1 = 0;

	switch (len & 3)
	{
	case 3:
		k1 ^= (uint32_t)tail[2] << 16;
	case 2:
		k1 ^= (uint32_t)tail[1] << 8;
	case 1:
		k1 ^= tail[0];
		k1 *= c1;
		k1 = ROTL32(k1, 15);
		k1 *= c2;
		h1 ^= k1;
	};

	h1 ^= len;

	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;

	return h1;
}

void
MurmurHash3_128(const void *key, const Size len, const uint64_t seed, void *out)
{
	const uint8_t * data = (const uint8_t*)key;
	const int nblocks = len / 16;

	uint64_t h1 = seed;
	uint64_t h2 = seed;

	uint64_t c1 = 0x87c37b91114253d5;
	uint64_t c2 = 0x4cf5ad432745937f;

	//----------
	// body
	int i;
	const uint64_t * blocks = (const uint64_t *)(data);

	const uint8_t * tail;
	uint64_t k1 = 0;
	uint64_t k2 = 0;

	for (i = 0; i < nblocks; i++)
	{
		uint64_t k1 = getblock64(blocks, i * 2 + 0);
		uint64_t k2 = getblock64(blocks, i * 2 + 1);

		k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;

		h1 = rotl64(h1, 27); h1 += h2; h1 = h1 * 5 + 0x52dce729;

		k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;

		h2 = rotl64(h2, 31); h2 += h1; h2 = h2 * 5 + 0x38495ab5;
	}

	//----------
	// tail

	tail = (const uint8_t*)(data + nblocks * 16);

	switch (len & 15)
	{
	case 15: k2 ^= ((uint64_t)(tail[14])) << 48;
	case 14: k2 ^= ((uint64_t)(tail[13])) << 40;
	case 13: k2 ^= ((uint64_t)(tail[12])) << 32;
	case 12: k2 ^= ((uint64_t)(tail[11])) << 24;
	case 11: k2 ^= ((uint64_t)(tail[10])) << 16;
	case 10: k2 ^= ((uint64_t)(tail[9])) << 8;
	case  9: k2 ^= ((uint64_t)(tail[8])) << 0;
		k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;

	case  8: k1 ^= ((uint64_t)(tail[7])) << 56;
	case  7: k1 ^= ((uint64_t)(tail[6])) << 48;
	case  6: k1 ^= ((uint64_t)(tail[5])) << 40;
	case  5: k1 ^= ((uint64_t)(tail[4])) << 32;
	case  4: k1 ^= ((uint64_t)(tail[3])) << 24;
	case  3: k1 ^= ((uint64_t)(tail[2])) << 16;
	case  2: k1 ^= ((uint64_t)(tail[1])) << 8;
	case  1: k1 ^= ((uint64_t)(tail[0])) << 0;
		k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len; h2 ^= len;

	h1 += h2;
	h2 += h1;

	h1 = fmix64(h1);
	h2 = fmix64(h2);

	h1 += h2;
	h2 += h1;

	((uint64_t*)out)[0] = h1;
	((uint64_t*)out)[1] = h2;
}

uint64_t
MurmurHash3_64(const void *key, const Size len, const uint64_t seed)
{
	uint64_t hash[2];
	MurmurHash3_128(key, len, seed, &hash);
	return hash[0];
}


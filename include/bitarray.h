#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define BIT_ARRAY_SIZE(elems) (((elems) + 7) / 8)

static inline bool bit_array_get(uint8_t *arr, size_t pos)
{
	return arr[pos / 8] & (1 << (pos % 8));
}

static inline void bit_array_set(uint8_t *arr, size_t pos, bool val)
{
	uint8_t bit = (1 << (pos % 8));
	arr[pos / 8] = val ? arr[pos / 8] | bit : arr[pos / 8] & ~bit;
}

// Set the bit at the given position, and return the value _before_ it.
static inline bool bit_array_test_set(uint8_t *arr, size_t pos)
{
	uint8_t bit = (1 << (pos % 8));
	bool res = arr[pos / 8] & bit;
	arr[pos / 8] |= bit;
	return res;
}

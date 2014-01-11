#pragma once

#include <stdint.h>
#include <cstdlib>
#include <limits.h>

#define int8	int8_t
#define int16   int16_t
#define int32	int32_t
#define int16	int16_t
#define int64	int64_t
#define uint8	uint8_t
#define uint16  uint16_t
#define uint32	uint32_t
#define uint16	uint16_t
#define uint64	uint64_t

#define DWORD	int32

#define INVALID_HANDLE_VALUE NULL

#define D3DFORMAT uint32

enum texture_format{
	fmt_unknown,
	fmt_x8r8g8b8,
	fmt_a8r8g8b8,
	fmt_a4r4g4b4,
	fmt_r5g6b5,
	fmt_a8,
	fmt_dxt1,
	fmt_dxt5,
};

/*
int max(int d1, int d2)
{
	return d1 > d2 ? d1 : d2;
}

int min(int d1, int d2)
{
	return d1 < d2 ? d1 : d2;
}
*/
#define MAX(d1, d2) ((d1) > (d2) ? (d1) : (d2))
#define MIN(d1, d2) ((d1) < (d2) ? (d1) : (d2))


#ifndef MACHGATE_ARM64_ENC_H
#define MACHGATE_ARM64_ENC_H

#include <stdint.h>

#define ENC_NOP 0xd503201fu

static inline int arm64_branch_in_range(int64_t byte_offset)
{
	return (byte_offset & 3) == 0 &&
	       byte_offset >= -(INT64_C(1) << 27) &&
	       byte_offset < (INT64_C(1) << 27);
}

static inline uint32_t enc_b(int32_t word_offset)
{
	return 0x14000000u | ((uint32_t)word_offset & 0x03ffffffu);
}

#endif

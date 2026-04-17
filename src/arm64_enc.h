#ifndef _ARM64_ENC_H_
#define _ARM64_ENC_H_

#include <stdint.h>

#define ENC_NOP 0xD503201Fu
#define ENC_RET 0xD65F03C0u

/* B <target>: word_offset is (target - pc) / 4, must fit in 26-bit signed. */
static inline uint32_t enc_b(int32_t word_offset)
{
	return 0x14000000u | ((uint32_t)word_offset & 0x03FFFFFFu);
}

static inline uint32_t enc_bl(int32_t word_offset)
{
	return 0x94000000u | ((uint32_t)word_offset & 0x03FFFFFFu);
}

static inline int arm64_branch_in_range(int64_t byte_offset)
{
	if (byte_offset & 3) return 0;
	int64_t word_offset = byte_offset >> 2;
	return word_offset >= -(1 << 25) && word_offset < (1 << 25);
}

#endif /* _ARM64_ENC_H_ */

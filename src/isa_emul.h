#ifndef _ISA_EMUL_H_
#define _ISA_EMUL_H_

#include <stdint.h>
#include <stddef.h>

/*
 * ISA extension emulation for ARMv8.0 targets.
 *
 * Apple Silicon uses instructions from ARMv8.1+ (LSE atomics: LDADD,
 * SWP, CAS, etc.) and ARMv8.2+ (SHA3: BCAX) that are not available
 * on ARMv8.0 cores (Cortex-A35/A53/A55).
 *
 * This module replaces each unsupported instruction in-place with a
 * branch to an island containing an equivalent ARMv8.0 sequence.
 */

/* Patch unsupported ISA extension instructions in a code region.
 * code:      start of executable code (must be writable)
 * code_size: size of code region in bytes
 * pool:      pointer to current position in island pool (advanced on return)
 * pool_end:  end of island pool
 * Returns:   number of instructions patched */
int isa_emul_patch(uint32_t *code, size_t code_size,
                   uint32_t **pool, uint32_t *pool_end);

size_t isa_emul_estimate_pool_size(uint32_t *code, size_t code_size);

#endif /* _ISA_EMUL_H_ */

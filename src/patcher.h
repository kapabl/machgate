/*
 * Generic binary patcher for loaded Mach-O binaries.
 *
 * Reads a patch file and applies instruction-level patches to the
 * loaded binary. Used for game-specific fixes like bypassing Steam
 * checks, framerate caps, etc.
 *
 * Legacy format (still supported, one per line):
 *   <virtual_address_hex> <instruction_hex>  [# comment]
 *
 * Symbolic format:
 *   at  <locexpr>   <insnexpr>   [; expect <hex>]
 *   let <label> =   <locexpr>
 *
 *   <locexpr>  = sym:<Name>[+|-<hex>]
 *              | sym:<Name> find <pattern> [within <hex>]
 *              | <label>[+<hex>]
 *              | <hexaddr>
 *
 *   <insnexpr> = <hex> | nop | ret
 *              | b <locexpr> | bl <locexpr>
 *
 *   <pattern>  = 8 hex nibbles with `?` wildcards (e.g. 531F79??)
 *
 * The evaluator is all-or-nothing: any parse/resolve/encode error aborts
 * the whole file before any bytes are written.
 */

#ifndef PATCHER_H
#define PATCHER_H

#include <stdint.h>

/*
 * Apply patches from a file to the loaded binary.
 *
 * Parameters:
 *   patch_file - path to the patch config file
 *   mh         - loaded Mach-O header (may be NULL; only legacy absolute-
 *                address lines will resolve in that case)
 *   slide      - ASLR slide (added to absolute addresses)
 *
 * Returns:
 *   Number of patches applied, or -1 on error.
 */
int patcher_apply(const char* patch_file, void* mh, uintptr_t slide);

#endif /* PATCHER_H */

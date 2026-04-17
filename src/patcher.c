/*
 * Generic binary patcher for loaded Mach-O binaries.
 * See patcher.h for the file-format grammar.
 *
 * All-or-nothing evaluation: parse → resolve → encode → write. Any failure
 * in the first three passes aborts the file with the original bytes intact.
 */

#include "patcher.h"
#include "resolver.h"
#include "arm64_enc.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

/* ---------- Location expression ---------- */

enum loc_kind {
	LOC_ABS,         /* <hexaddr> (already slid) */
	LOC_SYM,         /* sym:<name>+/-<off> */
	LOC_SYM_FIND,    /* sym:<name> find <pattern> [within <cap>] */
	LOC_LABEL,       /* <label>+<off> */
};

#define LOC_MAX_PATTERNS 8

struct loc_expr {
	enum loc_kind kind;
	char sym[256];       /* or label name */
	int64_t offset;
	/* pattern/within only for LOC_SYM_FIND */
	uint32_t mask[LOC_MAX_PATTERNS];
	uint32_t value[LOC_MAX_PATTERNS];
	int npatterns;
	uint64_t within;
};

/* ---------- Instruction expression ---------- */

enum insn_kind {
	INS_RAW,             /* raw 32-bit hex */
	INS_B,               /* b <locexpr> */
	INS_BL,              /* bl <locexpr> */
};

struct insn_expr {
	enum insn_kind kind;
	uint32_t raw;
	struct loc_expr target;  /* for INS_B/INS_BL */
};

/* ---------- Parsed line ---------- */

enum line_kind { LINE_AT, LINE_LET, LINE_ABS };

struct patch_line {
	enum line_kind kind;
	int src_line;
	struct loc_expr loc;     /* for LINE_AT */
	struct insn_expr insn;   /* for LINE_AT */
	int has_expect;
	uint32_t expected;
	char label_name[128];    /* for LINE_LET */
	/* resolved: */
	uintptr_t resolved_addr; /* LINE_AT */
	uint32_t resolved_insn;
};

struct label {
	char name[128];
	uintptr_t addr;
	int resolved;
};

/* ---------- Small helpers ---------- */

static void strip_comment(char* s)
{
	for (char* p = s; *p; p++) {
		if (*p == '#') { *p = '\0'; return; }
	}
}

static char* skip_ws(char* p) { while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++; return p; }

static int is_hex_digit(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/* Consume a hex literal (optional 0x prefix). Returns ptr past end, or NULL on fail. */
static char* parse_hex_u64(char* p, uint64_t* out)
{
	p = skip_ws(p);
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
	if (!is_hex_digit(*p)) return NULL;
	char* end;
	*out = strtoull(p, &end, 16);
	if (end == p) return NULL;
	return end;
}

/* Is this line in legacy absolute format?  First non-ws token is a bare hex
 * number (either 0x-prefixed or pure hex). */
static int looks_legacy(const char* p)
{
	while (*p == ' ' || *p == '\t') p++;
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) return 1;
	if (!is_hex_digit(*p)) return 0;
	/* Distinguish "at", "let" etc. from pure hex. Scan the first token. */
	const char* q = p;
	while (is_hex_digit(*q)) q++;
	return (*q == ' ' || *q == '\t' || *q == '\0' || *q == '\n');
}

/* Parse an 8-nibble pattern with ? wildcards into (mask, value). */
static int parse_pattern(const char* s, uint32_t* mask, uint32_t* value)
{
	uint32_t m = 0, v = 0;
	int n = 0;
	for (; n < 8 && s[n]; n++) {
		m <<= 4; v <<= 4;
		char c = s[n];
		if (c == '?') {
			/* mask nibble = 0, value nibble = 0 */
		} else if (c >= '0' && c <= '9') {
			m |= 0xF; v |= (c - '0');
		} else if (c >= 'a' && c <= 'f') {
			m |= 0xF; v |= (c - 'a' + 10);
		} else if (c >= 'A' && c <= 'F') {
			m |= 0xF; v |= (c - 'A' + 10);
		} else {
			return -1;
		}
	}
	if (n != 8) return -1;
	/* next char must be terminator-ish */
	if (s[8] && !isspace((unsigned char)s[8]) && s[8] != ';' && s[8] != '#')
		return -1;
	*mask = m; *value = v;
	return 0;
}

/* ---------- Location / instruction parsers ---------- */

static char* parse_loc_expr(char* p, struct loc_expr* out, const char* patch_file, int line_num)
{
	p = skip_ws(p);

	if (strncmp(p, "sym:", 4) == 0) {
		p += 4;
		size_t n = 0;
		while (*p && !isspace((unsigned char)*p) && *p != '+' && *p != '-' && *p != ';') {
			if (n + 1 >= sizeof(out->sym)) {
				fprintf(stderr, "patcher: %s:%d: symbol name too long\n", patch_file, line_num);
				return NULL;
			}
			out->sym[n++] = *p++;
		}
		out->sym[n] = '\0';
		if (n == 0) {
			fprintf(stderr, "patcher: %s:%d: empty symbol name\n", patch_file, line_num);
			return NULL;
		}

		/* Optional "+<hex>" or "-<hex>" directly appended */
		int64_t off = 0;
		if (*p == '+' || *p == '-') {
			int neg = (*p == '-'); p++;
			uint64_t v; char* q = parse_hex_u64(p, &v);
			if (!q) {
				fprintf(stderr, "patcher: %s:%d: bad offset after symbol\n", patch_file, line_num);
				return NULL;
			}
			off = neg ? -(int64_t)v : (int64_t)v;
			p = q;
		}
		out->offset = off;

		/* Optional " find <pattern> [<pattern>...] [within <hex>]" */
		char* save = skip_ws(p);
		if (strncmp(save, "find ", 5) == 0) {
			save = skip_ws(save + 5);
			out->npatterns = 0;
			for (;;) {
				if (out->npatterns >= LOC_MAX_PATTERNS) {
					fprintf(stderr, "patcher: %s:%d: too many find patterns\n",
					        patch_file, line_num);
					return NULL;
				}
				if (parse_pattern(save, &out->mask[out->npatterns],
				                  &out->value[out->npatterns]) != 0) {
					fprintf(stderr, "patcher: %s:%d: bad pattern (expect 8 hex nibbles with ?)\n",
					        patch_file, line_num);
					return NULL;
				}
				out->npatterns++;
				save += 8;
				save = skip_ws(save);
				/* Keep parsing patterns while the next token starts with a
				 * hex digit or '?'. Otherwise hand back to caller. */
				if (!(is_hex_digit(*save) || *save == '?'))
					break;
			}
			out->within = 0;
			if (strncmp(save, "within ", 7) == 0) {
				save = skip_ws(save + 7);
				uint64_t w; char* q = parse_hex_u64(save, &w);
				if (!q) {
					fprintf(stderr, "patcher: %s:%d: bad 'within' value\n",
					        patch_file, line_num);
					return NULL;
				}
				out->within = w; save = q;
			}
			out->kind = LOC_SYM_FIND;
			return save;
		}
		out->kind = LOC_SYM;
		return p;
	}

	/* Bare hex? Legacy-style absolute inside a new directive. */
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		uint64_t v; char* q = parse_hex_u64(p, &v);
		if (!q) return NULL;
		out->kind = LOC_ABS;
		out->offset = (int64_t)v;
		return q;
	}

	/* Otherwise treat as a label name, optional +<hex> */
	size_t n = 0;
	while (*p && !isspace((unsigned char)*p) && *p != '+' && *p != ';') {
		if (n + 1 >= sizeof(out->sym)) {
			fprintf(stderr, "patcher: %s:%d: label name too long\n", patch_file, line_num);
			return NULL;
		}
		out->sym[n++] = *p++;
	}
	out->sym[n] = '\0';
	if (n == 0) {
		fprintf(stderr, "patcher: %s:%d: expected symbol, label or hex\n",
		        patch_file, line_num);
		return NULL;
	}
	out->offset = 0;
	if (*p == '+') {
		p++;
		uint64_t v; char* q = parse_hex_u64(p, &v);
		if (!q) {
			fprintf(stderr, "patcher: %s:%d: bad offset after label\n",
			        patch_file, line_num);
			return NULL;
		}
		out->offset = (int64_t)v; p = q;
	}
	out->kind = LOC_LABEL;
	return p;
}

static char* parse_insn_expr(char* p, struct insn_expr* out,
                             const char* patch_file, int line_num)
{
	p = skip_ws(p);

	if (strncmp(p, "nop", 3) == 0 && (p[3] == '\0' || isspace((unsigned char)p[3]) || p[3] == ';')) {
		out->kind = INS_RAW; out->raw = ENC_NOP; return p + 3;
	}
	if (strncmp(p, "ret", 3) == 0 && (p[3] == '\0' || isspace((unsigned char)p[3]) || p[3] == ';')) {
		out->kind = INS_RAW; out->raw = ENC_RET; return p + 3;
	}
	if (strncmp(p, "bl ", 3) == 0) {
		out->kind = INS_BL;
		return parse_loc_expr(p + 3, &out->target, patch_file, line_num);
	}
	if (strncmp(p, "b ", 2) == 0) {
		out->kind = INS_B;
		return parse_loc_expr(p + 2, &out->target, patch_file, line_num);
	}
	/* Raw hex */
	uint64_t v; char* q = parse_hex_u64(p, &v);
	if (!q || v > 0xFFFFFFFFu) {
		fprintf(stderr, "patcher: %s:%d: expected nop/ret/b/bl or 32-bit hex\n",
		        patch_file, line_num);
		return NULL;
	}
	out->kind = INS_RAW; out->raw = (uint32_t)v;
	return q;
}

/* Optional trailing "; expect <hex>" */
static int parse_expect(char* p, struct patch_line* pl, const char* patch_file, int line_num)
{
	p = skip_ws(p);
	if (*p == '\0') return 0;
	if (*p != ';') {
		fprintf(stderr, "patcher: %s:%d: unexpected trailing text: %s\n",
		        patch_file, line_num, p);
		return -1;
	}
	p = skip_ws(p + 1);
	if (strncmp(p, "expect ", 7) != 0) {
		fprintf(stderr, "patcher: %s:%d: expected 'expect <hex>' after ';'\n",
		        patch_file, line_num);
		return -1;
	}
	p = skip_ws(p + 7);
	uint64_t v; char* q = parse_hex_u64(p, &v);
	if (!q || v > 0xFFFFFFFFu) {
		fprintf(stderr, "patcher: %s:%d: bad expect value\n", patch_file, line_num);
		return -1;
	}
	pl->has_expect = 1;
	pl->expected = (uint32_t)v;
	return 0;
}

/* ---------- Resolution ---------- */

static int find_label(struct label* labels, size_t n, const char* name, uintptr_t* out)
{
	for (size_t i = 0; i < n; i++) {
		if (strcmp(labels[i].name, name) == 0) {
			if (!labels[i].resolved) return -1;
			*out = labels[i].addr;
			return 0;
		}
	}
	return -1;
}

static int pattern_find_unique(uintptr_t start, uintptr_t end,
                               const uint32_t* masks, const uint32_t* values,
                               int nwords,
                               uintptr_t* out_hit, int* out_count)
{
	int count = 0;
	uintptr_t hit = 0;
	uintptr_t need = (uintptr_t)nwords * 4;
	if (need == 0 || end < start || (end - start) < need) {
		*out_count = 0; *out_hit = 0; return -1;
	}
	for (uintptr_t a = start; a + need <= end; a += 4) {
		int ok = 1;
		for (int k = 0; k < nwords; k++) {
			uint32_t w = *(const uint32_t*)(a + (uintptr_t)k * 4);
			if ((w & masks[k]) != values[k]) { ok = 0; break; }
		}
		if (ok) {
			if (++count == 1) hit = a;
			if (count > 1) break;
		}
	}
	*out_count = count;
	*out_hit = hit;
	return count == 1 ? 0 : -1;
}

static int resolve_loc(const struct loc_expr* loc, void* mh, uintptr_t slide,
                       struct label* labels, size_t nlabels,
                       uintptr_t* out, const char* patch_file, int line_num)
{
	if (loc->kind == LOC_ABS) {
		*out = (uintptr_t)loc->offset + slide;
		return 0;
	}
	if (loc->kind == LOC_LABEL) {
		uintptr_t base;
		if (find_label(labels, nlabels, loc->sym, &base) != 0) {
			fprintf(stderr, "patcher: %s:%d: unresolved label '%s'\n",
			        patch_file, line_num, loc->sym);
			return -1;
		}
		*out = base + (uintptr_t)loc->offset;
		return 0;
	}
	/* sym:... */
	if (!mh) {
		fprintf(stderr, "patcher: %s:%d: symbol '%s' needed but no Mach-O loaded\n",
		        patch_file, line_num, loc->sym);
		return -1;
	}
	uintptr_t sym_start = resolver_lookup_symbol(mh, slide, loc->sym);
	if (!sym_start) {
		fprintf(stderr, "patcher: %s:%d: symbol not found: %s\n",
		        patch_file, line_num, loc->sym);
		return -1;
	}
	if (loc->kind == LOC_SYM) {
		*out = sym_start + (uintptr_t)loc->offset;
		return 0;
	}
	/* LOC_SYM_FIND: bounded scan */
	uintptr_t seg_start, seg_end;
	if (resolver_symbol_extent(mh, slide, loc->sym, &seg_start, &seg_end) != 0) {
		fprintf(stderr, "patcher: %s:%d: can't determine extent of '%s'\n",
		        patch_file, line_num, loc->sym);
		return -1;
	}
	uintptr_t end = seg_end;
	if (loc->within && seg_start + loc->within < end)
		end = seg_start + loc->within;
	int count = 0;
	uintptr_t hit = 0;
	pattern_find_unique(seg_start, end, loc->mask, loc->value, loc->npatterns,
	                    &hit, &count);
	if (count == 0) {
		fprintf(stderr, "patcher: %s:%d: pattern not found in %s\n",
		        patch_file, line_num, loc->sym);
		return -1;
	}
	if (count > 1) {
		fprintf(stderr, "patcher: %s:%d: pattern ambiguous in %s (%d hits)\n",
		        patch_file, line_num, loc->sym, count);
		return -1;
	}
	*out = hit;
	return 0;
}

/* ---------- Page-at-a-time writer ---------- */

static int write_word(uintptr_t target, uint32_t insn, long page_size)
{
	uintptr_t page = target & ~(uintptr_t)(page_size - 1);
	if (mprotect((void*)page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
		fprintf(stderr, "patcher: mprotect failed at 0x%lx\n", (unsigned long)target);
		return -1;
	}
	uint32_t old = *(uint32_t*)target;
	*(uint32_t*)target = insn;
	__builtin___clear_cache((char*)target, (char*)(target + 4));
	mprotect((void*)page, page_size, PROT_READ | PROT_EXEC);
	fprintf(stderr, "patcher: 0x%lx: %08x -> %08x\n",
	        (unsigned long)target, old, insn);
	return 0;
}

/* ---------- Entry point ---------- */

int patcher_apply(const char* patch_file, void* mh, uintptr_t slide)
{
	FILE* f = fopen(patch_file, "r");
	if (!f) {
		fprintf(stderr, "patcher: cannot open %s\n", patch_file);
		return -1;
	}

	/* Pass 1: parse */
	size_t cap = 64, n = 0;
	struct patch_line* lines = calloc(cap, sizeof(*lines));
	size_t lcap = 16, lbcount = 0;
	struct label* labels = calloc(lcap, sizeof(*labels));
	if (!lines || !labels) { fclose(f); free(lines); free(labels); return -1; }

	char buf[512];
	int line_num = 0;
	int parse_failed = 0;

	while (fgets(buf, sizeof(buf), f)) {
		line_num++;
		strip_comment(buf);
		char* p = skip_ws(buf);
		if (*p == '\0' || *p == '\n') continue;

		if (n == cap) {
			cap *= 2;
			lines = realloc(lines, cap * sizeof(*lines));
			if (!lines) { parse_failed = 1; break; }
			memset(lines + n, 0, (cap - n) * sizeof(*lines));
		}
		struct patch_line* pl = &lines[n];
		pl->src_line = line_num;

		/* Legacy: <hex> <hex> */
		if (looks_legacy(p)) {
			uint64_t a, i;
			char* q = parse_hex_u64(p, &a);
			if (!q) { parse_failed = 1; break; }
			q = parse_hex_u64(q, &i);
			if (!q || i > 0xFFFFFFFFu) {
				fprintf(stderr, "patcher: %s:%d: bad legacy format\n", patch_file, line_num);
				parse_failed = 1; break;
			}
			pl->kind = LINE_ABS;
			pl->loc.kind = LOC_ABS;
			pl->loc.offset = (int64_t)a;
			pl->insn.kind = INS_RAW;
			pl->insn.raw = (uint32_t)i;
			n++; continue;
		}

		if (strncmp(p, "let ", 4) == 0) {
			p = skip_ws(p + 4);
			size_t ln = 0;
			while (*p && !isspace((unsigned char)*p) && *p != '=') {
				if (ln + 1 >= sizeof(pl->label_name)) { parse_failed = 1; break; }
				pl->label_name[ln++] = *p++;
			}
			pl->label_name[ln] = '\0';
			p = skip_ws(p);
			if (*p != '=') {
				fprintf(stderr, "patcher: %s:%d: expected '=' in let\n", patch_file, line_num);
				parse_failed = 1; break;
			}
			p = skip_ws(p + 1);
			char* q = parse_loc_expr(p, &pl->loc, patch_file, line_num);
			if (!q) { parse_failed = 1; break; }
			pl->kind = LINE_LET;
			n++; continue;
		}

		if (strncmp(p, "at ", 3) == 0) {
			p = skip_ws(p + 3);
			char* q = parse_loc_expr(p, &pl->loc, patch_file, line_num);
			if (!q) { parse_failed = 1; break; }
			q = parse_insn_expr(q, &pl->insn, patch_file, line_num);
			if (!q) { parse_failed = 1; break; }
			if (parse_expect(q, pl, patch_file, line_num) != 0) { parse_failed = 1; break; }
			pl->kind = LINE_AT;
			n++; continue;
		}

		fprintf(stderr, "patcher: %s:%d: unknown directive: %s\n",
		        patch_file, line_num, p);
		parse_failed = 1;
		break;
	}
	fclose(f);
	if (parse_failed) { free(lines); free(labels); return -1; }

	/* Pass 2a: pre-register label names (unresolved) so forward refs are detectable */
	for (size_t i = 0; i < n; i++) {
		if (lines[i].kind != LINE_LET) continue;
		if (lbcount == lcap) {
			lcap *= 2; labels = realloc(labels, lcap * sizeof(*labels));
			if (!labels) { free(lines); return -1; }
		}
		strncpy(labels[lbcount].name, lines[i].label_name, sizeof(labels[0].name) - 1);
		labels[lbcount].name[sizeof(labels[0].name) - 1] = '\0';
		labels[lbcount].resolved = 0;
		lbcount++;
	}

	/* Pass 2b: resolve let labels in file order. Each loc_expr can reference any
	 * earlier label (or a symbol/abs), which covers all useful patterns. */
	for (size_t i = 0; i < n; i++) {
		if (lines[i].kind != LINE_LET) continue;
		uintptr_t addr;
		if (resolve_loc(&lines[i].loc, mh, slide, labels, lbcount, &addr,
		                patch_file, lines[i].src_line) != 0) {
			free(lines); free(labels); return -1;
		}
		for (size_t j = 0; j < lbcount; j++) {
			if (strcmp(labels[j].name, lines[i].label_name) == 0) {
				labels[j].addr = addr;
				labels[j].resolved = 1;
				break;
			}
		}
	}

	/* Pass 2c: resolve every 'at' line's target address */
	for (size_t i = 0; i < n; i++) {
		if (lines[i].kind == LINE_LET) continue;
		if (resolve_loc(&lines[i].loc, mh, slide, labels, lbcount,
		                &lines[i].resolved_addr, patch_file, lines[i].src_line) != 0) {
			free(lines); free(labels); return -1;
		}
	}

	/* Pass 3: encode each instruction */
	for (size_t i = 0; i < n; i++) {
		if (lines[i].kind == LINE_LET) continue;
		struct patch_line* pl = &lines[i];
		if (pl->insn.kind == INS_RAW) {
			pl->resolved_insn = pl->insn.raw;
		} else {
			uintptr_t tgt;
			if (resolve_loc(&pl->insn.target, mh, slide, labels, lbcount,
			                &tgt, patch_file, pl->src_line) != 0) {
				free(lines); free(labels); return -1;
			}
			int64_t diff = (int64_t)tgt - (int64_t)pl->resolved_addr;
			if (!arm64_branch_in_range(diff)) {
				fprintf(stderr, "patcher: %s:%d: branch displacement 0x%lx out of range or misaligned\n",
				        patch_file, pl->src_line, (long)diff);
				free(lines); free(labels); return -1;
			}
			int32_t woff = (int32_t)(diff >> 2);
			pl->resolved_insn = (pl->insn.kind == INS_BL) ? enc_bl(woff) : enc_b(woff);
		}
		if (pl->has_expect) {
			uint32_t actual = *(uint32_t*)pl->resolved_addr;
			if (actual != pl->expected) {
				fprintf(stderr, "patcher: %s:%d: expect mismatch at 0x%lx: got %08x, expected %08x\n",
				        patch_file, pl->src_line,
				        (unsigned long)pl->resolved_addr, actual, pl->expected);
				free(lines); free(labels); return -1;
			}
		}
	}

	/* Pass 4: write */
	long page_size = sysconf(_SC_PAGESIZE);
	int applied = 0;
	for (size_t i = 0; i < n; i++) {
		if (lines[i].kind == LINE_LET) continue;
		if (write_word(lines[i].resolved_addr, lines[i].resolved_insn, page_size) != 0) {
			fprintf(stderr, "patcher: %s:%d: write failed mid-apply\n",
			        patch_file, lines[i].src_line);
			free(lines); free(labels); return -1;
		}
		applied++;
	}

	free(lines); free(labels);
	fprintf(stderr, "patcher: applied %d patches from %s\n", applied, patch_file);
	return applied;
}

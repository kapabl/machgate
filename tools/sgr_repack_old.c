/*
 * sgr_repack_old.c — Repack extracted files into an old-format Sugar .sgr archive
 *
 * Companion to sgr_extract_old.c. The old format entry header is
 * name_len(2) + name + data_size(4) + data (no type byte).
 *
 * Build: cc -o sgr_repack_old sgr_repack_old.c -lz
 * Usage: sgr_repack_old <input_dir> <output.sgr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <zlib.h>

static uint32_t sx, sy, sz, sw, sv;

static void sugar_srand(uint32_t seed)
{
    sx = seed;
    sy = (sx >> 1) + 0x159a55e5;
    sz = (sx >> 3) + 0x1f123bb5;
    sw = (sx >> 5) + 0x05491333;
    sv = (sx >> 7) + 0x34dad465;
}

static uint32_t sugar_rnd(void)
{
    uint32_t t = sz;
    uint32_t u = sx ^ (sx >> 7);
    sx = sy;
    sy = sz;
    sz = sw;
    sw = sv;
    sv = u ^ (u << 13) ^ sv ^ (sv << 6);
    return sv * ((t << 1) | 1);
}

static void write_be32(uint8_t *p, uint32_t v)
{
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

static void write_be16(uint8_t *p, uint16_t v)
{
    p[0] = (v >> 8) & 0xff;
    p[1] = v & 0xff;
}

static void encode_buffer(uint8_t *buf, size_t len, uint32_t seed)
{
    sugar_srand(seed);
    uint32_t skip = sugar_rnd() & 0xf;
    for (uint32_t i = 0; i < skip; i++)
        sugar_rnd();
    for (size_t i = 0; i < len; i += 2)
        buf[i] ^= (uint8_t)sugar_rnd();
}

struct buf {
    uint8_t *data;
    size_t len;
    size_t cap;
};

static void buf_init(struct buf *b)
{
    b->cap = 1024 * 1024;
    b->data = malloc(b->cap);
    b->len = 0;
}

static void buf_ensure(struct buf *b, size_t need)
{
    while (b->len + need > b->cap) {
        b->cap *= 2;
        b->data = realloc(b->data, b->cap);
    }
}

static void buf_append(struct buf *b, const void *src, size_t n)
{
    buf_ensure(b, n);
    memcpy(b->data + b->len, src, n);
    b->len += n;
}

static void buf_append_be16(struct buf *b, uint16_t v)
{
    uint8_t tmp[2];
    write_be16(tmp, v);
    buf_append(b, tmp, 2);
}

static void buf_append_be32(struct buf *b, uint32_t v)
{
    uint8_t tmp[4];
    write_be32(tmp, v);
    buf_append(b, tmp, 4);
}

static void buf_append_str(struct buf *b, const char *s)
{
    uint16_t len = (uint16_t)strlen(s);
    buf_append_be16(b, len);
    buf_append(b, s, len);
}

static const char *disk_name(const char *entry_name)
{
    const char *p = entry_name;
    while (strncmp(p, "../", 3) == 0)
        p += 3;
    return p;
}

static void encode_fs_name(const char *in, char *out, size_t outsz)
{
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 12 < outsz; i++) {
        const char *repl = NULL;
        switch (in[i]) {
            case '*':  repl = "__SGR_STAR__";   break;
            case '?':  repl = "__SGR_QMARK__";  break;
            case ':':  repl = "__SGR_COLON__";  break;
            case '<':  repl = "__SGR_LT__";     break;
            case '>':  repl = "__SGR_GT__";     break;
            case '|':  repl = "__SGR_PIPE__";   break;
            case '"':  repl = "__SGR_QUOT__";   break;
        }
        if (repl) {
            size_t n = strlen(repl);
            memcpy(out + j, repl, n); j += n;
        } else {
            out[j++] = in[i];
        }
    }
    out[j] = '\0';
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_dir> <output.sgr>\n", argv[0]);
        return 1;
    }

    const char *in_dir = argv[1];
    const char *out_path = argv[2];

    char manifest_path[4096];
    snprintf(manifest_path, sizeof(manifest_path), "%s/.sgr_manifest", in_dir);
    FILE *mf = fopen(manifest_path, "r");
    if (!mf) {
        fprintf(stderr, "Cannot open manifest %s: %s\n",
                manifest_path, strerror(errno));
        return 1;
    }

    char title[256] = {0}, dev[256] = {0}, root[256] = {0};
    #define MAX_ENTRIES 4096
    #define MAX_NAME 512
    char (*entries)[MAX_NAME] = calloc(MAX_ENTRIES, MAX_NAME);
    int nentries = 0;

    char line[8192];
    while (fgets(line, sizeof(line), mf)) {
        size_t l = strlen(line);
        while (l > 0 && (line[l-1] == '\n' || line[l-1] == '\r'))
            line[--l] = '\0';

        if (strncmp(line, "TITLE\t", 6) == 0) {
            snprintf(title, sizeof(title), "%s", line + 6);
        } else if (strncmp(line, "DEV\t", 4) == 0) {
            snprintf(dev, sizeof(dev), "%s", line + 4);
        } else if (strncmp(line, "ROOT\t", 5) == 0) {
            snprintf(root, sizeof(root), "%s", line + 5);
        } else if (strncmp(line, "ENTRY\t", 6) == 0) {
            if (nentries < MAX_ENTRIES) {
                /* Old-format manifest line is just ENTRY\t<name> */
                snprintf(entries[nentries++], MAX_NAME, "%s", line + 6);
            }
        }
    }
    fclose(mf);

    printf("Title: %s\n", title);
    printf("Dev: %s\n", dev);
    printf("Root: %s\n", root);
    printf("Entries: %d\n", nentries);

    struct buf archive;
    buf_init(&archive);

    buf_append_str(&archive, title);
    buf_append_str(&archive, dev);
    buf_append_str(&archive, root);

    for (int i = 0; i < nentries; i++) {
        const char *entry_name = entries[i];
        const char *dname = disk_name(entry_name);

        char encoded[4096];
        encode_fs_name(dname, encoded, sizeof(encoded));

        char fpath[8192];
        FILE *ef = NULL;
        if (!strchr(encoded, '/')) {
            snprintf(fpath, sizeof(fpath), "%s/%s.__sgr_entry__", in_dir, encoded);
            ef = fopen(fpath, "rb");
        }
        if (!ef) {
            snprintf(fpath, sizeof(fpath), "%s/%s", in_dir, encoded);
            ef = fopen(fpath, "rb");
        }
        if (!ef) {
            fprintf(stderr, "  ERROR: cannot read %s: %s\n", fpath, strerror(errno));
            return 1;
        }
        fseek(ef, 0, SEEK_END);
        long esize = ftell(ef);
        fseek(ef, 0, SEEK_SET);

        uint8_t *edata = malloc(esize);
        if (fread(edata, 1, esize, ef) != (size_t)esize) {
            fprintf(stderr, "  ERROR: short read on %s\n", fpath);
            free(edata);
            fclose(ef);
            return 1;
        }
        fclose(ef);

        size_t enc_size = esize + 4;
        uint8_t *enc_data = malloc(enc_size);
        memcpy(enc_data, edata, esize);
        free(edata);

        uint32_t entry_seed = (uint32_t)rand() ^ ((uint32_t)rand() << 16);
        encode_buffer(enc_data, esize, entry_seed);
        write_be32(enc_data + esize, entry_seed ^ 0x550f0f55);

        /* Old-format entry: name_len + name + size + data (no type byte) */
        uint16_t name_len = (uint16_t)strlen(entry_name);
        buf_append_be16(&archive, name_len);
        buf_append(&archive, entry_name, name_len);
        buf_append_be32(&archive, (uint32_t)enc_size);
        buf_append(&archive, enc_data, enc_size);
        free(enc_data);

        printf("  %s (%ld bytes)\n", entry_name, esize);
    }

    printf("Archive size (uncompressed): %zu\n", archive.len);

    uLongf comp_bound = compressBound(archive.len);
    uint8_t *compressed = malloc(comp_bound);
    uLongf comp_len = comp_bound;
    int zret = compress2(compressed, &comp_len, archive.data, archive.len, Z_DEFAULT_COMPRESSION);
    if (zret != Z_OK) {
        fprintf(stderr, "zlib compress failed: %d\n", zret);
        return 1;
    }

    printf("Compressed: %zu → %lu bytes\n", archive.len, (unsigned long)comp_len);

    size_t outer_len = 4 + comp_len;
    uint8_t *outer = malloc(outer_len);
    write_be32(outer, (uint32_t)archive.len);
    memcpy(outer + 4, compressed, comp_len);
    free(compressed);
    free(archive.data);

    srand(time(NULL));
    uint32_t seed = (uint32_t)rand() ^ ((uint32_t)rand() << 16);
    encode_buffer(outer, outer_len, seed);

    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create %s: %s\n", out_path, strerror(errno));
        free(outer);
        return 1;
    }
    fwrite(outer, 1, outer_len, out);

    uint8_t trailer[4];
    write_be32(trailer, seed ^ 0x550f0f55);
    fwrite(trailer, 1, 4, out);

    fclose(out);
    free(outer);

    free(entries);
    printf("Wrote %lu bytes to %s\n",
           (unsigned long)(outer_len + 4), out_path);
    return 0;
}

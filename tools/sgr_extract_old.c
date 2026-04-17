/*
 * sgr_extract_old.c — Extract PUNKCAKE Sugar engine .sgr archives (old format)
 *
 * Old Sugar format (Wratch's Den era) — reverse engineered from the old
 * Shotgun King decompile at shotgun_king/decompiled.old/method.sugar.c
 * (sugar::file::loadprj). Differs from the current format in one place:
 *
 *   Current entry: name_len(2) + name + type(1) + data_size(4) + data
 *   Old     entry: name_len(2) + name +          data_size(4) + data
 *
 * Everything else matches: same xoshiro128 outer XOR + zlib envelope, same
 * title/dev/root header, same per-entry XOR (stride 2) with 4-byte seed
 * trailer.
 *
 * Build: cc -o sgr_extract_old sgr_extract_old.c -lz
 * Usage: sgr_extract_old data.sgr [output_dir]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>
#include <zlib.h>

/* xoshiro128 PRNG — matches sugar::math */
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

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t read_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static void decode_buffer(uint8_t *buf, size_t len)
{
    size_t data_len = len - 4;
    uint32_t tail = read_be32(buf + data_len);
    sugar_srand(tail ^ 0x550f0f55);

    uint32_t skip = sugar_rnd() & 0xf;
    for (uint32_t i = 0; i < skip; i++)
        sugar_rnd();

    for (size_t i = 0; i < data_len; i += 2)
        buf[i] ^= (uint8_t)sugar_rnd();
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

static void handle_file_dir_conflict(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
        char moved[4096];
        snprintf(moved, sizeof(moved), "%s.__sgr_entry__", path);
        rename(path, moved);
    }
}

static int mkdirs(const char *path)
{
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            handle_file_dir_conflict(tmp);
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    handle_file_dir_conflict(tmp);
    return mkdir(tmp, 0755);
}

static int write_file(const char *dir, const char *name,
                      const uint8_t *data, size_t len)
{
    while (strncmp(name, "../", 3) == 0)
        name += 3;

    char encoded[4096];
    encode_fs_name(name, encoded, sizeof(encoded));

    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", dir, encoded);

    char *last_slash = strrchr(path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdirs(path);
        *last_slash = '/';
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "  ERROR: cannot write %s: %s\n", path, strerror(errno));
        return -1;
    }
    fwrite(data, 1, len, f);
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s data.sgr [output_dir]\n", argv[0]);
        return 1;
    }

    const char *sgr_path = argv[1];
    const char *out_dir = argc > 2 ? argv[2] : "sgr_out";

    FILE *f = fopen(sgr_path, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open %s: %s\n", sgr_path, strerror(errno));
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *raw = malloc(fsize);
    if (!raw || fread(raw, 1, fsize, f) != (size_t)fsize) {
        fprintf(stderr, "Read error\n");
        return 1;
    }
    fclose(f);

    printf("Read %ld bytes from %s\n", fsize, sgr_path);

    decode_buffer(raw, fsize);

    uint32_t dec_size = read_be32(raw);
    printf("Decompressed size: %u\n", dec_size);

    uint8_t *dec = malloc(dec_size);
    if (!dec) {
        fprintf(stderr, "Cannot allocate %u bytes\n", dec_size);
        return 1;
    }

    uLongf dest_len = dec_size;
    int zret = uncompress(dec, &dest_len, raw + 4, fsize - 8);
    if (zret != Z_OK) {
        fprintf(stderr, "zlib uncompress failed: %d\n", zret);
        free(raw);
        free(dec);
        return 1;
    }
    free(raw);

    printf("Decompressed %lu bytes\n", (unsigned long)dest_len);

    size_t pos = 0;

    uint16_t title_len = read_be16(dec + pos); pos += 2;
    char title[256] = {0};
    memcpy(title, dec + pos, title_len < 255 ? title_len : 255);
    pos += title_len;
    printf("Title: %s\n", title);

    uint16_t dev_len = read_be16(dec + pos); pos += 2;
    char dev[256] = {0};
    memcpy(dev, dec + pos, dev_len < 255 ? dev_len : 255);
    pos += dev_len;
    printf("Dev: %s\n", dev);

    uint16_t root_len = read_be16(dec + pos); pos += 2;
    char root[256] = {0};
    memcpy(root, dec + pos, root_len < 255 ? root_len : 255);
    pos += root_len;
    printf("Root: %s\n", root);

    mkdirs(out_dir);

    char manifest_path[4096];
    snprintf(manifest_path, sizeof(manifest_path), "%s/.sgr_manifest", out_dir);
    FILE *manifest = fopen(manifest_path, "w");
    if (!manifest) {
        fprintf(stderr, "Cannot create manifest %s: %s\n",
                manifest_path, strerror(errno));
        free(dec);
        return 1;
    }
    fprintf(manifest, "FORMAT\told\n");
    fprintf(manifest, "TITLE\t%s\n", title);
    fprintf(manifest, "DEV\t%s\n", dev);
    fprintf(manifest, "ROOT\t%s\n", root);

    /* Old format entry: name_len(2) + name + data_size(4) + data */
    int count = 0;
    while (pos + 6 < dest_len) {
        uint16_t name_len = read_be16(dec + pos); pos += 2;
        if (name_len == 0 || name_len > 2000 || pos + name_len + 4 > dest_len) break;

        char name[4096] = {0};
        memcpy(name, dec + pos, name_len < 4095 ? name_len : 4095);
        pos += name_len;

        uint32_t data_size = read_be32(dec + pos); pos += 4;
        if (pos + data_size > dest_len) {
            fprintf(stderr, "  WARNING: entry '%s' claims %u bytes but only %lu remain\n",
                    name, data_size, (unsigned long)(dest_len - pos));
            break;
        }

        printf("  %s (%u bytes)\n", name, data_size);
        fprintf(manifest, "ENTRY\t%s\n", name);

        if (data_size > 4) {
            uint8_t *entry_data = malloc(data_size);
            memcpy(entry_data, dec + pos, data_size);
            decode_buffer(entry_data, data_size);
            write_file(out_dir, name, entry_data, data_size - 4);
            free(entry_data);
        } else {
            write_file(out_dir, name, dec + pos, data_size);
        }
        pos += data_size;
        count++;
    }

    fclose(manifest);
    printf("\nExtracted %d files to %s/\n", count, out_dir);
    printf("Manifest written to %s\n", manifest_path);
    free(dec);
    return 0;
}

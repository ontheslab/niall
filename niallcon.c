/* niallcon.c - NIALLCON: Convert NIALL v3 save files to v4 format
   (Renamed from niallconv.c / NIALLCONV.COM for CP/M 8.3 filename compatibility)
   Compile: zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallcon.c -o NIALLCON
   Usage: NIALLCON infile outfile

   Reads a v3 binary NIALL save file (text link strings as produced by
   v1.13 / v1.14), converts to the v4 binary link record format used by
   v1.15+, and writes the result.

   END_TOKEN remapping:
     v3 END_TOKEN = MAX_WORDS at compile time (150 for v1.13, 250 for v1.14).
     Any id > disk_n in a link string is treated as END_TOKEN and remapped to
     500 (v4 END_TOKEN = MAX_WORDS in v1.15).

   v3 -> v4 conversion per record:
     Parse text "total|id(cnt)id(cnt)..." into binary:
     total(LE16) + n(byte) + [id(LE16) + cnt(LE16)] * n
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_WORD_LEN   31
#define MAX_LBUF      512    /* max v3 link string length */
#define MAX_PAIRS      50    /* max pairs per word (start token) */
#define BLNK_PAIR       4
#define BLNK_SZ(n)  (3 + (n)*4)
#define V4_END_TOKEN 1000    /* END_TOKEN value in v4 files (MAX_WORDS in v1.15) */

/* ---- binary helpers ---- */

static void wr_u16(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

/* ---- v3 link string parser ----
   Parses "total|id(cnt)id(cnt)..." into ids[]/cnts[].
   Any id > disk_n is remapped to V4_END_TOKEN (was old END_TOKEN).
   Returns number of valid pairs found (0..MAX_PAIRS).
*/

static int parse_links(const char *s, unsigned int disk_n,
                       unsigned int *ids, unsigned int *cnts)
{
    int n = 0;
    const char *p;

    if (!s || !*s) return 0;
    p = strchr(s, '|');
    if (!p) return 0;
    p++;   /* step past '|' */

    while (*p && n < MAX_PAIRS) {
        unsigned int id = 0, cnt = 0;
        const char *lp, *rp;

        /* skip whitespace */
        while (*p == ' ' || *p == '\r' || *p == '\n') p++;
        if (!*p) break;

        /* parse id digits */
        if (*p < '0' || *p > '9') break;
        while (*p >= '0' && *p <= '9') { id = id * 10 + (unsigned int)(*p - '0'); p++; }

        lp = p;
        if (*lp != '(') break;
        lp++;

        /* parse cnt digits */
        if (*lp < '0' || *lp > '9') break;
        while (*lp >= '0' && *lp <= '9') { cnt = cnt * 10 + (unsigned int)(*lp - '0'); lp++; }

        rp = lp;
        if (*rp != ')') break;

        if (id == 0 || cnt == 0) { p = rp + 1; continue; }  /* skip invalid */

        /* remap old END_TOKEN to v4 END_TOKEN */
        if (id > disk_n) id = V4_END_TOKEN;

        ids[n]  = id;
        cnts[n] = cnt;
        n++;
        p = rp + 1;
    }
    return n;
}

/* ---- static buffers to avoid CP/M stack overflow ---- */

static unsigned char wbuf[32];
static unsigned char lbuf[MAX_LBUF];
static unsigned char rec_out[3 + MAX_PAIRS * 4];  /* max output record */
static unsigned int  ids[MAX_PAIRS];
static unsigned int  cnts[MAX_PAIRS];

/* ---- write_all helper ---- */

static int write_all(FILE *fp, const unsigned char *buf, unsigned int n)
{
    return (unsigned int)fwrite(buf, 1, n, fp) == n;
}

/* ---- main ---- */

int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    unsigned char hdr[7];
    unsigned int disk_n, ver, i;
    unsigned short in_checksum = 0;    /* tracks v3 input payload checksum  */
    unsigned short out_checksum = 0;   /* tracks v4 output payload checksum */
    unsigned short checksum_stored;
    unsigned char wlen_byte;
    unsigned short llen_short;
    int j;

    if (argc != 3) {
        printf("Usage: NIALLCON infile outfile\n");
        printf("Converts NIALL v3 save files to v4 format.\n");
        return 1;
    }

    /* open input */
    fin = fopen(argv[1], "rb");
    if (!fin) { printf("Cannot open %s\n", argv[1]); return 1; }

    /* read and validate header */
    if (fread(hdr, 7, 1, fin) != 1) { printf("File too short\n"); fclose(fin); return 1; }
    if (hdr[0]!='N' || hdr[1]!='I' || hdr[2]!='A' || hdr[3]!='L') { printf("Bad magic\n"); fclose(fin); return 1; }

    ver    = (unsigned int)hdr[4];
    disk_n = (unsigned int)hdr[5] | ((unsigned int)hdr[6] << 8);

    printf("Input  : %s\n", argv[1]);
    printf("Version: %u\n", ver);
    printf("Words  : %u\n", disk_n);

    if (ver != 3) {
        printf("Error: expected v3 file (got v%u).\n", ver);
        printf("Use NIALLCHK to verify the file version.\n");
        fclose(fin);
        return 1;
    }

    /* open output */
    remove(argv[2]);
    fout = fopen(argv[2], "wb");
    if (!fout) { printf("Cannot create %s\n", argv[2]); fclose(fin); return 1; }

    /* write v4 header (same layout, version byte = 4) */
    hdr[4] = 4;
    if (!write_all(fout, hdr, 7)) { printf("Write failed\n"); fclose(fin); fclose(fout); return 1; }

    /* process each entry — checksum covers payload only (after header) */
    for (i = 0; i <= disk_n; i++) {
        unsigned int n, k;
        unsigned int total_out;

        /* read word length */
        if (fread(&wlen_byte, 1, 1, fin) != 1) { printf("Truncated at entry %u\n", i); fclose(fin); fclose(fout); return 1; }
        in_checksum += wlen_byte;

        /* read word bytes */
        memset(wbuf, 0, 32);
        if (wlen_byte > MAX_WORD_LEN) wlen_byte = MAX_WORD_LEN;
        if (wlen_byte) {
            if (fread(wbuf, wlen_byte, 1, fin) != 1) { printf("Truncated at entry %u (word)\n", i); fclose(fin); fclose(fout); return 1; }
            for (j = 0; j < (int)wlen_byte; j++) in_checksum += wbuf[j];
        }
        wbuf[wlen_byte] = 0;

        /* read llen (v3 link string length) */
        if (fread(&llen_short, sizeof(unsigned short), 1, fin) != 1) { printf("Truncated at entry %u (llen)\n", i); fclose(fin); fclose(fout); return 1; }
        in_checksum += llen_short & 0xFF;
        in_checksum += (llen_short >> 8) & 0xFF;

        /* read link string bytes */
        if (llen_short >= (unsigned short)MAX_LBUF) llen_short = (unsigned short)(MAX_LBUF - 1);
        memset(lbuf, 0, MAX_LBUF);
        if (llen_short) {
            if (fread(lbuf, llen_short, 1, fin) != 1) { printf("Truncated at entry %u (links)\n", i); fclose(fin); fclose(fout); return 1; }
            for (j = 0; j < (int)llen_short; j++) in_checksum += lbuf[j];
        }
        lbuf[llen_short] = 0;

        /* parse text link string; remap ids > disk_n to V4_END_TOKEN */
        n = (unsigned int)parse_links((char*)lbuf, disk_n, ids, cnts);

        /* recompute total as sum of counts (authoritative) */
        total_out = 0;
        for (k = 0; k < n; k++) total_out += cnts[k];

        /* build binary output record */
        wr_u16(rec_out, total_out);
        rec_out[2] = (unsigned char)n;
        for (k = 0; k < n; k++) {
            wr_u16(rec_out + 3 + k * BLNK_PAIR,     ids[k]);
            wr_u16(rec_out + 3 + k * BLNK_PAIR + 2, cnts[k]);
        }

        /* write v4 record: wlen + word bytes + binary record */
        fwrite(&wlen_byte, 1, 1, fout);
        out_checksum += wlen_byte;

        if (wlen_byte) {
            fwrite(wbuf, wlen_byte, 1, fout);
            for (j = 0; j < (int)wlen_byte; j++) out_checksum += wbuf[j];
        }

        {
            unsigned int rec_sz = (unsigned int)BLNK_SZ(n);
            unsigned int m;
            fwrite(rec_out, rec_sz, 1, fout);
            for (m = 0; m < rec_sz; m++) out_checksum += rec_out[m];
        }
    }

    /* read and verify v3 input checksum */
    if (fread(&checksum_stored, sizeof(unsigned short), 1, fin) == 1) {
        if (in_checksum != checksum_stored)
            printf("Warning: input checksum mismatch (file may be corrupt).\n");
    }
    fclose(fin);

    /* write v4 output checksum */
    fwrite(&out_checksum, sizeof(unsigned short), 1, fout);
    fclose(fout);

    printf("Output : %s\n", argv[2]);
    printf("Done. %u words converted.\n", disk_n);
    return 0;
}

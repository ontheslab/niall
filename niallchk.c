/* NIALLCHK.C - Binary NIALL save file checker (v3 and v4)
   Compile: zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallchk.c -o NIALLCHK
   Usage: NIALLCHK filename

   v3 file format (niall v1.13 / v1.14):
     Header (7 bytes): "NIAL" + version=3 + num_words LE
     Payload (num_words+1 records, entries 0..num_words):
       1 byte  : wlen (0..31)
       wlen    : word bytes (no NUL)
       2 bytes : llen (LE u16)
       llen    : link string bytes ("total|id(cnt)id(cnt)...")
     Footer: 2 bytes checksum LE (payload bytes only)

   v4 file format (niall v1.15+):
     Header (7 bytes): "NIAL" + version=4 + num_words LE
     Payload (num_words+1 records, entries 0..num_words):
       1 byte  : wlen (0..31)
       wlen    : word bytes (no NUL)
       3+n*4   : binary link record (n at byte 2, self-describing size)
                 bytes 0-1: unsigned short total (LE)
                 byte  2  : unsigned char  n
                 bytes 3+k*4: unsigned short id[k]  (LE)
                 bytes 5+k*4: unsigned short cnt[k] (LE)
     Footer: 2 bytes checksum LE (payload bytes only)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_WORD_LEN  31
#define MAX_LBUF     512    /* static read buffer for v3 link strings */
#define BLNK_PAIR      4    /* bytes per binary pair record (v4) */
#define BLNK_SZ(n)  (3 + (n)*4)

/* ---- v3 link string validation ---- */

/* Validate link string "total|id(cnt)id(cnt)..."
   Returns 1=OK, 0=BAD. Fills *total_val, *sum_cnt, *pair_count if non-NULL. */
static int validate_links(const char *s,
                          int *total_val, int *sum_cnt, int *pair_count)
{
    int total = 0, sum = 0, pairs = 0;
    const char *bar, *p, *t;

    if (total_val)  *total_val  = 0;
    if (sum_cnt)    *sum_cnt    = 0;
    if (pair_count) *pair_count = 0;

    if (!s || !*s) return 0;

    bar = strchr(s, '|');
    if (!bar) return 0;

    for (p = s; p < bar; p++)
        if (*p < '0' || *p > '9') return 0;

    total = atoi(s);
    if (total < 0) return 0;

    p = bar + 1;

    /* allow empty pairs area if total == 0 */
    t = p;
    while (*t == ' ' || *t == '\r' || *t == '\n') t++;
    if (*t == 0) {
        if (total != 0) return 0;
        if (total_val)  *total_val  = total;
        if (sum_cnt)    *sum_cnt    = sum;
        if (pair_count) *pair_count = pairs;
        return 1;
    }

    while (*p) {
        int id = 0, cnt = 0;

        if (*p == ' ' || *p == '\r' || *p == '\n') {
            while (*p == ' ' || *p == '\r' || *p == '\n') p++;
            if (*p != 0) return 0;
            break;
        }

        if (*p < '0' || *p > '9') return 0;
        while (*p >= '0' && *p <= '9') { id = id * 10 + (*p - '0'); p++; }

        if (*p != '(') return 0;
        p++;

        if (*p < '0' || *p > '9') return 0;
        while (*p >= '0' && *p <= '9') { cnt = cnt * 10 + (*p - '0'); p++; }

        if (*p != ')') return 0;
        p++;

        if (id < 1) return 0;
        if (cnt <= 0) return 0;

        sum += cnt;
        pairs++;
    }

    if (total != sum) return 0;

    if (total_val)  *total_val  = total;
    if (sum_cnt)    *sum_cnt    = sum;
    if (pair_count) *pair_count = pairs;
    return 1;
}

/* ---- shared helpers ---- */

static unsigned int rd_u16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

/* static to avoid CP/M stack overflow */
static unsigned char wbuf[32];
static unsigned char lbuf[MAX_LBUF];

/* ---- v3 check ---- */

static int check_v3(FILE *fp, unsigned int disk_n)
{
    unsigned int i;
    unsigned short checksum_calc = 0, checksum_saved;
    int errors = 0, warnings = 0;
    int total_pairs = 0;
    int min_pairs = 32767, max_pairs = 0;
    unsigned char wlen_byte;
    unsigned short llen_short;
    int j;

    printf("\nIdx  Word             Links                                    Status\n");
    printf("---- ---------------- ---------------------------------------- ------\n");

    for (i = 0; i <= disk_n; i++) {
        if (fread(&wlen_byte, 1, 1, fp) != 1) {
            printf("Truncated at entry %u (wlen)\n", i);
            return 1;
        }
        checksum_calc += wlen_byte;
        if (wlen_byte > MAX_WORD_LEN) {
            printf("Entry %u: wlen %u > MAX_WORD_LEN\n", i, (unsigned)wlen_byte);
            return 1;
        }
        if (i == 0 && wlen_byte != 0) { printf("Entry 0: wlen=%u (expected 0)\n", (unsigned)wlen_byte); warnings++; }

        memset(wbuf, 0, 32);
        if (wlen_byte) {
            if (fread(wbuf, wlen_byte, 1, fp) != 1) { printf("Truncated at entry %u (word)\n", i); return 1; }
            for (j = 0; j < (int)wlen_byte; j++) checksum_calc += wbuf[j];
        }
        wbuf[wlen_byte] = 0;

        if (i > 0 && wlen_byte > 0) {
            int bad_char = 0;
            for (j = 0; j < (int)wlen_byte; j++)
                if (!isalnum((unsigned char)wbuf[j])) { bad_char = 1; break; }
            if (bad_char) { printf("Entry %u: word '%s' invalid chars\n", i, (char*)wbuf); warnings++; }
        }

        if (fread(&llen_short, sizeof(unsigned short), 1, fp) != 1) { printf("Truncated at entry %u (llen)\n", i); return 1; }
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;
        if (llen_short >= (unsigned short)MAX_LBUF) { printf("Entry %u: llen %u >= buffer\n", i, (unsigned)llen_short); return 1; }

        memset(lbuf, 0, MAX_LBUF);
        if (llen_short) {
            if (fread(lbuf, llen_short, 1, fp) != 1) { printf("Truncated at entry %u (links)\n", i); return 1; }
            for (j = 0; j < (int)llen_short; j++) checksum_calc += lbuf[j];
        }
        lbuf[llen_short] = 0;

        {
            int total = 0, sum2 = 0, pairs = 0;
            int ok = validate_links((char*)lbuf, &total, &sum2, &pairs);
            char *label = (i == 0) ? (char*)"(start)" : (wlen_byte ? (char*)wbuf : (char*)"(empty)");
            if (!ok) errors++;
            printf("%4u %-16s %-40s %s\n", i, label, (char*)lbuf, ok ? "OK" : "BAD");
            if (ok) {
                total_pairs += pairs;
                if (pairs < min_pairs) min_pairs = pairs;
                if (pairs > max_pairs) max_pairs = pairs;
            }
        }
    }

    if (fread(&checksum_saved, sizeof(unsigned short), 1, fp) != 1) { printf("\nNo checksum at end of file\n"); return 1; }

    printf("\n");
    if (checksum_calc == checksum_saved)
        printf("Checksum : OK (0x%04X)\n", (unsigned)checksum_calc);
    else
        printf("Checksum : FAIL (calc=0x%04X saved=0x%04X)\n", (unsigned)checksum_calc, (unsigned)checksum_saved);

    if (errors) printf("Links    : %d error(s)\n", errors);
    else        printf("Links    : all valid\n");
    if (warnings) printf("Warnings : %d\n", warnings);
    if (!errors) {
        printf("Pairs    : %d total", total_pairs);
        if (disk_n > 0) printf(", %d..%d per word", min_pairs, max_pairs);
        printf("\n");
    }
    return (checksum_calc != checksum_saved || errors) ? 1 : 0;
}

/* ---- v4 check ---- */

static int check_v4(FILE *fp, unsigned int disk_n)
{
    unsigned int i;
    unsigned short checksum_calc = 0, checksum_saved;
    int errors = 0, warnings = 0;
    unsigned long total_pairs = 0;
    unsigned int min_pairs = 65535u, max_pairs = 0;
    unsigned char wlen_byte;
    unsigned char rec_hdr[3];
    unsigned int nrec, rec_sz;
    int j;

    /* temporary pairs buffer — largest record: 3 + 50*4 = 203 bytes */
    static unsigned char pbuf[403];

    printf("\nIdx  Word             Total Pairs  Status\n");
    printf("---- ---------------- ----- -----  ------\n");

    for (i = 0; i <= disk_n; i++) {
        if (fread(&wlen_byte, 1, 1, fp) != 1) { printf("Truncated at entry %u (wlen)\n", i); return 1; }
        checksum_calc += wlen_byte;
        if (wlen_byte > MAX_WORD_LEN) { printf("Entry %u: wlen %u > MAX_WORD_LEN\n", i, (unsigned)wlen_byte); return 1; }
        if (i == 0 && wlen_byte != 0) { printf("Entry 0: wlen=%u (expected 0)\n", (unsigned)wlen_byte); warnings++; }

        memset(wbuf, 0, 32);
        if (wlen_byte) {
            if (fread(wbuf, wlen_byte, 1, fp) != 1) { printf("Truncated at entry %u (word)\n", i); return 1; }
            for (j = 0; j < (int)wlen_byte; j++) checksum_calc += wbuf[j];
        }
        wbuf[wlen_byte] = 0;

        if (i > 0 && wlen_byte > 0) {
            int bad_char = 0;
            for (j = 0; j < (int)wlen_byte; j++)
                if (!isalnum((unsigned char)wbuf[j])) { bad_char = 1; break; }
            if (bad_char) { printf("Entry %u: word '%s' invalid chars\n", i, (char*)wbuf); warnings++; }
        }

        /* read 3-byte record header */
        if (fread(rec_hdr, 3, 1, fp) != 1) { printf("Truncated at entry %u (rec_hdr)\n", i); return 1; }
        checksum_calc += rec_hdr[0];
        checksum_calc += rec_hdr[1];
        checksum_calc += rec_hdr[2];

        nrec   = (unsigned int)rec_hdr[2];
        rec_sz = (unsigned int)BLNK_SZ(nrec);

        /* read pair bytes */
        if (nrec > 0) {
            unsigned int pairs_sz = nrec * (unsigned int)BLNK_PAIR;
            unsigned int k;
            if (pairs_sz > sizeof(pbuf)) { printf("Entry %u: n=%u exceeds buffer\n", i, nrec); return 1; }
            if (fread(pbuf, pairs_sz, 1, fp) != 1) { printf("Truncated at entry %u (pairs)\n", i); return 1; }
            for (k = 0; k < pairs_sz; k++) checksum_calc += pbuf[k];
        }

        {
            unsigned int total_field = rd_u16(rec_hdr);
            unsigned int sum = 0, k;
            int ok = 1;
            char *label = (i == 0) ? (char*)"(start)" : (wlen_byte ? (char*)wbuf : (char*)"(empty)");

            /* verify total == sum of counts */
            for (k = 0; k < nrec; k++) {
                sum += rd_u16(&pbuf[k * BLNK_PAIR + 2]);
                /* validate id != 0 (start token is never a valid link target) */
                if (rd_u16(&pbuf[k * BLNK_PAIR]) == 0) { ok = 0; }
            }
            if (total_field != sum) ok = 0;

            if (!ok) errors++;
            printf("%4u %-16s %5u %5u  %s\n", i, label, total_field, nrec, ok ? "OK" : "BAD");

            if (ok) {
                total_pairs += nrec;
                if (nrec < min_pairs) min_pairs = nrec;
                if (nrec > max_pairs) max_pairs = nrec;
            }
        }
    }

    if (fread(&checksum_saved, sizeof(unsigned short), 1, fp) != 1) { printf("\nNo checksum at end of file\n"); return 1; }

    printf("\n");
    if (checksum_calc == checksum_saved)
        printf("Checksum : OK (0x%04X)\n", (unsigned)checksum_calc);
    else
        printf("Checksum : FAIL (calc=0x%04X saved=0x%04X)\n", (unsigned)checksum_calc, (unsigned)checksum_saved);

    if (errors) printf("Links    : %d error(s)\n", errors);
    else        printf("Links    : all valid\n");
    if (warnings) printf("Warnings : %d\n", warnings);
    if (!errors) {
        printf("Pairs    : %lu total", total_pairs);
        if (disk_n > 0) printf(", %u..%u per word", min_pairs, max_pairs);
        printf("\n");
    }
    return (checksum_calc != checksum_saved || errors) ? 1 : 0;
}

/* ---- main ---- */

int main(int argc, char *argv[])
{
    FILE *fp;
    unsigned char hdr[7];
    unsigned int ver, disk_n;

    if (argc != 2) {
        printf("Usage: NIALLCHK filename\n");
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("Cannot open %s\n", argv[1]);
        return 1;
    }

    if (fread(hdr, 7, 1, fp) != 1) {
        printf("File too short (no header)\n");
        fclose(fp);
        return 1;
    }

    if (hdr[0]!='N' || hdr[1]!='I' || hdr[2]!='A' || hdr[3]!='L') {
        printf("Bad magic (not a NIALL file)\n");
        fclose(fp);
        return 1;
    }

    ver    = (unsigned int)hdr[4];
    disk_n = (unsigned int)hdr[5] | ((unsigned int)hdr[6] << 8);

    printf("NIALL Binary File Checker\n");
    printf("File   : %s\n", argv[1]);
    printf("Version: %u\n", ver);
    printf("Words  : %u\n", disk_n);

    if (ver == 3) {
        return check_v3(fp, disk_n);
    } else if (ver == 4) {
        return check_v4(fp, disk_n);
    } else {
        printf("Unsupported version %u (expected 3 or 4)\n", ver);
        fclose(fp);
        return 1;
    }
}

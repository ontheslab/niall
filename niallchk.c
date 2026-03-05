/* NIALLCHK.C - Binary NIALL v3 save file checker
   Compile: zcc +cpm -vn -create-app -compiler=sdcc niallchk.c -o NIALLCHK
   Usage: NIALLCHK filename

   v3 file format (matches niall.c):
     Header (7 bytes, no checksum in header):
       4 bytes : magic "NIAL"
       1 byte  : version (3)
       2 bytes : num_words (LE u16)
     Payload (num_words+1 records: word 0 then words 1..num_words):
       1 byte  : wlen (0..31)
       wlen    : word bytes (no NUL)
       2 bytes : llen (LE u16, 0..limit-1)
       llen    : links bytes (no NUL), format "total|id(cnt)id(cnt)..."
     Footer:
       2 bytes : unsigned short checksum (LE)
                 checksum covers payload bytes only, NOT the 7-byte header

   Validation performed:
     - Magic and version
     - Each record: size limits, no truncation
     - Entry 0 (start token): wlen must be 0
     - Word bytes (entries 1..n): must be a-z or 0-9 only
     - Link strings: format, total==sum(counts)
     - Link IDs: must be 1..disk_n or END_TOKEN; id=0 is never a valid target
     - Link IDs: must not reference words beyond disk_n (dangling refs)
     - Checksum match
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_WORD_LEN    31
#define MAX_LBUF       512    /* static read buffer — large enough for any link string */

/* Validate link string "total|id(cnt)id(cnt)..."
   max_valid_id: highest valid word index (disk_n); END_TOKEN (MAX_WORDS) also allowed.
   id=0 (start token) is never a valid link target.
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

        /* id=0 is start token - never a valid link target */
        if (id < 1) return 0;
        /* id must be an existing word (1..disk_n) or END_TOKEN (any id > disk_n) */
        /* END_TOKEN value varies by compile-time MAX_WORDS so accept any id > max_valid_id */
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

/* static to avoid CP/M stack overflow */
static unsigned char wbuf[32];
static unsigned char lbuf[MAX_LBUF];

int main(int argc, char *argv[])
{
    FILE *fp;
    unsigned char hdr[7];
    unsigned int ver, disk_n;
    unsigned short checksum_calc, checksum_saved;
    int i, j, errors, warnings;
    unsigned char wlen_byte;
    unsigned short llen_short;
    int total_pairs = 0;
    int min_pairs = 32767, max_pairs = 0;

    if (argc != 2) {
        printf("Usage: NIALLCHK filename\n");
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("Cannot open %s\n", argv[1]);
        return 1;
    }

    /* 7-byte header */
    if (fread(hdr, 7, 1, fp) != 1) {
        printf("File too short (no header)\n");
        fclose(fp);
        return 1;
    }

    if (hdr[0]!='N' || hdr[1]!='I' || hdr[2]!='A' || hdr[3]!='L') {
        printf("Bad magic (not a NIALL v3 file)\n");
        fclose(fp);
        return 1;
    }

    ver    = (unsigned int)hdr[4];
    disk_n = (unsigned int)hdr[5] | ((unsigned int)hdr[6] << 8);

    printf("NIALL Binary File Checker\n");
    printf("File   : %s\n", argv[1]);
    printf("Version: %u\n", ver);
    printf("Words  : %u\n", disk_n);

    if (ver != 3) {
        printf("Unsupported version %u (expected 3)\n", ver);
        fclose(fp);
        return 1;
    }

    printf("\nIdx  Word             Links                                    Status\n");
    printf("---- ---------------- ---------------------------------------- ------\n");

    checksum_calc = 0;
    errors = 0;
    warnings = 0;

    /* word 0 (start token) then words 1..disk_n = disk_n+1 records total */
    for (i = 0; i <= (int)disk_n; i++) {

        /* wlen (1 byte) */
        if (fread(&wlen_byte, 1, 1, fp) != 1) {
            printf("Truncated at entry %d (wlen)\n", i);
            fclose(fp);
            return 1;
        }
        checksum_calc += wlen_byte;

        if (wlen_byte > MAX_WORD_LEN) {
            printf("Entry %d: wlen %u > MAX_WORD_LEN\n", i, (unsigned)wlen_byte);
            fclose(fp);
            return 1;
        }

        /* entry 0 must have empty word text */
        if (i == 0 && wlen_byte != 0) {
            printf("Entry 0: wlen=%u (start token must have empty word)\n",
                   (unsigned)wlen_byte);
            warnings++;
        }

        memset(wbuf, 0, 32);
        if (wlen_byte) {
            if (fread(wbuf, wlen_byte, 1, fp) != 1) {
                printf("Truncated at entry %d (word)\n", i);
                fclose(fp);
                return 1;
            }
            for (j = 0; j < (int)wlen_byte; j++) checksum_calc += wbuf[j];
        }
        wbuf[wlen_byte] = 0;

        /* word characters must be a-z or 0-9 */
        if (i > 0 && wlen_byte > 0) {
            int bad_char = 0;
            for (j = 0; j < (int)wlen_byte; j++) {
                if (!isalnum((unsigned char)wbuf[j])) { bad_char = 1; break; }
            }
            if (bad_char) {
                printf("Entry %d: word '%s' contains invalid characters\n",
                       i, (char*)wbuf);
                warnings++;
            }
        }

        /* llen (2 bytes LE) */
        if (fread(&llen_short, sizeof(unsigned short), 1, fp) != 1) {
            printf("Truncated at entry %d (llen)\n", i);
            fclose(fp);
            return 1;
        }
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;

        if (llen_short >= (unsigned short)MAX_LBUF) {
            printf("Entry %d: llen %u >= buffer %u\n", i, (unsigned)llen_short, MAX_LBUF);
            fclose(fp);
            return 1;
        }

        memset(lbuf, 0, MAX_LBUF);
        if (llen_short) {
            if (fread(lbuf, llen_short, 1, fp) != 1) {
                printf("Truncated at entry %d (links)\n", i);
                fclose(fp);
                return 1;
            }
            for (j = 0; j < (int)llen_short; j++) checksum_calc += lbuf[j];
        }
        lbuf[llen_short] = 0;

        {
            int total = 0, sum2 = 0, pairs = 0;
            int ok = validate_links((char*)lbuf, &total, &sum2, &pairs);
            char *label = (i == 0) ? (char*)"(start)" :
                          (wlen_byte ? (char*)wbuf : (char*)"(empty)");

            if (!ok) errors++;

            printf("%4d %-16s %-40s %s\n",
                   i, label, (char*)lbuf,
                   ok ? "OK" : "BAD");

            if (ok) {
                total_pairs += pairs;
                if (pairs < min_pairs) min_pairs = pairs;
                if (pairs > max_pairs) max_pairs = pairs;
            }
        }
    }

    /* 2-byte checksum at end of file */
    if (fread(&checksum_saved, sizeof(unsigned short), 1, fp) != 1) {
        printf("\nNo checksum at end of file\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);

    printf("\n");
    if (checksum_calc == checksum_saved)
        printf("Checksum : OK (0x%04X)\n", (unsigned)checksum_calc);
    else
        printf("Checksum : FAIL (calc=0x%04X saved=0x%04X)\n",
               (unsigned)checksum_calc, (unsigned)checksum_saved);

    if (errors)
        printf("Links    : %d error(s)\n", errors);
    else
        printf("Links    : all valid\n");

    if (warnings)
        printf("Warnings : %d\n", warnings);

    if (!errors) {
        printf("Pairs    : %d total", total_pairs);
        if (disk_n > 0)
            printf(", %d..%d per word", min_pairs, max_pairs);
        printf("\n");
    }

    return (checksum_calc != checksum_saved || errors) ? 1 : 0;
}

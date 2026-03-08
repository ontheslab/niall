/* niallasc.c - NIALLASC: Convert NIALL ASCII text save files to v4 binary format
   Compile: zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallasc.c -o NIALLASC
   Usage: NIALLASC infile outfile

   Reads NIALL ASCII save files as used by the BBS version (two formats):

   Format A — LF/CRLF, word text + link on separate lines (NIALL.DAT, GTAMP.DAT):
     num_words[CRLF/LF]
     word0_text[CRLF/LF]     (start token, usually a single space)
     link_string0[CRLF/LF]   (total|id(cnt)id(cnt)... — no spaces between pairs)
     word1_text[CRLF/LF]
     link_string1[CRLF/LF]
     ...
     [0x1A CP/M EOF padding]

   Format B — old CRLF, no text line for word 0 (NIALLBBS.DAT main section):
     ' ' num_words ' '[CRLF]
     [blank CRLF line]
     link_string0_with_spaces[CRLF]   (start token links, spaces between pairs)
     word1_text[CRLF]
     link_string1[CRLF]
     ...

   Detection: after reading num_words, if the next line contains '|'
   it is the link string for word 0 (Format B). Otherwise it is the
   word 0 text line (Format A).

   Integrity handling:
     - Stored totals are ignored; total is recomputed as sum of counts.
     - Pairs with id==0 (start token), id>MAX_WORDS, or cnt<=0 are skipped.
     - Words that fail valid_word() are stored as empty (blank slot).
     - All issues are reported with entry number and details.

   AMOS END_TOKEN=3000 is remapped to END_TOKEN=1000 in v4 output.
   Any id > disk_n (other than 3000) is treated as a dangling ref and skipped.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_WORDS    1000
#define MAX_WORD_LEN   31
#define MAX_LBUF     8192    /* max link string length (start token can be 5000+ chars) */
#define MAX_PAIRS      50    /* max pairs per regular word in output */
#define START_MAX_PAIRS 100  /* max pairs for start token (word 0)   */
#define PARSE_MAX    1001    /* parse buffer size (filter before truncating) */
#define BLNK_PAIR       4
#define BLNK_SZ(n)  (3 + (n)*4)
#define END_TOKEN       1000    /* end-of-sentence sentinel (v4) */
#define AMOS_END_TOKEN  3000    /* end-of-sentence sentinel in AMOS BBS files */

/* ---- helpers ---- */

/* Read one full line into buf (up to size-1 chars), drain any excess so the
   next read starts at the correct line, then strip trailing CR/LF. */
static void read_line(FILE *fp, char *buf, int size)
{
    if (!fgets(buf, size, fp)) { buf[0] = 0; return; }
    if (!strchr(buf, '\n')) { int c; while ((c = fgetc(fp)) != '\n' && c != EOF) {} }
    buf[strcspn(buf, "\r\n")] = 0;
}


static void wr_u16(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

/* Strip non-alphanumeric chars from word text in-place.
   e.g. "don't" -> "dont", "f$@!" -> "f", "a1200" -> "a1200" (kept).
   Returns 0 if the result is empty. */
static int clean_word(char *w)
{
    char *r = w, *wr = w;
    if (!w) return 0;
    while (*r) {
        if (isalnum((unsigned char)*r)) *wr++ = *r;
        r++;
    }
    *wr = '\0';
    return (wr != w);
}

/* ---- link string parser ----
   Parses "total|id(cnt)id(cnt)..." into ids[]/cnts[].
   Handles both spaced ("5(3) 7(1)") and non-spaced ("5(3)7(1)") pair formats.
   Skips id==0 (start token — never a valid target), id>MAX_WORDS, cnt<=0.
   Returns number of valid pairs stored in ids[]/cnts[].
   Sets *bad_pairs to count of skipped/invalid pairs. */

static int parse_ascii_links(const char *s, int disk_n,
                              unsigned int *ids, unsigned int *cnts,
                              int *bad_pairs, int *remapped)
{
    int n = 0;
    const char *p;
    *bad_pairs = 0;
    *remapped  = 0;

    if (!s || !*s) return 0;
    p = strchr(s, '|');
    if (!p) return 0;
    p++;

    while (*p && n < PARSE_MAX) {
        unsigned int id = 0, cnt = 0;
        const char *lp, *rp;

        /* skip whitespace */
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
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
        p = rp + 1;

        /* validate pair */
        if (id == 0) {
            (*bad_pairs)++;
            continue;
        }
        if (id == (unsigned int)AMOS_END_TOKEN) {
            id = (unsigned int)END_TOKEN;
            (*remapped)++;
        } else if (id > (unsigned int)disk_n) {
            /* dangling reference beyond declared word count — skip */
            (*bad_pairs)++;
            continue;
        }
        if (cnt == 0) {
            (*bad_pairs)++;
            continue;
        }

        ids[n]  = id;
        cnts[n] = cnt;
        n++;
    }
    return n;
}

/* ---- static buffers to avoid CP/M stack overflow ---- */

static char          wbuf[128];   /* must be > MAX_WORD_LEN to avoid line desync */
static char          lbuf[MAX_LBUF];
static unsigned char rec_out[3 + START_MAX_PAIRS * 4];
static unsigned int  ids[PARSE_MAX];
static unsigned int  cnts[PARSE_MAX];

/* ---- filter_pairs ----
   1. Deduplicates: merges counts for same id.
   2. Removes singletons: drops pairs with cnt==1.
   3. Selects top max_out by count (highest-count pairs kept).
   Sets *dups = number of duplicate pairs merged,
        *singles = number of singleton pairs removed.
   Returns new pair count (<= max_out). */

static int filter_pairs(unsigned int *fids, unsigned int *fcnts, int n,
                        int max_out, int rm_singles, int *dups, int *singles)
{
    int i, j, out, kept, best;
    unsigned int best_cnt;
    *dups    = 0;
    *singles = 0;

    /* dedup: merge counts into first occurrence, mark dupes id=0 */
    for (i = 0; i < n; i++) {
        if (fids[i] == 0) continue;
        for (j = i + 1; j < n; j++) {
            if (fids[j] != 0 && fids[j] == fids[i]) {
                fcnts[i] += fcnts[j];
                fids[j] = 0;
                (*dups)++;
            }
        }
    }
    /* compact */
    out = 0;
    for (i = 0; i < n; i++)
        if (fids[i] != 0) { fids[out] = fids[i]; fcnts[out] = fcnts[i]; out++; }
    n = out;

    /* remove singletons — start token only (word 0 accumulates hundreds of
       weak sentence-starters; regular words keep all transitions) */
    if (rm_singles) {
        out = 0;
        for (i = 0; i < n; i++) {
            if (fcnts[i] > 1) { fids[out] = fids[i]; fcnts[out] = fcnts[i]; out++; }
            else (*singles)++;
        }
        if (out > 0) n = out;
        else *singles = 0;  /* all singletons — keep them anyway */
    }

    /* select top max_out by count */
    if (n > max_out) {
        kept = 0;
        while (kept < max_out) {
            best = -1; best_cnt = 0;
            for (i = kept; i < n; i++)
                if (fcnts[i] > best_cnt) { best_cnt = fcnts[i]; best = i; }
            if (best < 0) break;
            { unsigned int t; t = fids[kept];  fids[kept]  = fids[best];  fids[best]  = t; }
            { unsigned int t; t = fcnts[kept]; fcnts[kept] = fcnts[best]; fcnts[best] = t; }
            kept++;
        }
        n = max_out;
    }

    return n;
}

/* ---- write_all helper ---- */

static int write_all(FILE *fp, const unsigned char *buf, unsigned int n)
{
    return (unsigned int)fwrite(buf, 1, n, fp) == n;
}

/* ---- main ---- */

int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    int num_words, i, format_b;
    unsigned char hdr[7];
    unsigned short checksum = 0;
    int total_bad = 0, total_ok = 0, total_dups = 0, total_singles = 0, total_trunc = 0, total_remap = 0;
    unsigned char wlen_byte;

    if (argc != 3) {
        printf("Usage: NIALLASC infile outfile\n");
        printf("Converts NIALL ASCII save files to v4 binary format.\n");
        return 1;
    }

    /* open input in text mode — handles CRLF/LF and 0x1A as EOF */
    fin = fopen(argv[1], "r");
    if (!fin) { printf("Cannot open %s\n", argv[1]); return 1; }

    /* read word count */
    if (fscanf(fin, " %d", &num_words) != 1) {
        printf("Cannot read word count.\n");
        fclose(fin);
        return 1;
    }
    /* consume rest of first line */
    {
        int c;
        while ((c = fgetc(fin)) != '\n' && c != EOF) {}
    }

    if (num_words < 0 || num_words > MAX_WORDS) {
        printf("Word count %d out of range (0..%d).\n", num_words, MAX_WORDS);
        fclose(fin);
        return 1;
    }

    /* detect format: read the next line; if it contains '|' it is the
       link string for word 0 (Format B), otherwise it is the word text */
    /* skip blank lines, then check if it's a link string (Format B) or word text (Format A) */
    do {
        if (!fgets(lbuf, MAX_LBUF, fin)) {
            printf("Unexpected end of file.\n");
            fclose(fin);
            return 1;
        }
        if (!strchr(lbuf, '\n')) { int c; while ((c = fgetc(fin)) != '\n' && c != EOF) {} }
        lbuf[strcspn(lbuf, "\r\n")] = 0;
    } while (lbuf[0] == '\0');

    format_b = (strchr(lbuf, '|') != 0);

    printf("Input  : %s\n", argv[1]);
    printf("Words  : %d\n", num_words);
    printf("Format : %s\n", format_b ? "B (old CRLF, no word-0 text line)"
                                      : "A (LF, word text + link lines)");

    /* open output */
    remove(argv[2]);
    fout = fopen(argv[2], "wb");
    if (!fout) { printf("Cannot create %s\n", argv[2]); fclose(fin); return 1; }

    /* write v4 header */
    hdr[0]='N'; hdr[1]='I'; hdr[2]='A'; hdr[3]='L'; hdr[4]=4;
    hdr[5] = (unsigned char)(num_words & 0xFF);
    hdr[6] = (unsigned char)(num_words >> 8);
    if (!write_all(fout, hdr, 7)) {
        printf("Write failed.\n");
        fclose(fin); fclose(fout); return 1;
    }

    /* process each entry (0 = start token, 1..num_words = real words) */
    for (i = 0; i <= num_words; i++) {
        unsigned int n, k, total_out, rec_sz;
        int bad = 0, dups = 0, singles = 0, remap = 0;

        if (i == 0) {
            /* word 0 (start token): text is always " " */
            strcpy(wbuf, " ");
            if (format_b) {
                /* Format B: we already read the link string into lbuf above */
                /* (lbuf still holds it from the format detection read) */
            } else {
                /* Format A: lbuf holds the text we just read (should be " " or similar)
                   now read the link string */
                read_line(fin, lbuf, MAX_LBUF);
            }
        } else {
            /* For Format B: after word 0, the pattern is: word_text then link_string
               For Format A: same pattern from here on since lbuf was already consumed */
            if (format_b || i > 0) {
                /* read word text — drain any excess if line was longer than wbuf */
                if (!fgets(wbuf, (int)sizeof(wbuf), fin)) { wbuf[0] = 0; }
                if (!strchr(wbuf, '\n')) { int c; while ((c=fgetc(fin))!='\n' && c!=EOF){} }
                wbuf[strcspn(wbuf, "\r\n")] = 0;
                if (strlen(wbuf) > MAX_WORD_LEN) wbuf[MAX_WORD_LEN] = 0;

                /* read link string */
                read_line(fin, lbuf, MAX_LBUF);
            }

            /* clean word text: strip punctuation in-place */
            if (wbuf[0]) {
                char orig[32];
                strncpy(orig, wbuf, 31); orig[31] = 0;
                if (!clean_word(wbuf)) {
                    printf("  Entry %d: '%s' all-punct, using 'x'\n", i, orig);
                    strcpy(wbuf, "x");
                } else if (strcmp(orig, wbuf) != 0) {
                    printf("  Entry %d: '%s' -> '%s'\n", i, orig, wbuf);
                }
            } else {
                printf("  Entry %d: empty word text, using 'x'\n", i);
                strcpy(wbuf, "x");
            }
        }

        /* parse link string */
        n = (unsigned int)parse_ascii_links(lbuf, num_words, ids, cnts, &bad, &remap);
        total_bad  += bad;
        total_remap += remap;

        /* filter: dedup, remove singletons, keep top MAX_PAIRS by count */
        {
            int n_before = (int)n;
            n = (unsigned int)filter_pairs(ids, cnts, (int)n, (i == 0) ? START_MAX_PAIRS : MAX_PAIRS, (i == 0), &dups, &singles);
            if (dups)
                printf("  Entry %d: %d duplicate id(s) merged\n", i, dups);
            int max_p = (i == 0) ? START_MAX_PAIRS : MAX_PAIRS;
            if (n_before - dups - singles > max_p)
                total_trunc++;
            total_dups    += dups;
            total_singles += singles;
        }

        /* if all pairs were filtered/skipped, add a single END_TOKEN transition
           so the word is still a valid (sentence-ending) Markov chain node */
        if (n == 0) {
            ids[0]  = (unsigned int)END_TOKEN;
            cnts[0] = 1;
            n = 1;
        }

        /* recompute total (don't trust stored value) */
        total_out = 0;
        for (k = 0; k < n; k++) total_out += cnts[k];

        /* build binary record */
        wr_u16(rec_out, total_out);
        rec_out[2] = (unsigned char)n;
        for (k = 0; k < n; k++) {
            wr_u16(rec_out + 3 + k * BLNK_PAIR,     ids[k]);
            wr_u16(rec_out + 3 + k * BLNK_PAIR + 2, cnts[k]);
        }
        rec_sz = (unsigned int)BLNK_SZ(n);

        /* write: wlen + word bytes + binary record */
        wlen_byte = (i == 0) ? 0 : (unsigned char)strlen(wbuf);
        if (wlen_byte > MAX_WORD_LEN) wlen_byte = MAX_WORD_LEN;

        fwrite(&wlen_byte, 1, 1, fout);
        checksum += wlen_byte;

        if (wlen_byte) {
            unsigned int j;
            fwrite(wbuf, wlen_byte, 1, fout);
            for (j = 0; j < (unsigned int)wlen_byte; j++)
                checksum += (unsigned char)wbuf[j];
        }

        fwrite(rec_out, rec_sz, 1, fout);
        {
            unsigned int j;
            for (j = 0; j < rec_sz; j++)
                checksum += rec_out[j];
        }

        total_ok++;
    }

    fclose(fin);

    /* write v4 checksum */
    fwrite(&checksum, sizeof(unsigned short), 1, fout);
    fclose(fout);

    printf("Output : %s\n", argv[2]);
    printf("Entries: %d written\n", total_ok);
    if (total_remap)  printf("Remap  : %d end-token pair(s) (%d->%d)\n", total_remap, AMOS_END_TOKEN, END_TOKEN);
    if (total_bad)    printf("Bad    : %d invalid pair(s) skipped\n", total_bad);
    if (total_dups)   printf("Dups   : %d duplicate pair(s) merged\n", total_dups);
    if (total_singles)printf("Singles: %d singleton pair(s) removed\n", total_singles);
    if (total_trunc)  printf("Trunc  : %d entr%s capped at %d pairs\n",
                             total_trunc, total_trunc==1?"y":"ies", MAX_PAIRS);
    printf("Done.\n");
    return 0;
}

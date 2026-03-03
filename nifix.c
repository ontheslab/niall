/* nifix.c - NIALL dictionary repair tool for CP/M

   Repairs corrupted dictionary files by:
   - Validating transition IDs
   - Rebuilding totals from counts
   - Removing broken transitions
   - Fixing missing or invalid totals

   Input format:
     num_words
     word
     links
     word
     links
     ...

   Output is written to repaired.dat

   Compile:
     zcc +cpm -O3 nifix.c -o NIFIX
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_WORDS 1000
#define MAX_LINKS 512
#define MAX_WORD  32

typedef struct {
    char word[MAX_WORD];
    char links[MAX_LINKS];
} Entry;

Entry dict[MAX_WORDS + 1];
int num_words = 0;

/* ---------------- Utility ---------------- */

void strip_newline(char *s)
{
    s[strcspn(s, "\r\n")] = 0;
}

/* ---------------- Load Dictionary ---------------- */

int load_dict(const char *fname)
{
    FILE *fp = fopen(fname, "r");
    if (!fp) return 0;

    char line[512];

    if (!fgets(line, sizeof(line), fp)) return 0;
    num_words = atoi(line);
    if (num_words < 0 || num_words > MAX_WORDS) return 0;

    for (int i = 0; i <= num_words; i++) {
        if (!fgets(dict[i].word, MAX_WORD, fp)) return 0;
        strip_newline(dict[i].word);

        if (!fgets(dict[i].links, MAX_LINKS, fp)) return 0;
        strip_newline(dict[i].links);
    }

    fclose(fp);
    return 1;
}

/* ---------------- Repair One Link String ---------------- */

void repair_links(char *out, const char *in)
{
    int total = 0;
    char *p = (char *)in;
    char rebuilt[MAX_LINKS] = "";
    char temp[32];

    /* skip total */
    p = strchr(p, '|');
    if (!p) {
        strcpy(out, "0|");
        return;
    }
    p++;

    while (*p) {
        int id = atoi(p);
        char *o = strchr(p, '(');
        if (!o) break;
        int cnt = atoi(o + 1);

        if (id >= 0 && id <= MAX_WORDS && cnt > 0) {
            total += cnt;
            sprintf(temp, "%d(%d)", id, cnt);
            strcat(rebuilt, temp);
        }

        p = strchr(o, ')');
        if (!p) break;
        p++;
    }

    sprintf(out, "%d|%s", total, rebuilt);
}

/* ---------------- Save Dictionary ---------------- */

void save_dict(const char *fname)
{
    FILE *fp = fopen(fname, "w");
    if (!fp) return;

    fprintf(fp, "%d\n", num_words);

    for (int i = 0; i <= num_words; i++) {
        char fixed[MAX_LINKS];
        repair_links(fixed, dict[i].links);

        fprintf(fp, "%s\n", dict[i].word);
        fprintf(fp, "%s\n", fixed);
    }

    fclose(fp);
}

/* ---------------- Main ---------------- */

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: nifix filename\n");
        return 1;
    }

    if (!load_dict(argv[1])) {
        printf("Load failed.\n");
        return 1;
    }

    save_dict("REPAIRED.DAT");

    printf("Repair complete -> REPAIRED.DAT\n");
    return 0;
}

/* ---------------- niall_validate ---------------- */
/*
   Compile with:
   
   zcc +cpm -create-app nival.c -o NIVAL

   Returns:
     - The index of the word if found
     - 0 if the word does not exist

   Index 0 is marks the start-of-sentence token.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_WORDS 1000
#define MAX_LINKS_STR 512

int main(int argc, char *argv[])
{
    FILE *fp;
    int num_words;
    char word[32];
    char links[MAX_LINKS_STR];

    if (argc < 2) {
        printf("Validate NIALL DAT Files\n");
        printf("Usage: NIVAL filename\n");
        return 1;
    }

    fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Cannot open file.\n");
        return 1;
    }

    if (fscanf(fp, "%d\n", &num_words) != 1) {
        printf("Invalid header.\n");
        fclose(fp);
        return 1;
    }

    if (num_words < 0 || num_words > MAX_WORDS) {
        printf("Word count out of range: %d\n", num_words);
        fclose(fp);
        return 1;
    }

    for (int i = 0; i <= num_words; i++) {
        if (!fgets(word, 32, fp)) break;
        word[strcspn(word, "\r\n")] = 0;

        if (!fgets(links, MAX_LINKS_STR, fp)) break;
        links[strcspn(links, "\r\n")] = 0;

        char *p = strchr(links, '|');
        if (!p) {
            printf("Bad link format at word %d\n", i);
            continue;
        }

        p++;
        while (*p) {
            int id = atoi(p);
            p = strchr(p, '(');
            if (!p) break;
            int cnt = atoi(p + 1);

            if (id < 0 || id > MAX_WORDS)
                printf("Bad ID %d at word %d\n", id, i);
            if (cnt <= 0)
                printf("Bad count at word %d\n", i);

            p = strchr(p, ')');
            if (!p) break;
            p++;
        }
    }

    fclose(fp);
    printf("Validation complete.\n");
    return 0;
}
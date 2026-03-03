/* COMMANDS.C - Command handling for NIALL (HI-TECH C V3.09)
   Compile & Link, all modules:

     C -V HTNBASE.C REPLY.C COMMANDS.C

   Link:
     LINK -z -p text=0,data,bss -c 100h -o HTNIALL.COM CRTCPM.OBJ HTNBASE.OBJ REPLY.OBJ COMMANDS.OBJ LIBC.LIB
*/

#include "NIALL.H"

/* Magic number (defined in NIALL.H) */
char NIALL_MAGIC[6] = {'N','I','A','L','L','1'};

/* ---------------- Commands ---------------- */
void handle_command(cmd)
    char *cmd;
{
    int i;
    char *arg;
    FILE *fp;
    unsigned char wlen_byte;
    unsigned short llen_short;
    unsigned short checksum_calc;
    unsigned short checksum_saved;
    int ver;
    int loaded_words;
    char magic[6];
    int j;

    /* Initialise to avoid uninitialised use */
    fp = 0;
    ver = 0;
    loaded_words = 0;
    checksum_calc = 0;
    checksum_saved = 0;
    wlen_byte = 0;
    llen_short = 0;
    magic[0] = 0;
    j = 0;

    if (!strcmp(cmd, "#FRESH")) {

        free_dictionary_links();
        num_words = 0;

        strcpy(words[0].word, " ");
        if (!set_links_str(0, "")) {
            printf("Out of memory.\n");
            return;
        }

        words[END_TOKEN].word[0] = '.';
        words[END_TOKEN].word[1] = 0;
        if (!set_links_str(END_TOKEN, "")) {
            printf("Out of memory.\n");
            return;
        }

        printf("Dictionary cleared.\n");
    }
    else if (!strcmp(cmd, "#LIST")) {
        unsigned long lbytes;
        unsigned int maxblk;
        int nlinks;

        nlinks = 0;
        for (i = 0; i <= num_words; i++) {
            if (words[i].links == (char *)0) {
                printf("%d %s <NO LINKS>\n", i, words[i].word);
            } else {
                printf("%d %s %s\n", i, words[i].word, words[i].links);
                nlinks++;
            }
        }

        /* Added: memory footer (safe + useful on CP/M)
           - links_bytes_used(): exact heap bytes we allocated for links strings
           - heap_probe_maxblock(): estimate of largest contiguous free heap block
        */
        lbytes = links_bytes_used();
        maxblk = heap_probe_maxblock();

        printf("\n--- MEMORY ---\n");
        printf("Words: %d (max real %d, END_TOKEN %d)\n", num_words, END_TOKEN - 1, END_TOKEN);
        printf("Link entries allocated: %d\n", nlinks);
        printf("Link heap bytes used: %lu\n", lbytes);
        printf("Heap probe max block: %u\n\n", maxblk);
    }
    else if (!strcmp(cmd, "#MEM")) {
        unsigned long lbytes;
        unsigned int maxblk;
        int nlinks;

        /* Count allocated links entries */
        nlinks = 0;
        for (i = 0; i <= num_words; i++) {
            if (words[i].links != (char *)0)
                nlinks++;
        }

        lbytes = links_bytes_used();
        maxblk = heap_probe_maxblock();

        printf("\n--- MEMORY ---\n");
        printf("Words: %d (max real %d, END_TOKEN %d)\n", num_words, END_TOKEN - 1, END_TOKEN);
        printf("Link entries allocated: %d\n", nlinks);
        printf("Link heap bytes used: %lu\n", lbytes);
        printf("Heap probe max block: %u\n\n", maxblk);
    }
    else if (!strcmp(cmd, "#HELP")) {
        printf("Available commands:\n");
        printf(" #fresh     - Clear the dictionary\n");
        printf(" #list      - List dictionary contents\n");
        printf(" #save <fn> - Save dictionary to file (var binary)\n");
        printf(" #load <fn> - Load dictionary from file\n");
        printf(" #mem       - Show memory usage summary\n");
        printf(" #help      - Show this help\n");
        printf(" #quit      - Exit the program\n\n");
    }
    else if (!strcmp(cmd, "#QUIT")) {
        printf("Goodbye.\n");
        free_dictionary_links();
        exit(0);
    }
    else if (strncmp(cmd, "#SAVE ", 6) == 0) {
        arg = cmd + 6;
        while (*arg == ' ') arg++;
        if (!*arg) {
            printf("Usage: #SAVE filename\n");
            return;
        }

        fp = fopen(arg, "wb");
        if (fp == 0) {
            printf("Cannot create file: %s\n", arg);
            return;
        }

        /* Write header */
        fwrite(NIALL_MAGIC, 6, 1, fp);
        ver = 1;
        fwrite(&ver, sizeof(int), 1, fp);
        fwrite(&num_words, sizeof(int), 1, fp);

        /* Reset checksum */
        checksum_calc = 0;

        /* Save start token (0) */
        wlen_byte = strlen(words[0].word);
        fwrite(&wlen_byte, 1, 1, fp);
        checksum_calc += wlen_byte;
        for (j = 0; j < wlen_byte; j++) checksum_calc += (unsigned char)words[0].word[j];
        fwrite(words[0].word, wlen_byte, 1, fp);

        if (words[0].links == (char *)0) llen_short = 0;
        else llen_short = safe_links_len(words[0].links);
        fwrite(&llen_short, sizeof(short), 1, fp);
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;
        if (llen_short && words[0].links != (char *)0) {
            for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)words[0].links[j];
            fwrite(words[0].links, llen_short, 1, fp);
        }

        /* Save real words (1 to num_words) */
        for (i = 1; i <= num_words; i++) {
            wlen_byte = strlen(words[i].word);
            fwrite(&wlen_byte, 1, 1, fp);
            checksum_calc += wlen_byte;
            for (j = 0; j < wlen_byte; j++) checksum_calc += (unsigned char)words[i].word[j];
            fwrite(words[i].word, wlen_byte, 1, fp);

            if (words[i].links == (char *)0) llen_short = 0;
            else llen_short = safe_links_len(words[i].links);
            fwrite(&llen_short, sizeof(short), 1, fp);
            checksum_calc += llen_short & 0xFF;
            checksum_calc += (llen_short >> 8) & 0xFF;
            if (llen_short && words[i].links != (char *)0) {
                for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)words[i].links[j];
                fwrite(words[i].links, llen_short, 1, fp);
            }
        }

        /* Save end token (END_TOKEN) */
        wlen_byte = strlen(words[END_TOKEN].word);
        fwrite(&wlen_byte, 1, 1, fp);
        checksum_calc += wlen_byte;
        for (j = 0; j < wlen_byte; j++) checksum_calc += (unsigned char)words[END_TOKEN].word[j];
        fwrite(words[END_TOKEN].word, wlen_byte, 1, fp);

        if (words[END_TOKEN].links == (char *)0) llen_short = 0;
        else llen_short = safe_links_len(words[END_TOKEN].links);
        fwrite(&llen_short, sizeof(short), 1, fp);
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;
        if (llen_short && words[END_TOKEN].links != (char *)0) {
            for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)words[END_TOKEN].links[j];
            fwrite(words[END_TOKEN].links, llen_short, 1, fp);
        }

        /* Write checksum */
        fwrite(&checksum_calc, sizeof(short), 1, fp);

        fclose(fp);
        printf("Saved %d words to %s (var binary)\n", num_words, arg);
    }
    else if (strncmp(cmd, "#LOAD ", 6) == 0) {
        arg = cmd + 6;
        while (*arg == ' ') arg++;
        if (!*arg) {
            printf("Usage: #LOAD filename\n");
            return;
        }

        fp = fopen(arg, "rb");
        if (fp == 0) {
            printf("Cannot open file: %s\n", arg);
            return;
        }

        if (fread(magic, 6, 1, fp) != 1 ||
            strncmp(magic, NIALL_MAGIC, 6) != 0) {
            printf("Not a valid NIALL binary file: %s\n", arg);
            fclose(fp);
            return;
        }

        if (fread(&ver, sizeof(int), 1, fp) != 1 || ver != 1) {
            printf("Unsupported version\n");
            fclose(fp);
            return;
        }

        if (fread(&loaded_words, sizeof(int), 1, fp) != 1 || loaded_words > (MAX_WORDS - 1)) {
            printf("Invalid num_words\n");
            fclose(fp);
            return;
        }

        /* Clear dictionary */
        free_dictionary_links();
        num_words = 0;

        strcpy(words[0].word, " ");
        if (!set_links_str(0, "")) { printf("Out of memory.\n"); fclose(fp); return; }

        words[END_TOKEN].word[0] = '.';
        words[END_TOKEN].word[1] = 0;
        if (!set_links_str(END_TOKEN, "")) { printf("Out of memory.\n"); fclose(fp); return; }

        checksum_calc = 0;

        /* Load start token (0) */
        if (fread(&wlen_byte, 1, 1, fp) != 1) { printf("Truncated (start word len)\n"); fclose(fp); return; }
        checksum_calc += wlen_byte;
        if (wlen_byte > MAX_WORD_LEN) { printf("Invalid start word len\n"); fclose(fp); return; }

        if (fread(words[0].word, wlen_byte, 1, fp) != 1) { printf("Truncated (start word)\n"); fclose(fp); return; }
        words[0].word[wlen_byte] = 0;
        for (j = 0; j < wlen_byte; j++) checksum_calc += (unsigned char)words[0].word[j];

        if (fread(&llen_short, sizeof(short), 1, fp) != 1) { printf("Truncated (start links len)\n"); fclose(fp); return; }
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;

        if (llen_short > 0) {
            char *tmp;
            tmp = (char *)malloc((unsigned short)(llen_short + 1));
            if (tmp == (char *)0) { printf("Out of memory loading start links.\n"); fclose(fp); return; }
            if (fread(tmp, llen_short, 1, fp) != 1) { printf("Truncated (start links)\n"); free(tmp); fclose(fp); return; }
            tmp[llen_short] = 0;
            for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)tmp[j];
            if (!set_links_str(0, tmp)) { printf("Out of memory.\n"); free(tmp); fclose(fp); return; }
            free(tmp);
        } else {
            if (!set_links_str(0, "")) { printf("Out of memory.\n"); fclose(fp); return; }
        }

        /* Load real words (1..loaded_words) */
        for (i = 1; i <= loaded_words; i++) {

            if (fread(&wlen_byte, 1, 1, fp) != 1) { printf("Truncated at word %d (len)\n", i); fclose(fp); return; }
            checksum_calc += wlen_byte;
            if (wlen_byte > MAX_WORD_LEN) { printf("Invalid word len at %d\n", i); fclose(fp); return; }

            if (fread(words[i].word, wlen_byte, 1, fp) != 1) { printf("Truncated at word %d\n", i); fclose(fp); return; }
            words[i].word[wlen_byte] = 0;
            for (j = 0; j < wlen_byte; j++) checksum_calc += (unsigned char)words[i].word[j];

            if (fread(&llen_short, sizeof(short), 1, fp) != 1) { printf("Truncated at links len %d\n", i); fclose(fp); return; }
            checksum_calc += llen_short & 0xFF;
            checksum_calc += (llen_short >> 8) & 0xFF;

            if (llen_short > 0) {
                char *tmp2;
                tmp2 = (char *)malloc((unsigned short)(llen_short + 1));
                if (tmp2 == (char *)0) { printf("Out of memory loading links at %d\n", i); fclose(fp); return; }
                if (fread(tmp2, llen_short, 1, fp) != 1) { printf("Truncated at links %d\n", i); free(tmp2); fclose(fp); return; }
                tmp2[llen_short] = 0;
                for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)tmp2[j];
                if (!set_links_str(i, tmp2)) { printf("Out of memory.\n"); free(tmp2); fclose(fp); return; }
                free(tmp2);
            } else {
                if (words[i].links != (char *)0) { free(words[i].links); words[i].links = (char *)0; }
            }
        }

        /* Load end token (END_TOKEN) */
        if (fread(&wlen_byte, 1, 1, fp) != 1) { printf("Truncated (end word len)\n"); fclose(fp); return; }
        checksum_calc += wlen_byte;
        if (wlen_byte > MAX_WORD_LEN) { printf("Invalid end word len\n"); fclose(fp); return; }

        if (fread(words[END_TOKEN].word, wlen_byte, 1, fp) != 1) { printf("Truncated (end word)\n"); fclose(fp); return; }
        words[END_TOKEN].word[wlen_byte] = 0;
        for (j = 0; j < wlen_byte; j++) checksum_calc += (unsigned char)words[END_TOKEN].word[j];

        if (fread(&llen_short, sizeof(short), 1, fp) != 1) { printf("Truncated (end links len)\n"); fclose(fp); return; }
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;

        if (llen_short > 0) {
            char *tmp3;
            tmp3 = (char *)malloc((unsigned short)(llen_short + 1));
            if (tmp3 == (char *)0) { printf("Out of memory loading end links.\n"); fclose(fp); return; }
            if (fread(tmp3, llen_short, 1, fp) != 1) { printf("Truncated (end links)\n"); free(tmp3); fclose(fp); return; }
            tmp3[llen_short] = 0;
            for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)tmp3[j];
            if (!set_links_str(END_TOKEN, tmp3)) { printf("Out of memory.\n"); free(tmp3); fclose(fp); return; }
            free(tmp3);
        } else {
            if (!set_links_str(END_TOKEN, "")) { printf("Out of memory.\n"); fclose(fp); return; }
        }

        /* Read saved checksum */
        if (fread(&checksum_saved, sizeof(short), 1, fp) != 1) {
            printf("No checksum found\n");
            fclose(fp);
            return;
        }

        if (checksum_calc != checksum_saved) {
            printf("Checksum mismatch (calc %u, saved %u)\n", checksum_calc, checksum_saved);
            fclose(fp);
            return;
        }

        num_words = loaded_words;

        fclose(fp);
        printf("Loaded %d words from %s (var binary)\n", num_words, arg);
    }
    else {
        printf("Unknown command (Try #HELP): %s\n", cmd);
    }
}

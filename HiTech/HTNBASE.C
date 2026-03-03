/* HTNBASE.C - Main NIALL code (input + learning) without reply + commands
   Compile & Link, all modules:

     C -V HTNBASE.C REPLY.C COMMANDS.C MEM.C

   Link:
     LINK -z -p text=0,data,bss -c 100h -o HTNIALL.COM CRTCPM.OBJ HTNBASE.OBJ REPLY.OBJ COMMANDS.OBJ MEM.OBJ LIBC.LIB
*/

#include "NIALL.H"

/* Input and sentence buffers */
char input_buf[MAX_INPUT + 1];
char sentence_words[MAX_SENTENCE][32];
int sentence_len = 0;

/* Global dictionary */
WordEntry words[MAX_WORDS + 1];
int num_words = 0;

/* Flag: was the last input a command (#...) ? */
int comm = 0;

/* Reduce stack pressure by moving learn_sentence() working buffers to static storage */
static char ls_buf[MAX_LINKS_STR];
static int ls_next_ids[128];
static int ls_next_cnts[128];
static char ls_out[MAX_LINKS_STR];

/* ---------------- Function Prototypes ---------------- */
void user_input();
void learn_sentence();
void generate_reply();   /* External from REPLY.C */
void dict_check();       /* External from REPLY.C */
int find_word();
void strip_punctuation();
void normalise_spaces();
void handle_command();   /* External from COMMANDS.C */

/* ---------------- Helpers ---------------- */

int idx_ok(x)
    int x;
{
    return (x >= 0 && x <= END_TOKEN);
}

/* Validate a word token before learning it */
int valid_word(s)
    char *s;
{
    int i;
    char c;

    if (s == (char *)0) return 0;
    if (s[0] == 0) return 0;
    if (s[0] == ' ' && s[1] == 0) return 0;

    i = 0;
    while ((c = s[i]) != 0) {
        if (c != ' ') return 1;
        i++;
    }
    return 0;
}

/* Free all allocated links for dictionary entries */
void free_dictionary_links()
{
    int i;
    for (i = 0; i <= END_TOKEN; i++) {
        if (words[i].links != (char *)0) {
            free(words[i].links);
            words[i].links = (char *)0;
        }
    }
}

/* Safe limited strlen for links (avoids runaway if corrupted) */
unsigned short safe_links_len(s)
    char *s;
{
    unsigned short n;

    if (s == (char *)0) return 0;

    n = 0;
    while (n < (unsigned short)0xFFFF && s[n]) n++;
    return n;
}

/* Replace links string - with exact-sized heap copy.
   Returns 1 on success, 0 on OOM.
*/
int set_links_str(i, src)
    int i;
    char *src;
{
    char *p;
    unsigned short n;

    if (!idx_ok(i)) return 0;

    if (src == (char *)0) src = "";

    n = safe_links_len(src);

    p = (char *)malloc((unsigned short)(n + 1));
    if (p == (char *)0) return 0;

    if (n) memcpy(p, src, n);
    p[n] = 0;

    if (words[i].links != (char *)0) free(words[i].links);
    words[i].links = p;

    return 1;
}

/* Convenience: set links to "1|f(1)" */
int set_links_fmt_1(i, f)
    int i;
    int f;
{
    char tmp[32];

    sprintf(tmp, "1|%d(1)", f);
    return set_links_str(i, tmp);
}

/* ---------------- Main Program ---------------- */
main()
{
    int et;
    int i;

    printf("Welcome to NIALL (HI-TECH C V3.09 port) v1.00\n");
    printf("Non Intelligent (AMOS) Language Learner\n");
    printf("Adapted for old CP/M compiler on TRS-80 4P\n\n");

    for (i = 0; i <= END_TOKEN; i++) {
        words[i].links = (char *)0;
        words[i].word[0] = 0;
    }

    strcpy(words[0].word, " ");
    if (!set_links_str(0, "")) {
        printf("Out of memory initialising word 0.\n");
        exit(1);
    }

    et = END_TOKEN;
    words[et].word[0] = '.';
    words[et].word[1] = 0;
    if (!set_links_str(et, "")) {
        printf("Out of memory initialising END_TOKEN.\n");
        exit(1);
    }

    srand(123);

    while (1) {
        user_input();
        if (!comm) {
            generate_reply();
        }
        dict_check();
    }
}

/* ---------------- Input Handling ---------------- */
void user_input()
{
    int i, j, k;
    int pos;
    int word_start, word_end;
    int sent_start, sent_end;
    char *rc;

    comm = 0;
    printf("\nUSER: ");
    fflush(stdout);

    input_buf[0] = 0;
    rc = fgets(input_buf, MAX_INPUT, stdin);
    if (rc == (char *)0) return;

    if (input_buf[0] == '\n' || input_buf[0] == '\r' || input_buf[0] == 0) return;

    printf("DEBUG: Input read: %s\n", input_buf);

    for (i = 0; input_buf[i]; i++) {
        if (input_buf[i] == '\r' || input_buf[i] == '\n') {
            input_buf[i] = 0;
            break;
        }
    }

    if (input_buf[0] == '$') input_buf[0] = '#';

    if (input_buf[0] == '#') {
        handle_command(input_buf);
        comm = 1;
        return;
    }

    for (i = 0; input_buf[i]; i++) {
        char c = input_buf[i];
        if (c >= 'A' && c <= 'Z') input_buf[i] = c + ('a' - 'A');
    }

    for (i = 0; input_buf[i]; i++) {
        if (input_buf[i] == '\t') input_buf[i] = ' ';
    }

    strip_punctuation(input_buf);
    normalise_spaces(input_buf);

    pos = 0;
    while (input_buf[pos]) {

        while (input_buf[pos] == ' ') pos++;
        if (!input_buf[pos]) break;

        sent_start = pos;

        while (input_buf[pos] && input_buf[pos] != '.') pos++;
        sent_end = pos;

        sentence_len = 0;
        j = sent_start;

        while (j < sent_end) {

            while (j < sent_end && input_buf[j] == ' ') j++;
            if (j >= sent_end) break;

            word_start = j;

            while (j < sent_end && input_buf[j] != ' ') j++;
            word_end = j;

            if (word_end > word_start && sentence_len < MAX_SENTENCE) {
                k = 0;
                while (word_start + k < word_end && k < MAX_WORD_LEN) {
                    sentence_words[sentence_len][k] = input_buf[word_start + k];
                    k++;
                }
                sentence_words[sentence_len][k] = 0;

                if (sentence_words[sentence_len][0] != 0)
                    sentence_len++;
            }
        }

        if (sentence_len > 0 && num_words < (END_TOKEN - 1)) {
            learn_sentence();
        }

        if (input_buf[pos] == '.') pos++;
    }
}

/* ---------------- AMOS BASIC-style punctuation stripping ---------------- */
void strip_punctuation(s)
    char *s;
{
    char clean[MAX_INPUT + 1];
    int i = 0;
    int j = 0;
    char c;

    while (s[i]) {
        c = s[i];
        if (c == ',') c = '.';
        if ((c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == ' ' || c == '.') {
            clean[j++] = c;
        }
        i++;
    }
    clean[j] = 0;
    strcpy(s, clean);
}

/* Normalise whitespace in-place */
void normalise_spaces(s)
    char *s;
{
    int r = 0;
    int w = 0;
    int last_space = 1;
    char c;

    while ((c = s[r++]) != 0) {
        if (c == ' ') {
            if (!last_space) {
                s[w++] = ' ';
                last_space = 1;
            }
        } else {
            s[w++] = c;
            last_space = 0;
        }
    }
    while (w > 0 && s[w - 1] == ' ') w--;
    s[w] = 0;

    r = 0;
    w = 0;
    while ((c = s[r++]) != 0) {
        if (c == ' ') {
            if (s[r] == '.') continue;
            s[w++] = ' ';
        } else if (c == '.') {
            s[w++] = '.';
            if (s[r] == ' ') r++;
        } else {
            s[w++] = c;
        }
    }
    s[w] = 0;
}

/* ---------------- Dictionary Lookup ---------------- */
int find_word(w)
    char *w;
{
    int i;
    for (i = 1; i <= num_words; i++)
        if (!strcmp(words[i].word, w))
            return i;
    return 0;
}

/* ---------------- Learning (ANALYSESENTENCE) ---------------- */
void learn_sentence()
{
    int old;
    int h;
    int f;

    char *src;
    char *buf;
    int *ids;
    int *cnts;

    int n;
    char *pipe;
    char *p;
    char *lp;
    char *rp;

    int found;
    int i;
    int total;

    char tmp[32];
    int pos;

    old = 0;

    buf = ls_buf;
    ids = ls_next_ids;
    cnts = ls_next_cnts;

    for (h = 0; h <= sentence_len; h++) {

        if (h == sentence_len) {
            f = END_TOKEN;
        } else {

            if (!valid_word(sentence_words[h])) {
                old = 0;
                continue;
            }

            f = find_word(sentence_words[h]);

            if (!f) {

                if (num_words >= (END_TOKEN - 1))
                    return;

                num_words++;

                strncpy(words[num_words].word, sentence_words[h], MAX_WORD_LEN);
                words[num_words].word[MAX_WORD_LEN] = 0;

                words[num_words].links = (char *)0;
                f = num_words;
            }
        }

        if (!idx_ok(old)) old = 0;
        if (!idx_ok(f)) f = END_TOKEN;

        if (words[old].links == (char *)0 || words[old].links[0] == 0) {
            if (!set_links_fmt_1(old, f)) {
                printf("Out of memory updating links for word %d.\n", old);
                return;
            }
            old = f;
            continue;
        }

        src = words[old].links;
        strncpy(buf, src, MAX_LINKS_STR - 1);
        buf[MAX_LINKS_STR - 1] = 0;

        pipe = strchr(buf, '|');
        if (!pipe) {
            if (!set_links_fmt_1(old, f)) {
                printf("Out of memory resetting links for word %d.\n", old);
                return;
            }
            old = f;
            continue;
        }

        p = pipe + 1;
        n = 0;

        while (*p && n < 128) {
            lp = strchr(p, '(');
            rp = strchr(p, ')');
            if (!lp || !rp || rp < lp) break;

            *lp = 0;
            ids[n] = atoi(p);
            *lp = '(';

            cnts[n] = atoi(lp + 1);

            if (!idx_ok(ids[n])) break;
            if (cnts[n] < 1) cnts[n] = 1;

            n++;
            p = rp + 1;
        }

        if (n == 0) {
            if (!set_links_fmt_1(old, f)) {
                printf("Out of memory resetting links for word %d.\n", old);
                return;
            }
            old = f;
            continue;
        }

        found = 0;
        for (i = 0; i < n; i++) {
            if (ids[i] == f) {
                cnts[i]++;
                found = 1;
                break;
            }
        }
        if (!found && n < 128) {
            ids[n] = f;
            cnts[n] = 1;
            n++;
        }

        total = 0;
        for (i = 0; i < n; i++) total += cnts[i];

        ls_out[0] = 0;
        pos = 0;

        sprintf(tmp, "%d|", total);
        if ((int)strlen(tmp) >= MAX_LINKS_STR - 1) {
            if (!set_links_fmt_1(old, f)) {
                printf("Out of memory resetting links for word %d.\n", old);
                return;
            }
            old = f;
            continue;
        }
        strcpy(ls_out, tmp);
        pos = strlen(ls_out);

        for (i = 0; i < n; i++) {
            sprintf(tmp, "%d(%d)", ids[i], cnts[i]);
            if (pos + (int)strlen(tmp) >= MAX_LINKS_STR - 1) {
                break;
            }
            strcpy(ls_out + pos, tmp);
            pos += strlen(tmp);
            ls_out[pos] = 0;
        }

        if (!set_links_str(old, ls_out)) {
            printf("Out of memory updating links for word %d.\n", old);
            return;
        }

        old = f;
    }
}

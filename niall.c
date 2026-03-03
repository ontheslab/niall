/* niall.c - NIALL - Non Intelligent (AMOS) Language Learner (Markov-chain chatbot), CP/M C port of AMOS BASIC
   Original AMOS BASIC by Matthew Peck (1990)
   C port by Intangybles (Adventures in returning to 'C')

   Compile (production — fits NABU + TRS-80 Model 4P):
     zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niall.c -o NIALL

   Compile (debug — TRS-80 only, enables #debug command):
     zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size -DDEBUG niall.c -o NIALL

   Save file verifier:
     zcc +cpm -vn -create-app -compiler=sdcc niallchk.c -o NIALLCHK

   Target platforms : CP/M 2.2 — NABU, TRS-80 Model 4P
   Compiler        : z88dk with SDCC backend (Z80, 64 KB address space)

   -----------------------------------------------------------------------
   Version 1.13 (current)
   - Dict-full message now resets per learn_sentence call (once per input
     sentence) rather than once per program run, so it fires every time a
     sentence fails to add new words.
   - START_LINKS_STR restored to 512, START_LINK_PAIRS to 50: string
     savings in load_dictionary freed enough space.
     6+50*10=506 < 511. Final .COM: 48,274 bytes.
   - Removed unused #include <stdarg.h>.
   - Inlined put_u16_le/get_u16_le (each called once).

   Version 1.12
   - Code size reduction to fit NABU CP/M (smaller TPA than TRS-80 4P).
     z88dk bakes BSS as zeros into the .COM; every BSS byte costs one
     .COM byte. TRS-80 loads up to ~50 KB; NABU limit is ~48.5 KB.
     * debug_mode/dbg_hex_n/#debug machinery wrapped in #ifdef DEBUG;
       eliminates all debug branches from the production binary.
     * Build flag changed from --opt-code-speed to --opt-code-size.
     * MAX_SENTENCE reduced 64->32: saves 1,024 bytes BSS.
     * START_LINKS_STR 512->256, START_LINK_PAIRS 50->27: saves 768 bytes.
     Total .COM reduction: ~2,920 bytes (50,380 -> 47,460 bytes).
   - Fixed dict-full warning: static local 'warned' was never reset by
     #fresh; replaced with file-scope dict_full_warned.

   Version 1.11
   - Fixed last-word getting 0| links: dict-full path used return,
     removed the loop before END_TOKEN was linked. Changed to continue
     so the loop always reaches h==sentence_len.
   - Fixed odd start->END_TOKEN link: when all words in a sentence
     fail valid_word(), old stays 0, causing start_word_links to record a
     transition to END_TOKEN(150). Fixed by breaking early when old==0.
   - Added START_LINKS_STR/START_LINK_PAIRS separate buffer for words[0]
     (the start-of-sentence token needs to track far more unique first-words
     than regular words). All learn/generate/save/load/#list/#fresh paths
     updated to use start_word_links[] when word index == 0.
   - Moved ids[]/cnts[] and parse/rebuild buffers to static globals to
     avoid CP/M stack crash.

   Version 1.10
   - Fixed END_TOKEN slot corruption: guard was num_words >= MAX_WORDS,
     but num_words++ fires first, writing into words[MAX_WORDS] which is
     the END_TOKEN sentinel. Guard changed to num_words >= MAX_WORDS - 1.
   - Added MAX_LINK_PAIRS=11: caps tracked transitions so the rebuild loop
     can never write a total that exceeds the sum of stored pairs (which
     was corrupting save files when any word exceeded link capacity).
   - Dict-full message now reports correct capacity (MAX_WORDS - 1 = 149).
   - Removed wrongly placed num_words < MAX_WORDS guard in user_input so
     associations between known words are still learned when the list is full.

   Version 1.09
   - Reduced MAX_WORDS 1000->100, MAX_LINKS_STR 512->128 to fit CP/M RAM.
     Previous .COM was >64 KB (machine RAM) — could not load at all.
     New values: total .COM ~38 KB, fits with headroom below BDOS.
   - Reduced ids[]/cnts[] from 64 to 28 entries to match new link capacity.

   Version 1.08  (z88dk/SDCC port overhaul)
   - Replaced all raw POSIX fd I/O with stdio FILE* in binary mode.
     CP/M BDOS requires 128-byte sector alignment; raw write() of sub-sector
     sizes returns <=0. stdio buffers internally and handles alignment.
   - Added fflush(stdout) before all fgets() calls — mandatory on CP/M to
     prevent I/O lockup at the prompt.
   - Fixed extern int debug_mode initialiser (SDCC warning).
   - Fixed SDCC const-qualifier warnings (warning 196) in ternary expressions.
   - Removed dead END_TOKEN array initialisations (SDCC warning 165).
   - Removed dead c<0 check in generate_reply (SDCC warning 110).
   - Added remove(fname) before fopen("wb") for clean CP/M file creation.
   - Added fseek(fp, 0L, SEEK_SET) after fopen("rb") — CP/M FCB position
     counter is not reset by fopen alone.
   - Moved from v2 to v3 save format: 7-byte header (magic+ver+num_words,
     no checksum in header), variable-length records, 2-byte checksum at end.

   Version 1.07  (Move to Hi-Tech C & back — multiple sub-revisions a–j)
   - Diagnosed and fixed learn function not updating memory correctly.
   - Clarified link string invariant: total == sum of all weights,
     link_count == number of id(weight) pairs. Do not modify in-place with
     pointer arithmetic — always rebuild from the parsed array.
   - Added #listd diagnostic list command.
   - Added #debug command and file-based debug logging (later replaced with
     printf-based trace in the same version series).
   - Rebuilt load and save — moved to binary files for CP/M safety.
   - Identified that raw write() fails on CP/M sub-sector I/O (fixed in 1.08).
   - Added extra infinite-loop protection in reply generation.

   Version 1.06
   - Found and corrected mismatch between dictionary setup and reset.
   - Added extra infinite loop protection in reply generation.

   Version 1.05
   - Debugging assistance from GTAMP.
   - Added clear function to dictionary load and token reset to #fresh.
   - Added safety counter to response generation (infinite loop fix).

   Version 1.04
   - Added #save and #load commands (first attempt — broken).
   - Added filename prompt and default filename handling.

   Version 1.03
   - Added #help command.
   - Added section descriptions and partial documentation.

   Version 1.02
   - Fixed dictionary update bug and MAX_WORDS overflow warning.
   - Added punctuation stripping to match AMOS BASIC behaviour.
   - Fixed mixed break/return statements.
   - Added END_TOKEN (MAX_WORDS) as end-of-sentence sentinel.
   - Added capitalisation of first reply letter and trailing full stop.

   Version 1.01
   - First working version for NABU CP/M: List, Fresh, Quit functional.

   Version 1.00
   - Initial port begun on TRS-80 Model 4P — partly complete.
   -----------------------------------------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ------------------- Configuration -------------------- */
/*
   Memory parameters — every byte of BSS adds one byte to the .COM file
   (z88dk includes BSS as zeros in the CP/M flat binary). Values below
   are tuned so the total .COM fits in the NABU's TPA (~48.5 KB limit).

   END_TOKEN is the end-of-sentence sentinel. It occupies the slot at
   words[MAX_WORDS] and is never stored as a real dictionary word; it
   behaves like the "." token in the original AMOS BASIC version.

   Link string format: "total|id(cnt)id(cnt)..."
     total = sum of all transition counts from this word
     id    = dictionary index of the next word
     cnt   = number of times that transition was observed

   Pair size budget (worst case: "150(9999)" = 10 chars, header "99999|" = 6):
     Regular words : 6 + 11*10 = 116 < 128  — always fits in MAX_LINKS_STR
     Start token   : 6 + 50*10 = 506 < 511  — always fits in START_LINKS_STR
*/

#define MAX_WORDS        150   /* real words occupy indices 1..149            */
#define MAX_SENTENCE      32   /* max words parsed per input sentence         */
#define MAX_INPUT        255   /* max characters per input line               */
#define MAX_LINKS_STR    128   /* link string buffer per regular word         */
#define MAX_LINK_PAIRS    11   /* max tracked successors per regular word     */
#define START_LINKS_STR  512   /* link string buffer for words[0] start token */
#define START_LINK_PAIRS  50   /* max tracked successors for words[0]         */
#define MAX_WORD_LEN      31   /* max characters in a dictionary word         */

#define END_TOKEN (MAX_WORDS)  /* sentinel index — marks end of sentence      */

/* ------------------ Data Structures ------------------- */
/*
   WordEntry holds one dictionary entry. The links string encodes all
   observed transitions from that word in compact text form so it can
   be inspected, saved and loaded without binary struct concerns.

   words[0]          — start-of-sentence token (word text is a single space)
   words[1..n]       — real dictionary words, n == num_words
   words[END_TOKEN]  — not a real entry; index used as the end sentinel only
*/

typedef struct {
    char word[32];
    char links[MAX_LINKS_STR];
} WordEntry;

/* Dictionary */
WordEntry words[MAX_WORDS + 1];
int num_words = 0;

/* Input and sentence buffers */
char input_buf[MAX_INPUT + 1];
char sentence_words[MAX_SENTENCE][32];
int  sentence_len = 0;

/* -------------- I/O Buffers and Globals --------------- */
/*
   All large working buffers are static globals rather than stack locals.
   CP/M C stacks can be very small; large locals risk corrupting the heap
   or stdio state. Static placement keeps them in BSS where their size
   is already accounted for in the .COM file size.

   start_word_links — link string for words[0]; separate from WordEntry
   because the start token needs a much larger buffer than regular words.

   link_parse_buf / link_rebuild_buf — shared workspace for learn_sentence
   so it never needs a large stack allocation during sentence processing.
*/

static unsigned char io_wrec[32];                  /* save/load word scratch  */
static unsigned char io_lrec[MAX_LINKS_STR];       /* save/load links scratch */

static char start_word_links[START_LINKS_STR];     /* words[0] link string    */
static char link_parse_buf[START_LINKS_STR];       /* learn_sentence parse    */
static char link_rebuild_buf[START_LINKS_STR];     /* learn_sentence rebuild  */

/* Command flag — set by user_input() when a # command is processed so
   the main loop knows not to call generate_reply() for that turn. */
int comm = 0;

/* --------------------- Prototypes --------------------- */

void user_input(void);
void learn_sentence(void);
void generate_reply(void);
void handle_command(char *cmd);
int  find_word(const char *w);
void strip_punctuation(char *s);
void save_dictionary(const char *fname);
void load_dictionary(const char *fname);
int  valid_word(const char *w);
int  validate_links(const char *s, int *total_val, int *sum_cnt, int *pair_count);

/* --------- Debug Mode (compile with -DDEBUG) --------- */
/*
   The entire debug subsystem is compiled out of the normal binary.
   To enable it, add -DDEBUG to the build command (TRS-80 builds only —
   the larger code does not fit in the NABU's TPA).

   At runtime, toggle output with the #debug command.
   DBG() macro calls are available throughout but currently unused;
   explicit if (debug_mode) blocks are used in save/load for fine control.
*/

#ifdef DEBUG
int debug_mode = 0;                  /* 0 = off, 1 = on — toggled by #debug  */
#define DBG(...) do { if (debug_mode) printf(__VA_ARGS__); } while (0)
#endif

/* Reset each sentence so the message fires once per input line, not
   once per program run. Also reset by #fresh. */
static int dict_full_warned = 0;

/* ------------------------ Main ------------------------ */
/*
   Initialises the start-of-sentence token then enters the main loop.
   Each iteration reads one line of user input (learning from it if it
   is normal text) then generates a reply unless a command was entered.
*/

int main(void)
{
    printf("Welcome to NIALL (CP/M C port by Intangybles) v1.13\n");
    printf("Non Intelligent (AMOS) Language Learner\n");
    printf("Original by Matthew Peck (1990)\n\n");
    printf("Type #help - for Commands\n\n");

    strcpy(words[0].word, " ");
    words[0].links[0] = 0;

    while (1) {
        user_input();
        if (!comm)
            generate_reply();
    }
}

/* ------------------- Input Handling ------------------- */
/*
   Reads one line of user input, lowercases it, and either dispatches a
   # command or passes each sentence to the learning routine.

   - Commands (lines beginning with #) are handled before punctuation
     stripping so that filenames are not mangled.
   - Punctuation is stripped to match the original AMOS BASIC behaviour.
   - A single input line may contain multiple sentences separated by
     full stops; each is learned independently.
   - fflush(stdout) before fgets() is mandatory on CP/M — without it the
     buffered prompt may never appear and the program would often hang.
*/

void user_input(void)
{
    comm = 0;
    printf("\nUSER: ");
    fflush(stdout);

    fgets(input_buf, MAX_INPUT, stdin);
    if (input_buf[0] == '\n')
        return;

    /* convert to lowercase */
    for (int i = 0; input_buf[i]; i++)
        input_buf[i] = tolower((unsigned char)input_buf[i]);

    /* remove CR/LF */
    input_buf[strcspn(input_buf, "\r\n")] = 0;

    /* commands handled BEFORE stripping punctuation */
    if (input_buf[0] == '#') {
        handle_command(input_buf);
        comm = 1;
        return;
    }

    /* remove punctuation to match original AMOS BASIC */
    strip_punctuation(input_buf);

    /* split into sentences on '.' and learn each one */
    char *p = input_buf;

    while (*p) {
        sentence_len = 0;

        /* find end of this sentence */
        char *end = strchr(p, '.');
        if (end)
            *end = 0;

        /* split sentence into words */
        char *w = strtok(p, " ");
		while (w && sentence_len < MAX_SENTENCE) {
			strncpy(sentence_words[sentence_len], w, MAX_WORD_LEN);
			sentence_words[sentence_len][MAX_WORD_LEN] = 0;
			sentence_len++;
			w = strtok(NULL, " ");
		}

        if (sentence_len > 0)
			learn_sentence();

        if (!end)
            break;

        p = end + 1;
    }
}

/* ------------------ Valid Word Check ------------------ */
/*
   Returns 1 if the word is acceptable for the dictionary, 0 otherwise.
   A valid word must be non-empty and contain only letters or digits.
   This rejects whitespace, punctuation, and any stray control characters
   so they can never corrupt the link strings or dictionary indices.
   This is a must I found.
*/

int valid_word(const char *w)
{
    if (!w) return 0;
    if (*w == 0) return 0;

    while (*w) {
        if (!isalnum((unsigned char)*w))
            return 0;
        w++;
    }
    return 1;
}

/* --------------- Link String Validation --------------- */
/*
   Validates a link string in the form "total|id(cnt)id(cnt)..."

   Returns 1 if the string is structurally correct AND total equals the
   sum of all counts (AMOS-correct). Returns 0 otherwise.

   Option - fills the caller's output variables:
     total_val  — the parsed total field
     sum_cnt    — the computed sum of all counts
     pair_count — number of id(cnt) pairs found

   Used by #listd for diagnostic display and by load_dictionary to reject
   corrupt link strings before they enter the in-memory dictionary.
*/

int validate_links(const char *s, int *total_val, int *sum_cnt, int *pair_count)
{
    int total = 0;
    int sum = 0;
    int pairs = 0;

    if (total_val)  *total_val = 0;
    if (sum_cnt)    *sum_cnt = 0;
    if (pair_count) *pair_count = 0;

    if (!s || !*s)
        return 0;

    /* must contain '|' separating total from pairs */
    const char *bar = strchr(s, '|');
    if (!bar)
        return 0;

    /* total field must be digits only */
    for (const char *q = s; q < bar; q++) {
        if (*q < '0' || *q > '9')
            return 0;
    }

    total = atoi(s);
    if (total < 0)
        return 0;

    /* after '|' we expect zero or more id(cnt) pairs */
    const char *p = bar + 1;

    /* allow empty pairs section only when total == 0 */
    {
        const char *t = p;
        while (*t && isspace((unsigned char)*t))
            t++;
        if (*t == 0) {
            if (total != 0)
                return 0;
            if (total_val)  *total_val = total;
            if (sum_cnt)    *sum_cnt = sum;
            if (pair_count) *pair_count = pairs;
            return 1;
        }
    }

    while (*p) {
        int id = 0;
        int cnt = 0;

        /* trailing whitespace is allowed at end of string */
        if (isspace((unsigned char)*p)) {
            while (*p && isspace((unsigned char)*p))
                p++;
            if (*p != 0)
                return 0;
            break;
        }

        /* id: one or more digits */
        if (*p < '0' || *p > '9')
            return 0;

        while (*p >= '0' && *p <= '9') {
            id = id * 10 + (*p - '0');
            p++;
        }

        if (*p != '(')
            return 0;
        p++;

        /* cnt: one or more digits */
        if (*p < '0' || *p > '9')
            return 0;

        while (*p >= '0' && *p <= '9') {
            cnt = cnt * 10 + (*p - '0');
            p++;
        }

        if (*p != ')')
            return 0;
        p++;

        /* id must be a valid dictionary index or END_TOKEN */
        if (id < 0 || id > MAX_WORDS)
            return 0;

        if (cnt <= 0)
            return 0;

        sum += cnt;
        pairs++;
    }

    /* total must match the computed sum (core test/check) */
    if (total != sum)
        return 0;

    if (total_val)  *total_val = total;
    if (sum_cnt)    *sum_cnt = sum;
    if (pair_count) *pair_count = pairs;

    return 1;
}

/* --------------- Punctuation Stripping ---------------- */
/*
   Removes punctuation from the input string in-place, matching the
   behaviour of the original AMOS BASIC version.

   - Commas are converted to full stops (sentence separators).
   - Only letters, digits, spaces, and full stops are retained.
   - Everything else is discarded so that words are clean and
     punctuation never contaminates the dictionary.
*/

void strip_punctuation(char *s)
{
    char clean[MAX_INPUT + 1];
    int j = 0;

    for (int i = 0; s[i]; i++) {
        char c = s[i];

        /* commas become sentence separators, AMOS BASIC */
        if (c == ',')
			c = '.';

        /* retain only letters, digits, spaces, and full stops */
        if ((c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == ' ' || c == '.') {
            clean[j++] = c;
        }
    }

    clean[j] = 0;
    strcpy(s, clean);
}

/* ----------------- Dictionary Lookup ------------------ */
/*
   Searches the dictionary for an exact word match (case-sensitive;
   all input is lowercased).

   Returns the index (1..num_words) if found, or 0 if not found.
   Index 0 is the start-of-sentence token and is never returned
   as the result of a word lookup.
*/

int find_word(const char *w)
{
    for (int i = 1; i <= num_words; i++)
        if (!strcmp(words[i].word, w))
            return i;
    return 0;
}

/* ----------------- Sentence Learning ------------------ */
/*
   Processes sentence_words[0..sentence_len-1] and records the observed
   word transitions in the dictionary (Markov chain learning).

   For each adjacent pair (old -> f) in the sentence:
     - If f is a new word and the dictionary is not full, add it.
     - Parse the current link string for 'old' into ids[]/cnts[] arrays.
     - Increment the count for transition old->f, or append a new pair.
     - Recompute total as the sum of all counts and rebuild the string.
   The final word is linked to END_TOKEN to mark the sentence boundary.

   Key aspects maintained:
     - total always equals the sum of all cnt values in the link string.
     - The rebuilt string always fits within the buffer (MAX_LINK_PAIRS
       and START_LINK_PAIRS cap the number of tracked transitions).
     - If old==0 at the end of the sentence, no valid words were processed
       so the END_TOKEN link is skipped (prevents start->END entries).
*/

void learn_sentence(void)
{
    int old = 0;   /* index of the previous word (0 = start token) */

    /* Static arrays avoid large stack allocations on CP/M's small stack.
       Sized for the start token's larger capacity. */
    static int ids[START_LINK_PAIRS];
    static int cnts[START_LINK_PAIRS];

    /* Reset per sentence so the dict-full message fires once per input line */
    dict_full_warned = 0;

    for (int h = 0; h <= sentence_len; h++) {
        int f;

        /* final iteration: link the last valid word to END_TOKEN */
        if (h == sentence_len) {
            if (old == 0)   /* no valid words were processed — skip */
                break;
            f = END_TOKEN;
        } else {

            /* skip invalid words and reset the chain back to the start token */
            if (!valid_word(sentence_words[h])) {
                old = 0;
                continue;
            }

            f = find_word(sentence_words[h]);

            if (!f) {
                /* new word — add it if there is room */
                if (num_words >= MAX_WORDS - 1) {
                    if (!dict_full_warned) {
                        printf("Dictionary full (%d words).\n", MAX_WORDS - 1);
                        dict_full_warned = 1;
                    }
                    /* continue rather than return so the last successfully
                       added word still receives its END_TOKEN link */
                    continue;
                }
                num_words++;
                strncpy(words[num_words].word, sentence_words[h], MAX_WORD_LEN);
                words[num_words].word[MAX_WORD_LEN] = 0;
                words[num_words].links[0] = 0;
                f = num_words;
            }
        }

        /* select link buffer and capacity based on whether we are at the
           start token (old==0) or a regular word */
        {
            char *wlinks    = (old == 0) ? start_word_links : words[old].links;
            int   wlinks_sz = (old == 0) ? START_LINKS_STR  : MAX_LINKS_STR;
            int   wpairs    = (old == 0) ? START_LINK_PAIRS  : MAX_LINK_PAIRS;

            /* no existing links — write the first transition directly */
            if (!wlinks[0]) {
                snprintf(wlinks, wlinks_sz, "1|%d(1)", f);
                old = f;
                continue;
            }

            /* parse existing link string into ids[]/cnts[] arrays */
            int n = 0;
            strncpy(link_parse_buf, wlinks, wlinks_sz - 1);
            link_parse_buf[wlinks_sz - 1] = 0;

            char *p = strchr(link_parse_buf, '|');
            if (!p) {
                /* malformed — reset to a single transition */
                snprintf(wlinks, wlinks_sz, "1|%d(1)", f);
                old = f;
                continue;
            }
            p++;

            while (*p && n < wpairs) {
                char *lp = strchr(p, '(');
                char *rp = strchr(p, ')');

                if (!lp || !rp || rp < lp)
                    break;

                *lp = 0;
                ids[n] = atoi(p);
                *lp = '(';

                cnts[n] = atoi(lp + 1);

                if (ids[n] < 0 || ids[n] > MAX_WORDS)
                    break;

                if (cnts[n] < 1)
                    cnts[n] = 1;

                n++;
                p = rp + 1;
            }

            if (n == 0) {
                /* parse produced no valid pairs — reset */
                snprintf(wlinks, wlinks_sz, "1|%d(1)", f);
                old = f;
                continue;
            }

            /* increment existing transition or append a new one */
            {
                int found = 0;
                for (int i = 0; i < n; i++) {
                    if (ids[i] == f) {
                        cnts[i]++;
                        found = 1;
                        break;
                    }
                }
                if (!found && n < wpairs) {
                    ids[n] = f;
                    cnts[n] = 1;
                    n++;
                }
            }

            /* recalculate total as sum of all tracked weights */
            int total = 0;
            for (int i = 0; i < n; i++)
                total += cnts[i];

            /* rebuild the link string into the global buffer then copy back */
            int pos = 0;
            pos += snprintf(link_rebuild_buf + pos, wlinks_sz - pos, "%d|", total);
            for (int i = 0; i < n && pos < wlinks_sz - 1; i++) {
                pos += snprintf(link_rebuild_buf + pos, wlinks_sz - pos, "%d(%d)", ids[i], cnts[i]);
            }
            link_rebuild_buf[wlinks_sz - 1] = 0;

            strncpy(wlinks, link_rebuild_buf, wlinks_sz - 1);
            wlinks[wlinks_sz - 1] = 0;
        }
        old = f;
    }
}

/* ------------------ Reply Generation ------------------ */
/*
   Generates a reply by walking the Markov chain from the start token.

   Starting at words[0], the function repeatedly:
     - Reads the link string for the current word.
     - Selects the next word using weighted random selection across all
       observed transitions (higher counts = higher probability).
     - Appends the current word to the reply buffer.
     - Moves to the selected next word.
     - Stops when END_TOKEN is reached or the safety limit is hit.

   Output is capitalised, trimmed, and terminated with a full stop.
   The reply buffer is bounded — overflow is caught before it can
   corrupt memory adjacent to the reply array. <- Big Fix!
   A safety counter of 100 iterations prevents infinite chain loops.
*/

void generate_reply(void)
{
    if (!num_words) {
        printf("NIALL: I cannot speak yet!\n");
        return;
    }

    int c = 0;
    char reply[256] = "";
    int safety = 0;

    while (safety < 100) {
        safety++;

        /* >= catches END_TOKEN (== MAX_WORDS) as an early exit */
        if (c >= MAX_WORDS)
            break;

        char *cstr = (c == 0) ? start_word_links : words[c].links;

        /* no transitions from this word — cannot continue */
        if (!cstr[0])
            break;

        unsigned int total = (unsigned int)atoi(cstr);
        if (total == 0)
            break;

        int r = (rand() & 0x7fff);
        r = (r % (int)total) + 1;

        char *p = strchr(cstr, '(');
        int next = -1;

        while (p) {
            char *q = p - 1;
            while (q > cstr && *q != '|' && *q != ')')
                q--;

            if (*q == '|' || *q == ')')
                q++;

            int id = atoi(q);
            int cnt = atoi(p + 1);

            /* reject any corrupt or out-of-range IDs */
            if (id < 0 || id > MAX_WORDS)
                break;

            r -= cnt;
            if (r <= 0) {
                next = id;
                break;
            }

            p = strchr(p + 1, '(');
        }

        /* no valid next word was selected */
        if (next < 0)
            break;

        /* append current word — bounded to prevent reply buffer overflow */
		{
			size_t used = strlen(reply);
			size_t need = strlen(words[c].word) + 1 /* space */ + 1 /* NUL */;

			if (used + need > sizeof(reply))
				break;

			strcat(reply, words[c].word);
			strcat(reply, " ");
		}

        c = next;

        if (c == END_TOKEN)
            break;
    }

    /* trim leading spaces (left$) */
    while (reply[0] == ' ')
        memmove(reply, reply + 1, strlen(reply));

    /* trim trailing space (right$) */
    int len = strlen(reply);
    while (len > 0 && reply[len - 1] == ' ') {
        reply[len - 1] = 0;
        len--;
    }

    if (reply[0]) {
        /* capitalise first letter */
        if (reply[0] >= 'a' && reply[0] <= 'z')
            reply[0] = reply[0] - 'a' + 'A';

        /* append full stop — bounded append */
		{
			size_t used = strlen(reply);
			if (used + 2 <= sizeof(reply))
				strcat(reply, ".");
		}

        printf("NIALL: %s\n", reply);
    }
}

/* ----------------- Binary Save / Load ----------------- */
/*
   Version 3 binary file format — matches the Hi-Tech C COMMANDS.C pattern
   for compatibility between the two builds & it WORKS.

   All multi-byte integers are little-endian (LE). The checksum covers
   only the payload bytes, NOT the 7-byte header.

   Header (7 bytes):
     4 bytes  magic    "NIAL"
     1 byte   version  3
     2 bytes  num_words (LE unsigned short)

   Payload — one record per entry, entries 0..num_words:
     1 byte   wlen     word length (0..31); entry 0 always writes 0
     wlen     word bytes, no trailing NUL stored
     2 bytes  llen     link string length (LE unsigned short)
     llen     link string bytes, no trailing NUL stored

   Footer (2 bytes):
     unsigned short checksum (LE) — running byte sum of all payload bytes

   Note (CP/M files): CP/M allocates storage in 128-byte sectors so the
   physical file is always rounded up to the next sector boundary. Any
   bytes after the checksum are OS padding and should be ignored.

   AMOS (Style) ASCII files became too big for limited CP/M Storage
   and also introdiced - many - combinations of 'file' errors - moved
   to a binary file solution - Proved function in Hi-Tech 'C'.
*/

/* ------------------- Debug Helpers -------------------- */
/*
   dbg_hex_n prints a labelled hex dump of n bytes. It is only compiled
   into debug builds (-DDEBUG). At runtime, output is gated by debug_mode
   which is toggled with the #debug command.
*/

#ifdef DEBUG
static void dbg_hex_n(const char *tag, const unsigned char *p, unsigned int n)
{
    printf("%s", tag);
    for (unsigned int i = 0; i < n; i++) {
        printf("%02X", (unsigned)p[i]);
        if (i + 1 != n) printf(" ");
    }
    printf("\n");
}
#endif

/* -------------------- I/O Wrappers -------------------- */
/*
   write_all and read_all wrap fwrite/fread to return a success flag.
   Using stdio FILE* is a must on CP/M — raw POSIX write() calls failed on
   writes - assume because the BDOS requires 128-byte aligned transfers?
   The stdio handles internal buffering better.
*/

static int write_all(FILE *fp, void *buf, unsigned int n)
{
    return (unsigned int)fwrite(buf, 1, n, fp) == n;
}

static int read_all(FILE *fp, void *buf, unsigned int n)
{
    return (unsigned int)fread(buf, 1, n, fp) == n;
}

/* ---------------- Record Sanitisation ----------------- */
/*
   Prepares a word/link pair into the io_wrec/io_lrec static buffers
   ready for writing to disk. Rejects invalid word text and substitutes
   "0|" for a missing or empty link string so every saved record is valid.
*/

static void sanitise_record(unsigned char word_out[32], unsigned char links_out[MAX_LINKS_STR],
                            const char *w, const char *l)
{
    /* word: store either a valid word, or leave empty */
    memset(word_out, 0, 32);
    if (valid_word(w)) {
        strncpy((char*)word_out, w, 31);
        word_out[31] = 0;
    }

    /* links: store the existing string, or "0|" if absent */
    memset(links_out, 0, MAX_LINKS_STR);
    if (l && l[0]) {
        strncpy((char*)links_out, l, MAX_LINKS_STR - 1);
        links_out[MAX_LINKS_STR - 1] = 0;
    } else {
        strcpy((char*)links_out, "0|");
    }
}

/* ---------------- Save Dictionary (v3) ---------------- */

void save_dictionary(const char *fname)
{
    FILE *fp;
    unsigned char hdr[7];
    unsigned short checksum_calc;
    int i, j;
    unsigned char wlen_byte;
    unsigned short llen_short;

    /* Debug: log save start — only active in -DDEBUG builds */
#ifdef DEBUG
    if (debug_mode)
        printf("\n[SAVE] file='%s' num_words=%d\n", fname, num_words);
#endif

    remove(fname);                    /* delete old file for clean CP/M FCB  */
    fp = fopen(fname, "wb");
    if (fp == 0) { printf("Cannot save file.\n"); return; }
    fseek(fp, 0L, SEEK_SET);

    /* write 7-byte header: magic + version + num_words (no checksum here) */
    hdr[0]='N'; hdr[1]='I'; hdr[2]='A'; hdr[3]='L'; hdr[4]=3;
    hdr[5] = (unsigned char)(num_words & 0xff);
    hdr[6] = (unsigned char)(num_words >> 8);

    /* Debug: dump header bytes */
#ifdef DEBUG
    if (debug_mode) dbg_hex_n("[SAVE] hdr7: ", hdr, 7);
#endif

    if (!write_all(fp, hdr, 7)) { fclose(fp); printf("Save failed.\n"); return; }

    /* checksum begins AFTER the header */
    checksum_calc = 0;

    /* entry 0: start token — no word text; links come from start_word_links */
    {
        const char *sl = start_word_links[0] ? start_word_links : (char*)"0|";
        wlen_byte  = 0;
        llen_short = (unsigned short)strlen(sl);
        if (llen_short > (unsigned short)(START_LINKS_STR - 1))
            llen_short = (unsigned short)(START_LINKS_STR - 1);

        fwrite(&wlen_byte, 1, 1, fp);
        checksum_calc += wlen_byte;

        fwrite(&llen_short, sizeof(unsigned short), 1, fp);
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;
        for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)sl[j];
        if (llen_short) fwrite(sl, llen_short, 1, fp);
    }

    /* entries 1..num_words: real dictionary words */
    for (i = 1; i <= num_words; i++) {
        sanitise_record(io_wrec, io_lrec, words[i].word,
                       words[i].links[0] ? words[i].links : (char*)"0|");
        wlen_byte  = (unsigned char)strlen((char*)io_wrec);
        if (wlen_byte > 31) wlen_byte = 31;
        llen_short = (unsigned short)strlen((char*)io_lrec);
        if (llen_short > (unsigned short)(MAX_LINKS_STR - 1)) llen_short = (unsigned short)(MAX_LINKS_STR - 1);

        fwrite(&wlen_byte, 1, 1, fp);
        checksum_calc += wlen_byte;
        for (j = 0; j < (int)wlen_byte; j++) checksum_calc += (unsigned char)io_wrec[j];
        if (wlen_byte) fwrite(io_wrec, wlen_byte, 1, fp);

        fwrite(&llen_short, sizeof(unsigned short), 1, fp);
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;
        for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)io_lrec[j];
        if (llen_short) fwrite(io_lrec, llen_short, 1, fp);
    }

    /* write checksum at end of payload */
    fwrite(&checksum_calc, sizeof(unsigned short), 1, fp);

    fclose(fp);
    printf("Dictionary saved.\n");

    /* Debug: confirm checksum value */
#ifdef DEBUG
    if (debug_mode) printf("[SAVE] done (checksum=%u).\n", (unsigned int)checksum_calc);
#endif
}

/* ---------------- Load Dictionary (v3) ---------------- */

void load_dictionary(const char *fname)
{
    FILE *fp;
    unsigned char hdr[7];
    unsigned int disk_n, ver;
    unsigned short checksum_calc, checksum_saved;
    int i, j;
    unsigned char wlen_byte;
    unsigned short llen_short;

    /* Debug: log load start — only active in -DDEBUG builds */
#ifdef DEBUG
    if (debug_mode)
        printf("\n[LOAD] file='%s'\n", fname);
#endif

    fp = fopen(fname, "rb");
    if (fp == 0) { printf("Cannot load file.\n"); return; }
    fseek(fp, 0L, SEEK_SET);    /* force FCB position to start — CP/M safety */

    if (!read_all(fp, hdr, 7)) {
        fclose(fp);
        printf("Invalid file.\n");
        return;
    }

    /* Debug: dump header bytes */
#ifdef DEBUG
    if (debug_mode) dbg_hex_n("[LOAD] hdr7: ", hdr, 7);
#endif

    if (hdr[0]!='N' || hdr[1]!='I' || hdr[2]!='A' || hdr[3]!='L') {
        fclose(fp);
        /* Debug: report bad magic */
#ifdef DEBUG
        if (debug_mode) printf("[LOAD] bad magic\n");
#endif
        printf("Invalid file.\n");
        return;
    }

    ver    = (unsigned int)hdr[4];
    disk_n = (unsigned int)hdr[5] | ((unsigned int)hdr[6] << 8);
    if (disk_n > (unsigned int)MAX_WORDS) disk_n = (unsigned int)MAX_WORDS;

    /* Debug: log parsed header fields */
#ifdef DEBUG
    if (debug_mode) printf("[LOAD] ver=%u num_words=%u\n", ver, disk_n);
#endif

    if (ver != 3) {
        fclose(fp);
        /* Debug: report unsupported version */
#ifdef DEBUG
        if (debug_mode) printf("[LOAD] unsupported version %u\n", ver);
#endif
        printf("Wrong file version.\n");
        return;
    }

    /* reset dictionary before loading */
    num_words = 0;
    strcpy(words[0].word, " ");
    words[0].links[0] = 0;

    /* checksum begins AFTER the header */
    checksum_calc = 0;

    /* entry 0: start token — word text is discarded, links go to start_word_links */
    if (fread(&wlen_byte, 1, 1, fp) != 1) { fclose(fp); printf("Invalid file.\n"); return; }
    checksum_calc += wlen_byte;
    if (wlen_byte > 31) { fclose(fp); printf("Invalid file.\n"); return; }
    if (wlen_byte) {
        memset(io_wrec, 0, 32);
        if (!read_all(fp, io_wrec, wlen_byte)) { fclose(fp); printf("Invalid file.\n"); return; }
        for (j = 0; j < (int)wlen_byte; j++) checksum_calc += (unsigned char)io_wrec[j];
    }

    if (fread(&llen_short, sizeof(unsigned short), 1, fp) != 1) { fclose(fp); printf("Invalid file.\n"); return; }
    checksum_calc += llen_short & 0xFF;
    checksum_calc += (llen_short >> 8) & 0xFF;
    if (llen_short > (unsigned short)(START_LINKS_STR - 1)) llen_short = (unsigned short)(START_LINKS_STR - 1);
    memset(start_word_links, 0, START_LINKS_STR);
    if (llen_short) {
        if (!read_all(fp, start_word_links, llen_short)) { fclose(fp); printf("Invalid file.\n"); return; }
        for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)start_word_links[j];
    }
    start_word_links[llen_short] = 0;
    if (!start_word_links[0]) strcpy(start_word_links, "0|");

    /* entries 1..disk_n: real dictionary words */
    for (i = 1; i <= (int)disk_n; i++) {
        if (fread(&wlen_byte, 1, 1, fp) != 1) { fclose(fp); printf("Invalid file.\n"); return; }
        checksum_calc += wlen_byte;
        if (wlen_byte > 31) { fclose(fp); printf("Invalid file.\n"); return; }
        memset(io_wrec, 0, 32);
        if (wlen_byte) {
            if (!read_all(fp, io_wrec, wlen_byte)) { fclose(fp); printf("Invalid file.\n"); return; }
            for (j = 0; j < (int)wlen_byte; j++) checksum_calc += (unsigned char)io_wrec[j];
        }
        io_wrec[31] = 0;

        if (fread(&llen_short, sizeof(unsigned short), 1, fp) != 1) { fclose(fp); printf("Invalid file.\n"); return; }
        checksum_calc += llen_short & 0xFF;
        checksum_calc += (llen_short >> 8) & 0xFF;
        if (llen_short > (unsigned short)(MAX_LINKS_STR - 1)) llen_short = (unsigned short)(MAX_LINKS_STR - 1);
        memset(io_lrec, 0, MAX_LINKS_STR);
        if (llen_short) {
            if (!read_all(fp, io_lrec, llen_short)) { fclose(fp); printf("Invalid file.\n"); return; }
            for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)io_lrec[j];
        }
        io_lrec[MAX_LINKS_STR - 1] = 0;

        strncpy(words[i].word, (char*)io_wrec, 31); words[i].word[31] = 0;
        strncpy(words[i].links, (char*)io_lrec, MAX_LINKS_STR - 1); words[i].links[MAX_LINKS_STR - 1] = 0;
        if (!valid_word(words[i].word)) words[i].word[0] = 0;
        if (!words[i].links[0]) strcpy(words[i].links, "0|");

        /* reject structurally invalid link strings on load */
        {
            int t=0, s2=0, p=0;
            if (!validate_links(words[i].links, &t, &s2, &p))
                strcpy(words[i].links, "0|");
        }

        /* Debug: spot-check first and last records */
#ifdef DEBUG
        if (debug_mode && (i == 1 || i == (int)disk_n))
            printf("[LOAD] record %d word='%s'\n", i, words[i].word);
#endif
    }

    /* read and verify the trailing checksum */
    if (fread(&checksum_saved, sizeof(unsigned short), 1, fp) != 1) {
        fclose(fp);
        printf("No checksum found.\n");
        return;
    }

    fclose(fp);

    if (checksum_calc != checksum_saved) {
        printf("Bad Checksum (calc %u, saved %u).\n",
               (unsigned int)checksum_calc, (unsigned int)checksum_saved);
        return;
    }

    num_words = (int)disk_n;
    printf("Loaded %d words from %s\n", num_words, fname);

    /* Debug: confirm successful load */
#ifdef DEBUG
    if (debug_mode) printf("[LOAD] done.\n");
#endif
}

/* ------------------ Command Handler ------------------- */
/*
   Handles all commands that begin with '#'. Commands are detected before
   punctuation stripping so filenames reach here intact.

   Supported commands:
     #fresh        — clears the dictionary and resets all state
     #list         — prints all dictionary entries (compact)
     #listd        — prints dictionary with link validation (diagnostic;
                     not shown in #help but always available)
     #save [file]  — saves the dictionary to disk in v3 binary format;
                     prompts for a filename if none given (default NIALL.DAT)
     #load [file]  — loads a v3 binary dictionary from disk;
                     prompts for a filename if none given (default NIALL.DAT)
     #help         — lists the main commands
     #quit         — exits the program
     #debug        — toggles verbose save/load trace output;
                     only compiled in -DDEBUG builds (TRS-80 only)
*/

void handle_command(char *cmd)
{
	if (!strcmp(cmd, "#fresh")) {
		num_words = 0;
		dict_full_warned = 0;

		/* reset start token to initial state */
		strcpy(words[0].word, " ");
		words[0].links[0] = 0;
		start_word_links[0] = 0;

		printf("Dictionary cleared.\n");
	}
	else if (!strcmp(cmd, "#list")) {
		for (int i = 0; i <= num_words; i++) {
			const char *l = (i == 0) ? start_word_links : words[i].links;
			printf("%d %s %s\n", i,
				   words[i].word[0] ? (const char*)words[i].word : " ",
				   l[0] ? l : "0|");
		}
	}
	/* -------------- Diagnostic List (#listd) -------------- */
	/*
	   Extended version of #list that also validates each link string and
	   reports OK or BAD alongside the pair count. Use this after training
	   or loading to confirm no entries are malformed. Not shown in #help
	   to keep the help output short; the command is always available.
	*/
	else if (!strcmp(cmd, "#listd")) {
		for (int i = 0; i <= num_words; i++) {
			const char *w = words[i].word[0] ? (const char*)words[i].word : " ";
			const char *wl = (i == 0) ? start_word_links : words[i].links;
			const char *l = wl[0] ? wl : "0|";

			int total = 0, sum = 0, pairs = 0;
			int ok = validate_links(l, &total, &sum, &pairs);

			if (ok)
				printf("%d %s %s  [OK pairs=%d]\n", i, w, l, pairs);
			else
				printf("%d %s %s  [BAD]\n", i, w, l);
		}
	}
	else if (!strcmp(cmd, "#help")) {
        printf("Commands:\n");
        printf("  #fresh  - Clear dictionary\n");
        printf("  #list   - List dictionary\n");
        printf("  #save   - Save dictionary to disk\n");
        printf("  #load   - Load dictionary from disk\n");
        printf("  #quit   - Exit\n");
    }
    else if (!strncmp(cmd, "#save", 5)) {
        char fname[64] = "";

        if (sscanf(cmd + 5, "%s", fname) != 1) {
            printf("Filename (default NIALL.DAT): ");
            fflush(stdout);
            fgets(fname, 64, stdin);
            fname[strcspn(fname, "\r\n")] = 0;
        }

        if (fname[0] == 0)
            strcpy(fname, "NIALL.DAT");

        save_dictionary(fname);
    }
    else if (!strncmp(cmd, "#load", 5)) {
        char fname[64] = "";

        if (sscanf(cmd + 5, "%s", fname) != 1) {
            printf("Filename (default NIALL.DAT): ");
            fflush(stdout);
            fgets(fname, 64, stdin);
            fname[strcspn(fname, "\r\n")] = 0;
        }

        if (fname[0] == 0)
            strcpy(fname, "NIALL.DAT");

        load_dictionary(fname);
    }
    /* Debug: #debug command — only compiled in -DDEBUG builds */
#ifdef DEBUG
	else if (!strcmp(cmd, "#debug")) {
		debug_mode = !debug_mode;
		printf("Debug mode %s\n", debug_mode ? "ON" : "OFF");
	}
#endif
    else if (!strcmp(cmd, "#quit")) {
        printf("Goodbye.\n");
        exit(0);
    }
}

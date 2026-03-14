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
   Version 1.14 (current)
   - Phase 3: Option B Shared Link Pool.
     Replaced fixed WordEntry words[] array with parallel arrays:
       word_text[MAX_WORDS+1][MAX_WORD_LEN+1] — word text for each slot
       link_off[MAX_WORDS+1] / link_len[MAX_WORDS+1] — pool index/size
       link_pool[LINK_POOL] — shared 14 KB link string storage
     All words including the start token (index 0) use the same pool;
     no special buffer for index 0. MAX_WORDS increased from 150 to 250.
     START_LINK_PAIRS=50 (start token) / MAX_LINK_PAIRS=25 (all others).
     BSS stays ~27,400 bytes; .COM target remains under 48.5 KB.
     Save/load file format (v3 binary) unchanged.
   - Removed: WordEntry struct, start_word_links[], MAX_LINKS_STR,
     START_LINKS_STR (old fixed-buffer version).
   - Added: pool_alloc(), pool_commit(), init_dict(), LINK_POOL.

   Version 1.13
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

   Version 1.07  (Move to Hi-Tech C & back — multiple sub-revisions a-j)
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

   END_TOKEN is the end-of-sentence sentinel. It occupies index MAX_WORDS
   and is never stored as a real dictionary word; it behaves like the "."
   token in the original AMOS BASIC version.

   Link string format: "total|id(cnt)id(cnt)..."
     total = sum of all transition counts from this word
     id    = dictionary index of the next word
     cnt   = number of times that transition was observed

   Pair size budget (worst case id=300, cnt<=9999 -> "300(9999)"=9 chars,
   header up to "299970|"=7 chars for 30 pairs of max cnt):
     Any word: 7 + 30*9 = 277 — snprintf truncates at 255 gracefully.
     In practice (CP/M chatbot usage) link strings stay well under 256.

   BSS breakdown (approx, int=2 bytes on Z80/SDCC):
     word_text[251][32]    8,032 bytes
     link_off[251]           502 bytes
     link_len[251]           502 bytes
     link_pool[14336]     14,336 bytes
     scratch / other       2,212 bytes
     Total               ~25,584 bytes  — .COM target: < 48,500 bytes
*/

#define MAX_WORDS       250    /* real words: indices 1..249; END_TOKEN at 250 */
#define MAX_SENTENCE     32    /* max words parsed per input sentence           */
#define MAX_INPUT       255    /* max characters per input line                 */
#define MAX_LINK_PAIRS   25    /* max tracked successors per regular word       */
#define START_LINK_PAIRS 50    /* max tracked successors for start token (w[0]) */
#define LINK_POOL     14336    /* shared link string pool (14 KB)               */
#define MAX_WORD_LEN     31    /* max characters in a dictionary word           */

#define END_TOKEN (MAX_WORDS)  /* sentinel index — marks end of sentence        */

/* ------------------ Data Structures ------------------- */
/*
   Parallel arrays replace the old WordEntry struct. Every word including
   the start token (index 0) uses the same shared link_pool. No special-
   casing for index 0 anywhere in the code.

   word_text[i]   — NUL-terminated text for dictionary entry i
   link_off[i]    — byte offset into link_pool for entry i's link string
   link_len[i]    — bytes reserved for entry i in link_pool
   link_pool      — shared storage for all link strings
   pool_hwm       — high-water mark: next free byte in link_pool

   Index 0           = start-of-sentence token (word text is " ")
   Index 1..num_words = real dictionary words
   Index END_TOKEN    = end-of-sentence sentinel (never has real links)
*/

char         word_text[MAX_WORDS + 1][MAX_WORD_LEN + 1]; /* 251x32 =  8,032 bytes */
unsigned int link_off[MAX_WORDS + 1];                     /* 251x2  =    502 bytes */
unsigned int link_len[MAX_WORDS + 1];                     /* 251x2  =    502 bytes */
char         link_pool[LINK_POOL];                        /* shared = 14,336 bytes */
unsigned int pool_hwm;                                    /* high-water mark        */

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

   link_parse_buf / link_rebuild_buf — shared workspace for learn_sentence
   so it never needs a large stack allocation during sentence processing.
   io_lrec enlarged to 256 to match the larger possible link strings.
*/

static unsigned char io_wrec[32];        /* save/load word scratch  */
static unsigned char io_lrec[256];       /* save/load links scratch */

static char link_parse_buf[256];         /* learn_sentence parse    */
static char link_rebuild_buf[256];       /* learn_sentence rebuild  */

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
static void init_dict(void);
static int  pool_alloc(unsigned int idx, unsigned int len);
static int  pool_commit(unsigned int idx, char *buf);

/* --------- Debug Mode (compile with -DDEBUG) --------- */
/*
   The entire debug subsystem is compiled out of the normal binary.
   To enable it, add -DDEBUG to the build command (TRS-80 builds only —
   the larger code does not fit in the NABU's TPA).

   At runtime, toggle output with the #debug command.
*/

#ifdef DEBUG
int debug_mode = 0;                  /* 0 = off, 1 = on — toggled by #debug  */
#define DBG(...) do { if (debug_mode) printf(__VA_ARGS__); } while (0)
#endif

/* Reset each sentence so the message fires once per input line, not
   once per program run. Also reset by #fresh. */
static int dict_full_warned = 0;

/* -------------------- Pool Management ----------------- */
/*
   pool_alloc reserves len bytes for word idx at the current HWM.
   Returns 1 on success, 0 if the pool is full.
   The caller is responsible for writing content to link_pool[link_off[idx]].

   pool_commit writes the NUL-terminated string in buf to word idx's pool
   slot. If the rebuilt string fits in the existing slot it is written
   in-place. Otherwise a new slot is appended at HWM (the old bytes are
   wasted; pool fragmentation is acceptable for CP/M chatbot vocabulary sizes).
   Returns 1 on success, 0 if the pool is full.
*/

static int pool_alloc(unsigned int idx, unsigned int len)
{
    if (pool_hwm + len > (unsigned int)LINK_POOL) return 0; /* pool's chockers — bail out before writing anything */
    link_off[idx] = pool_hwm;   /* record where this word's link string starts in the pool */
    link_len[idx] = len;        /* record how many bytes are reserved for it */
    pool_hwm += len;            /* advance the high-water mark past the newly reserved slot */
    return 1;                   /* caller must now write the actual content at link_pool[link_off[idx]] */
}

static int pool_commit(unsigned int idx, char *buf)
{
    unsigned int need = (unsigned int)strlen(buf) + 1; /* bytes required: string length plus NUL terminator */
    if (need <= link_len[idx]) {                       /* rebuilt string fits within the existing slot */
        memcpy(&link_pool[link_off[idx]], buf, need);  /* overwrite in-place — no pool space wasted */
        return 1;
    }
    if (pool_hwm + need > (unsigned int)LINK_POOL) return 0; /* no room to grow — pool is full */
    link_off[idx] = pool_hwm;            /* redirect this word to a fresh slot at the end of used space */
    link_len[idx] = need;                /* update the reserved size to match the larger string */
    memcpy(&link_pool[pool_hwm], buf, need); /* write the new string at the high-water mark */
    pool_hwm += need;                    /* advance HWM past the new slot; old slot bytes are abandoned */
    return 1;
}

/* -------------------- init_dict() --------------------- */
/*
   Resets the entire dictionary to its initial state.
   Called from main() on startup and from the #fresh command.
   Sets up the start-of-sentence token at index 0 with an initial
   "0|" link string pre-allocated in the pool at offset 0.
*/

static void init_dict(void)
{
    /* num_words=0 makes all slots 1+ inaccessible; each slot is fully
       written (word_text, pool_alloc) before it is ever read, so no
       memset of the full arrays is needed. */
    num_words = 0;   /* no real words yet — all array slots 1+ are invisible to the rest of the code */
    pool_hwm  = 3;   /* first 3 bytes are already spoken for by the start token's "0|" string below */

    /* words[0] = start-of-sentence token */
    word_text[0][0] = ' '; word_text[0][1] = '\0';           /* start token text is a single space, matching AMOS BASIC */
    link_pool[0] = '0'; link_pool[1] = '|'; link_pool[2] = '\0'; /* "0|" means total=0, no transitions recorded yet */
    link_off[0] = 0; link_len[0] = 3;  /* start token owns bytes 0..2 in the pool; HWM starts at 3 */
}

/* ------------------------ Main ------------------------ */
/*
   Initialises the dictionary then enters the main loop.
   Each iteration reads one line of user input (learning from it if it
   is normal text) then generates a reply unless a command was entered.
*/

int main(void)
{
    puts("Welcome to NIALL (CP/M C port by Intangybles) v1.14\nNon Intelligent (AMOS) Language Learner\nOriginal by Matthew Peck (1990)\n\nType #help - for Commands\n");

    init_dict();

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

   Fills the caller's output variables:
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
        if (!strcmp(word_text[i], w))
            return i;
    return 0;
}

/* ----------------- Sentence Learning ------------------ */
/*
   Processes sentence_words[0..sentence_len-1] and records the observed
   word transitions in the dictionary (Markov chain learning).

   For each adjacent pair (old -> f) in the sentence:
     - If f is a new word and the dictionary is not full, add it with an
       initial "0|" pool slot.
     - Parse the current link string for 'old' into ids[]/cnts[] arrays.
     - Increment the count for transition old->f, or append a new pair.
     - Recompute total as the sum of all counts and commit to the pool.
   The final word is linked to END_TOKEN to mark the sentence boundary.

   All words (including start token at index 0) use the shared pool via
   pool_commit; no special-casing for index 0.
*/

void learn_sentence(void)
{
    int old = 0;   /* index of the previous word (0 = start token) */

    /* Static arrays avoid large stack allocations on CP/M's small stack.
       Sized to START_LINK_PAIRS (the larger limit) so both word types fit. */
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
                num_words++;                                                    /* claim the next available dictionary slot */
                strncpy(word_text[num_words], sentence_words[h], MAX_WORD_LEN); /* copy the word text into that slot */
                word_text[num_words][MAX_WORD_LEN] = 0;                         /* guarantee NUL termination at the boundary */

                /* allocate initial "0|" pool slot for this new word */
                if (pool_hwm + 3 > (unsigned int)LINK_POOL) { /* not enough room for even a bare "0|" entry */
                    printf("Link pool full.\n");
                    num_words--;   /* undo the slot claim so the dictionary stays consistent */
                    continue;
                }
                link_pool[pool_hwm]   = '0';   /* write "0|" directly at the high-water mark... */
                link_pool[pool_hwm+1] = '|';   /* ...it means: total=0, no transitions observed yet */
                link_pool[pool_hwm+2] = '\0';  /* NUL-terminate so the string is immediately usable */
                pool_alloc(num_words, 3);       /* register the 3-byte slot and advance HWM past it */
                f = num_words;                  /* this new word is now the transition target (f) */
            }
        }

        /* update link string for 'old' — uniform path for all words including the start token */
        {
            char *wlinks = &link_pool[link_off[old]]; /* point directly at 'old' word's link string in the shared pool */
            int max_pairs = (old == 0) ? START_LINK_PAIRS : MAX_LINK_PAIRS; /* start token gets larger cap */
            int n = 0;

            /* parse existing link string into ids[]/cnts[] arrays */
            strncpy(link_parse_buf, wlinks, 255);  /* work on a copy — parsing will temporarily modify the string */
            link_parse_buf[255] = 0;               /* guarantee NUL termination regardless of actual link string length */

            char *p = strchr(link_parse_buf, '|'); /* locate the '|' that separates the total from the pair list */
            if (!p) {
                /* malformed — reset to a single transition */
                snprintf(link_rebuild_buf, 256, "1|%d(1)", f); /* build a clean single-pair string from scratch */
                pool_commit(old, link_rebuild_buf);             /* write it back, growing the pool slot if needed */
                old = f;
                continue;
            }
            p++;   /* step past '|' to the first id(cnt) pair */

            while (*p && n < max_pairs) { /* parse each id(cnt) pair; stop at the cap or end of string */
                char *lp = strchr(p, '(');     /* find the opening parenthesis for this pair */
                char *rp = strchr(p, ')');     /* find the closing parenthesis */

                if (!lp || !rp || rp < lp)    /* malformed pair structure — abandon parsing here */
                    break;

                *lp = 0;               /* temporarily NUL-terminate the id digits so atoi reads only them */
                ids[n] = atoi(p);      /* parse the target word index */
                *lp = '(';             /* restore the '(' so the buffer looks normal if we need it again */

                cnts[n] = atoi(lp + 1); /* parse the transition count from inside the parentheses */

                if (ids[n] < 0 || ids[n] > MAX_WORDS) /* reject any index outside the valid dictionary range */
                    break;

                if (cnts[n] < 1)
                    cnts[n] = 1;   /* clamp zero or negative counts to 1 — keeps the total invariant intact */

                n++;
                p = rp + 1;   /* advance past ')' ready for the next pair */
            }

            if (n == 0) {
                /* no valid pairs parsed — either a fresh word whose slot holds "0|", or a corrupt string */
                snprintf(link_rebuild_buf, 256, "1|%d(1)", f); /* first observed transition from this word */
                pool_commit(old, link_rebuild_buf);             /* commit — likely grows past the initial 3-byte "0|" slot */
                old = f;
                continue;
            }

            /* increment existing transition or append a new one */
            {
                int found = 0;
                for (int i = 0; i < n; i++) {
                    if (ids[i] == f) {
                        cnts[i]++;   /* we've seen this transition before — bump its weight */
                        found = 1;
                        break;
                    }
                }
                if (!found && n < max_pairs) {
                    ids[n] = f;    /* brand new transition — append it to the array */
                    cnts[n] = 1;   /* starts with a weight of 1 */
                    n++;
                }
                /* if !found && n == max_pairs: at capacity, new transition is silently dropped */
            }

            /* recalculate total as sum of all tracked weights */
            int total = 0;
            for (int i = 0; i < n; i++)
                total += cnts[i];   /* total is always recomputed — never cached — to stay consistent */

            /* rebuild the link string and commit to the pool */
            int pos = 0;
            pos += snprintf(link_rebuild_buf + pos, 256 - pos, "%d|", total); /* write the total field first */
            for (int i = 0; i < n && pos < 255; i++) {
                pos += snprintf(link_rebuild_buf + pos, 256 - pos, "%d(%d)", ids[i], cnts[i]); /* append each pair */
            }
            link_rebuild_buf[255] = 0;   /* hard NUL guard in case snprintf filled the buffer right to the edge */

            pool_commit(old, link_rebuild_buf); /* write back: in-place if it fits, append at HWM if the string grew */
        }
        old = f;   /* advance the chain — 'old' becomes the current word, ready for the next iteration */
    }
}

/* ------------------ Reply Generation ------------------ */
/*
   Generates a reply by walking the Markov chain from the start token.

   Starting at words[0], the function repeatedly:
     - Reads the link string for the current word from the shared pool.
     - Selects the next word using weighted random selection across all
       observed transitions (higher counts = higher probability).
     - Appends the current word to the reply buffer.
     - Moves to the selected next word.
     - Stops when END_TOKEN is reached or the safety limit is hit.

   Output is capitalised, trimmed, and terminated with a full stop.
   The reply buffer is bounded — overflow is caught before it can
   corrupt memory adjacent to the reply array.
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

        /* >= catches END_TOKEN (==MAX_WORDS); > num_words catches old files
           where END_TOKEN was 150 and that ID is now a valid-range index */
        if (c >= MAX_WORDS || c > num_words)
            break;

        char *cstr = &link_pool[link_off[c]]; /* fetch this word's link string directly from the shared pool */

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
			size_t need = strlen(word_text[c]) + 1 /* space */ + 1 /* NUL */;

			if (used + need > sizeof(reply))
				break;

			strcat(reply, word_text[c]);
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
   Version 3 binary file format — unchanged from v1.13.
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
*/

/* ------------------- Debug Helpers -------------------- */

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
   Using stdio FILE* is required on CP/M — raw POSIX write() calls fail
   because BDOS requires 128-byte aligned transfers. The stdio layer
   handles internal buffering and alignment.
*/

static int write_all(FILE *fp, void *buf, unsigned int n)
{
    return (unsigned int)fwrite(buf, 1, n, fp) == n;
}

static int read_all(FILE *fp, void *buf, unsigned int n)
{
    return (unsigned int)fread(buf, 1, n, fp) == n;
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
    const char *lptr;
    const char *wptr;

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

    /* entry 0: start token — wlen=0, links from pool slot 0 */
    {
        lptr = &link_pool[link_off[0]];
        if (!lptr[0]) lptr = "0|";
        llen_short = (unsigned short)strlen(lptr);
        if (llen_short > 255) llen_short = 255;

        wlen_byte = 0;
        fwrite(&wlen_byte, 1, 1, fp);
        checksum_calc += wlen_byte;

        fwrite(&llen_short, sizeof(unsigned short), 1, fp);
        checksum_calc += (unsigned char)(llen_short & 0xFF);
        checksum_calc += (unsigned char)((llen_short >> 8) & 0xFF);
        for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)lptr[j];
        if (llen_short) fwrite(lptr, llen_short, 1, fp);
    }

    /* entries 1..num_words: real dictionary words */
    for (i = 1; i <= num_words; i++) {
        wptr = word_text[i];
        lptr = &link_pool[link_off[i]];

        /* word text */
        memset(io_wrec, 0, 32);
        if (valid_word(wptr)) {
            strncpy((char*)io_wrec, wptr, 31);
            io_wrec[31] = 0;
        }
        wlen_byte = (unsigned char)strlen((char*)io_wrec);
        if (wlen_byte > 31) wlen_byte = 31;

        /* link string */
        memset(io_lrec, 0, 256);
        if (lptr && lptr[0]) {
            strncpy((char*)io_lrec, lptr, 255);
            io_lrec[255] = 0;
        } else {
            strcpy((char*)io_lrec, "0|");
        }
        llen_short = (unsigned short)strlen((char*)io_lrec);
        if (llen_short > 255) llen_short = 255;

        fwrite(&wlen_byte, 1, 1, fp);
        checksum_calc += wlen_byte;
        for (j = 0; j < (int)wlen_byte; j++) checksum_calc += (unsigned char)io_wrec[j];
        if (wlen_byte) fwrite(io_wrec, wlen_byte, 1, fp);

        fwrite(&llen_short, sizeof(unsigned short), 1, fp);
        checksum_calc += (unsigned char)(llen_short & 0xFF);
        checksum_calc += (unsigned char)((llen_short >> 8) & 0xFF);
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

/* Load error helpers — one copy of each string + fclose pattern */
static void load_err(FILE *fp) { fclose(fp); printf("Invalid file.\n"); }
static void load_erp(FILE *fp) { fclose(fp); printf("Pool full.\n"); }

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

    if (!read_all(fp, hdr, 7)) { load_err(fp); return; }

    /* Debug: dump header bytes */
#ifdef DEBUG
    if (debug_mode) dbg_hex_n("[LOAD] hdr7: ", hdr, 7);
#endif

    if (hdr[0]!='N' || hdr[1]!='I' || hdr[2]!='A' || hdr[3]!='L') {
#ifdef DEBUG
        if (debug_mode) printf("[LOAD] bad magic\n");
#endif
        load_err(fp); return;
    }

    ver    = (unsigned int)hdr[4];
    disk_n = (unsigned int)hdr[5] | ((unsigned int)hdr[6] << 8);
    if (disk_n > (unsigned int)(MAX_WORDS - 1)) disk_n = (unsigned int)(MAX_WORDS - 1);

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
        printf("Wrong version.\n");
        return;
    }

    /* reset dictionary before loading */
    init_dict();

    /* checksum begins AFTER the header */
    checksum_calc = 0;

    /* entry 0: start token — word text ignored; links go to pool slot 0 */
    if (fread(&wlen_byte, 1, 1, fp) != 1) { load_err(fp); return; }
    checksum_calc += wlen_byte;
    if (wlen_byte > 31) { load_err(fp); return; }
    if (wlen_byte) {
        memset(io_wrec, 0, 32);
        if (!read_all(fp, io_wrec, wlen_byte)) { load_err(fp); return; }
        for (j = 0; j < (int)wlen_byte; j++) checksum_calc += (unsigned char)io_wrec[j];
    }

    if (fread(&llen_short, sizeof(unsigned short), 1, fp) != 1) { load_err(fp); return; }
    checksum_calc += (unsigned char)(llen_short & 0xFF);
    checksum_calc += (unsigned char)((llen_short >> 8) & 0xFF);
    if (llen_short > 255) llen_short = 255;

    if (llen_short > 0) {
        unsigned int need = (unsigned int)llen_short + 1; /* bytes required: link string plus NUL terminator */
        /* fit in the slot from init_dict if possible, else append at HWM */
        if (need <= link_len[0]) {
            /* start token's existing 3-byte "0|" slot is large enough — read directly into it */
            if (!read_all(fp, &link_pool[link_off[0]], llen_short)) { load_err(fp); return; }
            link_pool[link_off[0] + llen_short] = '\0'; /* NUL-terminate after the bytes we just read */
        } else {
            /* saved start token links are longer than 3 bytes — grow by appending a new slot at HWM */
            if (pool_hwm + need > (unsigned int)LINK_POOL) { load_erp(fp); return; }
            link_off[0] = pool_hwm;   /* redirect entry 0 to the new, larger slot */
            link_len[0] = need;       /* update its reserved size */
            if (!read_all(fp, &link_pool[pool_hwm], llen_short)) { load_err(fp); return; }
            link_pool[pool_hwm + llen_short] = '\0'; /* NUL-terminate the newly read data */
            pool_hwm += need;         /* advance HWM past the new slot so future allocations don't overlap */
        }
        for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)link_pool[link_off[0] + j];
    }
    /* if llen_short == 0 the "0|" from init_dict stays — no action needed */

    if (!link_pool[link_off[0]]) {
        /* if the slot ended up empty for any reason, restore a valid "0|" string */
        link_pool[link_off[0]]   = '0';
        link_pool[link_off[0]+1] = '|';
        link_pool[link_off[0]+2] = '\0';
        link_len[0] = 3;
    }

    /* entries 1..disk_n: real dictionary words */
    for (i = 1; i <= (int)disk_n; i++) {
        if (fread(&wlen_byte, 1, 1, fp) != 1) { load_err(fp); return; }
        checksum_calc += wlen_byte;
        if (wlen_byte > 31) { load_err(fp); return; }
        memset(io_wrec, 0, 32);
        if (wlen_byte) {
            if (!read_all(fp, io_wrec, wlen_byte)) { load_err(fp); return; }
            for (j = 0; j < (int)wlen_byte; j++) checksum_calc += (unsigned char)io_wrec[j];
        }
        io_wrec[31] = 0;

        if (fread(&llen_short, sizeof(unsigned short), 1, fp) != 1) { load_err(fp); return; }
        checksum_calc += (unsigned char)(llen_short & 0xFF);
        checksum_calc += (unsigned char)((llen_short >> 8) & 0xFF);
        if (llen_short > 255) llen_short = 255;
        memset(io_lrec, 0, 256);
        if (llen_short) {
            if (!read_all(fp, io_lrec, llen_short)) { load_err(fp); return; }
            for (j = 0; j < (int)llen_short; j++) checksum_calc += (unsigned char)io_lrec[j];
        }
        io_lrec[255] = 0;

        strncpy(word_text[i], (char*)io_wrec, MAX_WORD_LEN); /* copy the word text into the dictionary slot */
        word_text[i][MAX_WORD_LEN] = 0;                      /* guarantee NUL termination at the boundary */
        if (!valid_word(word_text[i])) word_text[i][0] = 0;  /* blank out any entry that fails the validity check */

        /* allocate pool slot and store link string */
        {
            unsigned int need = llen_short ? (unsigned int)llen_short + 1 : 3; /* NUL-terminated string, or bare "0|" */
            if (!pool_alloc(i, need)) { load_erp(fp); return; } /* reserve space at HWM */
            if (llen_short) {
                memcpy(&link_pool[link_off[i]], io_lrec, llen_short); /* copy link bytes straight into the new slot */
                link_pool[link_off[i] + llen_short] = '\0';            /* NUL-terminate the stored link string */
            } else {
                link_pool[link_off[i]]   = '0';
                link_pool[link_off[i]+1] = '|';
                link_pool[link_off[i]+2] = '\0';
            }
        }

        /* reject structurally invalid link strings on load */
        {
            int t=0, s2=0, p=0;
            if (!validate_links(&link_pool[link_off[i]], &t, &s2, &p)) {
                link_pool[link_off[i]]   = '0';
                link_pool[link_off[i]+1] = '|';
                link_pool[link_off[i]+2] = '\0';
                link_len[i] = 3;
            }
        }

        /* Debug: spot-check first and last records */
#ifdef DEBUG
        if (debug_mode && (i == 1 || i == (int)disk_n))
            printf("[LOAD] record %d word='%s'\n", i, word_text[i]);
#endif
    }

    /* read and verify the trailing checksum */
    if (fread(&checksum_saved, sizeof(unsigned short), 1, fp) != 1) {
        fclose(fp);
        printf("No checksum.\n");
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

/* -------------------- Pager --------------------------- */
/*
   Increments *ln each call. When it reaches PAGE_LINES, prints a prompt
   and waits for a keypress. Returns 1 to continue listing, 0 if the
   user pressed q/Q to quit.
*/
#define PAGE_LINES 22
static int page_pause(int *ln)
{
    int c;
    if (++(*ln) < PAGE_LINES) return 1;
    *ln = 0;
    printf("-- any key / q=quit --");
    fflush(stdout);
    c = getchar();
    printf("\n");
    return (c != 'q' && c != 'Q');
}

/* ------------------ Command Handler ------------------- */
/*
   Handles all commands that begin with '#'. Commands are detected before
   punctuation stripping so filenames reach here intact.

   Supported commands:
     #fresh        — clears the dictionary and resets all state
     #list         — prints all dictionary entries with link validation
     #save [file]  — saves the dictionary to disk in v3 binary format;
                     prompts for a filename if none given (NIALL.DAT)
     #load [file]  — loads a v3 binary dictionary from disk;
                     prompts for a filename if none given (NIALL.DAT)
     #help         — lists the main commands
     #quit         — exits the program
     #debug        — toggles verbose save/load trace output;
                     only compiled in -DDEBUG builds (TRS-80 only)
*/

void handle_command(char *cmd)
{
	if (!strcmp(cmd, "#fresh")) {
		dict_full_warned = 0;
		init_dict();
		printf("Dictionary cleared.\n");
	}
	else if (!strcmp(cmd, "#list")) {
		int ln = 0;
		for (int i = 0; i <= num_words; i++) {
			char *l = &link_pool[link_off[i]];
			char *w = word_text[i][0] ? word_text[i] : (char*)" ";
			int total = 0, sum = 0, pairs = 0;
			if (!l[0]) l = (char*)"0|";
			int ok = validate_links(l, &total, &sum, &pairs);
			if (ok) printf("%d %s %s  [OK pairs=%d]\n", i, w, l, pairs);
			else    printf("%d %s %s  [BAD]\n", i, w, l);
			if (!page_pause(&ln)) break;
		}
	}
	else if (!strcmp(cmd, "#help")) {
        puts("Commands:\n  #fresh  - Clear dictionary\n  #list   - List dictionary\n  #save   - Save dictionary to disk\n  #load   - Load dictionary from disk\n  #quit   - Exit");
    }
    else if (!strncmp(cmd, "#save", 5)) {
        char fname[64] = "";

        if (sscanf(cmd + 5, "%s", fname) != 1) {
            printf("Filename (NIALL.DAT): ");
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
            printf("Filename (NIALL.DAT): ");
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

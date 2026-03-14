/* niall.c - NIALL - Non Intelligent (AMOS) Language Learner (Markov-chain chatbot), CP/M C port of AMOS BASIC
   Original AMOS BASIC by Matthew Peck (1990)
   C port by Intangybles (Adventures in returning to 'C')

   Compile CP/M (production — NABU, TRS-80 Model 4P, Kaypro):
     zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niall.c -o NIALL

   Compile CP/M (debug — enables #debug command):
     zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size -DDEBUG niall.c -o NIALL

   Compile NABU native (homebrew — saves to IA file store):
     zcc +nabu -vn -create-app -compiler=sdcc --opt-code-size niall.c -o NIALLN

   Save file verifier:
     zcc +cpm -vn -create-app -compiler=sdcc --opt-code-size niallchk.c -o NIALLCHK

   Target platforms : CP/M 2.2   — NABU, TRS-80 Model 4P, Kaypro
                      NABU native — homebrew .nabu segment, saves to IA file store
   Compiler        : z88dk with SDCC backend (Z80, 64 KB address space)

   -----------------------------------------------------------------------
   Version 1.2 (current)
   - Phase 5: NABU native build support.
     Same binary save/load format (v4) — dictionaries are portable between
     CP/M and NABU native builds.
     Added NABU_XXXX() platform macro block: all platform-specific I/O
     (print, input, file) is abstracted into a single block near the top
     of the file, keeping the algorithm code clean.
     NABU native uses z88dk +nabu target with NABULIB VDP text output and
     RetroNET-FileStore IA file I/O.
     Compile: zcc +nabu -vn -create-app -compiler=sdcc --opt-code-size niall.c -o NIALLN
     Dictionary default path on NABU IA store: NIALL.DAT
   - Version bump: v1.15 -> v1.2.

   Version 1.15
     Replaced text link strings and fixed word_text slots with binary data:
       wtext_pool[WTEXT_POOL]  — packed NUL-terminated word strings (7.5 KB)
       wtext_off[MAX_WORDS+1]  — byte offset of each word in wtext_pool
       blink_pool[BLINK_POOL]  — packed binary link records (17 KB)
       blink_off[MAX_WORDS+1]  — byte offset of each word's link record
     Binary link record format (3 + n*4 bytes):
       bytes 0-1   : unsigned short total (LE) — sum of all counts
       byte  2     : unsigned char  n    — number of successor pairs
       bytes 3+k*4 : unsigned short id[k]  (LE) — target word index
       bytes 5+k*4 : unsigned short cnt[k] (LE) — transition count
     Count increments are always in-place (no pool growth).
     New pair additions copy old record + append at HWM (old abandoned).
     MAX_WORDS increased from 250 to 1000.
     Save/load file format: v4 binary (not compatible with v3).
     Use NIALLCON to migrate v3 save files to v4.
   - Removed: word_text[][], link_off[], link_len[], link_pool[], pool_hwm,
     link_parse_buf[], link_rebuild_buf[], pool_alloc(), pool_commit(),
     validate_links(), io_lrec[].
   - Added: wtext_pool[], wtext_off[], blink_pool[], blink_off[],
     wtext_hwm, blink_hwm, rd_u16(), wr_u16(), WTEXT(), BLNK_SZ(),
     wtext_add(), link_update().
   - Tuning (NABU TPA budget — every BSS byte = 1 .COM byte):
     START_PAIRS 50->100: doubles start-token sentence variety (zero .COM cost).
     BLINK_POOL 14336->17408: more link transitions before pool-full.
     MAX_INPUT 255->159: 2 lines on 80-col display; saves 96 bytes.
     MAX_SENTENCE 32->30: saves 64 bytes BSS.
     reply_buf 256->160: matches input width; saves 96 bytes.
   - Fixed: new-word pool pre-check now reserves BLNK_SZ(0)+BLNK_SZ(1)=10 bytes
     (was only BLNK_SZ(0)=3); prevents words being added with no links when the
     pool is nearly full.
   - New companion tools: NIALLCON (v3->v4), NIALLASC (AMOS BBS ASCII->v4).
   - NIALL.COM: 47,809 bytes; ~691 bytes headroom on NABU CP/M.

   Version 1.14
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

/* ------------------- Platform Abstraction (NABU_XXXX) ----------------- */
/*
   All platform-specific I/O is concentrated here. The rest of the code
   uses NABU_XXXX() macros throughout and never calls stdio or NABU-LIB
   functions directly.

   CP/M build  (+cpm)  — standard stdio, no extra includes.
   NABU native (+nabu) — VDP text output via NABULIB, keyboard via readLine(),
                         file I/O via RetroNET-FileStore.
*/

#ifdef __NABU__

#include <stdarg.h>
#define BIN_TYPE BIN_HOMEBREW
#define FONT_LM80C
#include "../NABULIB/NABU-LIB.h"
#include "../NABULIB/RetroNET-FileStore.h"
#include "../NABULIB/patterns.h"

/* Format buffer used by nabu_printf — only present in NABU build. */
static char nabu_pb[160];

/* nabu_puts: print a NUL-terminated string, converting '\n' to vdp_newLine(). */
static void nabu_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') { vdp_newLine(); s++; }
        else            { vdp_write((uint8_t)*s++); }
    }
}

/* nabu_printf: vsprintf into nabu_pb then output via nabu_puts. */
static void nabu_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(nabu_pb, fmt, ap);
    va_end(ap);
    nabu_puts(nabu_pb);
}

/* nabu_getline: read one line from keyboard, null-terminate.
   Advances the VDP cursor to the next line after the user presses Enter. */
static void nabu_getline(char *buf, int maxlen)
{
    uint8_t len = readLine((uint8_t*)buf, (uint8_t)(maxlen - 1));
    buf[len] = '\0';
    vdp_newLine();
}

#ifdef VDP_80COL
#define NABU_INIT()         do { initNABULib();                                \
                                 vdp_initTextMode80(0x0F, 0x01, true);         \
                                 vdp_loadASCIIFont(ASCII); } while (0)
#else
#define NABU_INIT()         do { initNABULib();                                \
                                 vdp_initTextMode(0x0F, 0x01, true);           \
                                 vdp_loadASCIIFont(ASCII); } while (0)
#endif
#define NABU_PRINTF(...)    nabu_printf(__VA_ARGS__)
#define NABU_FLUSH()        /* no-op — VDP output is immediate */
#define NABU_GETCHAR()      ((int)getChar())
#define NABU_GETLINE(b,n)   nabu_getline(b, n)
#define NABU_DEFAULT_FILE   "NIALL.DAT"
#define NIALL_PLATFORM      "NABU Native"

#else /* CP/M */

#define NABU_INIT()         /* no-op */
#define NABU_PRINTF(...)    printf(__VA_ARGS__)
#define NABU_FLUSH()        fflush(stdout)
#define NABU_GETCHAR()      getchar()
#define NABU_GETLINE(b,n)   fgets(b, n, stdin)
#define NABU_DEFAULT_FILE   "NIALL.DAT"
#define NIALL_PLATFORM      "CP/M C port"

#endif /* __NABU__ */

#define NIALL_VERSION       "1.2"
#ifdef __NABU__
#define NIALL_STORAGE       "IA"
#else
#define NIALL_STORAGE       "disk"
#endif

/* Title line separator: 40-col NABU gets a newline so the platform string
   wraps to its own line; 80-col and CP/M keep it on one line with a space. */
#if defined(__NABU__) && !defined(VDP_80COL)
#define NIALL_TITLE_SEP     "\n"
#else
#define NIALL_TITLE_SEP     " "
#endif

/* ------------------- Configuration -------------------- */
/*
   Memory parameters — every byte of BSS adds one byte to the .COM file
   (z88dk includes BSS as zeros in the CP/M flat binary). Values below
   are tuned so the total .COM fits in the NABU's TPA (~48.5 KB limit).

   Binary link record format: 3 + n*4 bytes
     bytes 0-1   : unsigned short total (LE) — sum of all transition counts
     byte  2     : unsigned char  n     — number of id/cnt pairs (0..START_PAIRS)
     bytes 3+k*4 : unsigned short id[k] (LE) — target word index
     bytes 5+k*4 : unsigned short cnt[k](LE) — transition count

   Count increments are in-place (record size unchanged).
   New pair additions grow the record: old record abandoned, new record
   written at HWM. Pool fragmentation is acceptable — same strategy as v1.14.

   BSS breakdown (int = 2 bytes on Z80/SDCC):
     wtext_pool[7680]        7,680 bytes
     wtext_off[1001]         2,002 bytes
     blink_pool[17408]      17,408 bytes
     blink_off[1001]         2,002 bytes
     sentence_words[30][32]    960 bytes
     input_buf[160]            160 bytes
     reply_buf[160]            160 bytes
     scratch / other          ~100 bytes
     Total                ~30,472 bytes  — .COM: 47,809 bytes; headroom ~691 bytes
*/

#define MAX_WORDS      1000    /* real words: indices 1..999; END_TOKEN at 1000 */
#define MAX_SENTENCE     30    /* max words parsed per input sentence           */
#define MAX_INPUT       159    /* max characters per input line (2x80 col)      */
#define MAX_PAIRS        25    /* max tracked successors per regular word       */
#define START_PAIRS     100    /* max tracked successors for start token (w[0]) */
#define WTEXT_POOL     7680    /* 7.5 KB word text string pool                  */
#define BLINK_POOL    17408    /* 17 KB binary link record pool                 */
#define MAX_WORD_LEN     31    /* max characters in a dictionary word           */

#define END_TOKEN  (MAX_WORDS) /* sentinel index — marks end of sentence        */

/* Binary link record helpers */
#define BLNK_HDR   3           /* header bytes (total LE16 + n byte)            */
#define BLNK_PAIR  4           /* bytes per pair (id LE16 + cnt LE16)           */
#define BLNK_SZ(n) (3 + (n)*4) /* total record size for n pairs                 */

/* ------------------ Data Structures ------------------- */
/*
   Word text pool — packed NUL-terminated strings replace word_text[][].
   Binary link record pool — packed binary records replace text link_pool[].

   Index 0           = start-of-sentence token (text is " ")
   Index 1..num_words = real dictionary words
   Index END_TOKEN    = end-of-sentence sentinel (no record stored)

   wtext_off[i] = byte offset of word i's NUL-terminated text in wtext_pool
   blink_off[i] = byte offset of word i's binary link record in blink_pool
   WTEXT(i)     = pointer to word i's text string
*/

char         wtext_pool[WTEXT_POOL];       /* 5,120 bytes — packed word strings */
unsigned int wtext_off[MAX_WORDS + 1];     /* 1,002 bytes — text offset per word*/
unsigned int wtext_hwm;                    /* high-water mark in wtext_pool     */

char         blink_pool[BLINK_POOL];       /* 17,408 bytes — packed link records*/
unsigned int blink_off[MAX_WORDS + 1];     /* 1,002 bytes — record offset/word  */
unsigned int blink_hwm;                    /* high-water mark in blink_pool     */

int num_words;

/* Input and sentence buffers */
char input_buf[MAX_INPUT + 1];
char sentence_words[MAX_SENTENCE][32];
int  sentence_len;

/* I/O scratch and reply buffers (global to avoid CP/M stack pressure) */
static unsigned char io_wrec[32];
static char          reply_buf[160];

/* Command flag — set when a # command is processed */
int comm;

/* Reset per sentence so the dict-full message fires once per input line */
static int dict_full_warned;

/* Set by page_pause when user presses n/N — cleared at start of each #list */
static int pg_nonstop;

/* Word text access macro */
#define WTEXT(i) (&wtext_pool[wtext_off[i]])

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
static void init_dict(void);
static int  wtext_add(unsigned int idx, const char *w);
static void link_update(unsigned int old, unsigned int f);
static unsigned int rd_u16(const char *p);
static void wr_u16(char *p, unsigned int v);

/* --------- Debug Mode (compile with -DDEBUG) --------- */
/*
   The entire debug subsystem is compiled out of the normal binary.
   To enable it, add -DDEBUG to the build command (TRS-80 builds only —
   the larger code does not fit in the NABU's TPA).

   At runtime, toggle output with the #debug command.
*/

#ifdef DEBUG
int debug_mode = 0;
#define DBG(...) do { if (debug_mode) printf(__VA_ARGS__); } while (0)
#endif

/* --------------------- Binary Helpers ----------------- */
/*
   rd_u16 reads a little-endian unsigned short from two adjacent bytes.
   wr_u16 writes a little-endian unsigned short to two adjacent bytes.
   Used throughout the binary link record operations.
*/

static unsigned int rd_u16(const char *p)
{
    return (unsigned int)(unsigned char)p[0] |
           ((unsigned int)(unsigned char)p[1] << 8);
}

static void wr_u16(char *p, unsigned int v)
{
    p[0] = (char)(v & 0xFF);
    p[1] = (char)((v >> 8) & 0xFF);
}

/* -------------------- Word Text Pool ------------------ */
/*
   wtext_add stores a NUL-terminated word string in wtext_pool at the
   current HWM and records its offset in wtext_off[idx].
   Returns 1 on success, 0 if the pool is full.
*/

static int wtext_add(unsigned int idx, const char *w)
{
    unsigned int len = (unsigned int)strlen(w) + 1;
    if (wtext_hwm + len > (unsigned int)WTEXT_POOL) return 0;
    strcpy(&wtext_pool[wtext_hwm], w);
    wtext_off[idx] = wtext_hwm;
    wtext_hwm += len;
    return 1;
}

/* -------------------- init_dict() --------------------- */
/*
   Resets the entire dictionary to its initial state.
   Called from main() on startup and from the #fresh command.
   Sets up the start-of-sentence token at index 0.
*/

static void init_dict(void)
{
    num_words = 0;

    /* word 0 = start-of-sentence token (single space, matching AMOS BASIC) */
    wtext_pool[0] = ' '; wtext_pool[1] = '\0';
    wtext_off[0] = 0;
    wtext_hwm = 2;

    /* word 0 link record: total=0, n=0 (3 bytes) */
    wr_u16(&blink_pool[0], 0);
    blink_pool[2] = 0;
    blink_off[0] = 0;
    blink_hwm = (unsigned int)BLNK_SZ(0);  /* = 3 */
}

/* -------------------- link_update() ------------------- */
/*
   Updates the binary link record for word 'old': adds or increments the
   transition old -> f.

   If transition f already exists: increment count in-place (no pool growth).
   If new: grow the record by appending a larger copy at HWM (old abandoned).
   If at capacity (MAX_PAIRS / START_PAIRS): silently drop the transition.
*/

static void link_update(unsigned int old, unsigned int f)
{
    char *rec = &blink_pool[blink_off[old]];
    unsigned int total = rd_u16(rec);
    unsigned int n     = (unsigned int)(unsigned char)rec[2];
    unsigned int max_p = (old == 0) ? (unsigned int)START_PAIRS : (unsigned int)MAX_PAIRS;
    unsigned int k, new_sz, old_sz;

    /* search for existing pair — increment in-place if found */
    for (k = 0; k < n; k++) {
        if (rd_u16(rec + BLNK_HDR + k * BLNK_PAIR) == f) {
            unsigned int cnt = rd_u16(rec + BLNK_HDR + k * BLNK_PAIR + 2);
            wr_u16(rec + BLNK_HDR + k * BLNK_PAIR + 2, cnt + 1);
            wr_u16(rec, total + 1);
            return;
        }
    }

    /* not found — drop silently if at capacity */
    if (n >= max_p) return;

    /* append new record at HWM with n+1 pairs */
    new_sz = (unsigned int)BLNK_SZ(n + 1);
    if (blink_hwm + new_sz > (unsigned int)BLINK_POOL) {
        NABU_PRINTF("Link pool full.\n");
        return;
    }
    old_sz = (unsigned int)BLNK_SZ(n);
    memcpy(&blink_pool[blink_hwm], rec, old_sz);           /* copy existing  */
    wr_u16(&blink_pool[blink_hwm], total + 1);             /* new total      */
    blink_pool[blink_hwm + 2] = (char)(n + 1);            /* new pair count */
    wr_u16(&blink_pool[blink_hwm + BLNK_HDR + n * BLNK_PAIR],     f); /* id */
    wr_u16(&blink_pool[blink_hwm + BLNK_HDR + n * BLNK_PAIR + 2], 1); /* cnt=1 */
    blink_off[old] = blink_hwm;
    blink_hwm += new_sz;
}

/* ------------------------ Main ------------------------ */
/*
   Initialises the dictionary then enters the main loop.
   Each iteration reads one line of user input (learning from it if it
   is normal text) then generates a reply unless a command was entered.
*/

int main(void)
{
    NABU_INIT();
    NABU_PRINTF("Welcome to NIALL" NIALL_TITLE_SEP "(" NIALL_PLATFORM " by Intangybles) v" NIALL_VERSION "\nNon Intelligent (AMOS) Language Learner\nby Matthew Peck (1990)\n\nTry - #help\n\n");

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
    NABU_PRINTF("\nUSER: ");
    NABU_FLUSH();

    NABU_GETLINE(input_buf, MAX_INPUT);
    if (input_buf[0] == '\n')
        return;

    /* convert to lowercase */
    {
        int i;
        for (i = 0; input_buf[i]; i++)
            input_buf[i] = tolower((unsigned char)input_buf[i]);
    }

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
    {
        char *p = input_buf;
        while (*p) {
            sentence_len = 0;

            /* find end of this sentence */
            char *end = strchr(p, '.');
            if (end) *end = 0;

            /* split sentence into words */
            {
                char *w = strtok(p, " ");
                while (w && sentence_len < MAX_SENTENCE) {
                    strncpy(sentence_words[sentence_len], w, MAX_WORD_LEN);
                    sentence_words[sentence_len][MAX_WORD_LEN] = 0;
                    sentence_len++;
                    w = strtok(NULL, " ");
                }
            }

            if (sentence_len > 0)
                learn_sentence();

            if (!end) break;
            p = end + 1;
        }
    }
}

/* ------------------ Valid Word Check ------------------ */
/*
   Returns 1 if the word is acceptable for the dictionary, 0 otherwise.
   A valid word must be non-empty and contain only letters or digits.
*/

int valid_word(const char *w)
{
    if (!w || !*w) return 0;
    while (*w) {
        if (!isalnum((unsigned char)*w)) return 0;
        w++;
    }
    return 1;
}

/* --------------- Punctuation Stripping ---------------- */
/*
   Removes punctuation from the input string in-place, matching the
   behaviour of the original AMOS BASIC version.
   Commas are converted to full stops (sentence separators).
*/

void strip_punctuation(char *s)
{
    char clean[MAX_INPUT + 1];
    int j = 0;
    int i;

    for (i = 0; s[i]; i++) {
        char c = s[i];
        if (c == ',') c = '.';
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == ' ' || c == '.')
            clean[j++] = c;
    }
    clean[j] = 0;
    strcpy(s, clean);
}

/* ----------------- Dictionary Lookup ------------------ */
/*
   Searches the dictionary for an exact word match.
   Returns the index (1..num_words) if found, or 0 if not found.
*/

int find_word(const char *w)
{
    int i;
    for (i = 1; i <= num_words; i++)
        if (!strcmp(WTEXT(i), w))
            return i;
    return 0;
}

/* ----------------- Sentence Learning ------------------ */
/*
   Processes sentence_words[0..sentence_len-1] and records observed
   word transitions in the dictionary (Markov chain learning).

   For each adjacent pair (old -> f) in the sentence:
     - If f is a new word and space is available, add it to both pools.
     - Call link_update(old, f) to record the transition in binary.
   The final word is linked to END_TOKEN to mark the sentence boundary.
*/

void learn_sentence(void)
{
    int old = 0;   /* index of the previous word (0 = start token) */
    int h;

    /* reset per sentence so the dict-full message fires once per input line */
    dict_full_warned = 0;

    for (h = 0; h <= sentence_len; h++) {
        int f;

        /* final iteration: link the last valid word to END_TOKEN */
        if (h == sentence_len) {
            if (old == 0) break;   /* no valid words processed — skip */
            f = END_TOKEN;
        } else {
            /* skip invalid words and reset the chain back to start token */
            if (!valid_word(sentence_words[h])) {
                old = 0;
                continue;
            }

            f = find_word(sentence_words[h]);

            if (!f) {
                /* new word — pre-check both pools before committing */
                unsigned int wlen = (unsigned int)strlen(sentence_words[h]) + 1;

                if (num_words >= MAX_WORDS - 1) {
                    if (!dict_full_warned) {
                        NABU_PRINTF("Dictionary full (%d words).\n", MAX_WORDS - 1);
                        dict_full_warned = 1;
                    }
                    continue;
                }
                if (wtext_hwm + wlen > (unsigned int)WTEXT_POOL) {
                    NABU_PRINTF("Word pool full.\n");
                    continue;
                }
                if (blink_hwm + (unsigned int)(BLNK_SZ(0) + BLNK_SZ(1)) > (unsigned int)BLINK_POOL) {
                    NABU_PRINTF("Link pool full.\n");
                    continue;
                }

                /* commit both at once — no partial failure possible now */
                num_words++;
                strcpy(&wtext_pool[wtext_hwm], sentence_words[h]);
                wtext_off[num_words] = wtext_hwm;
                wtext_hwm += wlen;

                wr_u16(&blink_pool[blink_hwm], 0);  /* total = 0 */
                blink_pool[blink_hwm + 2] = 0;       /* n = 0     */
                blink_off[num_words] = blink_hwm;
                blink_hwm += (unsigned int)BLNK_SZ(0);

                f = num_words;
            }
        }

        /* record transition old -> f in binary link record */
        link_update((unsigned int)old, (unsigned int)f);
        old = f;
    }
}

/* ------------------ Reply Generation ------------------ */
/*
   Generates a reply by walking the Markov chain from the start token.

   Starting at word 0, the function repeatedly:
     - Reads the binary link record for the current word.
     - Selects the next word using weighted random selection.
     - Appends the current word to the reply buffer.
     - Moves to the selected next word.
     - Stops when END_TOKEN is reached or the safety limit is hit.

   Output is capitalised, trimmed, and terminated with a full stop.
   A safety counter of 100 iterations prevents infinite chain loops.
*/

void generate_reply(void)
{
    int c, safety, k;
    unsigned int total, n, r, sum;
    char *rec;

    if (!num_words) {
        NABU_PRINTF("NIALL: I cannot speak yet!\n");
        return;
    }

    c = 0;
    reply_buf[0] = 0;
    safety = 0;

    while (safety < 100) {
        safety++;

        /* >= catches END_TOKEN (==MAX_WORDS); > num_words catches old files */
        if (c >= MAX_WORDS || c > num_words)
            break;

        rec   = &blink_pool[blink_off[c]];
        total = rd_u16(rec);
        n     = (unsigned int)(unsigned char)rec[2];

        if (!total || !n)
            break;

        r = ((unsigned int)(rand() & 0x7fff) % total) + 1;
        sum = 0;

        {
            int found = 0;
            unsigned int next = 0;

            for (k = 0; k < (int)n; k++) {
                sum += rd_u16(rec + BLNK_HDR + k * BLNK_PAIR + 2);
                if (r <= sum) {
                    next = rd_u16(rec + BLNK_HDR + k * BLNK_PAIR);
                    found = 1;
                    break;
                }
            }

            if (!found) break;

            /* append current word to reply — bounded to prevent overflow */
            {
                size_t used = strlen(reply_buf);
                size_t need = strlen(WTEXT(c)) + 1 + 1;  /* word + space + NUL */
                if (used + need > sizeof(reply_buf)) break;
                strcat(reply_buf, WTEXT(c));
                strcat(reply_buf, " ");
            }

            c = (int)next;
        }

        if (c == END_TOKEN) break;
    }

    /* trim leading spaces (start token appends " " as first word) */
    {
        int i = 0;
        while (reply_buf[i] == ' ') i++;
        if (i) memmove(reply_buf, reply_buf + i, strlen(reply_buf) - i + 1);
    }

    /* trim trailing space */
    {
        int len = (int)strlen(reply_buf);
        while (len > 0 && reply_buf[len - 1] == ' ') reply_buf[--len] = 0;
    }

    if (reply_buf[0]) {
        /* capitalise first letter */
        if (reply_buf[0] >= 'a' && reply_buf[0] <= 'z')
            reply_buf[0] = reply_buf[0] - 'a' + 'A';

        /* append full stop — bounded */
        {
            size_t used = strlen(reply_buf);
            if (used + 2 <= sizeof(reply_buf))
                strcat(reply_buf, ".");
        }

        NABU_PRINTF("NIALL: %s\n", reply_buf);
    }
}

/* ----------------- Binary Save / Load ----------------- */
/*
   Version 4 binary file format.
   All multi-byte integers are little-endian (LE). The checksum covers
   only the payload bytes, NOT the 7-byte header.

   Header (7 bytes):
     4 bytes  magic    "NIAL"
     1 byte   version  4
     2 bytes  num_words (LE unsigned short)

   Payload — one record per entry, entries 0..num_words:
     1 byte     wlen  word length (0..31); entry 0 always 0
     wlen bytes word text, no trailing NUL stored
     3+n*4 bytes binary link record (n at offset 2, self-describing size)

   Footer (2 bytes):
     unsigned short checksum (LE) — running byte sum of all payload bytes

   Note: v1.15 does NOT load v3 files. Use NIALLCON to convert v3 -> v4.
*/

/* ------------------- Debug Helpers -------------------- */

#ifdef DEBUG
static void dbg_hex_n(const char *tag, const unsigned char *p, unsigned int n)
{
    unsigned int i;
    printf("%s", tag);
    for (i = 0; i < n; i++) {
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
   because BDOS requires 128-byte aligned transfers.
*/

static int write_all(FILE *fp, void *buf, unsigned int n)
{
    return (unsigned int)fwrite(buf, 1, n, fp) == n;
}

static int read_all(FILE *fp, void *buf, unsigned int n)
{
    return (unsigned int)fread(buf, 1, n, fp) == n;
}

/* ---------------- Save Dictionary (v4) ---------------- */

void save_dictionary(const char *fname)
{
    unsigned char hdr[7];
    unsigned short checksum_calc;
    int i;
    unsigned char wlen_byte;
    char *rec;
    unsigned int n, rec_sz;

    /* build the 7-byte header — same for both platforms */
    hdr[0]='N'; hdr[1]='I'; hdr[2]='A'; hdr[3]='L'; hdr[4]=4;
    hdr[5] = (unsigned char)(num_words & 0xFF);
    hdr[6] = (unsigned char)(num_words >> 8);
    checksum_calc = 0;

#ifdef __NABU__
    {
        uint8_t handle;
        uint8_t flen = (uint8_t)strlen(fname);

        handle = rn_fileOpen(flen, (uint8_t*)fname, OPEN_FILE_FLAG_READWRITE, 0xFF);
        if (handle == 0xFF) { NABU_PRINTF("File error.\n"); return; }
        rn_fileHandleEmptyFile(handle);

        rn_fileHandleAppend(handle, 0, 7, hdr);

        for (i = 0; i <= num_words; i++) {
            unsigned int j;

            if (i == 0) {
                wlen_byte = 0;
            } else {
                wlen_byte = (unsigned char)strlen(WTEXT(i));
                if (wlen_byte > MAX_WORD_LEN) wlen_byte = MAX_WORD_LEN;
            }
            rn_fileHandleAppend(handle, 0, 1, &wlen_byte);
            checksum_calc += wlen_byte;

            if (wlen_byte) {
                rn_fileHandleAppend(handle, 0, wlen_byte, (uint8_t*)WTEXT(i));
                for (j = 0; j < (unsigned int)wlen_byte; j++)
                    checksum_calc += (unsigned char)WTEXT(i)[j];
            }

            rec    = &blink_pool[blink_off[i]];
            n      = (unsigned int)(unsigned char)rec[2];
            rec_sz = (unsigned int)BLNK_SZ(n);
            rn_fileHandleAppend(handle, 0, (uint16_t)rec_sz, (uint8_t*)rec);
            for (j = 0; j < rec_sz; j++)
                checksum_calc += (unsigned char)rec[j];
        }

        rn_fileHandleAppend(handle, 0, 2, (uint8_t*)&checksum_calc);
        rn_fileHandleClose(handle);
        NABU_PRINTF("Dictionary saved.\n");
    }
#else
    {
        FILE *fp;

#ifdef DEBUG
        if (debug_mode)
            printf("\n[SAVE] file='%s' num_words=%d\n", fname, num_words);
#endif
        remove(fname);
        fp = fopen(fname, "wb");
        if (!fp) { NABU_PRINTF("File error.\n"); return; }
        fseek(fp, 0L, SEEK_SET);

#ifdef DEBUG
        if (debug_mode) dbg_hex_n("[SAVE] hdr7: ", hdr, 7);
#endif

        if (!write_all(fp, hdr, 7)) { fclose(fp); NABU_PRINTF("Save failed.\n"); return; }

        for (i = 0; i <= num_words; i++) {
            unsigned int j;

            if (i == 0) {
                wlen_byte = 0;
            } else {
                wlen_byte = (unsigned char)strlen(WTEXT(i));
                if (wlen_byte > MAX_WORD_LEN) wlen_byte = MAX_WORD_LEN;
            }
            fwrite(&wlen_byte, 1, 1, fp);
            checksum_calc += wlen_byte;

            if (wlen_byte) {
                fwrite(WTEXT(i), wlen_byte, 1, fp);
                for (j = 0; j < (unsigned int)wlen_byte; j++)
                    checksum_calc += (unsigned char)WTEXT(i)[j];
            }

            rec    = &blink_pool[blink_off[i]];
            n      = (unsigned int)(unsigned char)rec[2];
            rec_sz = (unsigned int)BLNK_SZ(n);
            fwrite(rec, rec_sz, 1, fp);
            for (j = 0; j < rec_sz; j++)
                checksum_calc += (unsigned char)rec[j];
        }

        fwrite(&checksum_calc, sizeof(unsigned short), 1, fp);
        fclose(fp);
        NABU_PRINTF("Dictionary saved.\n");

#ifdef DEBUG
        if (debug_mode) printf("[SAVE] done (checksum=%u).\n", (unsigned int)checksum_calc);
#endif
    }
#endif /* __NABU__ */
}

/* ---------------- Load Dictionary (v4) ---------------- */

void load_dictionary(const char *fname)
{
    unsigned char hdr[7];
    unsigned int disk_n, ver, i;
    unsigned short checksum_calc, checksum_saved;
    unsigned char wlen_byte;
    unsigned char rec_hdr[3];
    unsigned int nrec, rec_sz;

#ifdef __NABU__
    {
        uint8_t handle;
        uint8_t flen = (uint8_t)strlen(fname);

        handle = rn_fileOpen(flen, (uint8_t*)fname, OPEN_FILE_FLAG_READONLY, 0xFF);
        if (handle == 0xFF) { NABU_PRINTF("Cannot load file.\n"); return; }

        if (rn_fileHandleReadSeq(handle, hdr, 0, 7) != 7) {
            rn_fileHandleClose(handle); NABU_PRINTF("Invalid file.\n"); return;
        }
        if (hdr[0]!='N' || hdr[1]!='I' || hdr[2]!='A' || hdr[3]!='L') {
            rn_fileHandleClose(handle); NABU_PRINTF("Invalid file.\n"); return;
        }
        ver    = (unsigned int)hdr[4];
        disk_n = (unsigned int)hdr[5] | ((unsigned int)hdr[6] << 8);
        if (ver != 4) {
            rn_fileHandleClose(handle); NABU_PRINTF("Wrong version.\n"); return;
        }
        if (disk_n > (unsigned int)(MAX_WORDS - 1))
            disk_n = (unsigned int)(MAX_WORDS - 1);

        init_dict();
        checksum_calc = 0;

        for (i = 0; i <= disk_n; i++) {
            int j;

            if (rn_fileHandleReadSeq(handle, &wlen_byte, 0, 1) != 1) {
                rn_fileHandleClose(handle); NABU_PRINTF("Invalid file.\n"); return;
            }
            checksum_calc += wlen_byte;
            if (wlen_byte > MAX_WORD_LEN) {
                rn_fileHandleClose(handle); NABU_PRINTF("Invalid file.\n"); return;
            }

            if (i == 0) {
                if (wlen_byte) {
                    memset(io_wrec, 0, 32);
                    if (rn_fileHandleReadSeq(handle, io_wrec, 0, wlen_byte) != wlen_byte) {
                        rn_fileHandleClose(handle); NABU_PRINTF("Invalid file.\n"); return;
                    }
                    for (j = 0; j < (int)wlen_byte; j++)
                        checksum_calc += io_wrec[j];
                }
            } else {
                memset(io_wrec, 0, 32);
                if (wlen_byte) {
                    if (rn_fileHandleReadSeq(handle, io_wrec, 0, wlen_byte) != wlen_byte) {
                        rn_fileHandleClose(handle); NABU_PRINTF("Invalid file.\n"); return;
                    }
                    for (j = 0; j < (int)wlen_byte; j++)
                        checksum_calc += io_wrec[j];
                }
                io_wrec[wlen_byte] = 0;
                if (!valid_word((char*)io_wrec)) io_wrec[0] = 0;
                if (!wtext_add(i, (char*)io_wrec)) {
                    rn_fileHandleClose(handle); NABU_PRINTF("Pool full.\n"); return;
                }
            }

            if (rn_fileHandleReadSeq(handle, rec_hdr, 0, 3) != 3) {
                rn_fileHandleClose(handle); NABU_PRINTF("Invalid file.\n"); return;
            }
            checksum_calc += rec_hdr[0];
            checksum_calc += rec_hdr[1];
            checksum_calc += rec_hdr[2];

            nrec   = (unsigned int)rec_hdr[2];
            rec_sz = (unsigned int)BLNK_SZ(nrec);
            if (blink_hwm + rec_sz > (unsigned int)BLINK_POOL) {
                rn_fileHandleClose(handle); NABU_PRINTF("Pool full.\n"); return;
            }
            blink_pool[blink_hwm + 0] = (char)rec_hdr[0];
            blink_pool[blink_hwm + 1] = (char)rec_hdr[1];
            blink_pool[blink_hwm + 2] = (char)rec_hdr[2];
            if (nrec > 0) {
                unsigned int pairs_sz = nrec * (unsigned int)BLNK_PAIR;
                unsigned int k;
                if (rn_fileHandleReadSeq(handle, (uint8_t*)&blink_pool[blink_hwm + 3], 0,
                                         (uint16_t)pairs_sz) != (uint16_t)pairs_sz) {
                    rn_fileHandleClose(handle); NABU_PRINTF("Invalid file.\n"); return;
                }
                for (k = 0; k < pairs_sz; k++)
                    checksum_calc += (unsigned char)blink_pool[blink_hwm + 3 + k];
            }
            blink_off[i] = blink_hwm;
            blink_hwm += rec_sz;
        }

        if (rn_fileHandleReadSeq(handle, (uint8_t*)&checksum_saved, 0, 2) != 2) {
            rn_fileHandleClose(handle); NABU_PRINTF("No checksum.\n"); return;
        }
        rn_fileHandleClose(handle);
        if (checksum_calc != checksum_saved) { NABU_PRINTF("Bad checksum.\n"); return; }
        num_words = (int)disk_n;
        NABU_PRINTF("Loaded %d words from %s\n", num_words, fname);
    }
#else
    {
        FILE *fp;

#ifdef DEBUG
        if (debug_mode)
            printf("\n[LOAD] file='%s'\n", fname);
#endif
        fp = fopen(fname, "rb");
        if (!fp) { NABU_PRINTF("Cannot load file.\n"); return; }
        fseek(fp, 0L, SEEK_SET);

        if (!read_all(fp, hdr, 7)) { fclose(fp); NABU_PRINTF("Invalid file.\n"); return; }

#ifdef DEBUG
        if (debug_mode) dbg_hex_n("[LOAD] hdr7: ", hdr, 7);
#endif

        if (hdr[0]!='N' || hdr[1]!='I' || hdr[2]!='A' || hdr[3]!='L') {
            fclose(fp); NABU_PRINTF("Invalid file.\n"); return;
        }
        ver    = (unsigned int)hdr[4];
        disk_n = (unsigned int)hdr[5] | ((unsigned int)hdr[6] << 8);

#ifdef DEBUG
        if (debug_mode) printf("[LOAD] ver=%u num_words=%u\n", ver, disk_n);
#endif

        if (ver != 4) {
            fclose(fp); NABU_PRINTF("Wrong version.\n"); return;
        }
        if (disk_n > (unsigned int)(MAX_WORDS - 1))
            disk_n = (unsigned int)(MAX_WORDS - 1);

        init_dict();
        checksum_calc = 0;

        for (i = 0; i <= disk_n; i++) {
            int j;

            if (fread(&wlen_byte, 1, 1, fp) != 1) {
                fclose(fp); NABU_PRINTF("Invalid file.\n"); return;
            }
            checksum_calc += wlen_byte;
            if (wlen_byte > MAX_WORD_LEN) {
                fclose(fp); NABU_PRINTF("Invalid file.\n"); return;
            }

            if (i == 0) {
                if (wlen_byte) {
                    memset(io_wrec, 0, 32);
                    if (!read_all(fp, io_wrec, wlen_byte)) {
                        fclose(fp); NABU_PRINTF("Invalid file.\n"); return;
                    }
                    for (j = 0; j < (int)wlen_byte; j++)
                        checksum_calc += io_wrec[j];
                }
            } else {
                memset(io_wrec, 0, 32);
                if (wlen_byte) {
                    if (!read_all(fp, io_wrec, wlen_byte)) {
                        fclose(fp); NABU_PRINTF("Invalid file.\n"); return;
                    }
                    for (j = 0; j < (int)wlen_byte; j++)
                        checksum_calc += io_wrec[j];
                }
                io_wrec[wlen_byte] = 0;
                if (!valid_word((char*)io_wrec)) io_wrec[0] = 0;
                if (!wtext_add(i, (char*)io_wrec)) {
                    fclose(fp); NABU_PRINTF("Pool full.\n"); return;
                }
            }

            if (!read_all(fp, rec_hdr, 3)) {
                fclose(fp); NABU_PRINTF("Invalid file.\n"); return;
            }
            checksum_calc += rec_hdr[0];
            checksum_calc += rec_hdr[1];
            checksum_calc += rec_hdr[2];

            nrec   = (unsigned int)rec_hdr[2];
            rec_sz = (unsigned int)BLNK_SZ(nrec);
            if (blink_hwm + rec_sz > (unsigned int)BLINK_POOL) {
                fclose(fp); NABU_PRINTF("Pool full.\n"); return;
            }
            blink_pool[blink_hwm + 0] = (char)rec_hdr[0];
            blink_pool[blink_hwm + 1] = (char)rec_hdr[1];
            blink_pool[blink_hwm + 2] = (char)rec_hdr[2];
            if (nrec > 0) {
                unsigned int pairs_sz = nrec * (unsigned int)BLNK_PAIR;
                unsigned int k;
                if (!read_all(fp, &blink_pool[blink_hwm + 3], pairs_sz)) {
                    fclose(fp); NABU_PRINTF("Invalid file.\n"); return;
                }
                for (k = 0; k < pairs_sz; k++)
                    checksum_calc += (unsigned char)blink_pool[blink_hwm + 3 + k];
            }
            blink_off[i] = blink_hwm;
            blink_hwm += rec_sz;

#ifdef DEBUG
            if (debug_mode && (i == 0 || i == disk_n))
                printf("[LOAD] record %u '%s'\n", i,
                       i == 0 ? "(start)" : WTEXT(i));
#endif
        }

        if (fread(&checksum_saved, sizeof(unsigned short), 1, fp) != 1) {
            fclose(fp); NABU_PRINTF("No checksum.\n"); return;
        }
        fclose(fp);
        if (checksum_calc != checksum_saved) { NABU_PRINTF("Bad checksum.\n"); return; }
        num_words = (int)disk_n;
        NABU_PRINTF("Loaded %d words from %s\n", num_words, fname);

#ifdef DEBUG
        if (debug_mode) printf("[LOAD] done.\n");
#endif
    }
#endif /* __NABU__ */
}

/* -------------------- Pager --------------------------- */
/*
   Increments *ln each call. When it reaches PAGE_LINES, prints a prompt
   and waits for a keypress. Returns 1 to continue listing, 0 to quit.
*/

#define PAGE_LINES 22

static int page_pause(int *ln)
{
    int c;
    if (pg_nonstop) return 1;
    if (++(*ln) < PAGE_LINES) return 1;
    *ln = 0;
    NABU_PRINTF("-- any key / n=nonstop / q=quit --");
    NABU_FLUSH();
    c = NABU_GETCHAR();
    NABU_PRINTF("\n");
    if (c == 'q' || c == 'Q') return 0;
    if (c == 'n' || c == 'N') { pg_nonstop = 1; return 1; }
    return 1;
}

/* ------------------ Command Handler ------------------- */
/*
   Handles all commands that begin with '#'.

   Supported commands:
     #fresh        — clears the dictionary and resets all state
     #list         — prints all dictionary entries (binary decoded)
     #save [file]  — saves the dictionary in v4 binary format
     #load [file]  — loads a v4 binary dictionary from disk
     #help         — lists the main commands
     #quit         — exits the program
     #debug        — toggles verbose trace; only in -DDEBUG builds
*/

void handle_command(char *cmd)
{
    if (!strcmp(cmd, "#fresh")) {
        dict_full_warned = 0;
        init_dict();
        NABU_PRINTF("Dictionary cleared.\n");
    }
    else if (!strcmp(cmd, "#list")) {
        int ln = 0;
        int i, k;
        pg_nonstop = 0;
        for (i = 0; i <= num_words; i++) {
            char *rec   = &blink_pool[blink_off[i]];
            unsigned int total = rd_u16(rec);
            unsigned int n     = (unsigned int)(unsigned char)rec[2];
            char *w = (i == 0) ? (char*)"(start)" : WTEXT(i);
            NABU_PRINTF("%d %s [t=%u p=%u]", i, w, total, n);
            if (n) NABU_PRINTF(" ");
            for (k = 0; k < (int)n; k++) {
                NABU_PRINTF("%u(%u)",
                    rd_u16(rec + BLNK_HDR + k * BLNK_PAIR),
                    rd_u16(rec + BLNK_HDR + k * BLNK_PAIR + 2));
            }
            NABU_PRINTF("\n");
            if (!page_pause(&ln)) break;
        }
    }
    else if (!strcmp(cmd, "#help")) {
        NABU_PRINTF("Commands:\n  #fresh  - Clear dictionary\n  #list   - List dictionary\n  #save   - Save dictionary to " NIALL_STORAGE "\n  #load   - Load dictionary from " NIALL_STORAGE "\n  #quit   - Exit\n");
    }
    else if (!strncmp(cmd, "#save", 5)) {
        char fname[64] = "";
        if (sscanf(cmd + 5, "%s", fname) != 1) {
            NABU_PRINTF("Filename (%s): ", NABU_DEFAULT_FILE);
            NABU_FLUSH();
            NABU_GETLINE(fname, 64);
            fname[strcspn(fname, "\r\n")] = 0;
        }
        if (fname[0] == 0) strcpy(fname, NABU_DEFAULT_FILE);
        save_dictionary(fname);
    }
    else if (!strncmp(cmd, "#load", 5)) {
        char fname[64] = "";
        if (sscanf(cmd + 5, "%s", fname) != 1) {
            NABU_PRINTF("Filename (%s): ", NABU_DEFAULT_FILE);
            NABU_FLUSH();
            NABU_GETLINE(fname, 64);
            fname[strcspn(fname, "\r\n")] = 0;
        }
        if (fname[0] == 0) strcpy(fname, NABU_DEFAULT_FILE);
        load_dictionary(fname);
    }
#ifdef DEBUG
    else if (!strcmp(cmd, "#debug")) {
        debug_mode = !debug_mode;
        printf("Debug mode %s\n", debug_mode ? "ON" : "OFF");
    }
#endif
    else if (!strcmp(cmd, "#quit")) {
        NABU_PRINTF("Bye.\n");
        exit(0);
    }
}

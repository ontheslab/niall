// nialllt.c - NIALL IA Latency Test v1.0
//
// Measures rn_fileHandleRead and rn_fileHandleReplace round-trip timing
// to calibrate Phase 6 paged dictionary cache depth.
//
// Three scenarios (same binary, different hardware/config):
//   1. Loopback   - emulator + IA on same PC (127.0.0.1)
//   2. LAN        - emulator on PC, IA on remote machine (e.g. 192.168.0.137)
//   3. RS-422     - real NABU hardware via RS-422 serial to IA adapter
//
// Build:
//   zcc +nabu -vn -create-app -compiler=sdcc --opt-code-size nialllt.c -o NIALLLT

#define BIN_TYPE BIN_HOMEBREW
#define FONT_LM80C
#include "../NABULIB/NABU-LIB.h"
#include "../NABULIB/RetroNET-FileStore.h"
#include "../NABULIB/RetroNET-IAControl.h"
#include "../NABULIB/patterns.h"

#define TEST_FILE     "NIALLLT.DAT"
#define TEST_FILE_LEN 11
#define BLOCK_SIZE    128       /* planned NIALL Phase 6 link block size */
#define NUM_BLOCKS    100       /* blocks pre-written to test file       */
#define NUM_OPS       50        /* operations per test                   */

static uint8_t  g_block[BLOCK_SIZE];
static uint8_t  g_t1[64];
static uint8_t  g_t2[64];
static uint16_t g_rng = 0x1234;

/* ---- minimal output helpers (no printf dependency) ---- */

static void out_str(const char *s) {
    while (*s) {
        if (*s == '\n') vdp_newLine();
        else vdp_write((uint8_t)*s);
        s++;
    }
}

static void out_u32(uint32_t v) {
    char    buf[11];
    int8_t  i = 10;
    buf[10] = '\0';
    if (v == 0) { vdp_write('0'); return; }
    while (v > 0) {
        buf[--i] = (char)('0' + (v % 10));
        v /= 10;
    }
    out_str(buf + i);
}

/* ---- timing via ia_getCurrentDateTimeStr (ms precision) ---- */

/* Returns ms since midnight from a "HH:mm:ss.fff" string */
static uint32_t parse_ms(uint8_t *s) {
    uint32_t h   = (uint32_t)((s[0]-'0') * 10 + (s[1]-'0'));
    uint32_t m   = (uint32_t)((s[3]-'0') * 10 + (s[4]-'0'));
    uint32_t sec = (uint32_t)((s[6]-'0') * 10 + (s[7]-'0'));
    uint32_t ms  = (uint32_t)((s[9]-'0') * 100 + (s[10]-'0') * 10 + (s[11]-'0'));
    return ((h * 60 + m) * 60 + sec) * 1000 + ms;
}

static void get_time(uint8_t *buf) {
    ia_getCurrentDateTimeStr((uint8_t *)"HH:mm:ss.fff", 12, buf);
}

/* Print result line: "Label:  NNNms total  N.Nms/op" */
static void show_result(const char *label, uint32_t elapsed) {
    uint32_t ms_per = elapsed / (uint32_t)NUM_OPS;
    uint32_t frac   = (elapsed * 10 / (uint32_t)NUM_OPS) % 10;
    out_str(label);
    out_str(": ");
    out_u32(elapsed);
    out_str("ms tot  ");
    out_u32(ms_per);
    out_str(".");
    out_u32(frac);
    out_str("ms/op\n");
}

/* ---- simple LCG for pseudo-random block indices ---- */
static uint8_t rand_block(void) {
    g_rng = g_rng * 6364 + 1;
    return (uint8_t)(g_rng % NUM_BLOCKS);
}

/* ---- main ---- */

void main(void) {
    uint8_t  fh, blk;
    uint16_t i;
    uint32_t t_start, t_end;

    initNABULib();
    vdp_initTextMode(0x0F, 0x01, true);
    vdp_loadASCIIFont(ASCII);

    out_str("NIALL IA Latency Test v1.0\n");
    out_str("Block:");  out_u32(BLOCK_SIZE);
    out_str("B  Ops:"); out_u32(NUM_OPS);
    out_str("\n\n");

    /* Fill block with a recognisable test pattern */
    for (i = 0; i < BLOCK_SIZE; i++)
        g_block[i] = (uint8_t)i;

    /* Create / clear test file and write NUM_BLOCKS blocks */
    out_str("Preparing test file...\n");
    fh = rn_fileOpen(TEST_FILE_LEN, (uint8_t *)TEST_FILE,
                     OPEN_FILE_FLAG_READWRITE, 0xff);
    rn_fileHandleEmptyFile(fh);
    for (i = 0; i < NUM_BLOCKS; i++) {
        g_block[0] = (uint8_t)i;   /* unique marker so blocks are distinct */
        rn_fileHandleAppend(fh, 0, BLOCK_SIZE, g_block);
    }
    out_str("File: ");
    out_u32((uint32_t)NUM_BLOCKS * BLOCK_SIZE);
    out_str(" bytes\n\n");

    /* ---- Test 1: Sequential reads ---- */
    out_str("[1] Sequential read x");
    out_u32(NUM_OPS); out_str("\n");
    get_time(g_t1);
    for (i = 0; i < NUM_OPS; i++)
        rn_fileHandleRead(fh, g_block, 0,
                          (uint32_t)i * BLOCK_SIZE, BLOCK_SIZE);
    get_time(g_t2);
    t_start = parse_ms(g_t1);
    t_end   = parse_ms(g_t2);
    show_result("Seq read ", t_end - t_start);

    /* ---- Test 2: Random reads ---- */
    out_str("[2] Random read x");
    out_u32(NUM_OPS); out_str("\n");
    g_rng = 0x1234;
    get_time(g_t1);
    for (i = 0; i < NUM_OPS; i++)
        rn_fileHandleRead(fh, g_block, 0,
                          (uint32_t)rand_block() * BLOCK_SIZE, BLOCK_SIZE);
    get_time(g_t2);
    t_start = parse_ms(g_t1);
    t_end   = parse_ms(g_t2);
    show_result("Rnd read ", t_end - t_start);

    /* ---- Test 3: Random writes (in-place replace) ---- */
    out_str("[3] Random write x");
    out_u32(NUM_OPS); out_str("\n");
    g_rng = 0x1234;
    get_time(g_t1);
    for (i = 0; i < NUM_OPS; i++) {
        blk = rand_block();
        g_block[0] = blk;
        rn_fileHandleReplace(fh, (uint32_t)blk * BLOCK_SIZE,
                             0, BLOCK_SIZE, g_block);
    }
    get_time(g_t2);
    t_start = parse_ms(g_t1);
    t_end   = parse_ms(g_t2);
    show_result("Rnd write", t_end - t_start);

    /* Cleanup */
    rn_fileHandleClose(fh);
    rn_fileDelete(TEST_FILE_LEN, (uint8_t *)TEST_FILE);

    out_str("\nNote: timing includes 2x ia_getCurrentDateTimeStr\n");
    out_str("overhead per test (~same on all scenarios).\n");
    out_str("\nDone. Press any key.\n");
    getChar();
}

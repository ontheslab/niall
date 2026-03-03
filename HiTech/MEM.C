/* MEM.C - Memory reporting helpers for NIALL (HI-TECH C V3.09, CP/M)
   This file is kept small on purpose to avoid CGEN out-of-memory.

   Compile & Link:
     C -V HTNBASE.C REPLY.C COMMANDS.C MEM.C
*/

#include "NIALL.H"

/* Exact total bytes used by all links heap strings.
   Includes the terminating NUL for each allocated links string.
*/
unsigned long links_bytes_used()
{
    unsigned long total;
    int i;
    unsigned short n;

    total = 0;
    for (i = 0; i <= END_TOKEN; i++) {
        if (words[i].links != (char *)0) {
            n = safe_links_len(words[i].links);
            total += (unsigned long)n + 1;
        }
    }
    return total;
}

/* Practical estimate of the largest contiguous heap block available.
   K&R / HI-TECH safe version.

   Strategy:
   - Try malloc starting at PROBE_START
   - Step down by PROBE_STEP until it succeeds
*/
unsigned int heap_probe_maxblock()
{
    unsigned int sz;
    char *p;

#define PROBE_START 16384
#define PROBE_STEP   256
#define PROBE_MIN     64

    sz = PROBE_START;

    while (1) {

        p = (char *)malloc(sz);
        if (p != (char *)0) {
            free(p);
            return sz;
        }

        if (sz <= PROBE_MIN)
            break;

        sz = sz - PROBE_STEP;
    }

    p = (char *)malloc(64);
    if (p != (char *)0) { free(p); return 64; }

    p = (char *)malloc(32);
    if (p != (char *)0) { free(p); return 32; }

    return 0;
}

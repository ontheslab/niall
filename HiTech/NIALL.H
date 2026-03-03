/* NIALL.H - Shared definitions for HTNBASE.C, REPLY.C and COMMANDS.C
   Compile & Link, all modules:
     C -V HTNBASE.C REPLY.C COMMANDS.C
 */

#ifndef NIALL_H
#define NIALL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Configuration (same as main file)
   - END_TOKEN must equal MAX_WORDS for AMOS compatibility
   - Real dictionary words are capped at MAX_WORDS-1
*/
#define MAX_WORDS 500        /* Adjust as needed (real words capped to MAX_WORDS-1) */
#define MAX_SENTENCE 64
#define MAX_INPUT 255

/* MAX_LINKS_STR is now ONLY a temporary rebuild buffer size.
   links are stored as variable-length strings on the heap.
*/
#define MAX_LINKS_STR 256

#define MAX_WORD_LEN 31
#define END_TOKEN (MAX_WORDS)

/* Magic number for binary save files (6 bytes) */
extern char NIALL_MAGIC[6];

/* Dictionary entry
   NOTE:
   - links is a pointer to a heap string sized exactly to its content
   - links may be NULL meaning "no links yet"
*/
typedef struct {
    char word[32];
    char *links;
} WordEntry;

/* Global dictionary (declared in HTNBASE.C, extern here) */
extern WordEntry words[MAX_WORDS + 1];
extern int num_words;

/* Shared helpers (implemented in HTNBASE.C) */
int idx_ok();
int valid_word();
void free_dictionary_links();

/* Links helpers (variable-length) */
int set_links_str();        /* replace links with given string (malloc exact) */
int set_links_fmt_1();      /* set "1|f(1)" format */
unsigned short safe_links_len();

/* Memory reporting helpers (implemented in HTNBASE.C) */
unsigned long links_bytes_used();
unsigned int heap_probe_maxblock();

#endif /* NIALL_H */

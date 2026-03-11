# NIALL - Niall Adventures and Ports

## Non Intelligent (AMOS) Language Learner (Markov Chain Chatbot) for CP/M

**NIALL** is a faithful 8-bit recreation of a **1990 AMOS BASIC (Non Intelligent (AMOS) Language Learner)** Markov chain chatbot, ported to C for CP/M-80. You teach it sentences, it learns word transitions with weighted probabilities, and generates replies by walking the Markov chain.

## Target Platform

- **OS:** CP/M 2.2 (TRS-80 Model 4P, NABU CP/M)
- **CPU:** Z80, 64KB address space
- **Compiler:** z88dk with SDCC

## Building

```
build.bat          # release build
build.bat debug    # debug build (enables #DEBUG command and save/load tracing)
```

Requires [z88dk](https://github.com/z88dk/z88dk) installed at `C:\z88dk\`.

This produces:
- `NIALL.COM` — the chatbot
- `NIALLCHK.COM` — save file verifier
- `NIALLCON.COM` — v3 → v4 save file converter
- `NIALLASC.COM` — AMOS BBS ASCII importer

## Usage

Run `NIALL` on a CP/M system. Type sentences to teach NIALL, and it will reply.

| Command    | Description                                  |
|------------|----------------------------------------------|
| `#FRESH`   | Clear the dictionary                         |
| `#LIST`    | Show dictionary with full validation output  |
| `#SAVE`    | Save dictionary to disk                      |
| `#LOAD`    | Load dictionary from disk                    |
| `#HELP`    | Show command list                            |
| (any text) | Teach NIALL and get a reply                  |

To verify a saved dictionary file: `NIALLCHK filename`
To convert a v3 save file to v4: `NIALLCON old.dat new.dat`
To import an AMOS BBS ASCII save file: `NIALLASC input.dat output.dat`

## Project History

### Original: AMOS BASIC (1990)
NIALL began as an AMOS BASIC program on the Amiga. The original source is preserved in the `AMOS/` folder. See ".bas" file for details.

### BBS Version: Compiled QuickBASIC (1993)
I **ported** NIALL into an **online** version to run as a door on a DOS-based BBS. An older version of the QuickBASIC source is in `NiallBBS/`, along with the in-program help file. The compiled binary is available in the release zip and is currently running as a door on **Amiga Retro Brisbane BBS**.

### Phase 1: Hi-Tech C 3.09
Ported to C using Hi-Tech C V3.09 — a period-correct K&R C compiler for CP/M. This version works but is memory-limited; fixed-size arrays cap the dictionary at around 48 words in practice.

### Phase 2: z88dk + SDCC
The active port moves to z88dk with the SDCC backend to overcome Hi-Tech C's memory limits. Still models link data as text strings — faithful to the AMOS approach — but with improved save/load, checksum verification, and an expanding vocabulary.

### Notes on Phase 1 & 2
Both are an attempt (exercise) to model the NIALL (Markov chain) data in exactly the same way as the AMOS Version — as a text string array. Not the most efficient method in the small TPA of a CP/M computer, but it's all about the practice and relearning what was once familiar.

### Phase 3: Shared Link Pool (v1.10–v1.14)
Moving away from the AMOS-compatible fixed-buffer approach. Replaced the fixed `WordEntry` struct array with a shared 14 KB text link pool and parallel offset arrays, pushing vocabulary from ~150 to 250 words while staying within the NABU TPA. Save format: v3 binary.

### Phase 4: Binary Architecture (v1.15)
Complete replacement of text link strings with packed binary link records. A compact string pool replaces fixed 32-byte word slots. No text parsing anywhere now.

- **Vocabulary:** 1000 words (up from 250)
- **Link format:** `3 + n×4` byte binary records — no ASCII encoding or parsing
- **Save format:** v4 binary (use `NIALLCON` to migrate v3 files)
- **New tools:** `NIALLCON` (v3→v4 converter), `NIALLASC` (AMOS BBS ASCII importer)
- **Size:** 47,809 bytes — fits the NABU CP/M TPA with ~691 bytes to spare

### Phase 5: New Platforms (in progress)
Taking NIALL beyond CP/M — same algorithm, new environments.

- **Python port** (`python/niall.py`) — runs on any modern OS with Python 3, no dependencies. Uses JSON for save files (human readable, editable in a text editor). Useful for building and testing large dictionaries without needing real hardware. Same word rules and commands as the CP/M version.
- **TRS-80 native (TRSDOS/LS-DOS)** — looked into it -  but parked; z88dk CMD format compatibility with LS-DOS 6.x? (well I could not get ti) needs work before I can get a native build going.

## Files

| File            | Description                                        |
|-----------------|----------------------------------------------------|
| `niall.c`       | Main source — learning, reply generation           |
| `niallchk.c`    | Save file verifier (v3 and v4)                     |
| `niallcon.c`    | v3 → v4 save file converter (renamed from niallconv.c for CP/M 8.3) |
| `niallasc.c`    | AMOS BBS ASCII → v4 importer                       |
| `build.bat`     | Build script for all tools                         |
| `AMOS/`         | Original 1990 AMOS BASIC source                    |
| `HiTech/`       | Hi-Tech C port (Phase 1)                           |
| `NiallBBS/`     | BBS door version (1993) — source, help, binary zip |
| `python/`       | Python port (Phase 5)                              |
| `sample/`       | Sample dictionary data files                       |

## Why you ask?

Practice (Adventures) in "Classic" 'C' & ANSI 'C' - I used (I'm sure you all remember good old "Brain Damage Software") "BDS C" & Lattice 'C' on multiple platforms (in the long ago) and HiTech 'C' towards the end of CP/M.


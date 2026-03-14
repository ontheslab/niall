# NIALL - Niall Adventures and Ports

## Non Intelligent (AMOS) Language Learner — Markov Chain Chatbot for CP/M

**NIALL** is a faithful 8-bit recreation of a **1990 AMOS BASIC** Markov chain chatbot, ported to C for CP/M-80. You teach it sentences, it learns word transitions with weighted probabilities, and generates replies by walking the Markov chain.

## Target Platforms

- **CP/M 2.2** — NABU, TRS-80 Model 4P, Kaypro II/10, Coleco ADAM, RomWBW, and compatible Z80 systems
- **NABU Native** — homebrew binary using NABU-LIB (VDP display, RetroNET IA file storage)
- **CPU:** Z80, 64KB address space
- **Compiler:** z88dk with SDCC

## Building

```
build.bat          # CP/M release (NIALL.COM + tools)
build.bat nabu     # NABU native 40-col TMS9918  → NIALLN.nabu
build.bat nabu80   # NABU native 80-col F18A     → NIALLN80.nabu
build.bat latency  # NABU IA latency test tool   → NIALLLT.nabu
```

Requires [z88dk](https://github.com/z88dk/z88dk) installed at `C:\z88dk\`.

CP/M build produces:

| File           | Size   | Description                          |
|----------------|--------|--------------------------------------|
| `NIALL.COM`    | 48,144 | The chatbot                          |
| `NIALLCHK.COM` | 14,727 | Save file verifier                   |
| `NIALLCON.COM` | 12,118 | v3 → v4 save file converter          |
| `NIALLASC.COM` | 28,044 | AMOS BBS ASCII → v4 importer         |

NABU builds:

| File           | Size   | Description                          |
|----------------|--------|--------------------------------------|
| `NIALLN.nabu`  | 57,460 | NABU native — 40-col TMS9918 (default)|
| `NIALLN80.nabu`| 57,474 | NABU native — 80-col F18A option     |

## Usage

Run `NIALL` on CP/M or load `NIALLN.nabu` on NABU. Type sentences to teach NIALL and it will reply.

| Command          | Description                                          |
|------------------|------------------------------------------------------|
| `#FRESH`         | Clear the dictionary                                 |
| `#LIST`          | Show dictionary with totals and transition pairs     |
| `#MEM`           | Memory stats — words, text pool, cache (NABU only)   |
| `#SAVE [file]`   | Save dictionary (default: NIALL.DAT)                 |
| `#LOAD [file]`   | Load dictionary (default: NIALL.DAT)                 |
| `#HELP`          | Show command list                                    |
| (any text)       | Teach NIALL a sentence and get a reply               |

To verify a saved dictionary: `NIALLCHK filename`
To convert a v3 save file to v4: `NIALLCON old.dat new.dat`
To import an AMOS BBS ASCII save file: `NIALLASC input.dat output.dat`

## Save File Formats

| Version | Platform      | Max words | Notes                              |
|---------|---------------|-----------|------------------------------------|
| v4      | CP/M          | 999       | NIALL.COM v1.15+                   |
| v5      | NABU native   | 1,999     | NIALLN.nabu v1.30+; v4 auto-converts on load |

Use `NIALLCHK` to verify any save file. Use the Python tools to convert between formats on PC.

## Project History

### Original: AMOS BASIC (1990)
NIALL began as an AMOS BASIC program on the Amiga. The original source is preserved in the `AMOS/` folder.

### BBS Version: Compiled QuickBASIC (1993)
Ported to run as a door on a DOS-based BBS. Source is in `NiallBBS/`, along with the in-program help file. The compiled binary is currently running as a door on **Amiga Retro Brisbane BBS**.

### Phase 1: Hi-Tech C 3.09
Ported to C using Hi-Tech C V3.09 — a period-correct K&R C compiler for CP/M. This version works but is memory-limited; fixed-size arrays cap the dictionary at around 48 words in practice. Source in `HiTech/`.

### Phase 2: z88dk + SDCC
Moved to z88dk with the SDCC backend to overcome Hi-Tech C's memory limits. Text string link format faithful to the original AMOS approach, with improved save/load and checksum verification.

### Phase 3: Shared Link Pool (v1.10–v1.14)
Replaced the fixed `WordEntry` struct array with a shared 14 KB text link pool and parallel offset arrays, pushing vocabulary from ~150 to 250 words while staying within the NABU TPA. Save format: v3 binary.

### Phase 4: Binary Architecture (v1.15)
Complete replacement of text link strings with packed binary link records. A compact string pool replaces fixed 32-byte word slots.

- **Vocabulary:** 999 words (up from 250)
- **Link format:** `3 + n×4` byte binary records — no ASCII encoding or parsing
- **Save format:** v4 binary (use `NIALLCON` to migrate v3 files)
- **New tools:** `NIALLCON` (v3→v4), `NIALLASC` (AMOS BBS ASCII importer)
- **Size:** 47,809 bytes — fits the NABU CP/M TPA with ~691 bytes headroom

### Phase 5: New Platforms (v1.2)
Taking NIALL beyond CP/M — same algorithm, new environments.

- **NABU Native** (`NIALLN.nabu`) — homebrew binary using NABU-LIB. VDP text display (40-col TMS9918 default, 80-col F18A option via `NIALLN80.nabu`), RetroNET IA file storage. Single source file compiles for both CP/M and NABU native via `NABU_XXXX()` platform macros. Dictionary portable between CP/M and NABU via v4 binary format.
- **Python toolkit** (`python/`) — three companion tools for working with dictionaries on a modern PC (see below).

### Phase 6: IA-Paged Dictionary (v1.30)
The headline feature of v1.30: NIALL's link data moves off-chip to the **RetroNET Internet Adapter file store**, freeing RAM for a dramatically larger vocabulary. The CP/M build is unchanged throughout.

- **NABU vocabulary raised to 1,999 words** — link blocks stored as 256-byte fixed records in `NIALLPG.DAT` on the IA, accessed via random read/write
- **24-slot LRU cache** keeps the hottest link blocks in RAM; evicted dirty blocks are written back automatically. Start token always stays in RAM (1 KB, 255 pairs)
- **Save format v5** — NABU native saves as v5 binary (max 1,999 words, 63 pairs/word). Loading a v4 CP/M file on NABU translates the end-token automatically
- **Progress display** — `#save` and `#load` show a dot every 25 words plus final transition count
- **`#mem` command** (NABU only) — live memory stats: words used/free, text pool usage, LRU cache slots
- **Ghost screen fix** — `vdp_clearScreen()` on startup prevents stale VRAM showing after `#quit` restart
- **Debug subsystem removed** — cleaner production binary; all `#ifdef DEBUG` machinery stripped
- **RomWBW tested** — CP/M build confirmed working on NABU with RomWBW add-in card, including IA network file storage (Thanks GTAMP!)

| Limit            | NABU native  | CP/M        |
|------------------|--------------|-------------|
| Max words        | 1,999        | 999         |
| Start-token pairs| 255          | 100         |
| Regular pairs    | 63           | 25          |
| Text pool        | 15,360 bytes | 7,680 bytes |
| Link storage     | IA file      | 17,408 bytes RAM |

## Python Toolkit (`python/`)

Three tools for working with NIALL dictionaries on a modern PC — stdlib only, no dependencies.

| Tool            | Description                                                        |
|-----------------|--------------------------------------------------------------------|
| `niall.py`      | Full Python chatbot. JSON save, loads **any** NIALL format via niallconv. `#export` (v5 NABU) and `#exportcpm` (v4 CP/M) for transferring trained dictionaries to hardware |
| `niallconv.py`  | Universal format converter. Reads AMOS ASCII, v3, v4, v5, JSON. Writes v4, v5, JSON. Full validation and auto-correction. `inspect` command writes clean versions of all formats |
| `nialled.py`    | Interactive dictionary editor. Browse, edit, repair, merge, prune. Exports v4 and v5 binary |

Example of using a PC for building a large NABU dictionary:

```
python niall.py
USER: #load RETRO.DAT       ← any format, auto-detected
USER: [train many sentences]
USER: #export NIALL.DAT     ← v5 for NABU
```

Copy `NIALL.DAT` to the IA file store, then `#load` on the NABU.

## Files

| File / Folder   | Description                                                        |
|-----------------|--------------------------------------------------------------------|
| `niall.c`       | Main source — all platforms, single file                           |
| `niallchk.c`    | Save file verifier (v3, v4)                                        |
| `niallcon.c`    | v3 → v4 save file converter                                        |
| `niallasc.c`    | AMOS BBS ASCII → v4 importer                                       |
| `nialllt.c`     | NABU IA latency test tool                                          |
| `build.bat`     | Build script for all targets                                       |
| `AMOS/`         | Original 1990 AMOS BASIC source                                    |
| `HiTech/`       | Hi-Tech C port (Phase 1)                                           |
| `NiallBBS/`     | BBS door version (1993) — source, help, binary                     |
| `python/`       | Python port and tools (Phase 5+)                                   |
| `sample/`       | Sample dictionary files (v4 CP/M, v5 NABU, AMOS ASCII)            |
| `docs/`         | Version history, architecture notes, testing guide                 |

## Why?

Practice (Adventures) in "Classic" C and ANSI C — I used BDS C and Lattice C on multiple platforms back in the day, and Hi-Tech C towards the end of CP/M. This project is a journey back through that world, with a few modern twists.

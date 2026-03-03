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
- `NIALLCHK.COM` — save file verifier utility

## Usage

Run `NIALL` on a CP/M system. Type sentences to teach NIALL, and it will reply.

| Command       | Description                        |
|---------------|------------------------------------|
| `#FRESH`      | Clear the dictionary               |
| `#LIST`       | Show dictionary (summary)          |
| `#LISTD`      | Show dictionary (detailed)         |
| `#SAVE`       | Save dictionary to disk            |
| `#LOAD`       | Load dictionary from disk          |
| `#MEM`        | Show memory usage                  |
| `#HELP`       | Show command list                  |
| (any text)    | Teach NIALL and get a reply        |

To verify a saved dictionary file: `NIALLCHK filename`

## Project History

### Original: AMOS BASIC (1990)
NIALL began as an AMOS BASIC program on the Amiga. The original source is preserved in the `AMOS/` folder. See ".bas" file for details.

### BBS Version: Compiled QuickBASIC (1993)
I **ported** NIALL into an **online** version for the BBS (I can't recall which BBS now, it was a DOS based BBS), I can't locate the code (.bas file) for it presently but the binary is available and running on "Amiga Retro Brisbane BBS".

### Phase 1: Hi-Tech C 3.09
Ported to C using Hi-Tech C V3.09 — a period-correct K&R C compiler for CP/M. This version works but is memory-limited; fixed-size arrays cap the dictionary at around 48 words in practice.

### Phase 2: z88dk + SDCC (Current)
The active port uses z88dk to overcome memory constraints. Compiles under z88dk and runs on CP/M hardware.

### Notes on Phase 1 & 2
Both are an attempt (exercise) to model the NIALL (Markov chain) data in exactly the same way as the AMOS Version. As a TEXT string array - not the most efficient method in the small TPA of a CP/M computer :smile: - Its all about the practice and relearning what was once familiar.

### Phase 3: Evolution & More ports
Going to move away from the AMOS "compatible" internal handling and see how efficient I can make it, as well as some more ports.

## Files

| File            | Description                                  |
|-----------------|----------------------------------------------|
| `niall.c`       | Main source — learning, reply generation     |
| `niallchk.c`    | Standalone save file verifier                |
| `build.bat`     | Build script for both tools                  |
| `AMOS/`         | Original 1990 AMOS BASIC source              |
| `HiTech/`       | Hi-Tech C port (Phase 1)                     |
| `sample/`       | Sample dictionary data files                 |

## Why you ask?

Practice (Adventures) in "Classic" 'C' & ANSI 'C' - I used (I'm sure you all remember good old "Brain Damage Software") "BDS C" & Lattice 'C' on multiple platforms (in the long ago) and HiTech 'C' towards the end of CP/M.


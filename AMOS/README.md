# NIALL - Original AMOS BASIC Source (1990)

This folder contains the original NIALL source as written on the Amiga in 1990 using AMOS BASIC.

## Files

| File              | Description                                      |
|-------------------|--------------------------------------------------|
| `NIALL.AMOS`      | Original AMOS BASIC source file                  |
| `NIALL.BAS`       | BASIC text export of the source                  |
| `nialla18.lha`    | Original archived release (AMOS 1.8)             |
| `AMOS_readme.doc` | Original readme from the release                 |

## About

NIALL (**N**on **I**ntelligent **A**MOS **L**anguage **L**earner) was written in AMOS BASIC on the Amiga. It uses a Markov chain to learn word transitions from sentences typed by the user, then generates replies by walking the chain.

The internal data model stores word transitions as plain text string arrays — a design choice that has been preserved faithfully in the CP/M ports for compatibility and as a personal exercise.

## Running

You will need an Amiga (or emulator such as WinUAE/FS-UAE) with AMOS BASIC installed to run the `.AMOS` source. The `.lha` archive contains the original distribution.

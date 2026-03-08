# NIALL - BBS Version (1993)

This folder contains the BBS port of NIALL, compiled as a door program for use on a DOS-based BBS system.

## Files

| File           | Description                                                               |
|----------------|---------------------------------------------------------------------------|
| `NIALLBBS.ZIP` | Original BBS door binary release                                          |
| `NIALLBBS.BAS` | Older BASIC source — an earlier version of the live BBS program (QuickBASIC) |
| `NIALLBBS.HLP` | In-program help text — command list and original credits (shown via `#?`) |

## About

Around 1993, I ported NIALL into an online version to run as a door on a DOS-based BBS. `NIALLBBS.BAS` is an older version of the source — not the exact version currently running live, but a good earlier example showing the core Markov learning logic, the BBS door interface (DORINFO1.DEF, COM port handling, ANSI colour), and the ASCII save format (`niall.dat`). The live binary is in the zip.

The BBS version is currently running as a door on **Amiga Retro Brisbane BBS**.

## Note

The zip file - see "Releases" - contains the Compiled executable, brun45.exe library, sample (local) dorinfo.def files and starter dictionary file (.dat). It will run locally in a compatible "DOS" environment.

## Updates

1. Forgot the HELP file, now added to the release.

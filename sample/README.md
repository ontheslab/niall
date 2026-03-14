# NIALL Sample Dictionary Files

Sample save files for **NIALL v1.15**. All files are v4 binary format and require NIALL v1.15 or later. Use `NIALLCHK` to verify any file before loading.

## Files

| File        | Description                                                                 |
|-------------|-----------------------------------------------------------------------------|
| `NIALL.DAT` | Basic starter — 8 words, a single taught sentence                           |
| `NIALL2.DAT`| More words added — a small but functional dictionary                        |
| `NIALL3.DAT`| Extended chat about the NABU, TRS-80 and computers from the 80s and 90s — a good dictionary to start with |

## File Format

All files are **v4 binary** (NIALL v1.15+). Not compatible with v1.14 or earlier.

To verify a file: `NIALLCHK filename`
To load into NIALL: `#LOAD filename`

## Example — NIALL.DAT

```
NIALL Binary File Checker
File   : NIALL.DAT
Version: 4
Words  : 8

Idx  Word             Total Pairs  Status
---- ---------------- ----- -----  ------
   0 (start)              2     2  OK
   1 my                   1     1  OK
   2 name                 1     1  OK
   3 is                   1     1  OK
   4 niall                1     1  OK
   5 how                  1     1  OK
   6 are                  1     1  OK
   7 you                  1     1  OK
   8 today                1     1  OK

Checksum : OK (0x0DAA)
Links    : all valid
Pairs    : 10 total, 1..2 per word
```

## Notes

- BBS dictionary files (converted from AMOS ASCII format via `NIALLASC`) will be added here.
- For v3 files (NIALL v1.14 or earlier), use `NIALLCON old.dat new.dat` to upgrade before loading.

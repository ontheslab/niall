# NIALL Sample Dictionary Files

Sample save files for NIALL v1.15+. Use `NIALLCHK` to verify any file before loading on CP/M, or `niallconv.py inspect` on PC.

## Root files

| File          | Format          | Words | Description                                                                 |
|---------------|-----------------|-------|-----------------------------------------------------------------------------|
| `NIALL.DAT`   | v4 binary (CP/M)|     8 | Basic starter — a single taught sentence                                    |
| `NIALL2.DAT`  | v4 binary (CP/M)|   145 | Small but functional dictionary                                             |
| `NIALL3.DAT`  | v4 binary (CP/M)|   299 | Chat about NABU, TRS-80 and 80s/90s computers — good starting point        |
| `RETRO.DAT`   | v4 binary (CP/M)|   299 | Retro computing themed dictionary (CP/M, v4)                                |
| `RETROV5.DAT` | v5 binary (NABU)| 1,215 | Retro computing themed dictionary (NABU native, v5) — load directly on NABU|

## BBS_DAT_FILES/

Dictionaries converted from AMOS ASCII BBS files to v4 binary via `NIALLASC`.

| File          | Format          | Words | Description              |
|---------------|-----------------|-------|--------------------------|
| `DISCO1.DAT`  | v4 binary (CP/M)|   798 | Discovery BBS dictionary |
| `NARNIA1.DAT` | v4 binary (CP/M)|   467 | Narnia BBS dictionary #1 |
| `NARNIA2.DAT` | v4 binary (CP/M)|   724 | Narnia BBS dictionary #2 |

## MASTER_BBS_BACKUPS/

Original AMOS ASCII source files from the BBS archives.

| File                        | Format     | Words | Description                    |
|-----------------------------|------------|-------|--------------------------------|
| `NIALL-DISCOVERY-BBS-1.DAT` | AMOS ASCII |   803 | Discovery BBS — original AMOS  |
| `NIALL-NARNIA-BBS-1.DAT`    | AMOS ASCII |   469 | Narnia BBS #1 — original AMOS  |
| `NIALL-NARNIA-BBS-2.DAT`    | AMOS ASCII |   725 | Narnia BBS #2 — original AMOS  |

## File Format Notes

| Format          | Platform      | Requires             | Max words |
|-----------------|---------------|----------------------|-----------|
| v4 binary (CP/M)| CP/M          | NIALL.COM v1.15+     | 999       |
| v5 binary (NABU)| NABU native   | NIALLN.nabu v1.30+   | 1,999     |
| AMOS ASCII      | PC/conversion | `niallconv.py` to convert | —    |

**On CP/M:** `#LOAD filename` — v4 only. Verify with `NIALLCHK filename`.

**On NABU:** `#load filename` — v5 only (RETROV5.DAT). Verify with `NIALLCHK`.

**On PC (Python):** `#load filename` in `niall.py` accepts all formats automatically. Use `niallconv.py` to convert between formats.

To convert AMOS ASCII to v4 or v5:
```
python niallconv.py to-bin  NIALL-NARNIA-BBS-1.DAT NARNIA1.DAT    # CP/M v4
python niallconv.py to-nabu NIALL-NARNIA-BBS-1.DAT NARNIA1V5.DAT  # NABU v5
```

For v3 files (NIALL.COM v1.14 or earlier), use `NIALLCON old.dat new.dat` on CP/M to upgrade to v4 first.

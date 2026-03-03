# NIALL - Hi-Tech C Port (Phase 1)

This folder contains the Hi-Tech C V3.09 port of NIALL for CP/M. Hi-Tech C is a period-correct K&R C compiler for CP/M and was the first C port of NIALL from the original AMOS BASIC source.

## Building

Compile and link all modules on a CP/M system with Hi-Tech C installed:

```
C -V HTNBASE.C REPLY.C COMMANDS.C MEM.C
```

## Files

| File          | Description                                      |
|---------------|--------------------------------------------------|
| `NIALL.H`     | Shared definitions, structs, and prototypes      |
| `HTNBASE.C`   | Core — input handling and sentence learning      |
| `REPLY.C`     | Markov chain reply generation                    |
| `COMMANDS.C`  | User command handling (#LIST, #SAVE, #LOAD etc.) |
| `MEM.C`       | Memory usage reporting (#MEM command)            |

## Notes

- Uses split compilation across multiple `.C` files to work around Hi-Tech C / ZAS assembler out-of-memory errors with large single files
- Uses binary save/load with checksums — text mode causes CP/M `^Z` (EOF) and CRLF corruption (and size issues)
- `MAX_WORDS` is set to 500 but practical stability caps out around 48 words due to fixed-size BSS arrays consuming too much of the 64KB address space
- This memory limitation is what drove the Phase 2 z88dk port
- HiTech version was invaluable in working out correct file save & load

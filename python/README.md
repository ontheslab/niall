# NIALL — Python Port

Python port of the 1990 AMOS BASIC Markov chain chatbot by Matthew Peck.
Faithful to the original algorithm — weighted word transition learning and reply generation — with no external dependencies.

## Requirements

Python 3.6 or later. No packages required — stdlib only.

`niall.py` optionally uses `niallconv.py` (included) to load any NIALL binary or AMOS ASCII file directly with `#load`. Without it, `#load` accepts JSON only.

## Usage

```
python niall.py
```

Type sentences to teach NIALL. It learns word transitions and generates replies by walking the Markov chain.

## Commands

| Command               | Description                                          |
|-----------------------|------------------------------------------------------|
| `#fresh`              | Clear the dictionary                                 |
| `#list`               | Show dictionary with totals and pairs                |
| `#save [file]`        | Save to JSON (default: `niall.json`)                 |
| `#load [file]`        | Load any NIALL format (default: `niall.json`)        |
| `#export [file]`      | Export v5 NABU binary (default: `NIALL.DAT`)         |
| `#exportcpm [file]`   | Export v4 CP/M binary (default: `NIALL.DAT`)         |
| `#help`               | Show command list                                    |
| `#quit`               | Exit                                                 |
| (any text)            | Teach NIALL and get a reply                          |

## Save Format

Dictionaries are saved as JSON — human readable, editable in any text editor, and cross-platform. Each word is stored as a key with a list of every word that has followed it and how many times:

```json
{
  "version": 1,
  "dictionary": {
    "__START__": {"my": 2, "how": 1},
    "my": {"name": 2},
    "name": {"is": 2},
    "is": {"niall": 1, "great": 1}
  }
}
```

The same dictionary in the original **AMOS ASCII format** looks like this:

```
 4

3| 1(2) 2(1)
my
2| 3(2)
how
1| 1000(1)
name
2| 4(2)
is
2| 1000(1)
niall
1| 1000(1)
great
1| 1000(1)
```

The idea is similar — both are text files you can open and read — but AMOS stores words by number (ID), not by name. The first line is the word count, then a blank line for the start token's links (`3| 1(2) 2(1)` = total 3, word 1 twice, word 2 once), followed by alternating word text and link string for each entry. To know that word 1 is `my` you have to count down the file. `1000` is the end-of-sentence marker. JSON stores the same information using the word text as the key directly, which makes it straightforward to read and edit without cross-referencing ID numbers.

To edit a dictionary interactively, use `nialled.py` (see below). To convert between formats, use `niallconv.py` (see below).

## Binary Export and Import

`#export` and `#exportcpm` write binary dictionaries that can be loaded directly into the C builds of NIALL.

`#load` accepts any NIALL format — JSON, v4 CP/M binary, v5 NABU binary, v3 legacy binary, or AMOS ASCII — detected automatically. This requires `niallconv.py` to be in the same directory (it is, by default). If `niallconv.py` is missing, `#load` falls back to JSON only.

| Command        | Format | Platform | Max words | File version |
|----------------|--------|----------|-----------|--------------|
| `#export`      | v5     | NABU native (NIALLN.nabu v1.30+) | 1,999 | 5 |
| `#exportcpm`   | v4     | CP/M (NIALL.COM v1.15+) | 999 | 4 |

Typical workflow for building a large NABU dictionary on PC:

```
python niall.py
USER: #load RETRO.DAT       (load any existing file — v4, v5, AMOS, or JSON)
USER: [train more sentences]
USER: #export NIALL.DAT     (write v5 for NABU)
USER: #exportcpm NIALL.DAT  (write v4 for CP/M)
```

Copy the exported `NIALL.DAT` to the NABU Internet Adapter file store, then `#load` on the NABU.

---

## nialled.py — Interactive Dictionary Editor

An interactive REPL for browsing and editing a NIALL dictionary. Loads any NIALL save format via `niallconv.py`.

```
python nialled.py [file]
```

The prompt shows the current filename, word count, and a `*` when there are unsaved changes:

```
nialled*[niall.json | 47w]>
```

### Commands

| Command | Description |
|---------|-------------|
| `list [filter]` | List all words with totals and pair counts; optional substring filter |
| `show <word\|#>` | Show outgoing transitions with percentages, plus incoming links |
| `chain <word\|#>` | Walk the most-likely Markov path from a word |
| `dead` | Words with no outgoing transitions (Markov dead ends) |
| `orphans` | Words that nothing transitions to (unreachable from start) |
| `stats` | Summary: word count, pair count, dead ends, orphans, most active words |
| `set <word\|#> <target\|#> <count>` | Add or update a transition; creates the target word if new |
| `del <word\|#> <target\|#>` | Remove one transition |
| `add <word>` | Add a new empty word |
| `remove <word\|#>` | Remove a word and all references to it |
| `rename <old\|#> <new>` | Rename a word everywhere it appears |
| `merge <src\|#> <dst\|#>` | Fold one word into another, summing counts, removing src |
| `prune <min>` | Drop all transitions with count below the threshold |
| `recount` | Validate all counts |
| `load [file]` | Load any NIALL format, replacing the current dictionary |
| `save [file]` | Save to JSON |
| `export [file]` | Export v4 CP/M binary (NIALL.DAT) |
| `export-nabu [file]` | Export v5 NABU binary (NIALL.DAT) |
| `help` | Show command list |
| `quit` | Exit (warns if there are unsaved changes) |

Words can always be referred to by name or by their `#` index shown in `list`.
Use `<start>` or `#0` for the start token, `<end>` for the end-of-sentence token.

```
set <start> hello 5   — add/strengthen "hello" as a sentence opener
set hello <end> 3     — make "hello" able to end a sentence
```

### Typical workflow

```
# Load a dictionary in any format
python nialled.py NIALL.DAT

# Browse
nialled> list
nialled> show hello
nialled> chain hello

# Edit
nialled> set hello world 3
nialled> del hello niall
nialled> rename niall amos
nialled> prune 2

# Save to JSON and export binaries for both platforms
nialled> save niall.json
nialled> export NIALL.DAT          # v4 for CP/M
nialled> export-nabu NIALL.DAT    # v5 for NABU
```

### Rescuing orphans

`orphans` finds words that nothing transitions to — they exist in the dictionary but
NIALL can never reach them during reply generation. Here is a real example of wiring
one back in by building a bridge through a new word:

```
# "alien" shows up as an orphan — nothing leads to it
# but show reveals it has a useful transition: alien -> breed
nialled> show alien

# "amiga" is already in the dictionary — a natural lead-in
# Add a new linking word "game" and wire everything together

nialled> add game
nialled> set <start> game 3      # "game" can start a sentence
nialled> set amiga game 5        # "amiga" leads to "game"
nialled> set game alien 4        # "game" leads to "alien"

# Verify the chain is now reachable
nialled> chain amiga
  amiga -> game -> alien -> breed -> ...

# Save
nialled> save
nialled> export-nabu NIALL.DAT
```

The `chain` command is useful after any edit to verify NIALL can actually walk the
path you intended before you export.

---

## niallconv.py — Universal Dictionary Converter

Converts between all NIALL save formats. Useful for moving a dictionary trained on one platform to another, or for editing a CP/M or NABU dictionary in a text editor.

```
python niallconv.py to-json [input [output.json]]
python niallconv.py to-bin  [input [output.dat]]    # v4 CP/M
python niallconv.py to-nabu [input [output.dat]]    # v5 NABU
python niallconv.py inspect [input]
```

### Commands

| Command    | Input      | Output                                                        |
|------------|------------|---------------------------------------------------------------|
| `to-json`  | any format | `niall.json` (editable JSON)                                  |
| `to-bin`   | any format | `NIALL.DAT` (v4 binary, CP/M, max 999 words)                  |
| `to-nabu`  | any format | `NIALL.DAT` (v5 binary, NABU, max 1999 words)                 |
| `inspect`  | any format | `<name>_clean.json` + `<name>_clean.dat` + `<name>_clean_nabu.dat` |

`inspect` is handy for a first look at an unknown file — it validates, auto-corrects, and writes clean copies in all three formats at once.

`to-bin` will refuse to convert if the word count exceeds the CP/M limit of 999 — use `to-nabu` instead.

### Supported input formats

| Format | Description |
|--------|-------------|
| AMOS ASCII | Original AMOS BASIC text format (Format A and B) |
| v3 binary | CP/M NIALL.COM v1.13/v1.14 binary save files |
| v4 binary | CP/M NIALL.COM v1.15+ binary save files |
| v5 binary | NABU NIALLN.nabu v1.30+ binary save files |
| JSON | Python port save files (niall.py) |

The format is detected automatically — no flag needed.

### Typical workflow

```
# Bring a CP/M or NABU dictionary into JSON for editing
python niallconv.py to-json NIALL.DAT

# Edit niall.json in any text editor, then push it back
python niallconv.py to-bin  niall.json NIALL.DAT    # CP/M
python niallconv.py to-nabu niall.json NIALL.DAT    # NABU

# Convert a CP/M v4 file directly to NABU v5
python niallconv.py to-nabu NIALL.DAT NIALL_NABU.DAT

# Inspect an old AMOS file and get clean copies of all formats
python niallconv.py inspect GTAMP.DAT
```

### Auto-corrections

All commands validate the input and apply fixes automatically, reporting each change to the screen:

- Word text stripped to alphanumeric (matching what NIALL.COM learns)
- Link totals recomputed from pair counts (stored value ignored)
- Duplicate target words merged
- Zero-count pairs removed
- Out-of-range word references skipped
- AMOS end-of-sentence token (3000) remapped to v4 token (1000)
- Start-token singleton pairs removed (words seen only once as a sentence opener, common in large AMOS files)
- Pairs over the per-word limit kept by highest count; excess dropped
- v3, v4, and v5 checksum mismatches flagged

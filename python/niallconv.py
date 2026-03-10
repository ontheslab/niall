#!/usr/bin/env python3
"""
niallconv.py — NIALL Universal Dictionary Converter
Reads any NIALL save format and produces clean v4 binary and/or JSON output,
with full validation and auto-correction.

Supported input formats
  AMOS ASCII  — original AMOS BASIC text format (Format A and Format B)
  v3 binary   — binary with text link strings (NIALL.COM v1.13/v1.14)
  v4 binary   — compact binary records (NIALL.COM v1.15+, NIALLN.nabu)
  JSON        — Python port save format (niall.py)

Usage
  python niallconv.py to-bin  [input [output.dat]]
  python niallconv.py to-json [input [output.json]]
  python niallconv.py inspect [input]

Commands
  to-bin   convert to clean v4 binary (default output: NIALL.DAT)
  to-json  convert to clean JSON      (default output: niall.json)
  inspect  full analysis + auto-correct, writes both .dat and .json

Auto-corrections reported and applied
  Word text stripped to alphanumeric (matching clean_word() in niall.c)
  Link totals recomputed from pair counts (stored total ignored)
  Duplicate target ids merged (counts summed)
  Zero-count pairs removed
  Out-of-range target ids skipped (dangling references)
  AMOS end-token 3000 remapped to v4 end-token 1000
  Start-token singleton pairs removed (transitions seen only once)
  Pairs over MAX_PAIRS/START_PAIRS: lowest-count transitions dropped
  v4 checksum verified; mismatch reported
  v3 checksum verified; mismatch reported
"""

import io
import json
import os
import re
import struct
import sys

# --------------------------------------------------------------------------
# Constants — must match niall.c
# --------------------------------------------------------------------------

START          = "__START__"
END            = "__END__"
MAGIC          = b"NIAL"
BIN_VERSION    = 4
MAX_WORDS      = 1000
END_TOKEN      = MAX_WORDS        # 1000 — end-of-sentence sentinel in v4
AMOS_END_TOKEN = 3000             # end-of-sentence sentinel in AMOS ASCII files
START_PAIRS    = 100              # max transitions for start token
MAX_PAIRS      = 25               # max transitions for regular words
MAX_WORD_LEN   = 31               # max characters in a dictionary word


# --------------------------------------------------------------------------
# Report — collects fixes and messages during conversion
# --------------------------------------------------------------------------

class Report:
    def __init__(self):
        self._items = []   # (entry_label, message)

    def fix(self, entry, msg):
        """Record a fix applied to a specific word/entry."""
        self._items.append((str(entry), msg))

    def note(self, msg):
        """Record a general informational note."""
        self._items.append((None, msg))

    def __bool__(self):
        return bool(self._items)

    def print_all(self):
        if not self._items:
            print("  No fixes needed — input is clean.")
            return
        for label, msg in self._items:
            if label is not None:
                print(f"  [{label}] {msg}")
            else:
                print(f"  {msg}")


# --------------------------------------------------------------------------
# Format detection
# --------------------------------------------------------------------------

def detect_format(raw):
    """
    Identify the NIALL file format from raw bytes.
    Returns one of: 'v4', 'v3', 'json', 'amos', 'unknown'.
    """
    # Strip UTF-8 BOM
    if raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]

    if len(raw) >= 5 and raw[:4] == MAGIC:
        ver = raw[4]
        if ver == 4:
            return 'v4'
        if ver == 3:
            return 'v3'
        return 'v_unknown'

    # Try text-based detection
    try:
        head = raw[:256].decode('ascii', errors='replace')
    except Exception:
        return 'unknown'

    stripped = head.lstrip()
    if stripped.startswith('{'):
        return 'json'
    # AMOS ASCII: first line is a number (possibly space-padded)
    m = re.match(r'\s*(\d+)', stripped)
    if m:
        return 'amos'

    return 'unknown'


# --------------------------------------------------------------------------
# Link-string parser — used for AMOS ASCII and v3 binary
# --------------------------------------------------------------------------

def _parse_link_string(s, disk_n, entry_label, report):
    """
    Parse a text link string of the form "total|id(cnt)id(cnt)..."
    and return a list of (id, count) tuples after validation.

    disk_n  — number of real words declared in the file header.
    Pairs with id==0, id dangling (> disk_n and != AMOS_END_TOKEN), or cnt==0
    are skipped with a fix report.
    AMOS_END_TOKEN (3000) is remapped to END_TOKEN (1000).
    """
    pairs = []
    if not s:
        return pairs

    bar = s.find('|')
    if bar < 0:
        report.fix(entry_label, f"link string has no '|' separator — no transitions loaded")
        return pairs

    p = s[bar + 1:]
    skipped = 0
    remapped = 0

    for m in re.finditer(r'(\d+)\((\d+)\)', p):
        wid  = int(m.group(1))
        cnt  = int(m.group(2))

        if cnt == 0:
            skipped += 1
            continue
        if wid == 0:
            # id 0 is the start token — never a valid transition target
            skipped += 1
            continue
        if wid == AMOS_END_TOKEN:
            wid = END_TOKEN
            remapped += 1
        elif wid > disk_n and wid != END_TOKEN:
            # Dangling reference beyond declared word count
            skipped += 1
            continue

        pairs.append((wid, cnt))

    if remapped:
        report.fix(entry_label, f"remapped {remapped} AMOS end-token(s) ({AMOS_END_TOKEN} → {END_TOKEN})")
    if skipped:
        report.fix(entry_label, f"skipped {skipped} invalid pair(s) (id==0, dangling ref, or cnt==0)")

    return pairs


# --------------------------------------------------------------------------
# Pair filtering — dedup, remove singletons, cap at limit
# --------------------------------------------------------------------------

def _filter_pairs(pairs, cap, rm_singletons, entry_label, report):
    """
    1. Merge duplicate ids (sum their counts).
    2. Optionally remove singletons (count == 1) — used for start token.
    3. Keep at most cap pairs (highest count wins).

    Returns the filtered list of (id, count) tuples.
    """
    # Merge duplicates
    merged = {}
    for wid, cnt in pairs:
        merged[wid] = merged.get(wid, 0) + cnt
    if len(merged) < len(pairs):
        dups = len(pairs) - len(merged)
        report.fix(entry_label, f"merged {dups} duplicate target id(s)")
    pairs = list(merged.items())   # (id, count)

    # Remove singletons (start token only)
    if rm_singletons:
        before = len(pairs)
        kept = [(i, c) for i, c in pairs if c > 1]
        if kept:                    # don't remove everything
            dropped = before - len(kept)
            if dropped:
                report.fix(entry_label, f"removed {dropped} singleton pair(s) (count==1, start-token)")
            pairs = kept

    # Sort descending by count, cap
    pairs.sort(key=lambda x: x[1], reverse=True)
    if len(pairs) > cap:
        report.fix(entry_label, f"capped at {cap} pairs (dropped {len(pairs) - cap} lowest-count transition(s))")
        pairs = pairs[:cap]

    return pairs


# --------------------------------------------------------------------------
# Common: convert raw (id, count) pairs → successor dict using index_to_word
# --------------------------------------------------------------------------

def _pairs_to_successors(pairs, index_to_word, entry_label, report):
    """
    Resolve numeric pair ids to word names and return a {word: count} dict.
    Unknown ids are reported and skipped.
    """
    successors = {}
    for wid, cnt in pairs:
        if wid == END_TOKEN:
            dst = END
        elif wid in index_to_word:
            dst = index_to_word[wid]
        else:
            report.fix(entry_label, f"unknown target id {wid} — skipped")
            continue
        successors[dst] = successors.get(dst, 0) + cnt
    return successors


# --------------------------------------------------------------------------
# Clean word text (match niall.c clean_word behaviour)
# --------------------------------------------------------------------------

def _clean_word_text(text, entry_label, report):
    """Strip everything except lowercase alphanumeric. Return cleaned text."""
    cleaned = re.sub(r'[^a-z0-9]', '', text.lower())
    if cleaned != text:
        if not cleaned:
            cleaned = 'x'
        report.fix(entry_label, f"word text cleaned: '{text}' → '{cleaned}'")
    return cleaned


# --------------------------------------------------------------------------
# Format parsers — each returns (dictionary, report, num_words_declared)
# --------------------------------------------------------------------------

def _parse_json(raw):
    """Parse a niall.py JSON save file."""
    report = Report()
    try:
        data = json.loads(raw.decode('utf-8', errors='replace'))
    except json.JSONDecodeError as e:
        _die(f"JSON parse error: {e}")

    if "dictionary" not in data:
        _die("Not a valid niall.json — 'dictionary' key missing.")

    raw_dict = data["dictionary"]

    # Light validation: ensure all counts are positive integers
    dictionary = {}
    for src, succs in raw_dict.items():
        clean_succs = {}
        for dst, cnt in succs.items():
            if not isinstance(cnt, int) or cnt <= 0:
                report.fix(src, f"successor '{dst}' has invalid count {cnt!r} — skipped")
                continue
            clean_succs[dst] = cnt
        dictionary[src] = clean_succs

    # Check for unknown successor words
    all_words = {w for w in dictionary if w not in (START, END)}
    for src, succs in dictionary.items():
        for dst in succs:
            if dst not in (END,) and dst not in dictionary:
                report.fix(src, f"successor '{dst}' not in dictionary — will be created as empty entry")

    num_words = sum(1 for w in dictionary if w not in (START, END))
    return dictionary, report, num_words


def _parse_v4(raw):
    """Parse a v4 binary NIALL save file."""
    report = Report()

    if len(raw) < 7:
        _die("File too short to be a valid v4 NIALL save file.")

    magic    = raw[0:4]
    ver      = raw[4]
    num_words = struct.unpack_from("<H", raw, 5)[0]

    if magic != MAGIC:
        _die_bad_magic(raw)
    if ver != BIN_VERSION:
        _die(f"Wrong binary version: got {ver}, expected {BIN_VERSION}.\n"
             f"       Use NIALLCONV.COM on CP/M to upgrade v3 files.")

    num_words = min(num_words, MAX_WORDS - 1)

    pos          = 7
    checksum_acc = 0

    def read_bytes(n, label=""):
        nonlocal pos, checksum_acc
        chunk = raw[pos:pos + n]
        if len(chunk) != n:
            _die(f"Unexpected end of file at offset {pos} (wanted {n} bytes{', reading ' + label if label else ''}).")
        for b in chunk:
            checksum_acc += b
        pos += n
        return chunk

    index_to_word = {0: START, END_TOKEN: END}
    raw_links     = {}   # index → [(target_id, count), ...]

    for i in range(num_words + 1):
        entry_label = f"entry {i}"
        wlen = read_bytes(1, "wlen")[0]
        if wlen > MAX_WORD_LEN:
            report.fix(i, f"word length {wlen} exceeds {MAX_WORD_LEN} — truncated")
            wlen = MAX_WORD_LEN

        if i == 0:
            if wlen:
                read_bytes(wlen, "start token text")   # consume + checksum but discard
        else:
            text = read_bytes(wlen, "word text").decode("ascii", errors="replace") if wlen else ""
            cleaned = _clean_word_text(text, i, report)
            index_to_word[i] = cleaned if cleaned else f"_word{i}"

        # Binary link record: total(LE16) + n_pairs(byte) + pairs(4 bytes each)
        rec_hdr  = read_bytes(3, "link record header")
        stored_total = struct.unpack_from("<H", rec_hdr, 0)[0]
        n_pairs  = rec_hdr[2]

        pairs = []
        for _ in range(n_pairs):
            pair_bytes = read_bytes(4, "link pair")
            tid = struct.unpack_from("<H", pair_bytes, 0)[0]
            cnt = struct.unpack_from("<H", pair_bytes, 2)[0]
            if cnt == 0:
                report.fix(i, f"pair with target {tid} has count 0 — removed")
                continue
            pairs.append((tid, cnt))

        computed_total = sum(c for _, c in pairs)
        if stored_total != computed_total:
            report.fix(i, f"link total corrected: stored {stored_total}, computed {computed_total}")

        raw_links[i] = pairs

    # Checksum footer
    if pos + 2 > len(raw):
        report.note("Warning: checksum footer missing (truncated file)")
    else:
        saved_cs = struct.unpack_from("<H", raw, pos)[0]
        calc_cs  = checksum_acc & 0xFFFF
        if calc_cs != saved_cs:
            report.note(f"Checksum mismatch — file may be corrupt "
                        f"(calculated {calc_cs:#06x}, stored {saved_cs:#06x})")

    # Resolve indices to words and build dictionary
    dictionary = {}
    for i in range(num_words + 1):
        src = index_to_word.get(i, f"_word{i}")
        pairs = _filter_pairs(
            raw_links.get(i, []),
            cap          = START_PAIRS if i == 0 else MAX_PAIRS,
            rm_singletons= False,   # v4 files already filtered on entry
            entry_label  = i,
            report       = report,
        )
        successors = _pairs_to_successors(pairs, index_to_word, i, report)
        if i > 0 or successors:
            dictionary[src] = successors

    return dictionary, report, num_words


def _parse_v3(raw):
    """Parse a v3 binary NIALL save file (text link strings)."""
    report = Report()

    if len(raw) < 7:
        _die("File too short to be a valid v3 NIALL save file.")

    magic    = raw[0:4]
    ver      = raw[4]
    num_words = struct.unpack_from("<H", raw, 5)[0]

    if magic != MAGIC:
        _die_bad_magic(raw)
    if ver != 3:
        _die(f"Expected v3 binary file (got version {ver}).")

    num_words = min(num_words, MAX_WORDS - 1)

    pos          = 7
    checksum_acc = 0

    def read_bytes(n, label=""):
        nonlocal pos, checksum_acc
        chunk = raw[pos:pos + n]
        if len(chunk) != n:
            _die(f"Unexpected end of file at offset {pos} (wanted {n} bytes{', reading ' + label if label else ''}).")
        for b in chunk:
            checksum_acc += b
        pos += n
        return chunk

    index_to_word = {0: START, END_TOKEN: END}
    raw_links     = {}

    for i in range(num_words + 1):
        # wlen + word text
        wlen = read_bytes(1, "wlen")[0]
        if wlen > MAX_WORD_LEN:
            wlen = MAX_WORD_LEN

        if i == 0:
            if wlen:
                read_bytes(wlen, "start token text")
        else:
            text = read_bytes(wlen, "word text").decode("ascii", errors="replace") if wlen else ""
            cleaned = _clean_word_text(text, i, report)
            index_to_word[i] = cleaned if cleaned else f"_word{i}"

        # v3 link string: llen(LE16) + ASCII text "total|id(cnt)id(cnt)..."
        llen_bytes = read_bytes(2, "link string length")
        llen = struct.unpack_from("<H", llen_bytes, 0)[0]

        link_str = ""
        if llen:
            link_bytes = read_bytes(llen, "link string")
            link_str   = link_bytes.decode("ascii", errors="replace")

        pairs = _parse_link_string(link_str, num_words, i, report)
        raw_links[i] = pairs

    # Checksum
    if pos + 2 > len(raw):
        report.note("Warning: checksum footer missing (truncated v3 file)")
    else:
        saved_cs = struct.unpack_from("<H", raw, pos)[0]
        calc_cs  = checksum_acc & 0xFFFF
        if calc_cs != saved_cs:
            report.note(f"v3 checksum mismatch (calculated {calc_cs:#06x}, stored {saved_cs:#06x})")

    # Build dictionary with filtering
    dictionary = {}
    for i in range(num_words + 1):
        src = index_to_word.get(i, f"_word{i}")
        pairs = _filter_pairs(
            raw_links.get(i, []),
            cap          = START_PAIRS if i == 0 else MAX_PAIRS,
            rm_singletons= False,
            entry_label  = i,
            report       = report,
        )
        successors = _pairs_to_successors(pairs, index_to_word, i, report)
        if i > 0 or successors:
            dictionary[src] = successors

    return dictionary, report, num_words


def _parse_amos(raw):
    """
    Parse an AMOS ASCII text save file (Format A or Format B).

    Format A (LF/CRLF, word text + link on separate lines):
      num_words
      word0_text     (start token, usually a single space)
      link_string0   (total|id(cnt)id(cnt)...)
      word1_text
      link_string1
      ...

    Format B (old CRLF, no text line for word 0):
      ' ' num_words ' '
      [blank line]
      link_string0_with_spaces
      word1_text
      link_string1
      ...

    Detection: after reading num_words, skip blanks; if next line has '|' → Format B.
    """
    report = Report()

    # Decode as text, stripping CP/M 0x1A EOF markers
    text = raw.replace(b'\x1a', b'').decode('ascii', errors='replace')
    lines = iter(text.splitlines())

    def next_line():
        try:
            return next(lines)
        except StopIteration:
            return None

    def next_nonblank():
        while True:
            ln = next_line()
            if ln is None:
                return None
            if ln.strip():
                return ln

    # Read num_words from first line
    first = next_nonblank()
    if first is None:
        _die("Empty AMOS ASCII file.")
    m = re.search(r'\d+', first)
    if not m:
        _die(f"Cannot read word count from first line: {first!r}")
    num_words = int(m.group())
    if num_words < 0 or num_words > MAX_WORDS:
        _die(f"Word count {num_words} out of range (0..{MAX_WORDS}).")

    # Format detection: read next non-blank line
    peek = next_nonblank()
    if peek is None:
        _die("AMOS ASCII file has only a word count line.")
    format_b = ('|' in peek)
    report.note(f"AMOS ASCII Format {'B (old CRLF)' if format_b else 'A (LF)'} detected, {num_words} words")

    index_to_word = {0: START, END_TOKEN: END}
    raw_links     = {}

    for i in range(num_words + 1):
        if i == 0:
            # Start token
            if format_b:
                link_str = peek   # peek is already the link string
            else:
                # peek is the word0 text line; read the link string
                link_str = next_line() or ""
        else:
            if i == 1 and not format_b:
                word_text = peek   # peek is word 1 text in Format A
            else:
                word_text = next_line() or ""

            # Clean word text
            cleaned = re.sub(r'[^a-z0-9]', '', word_text.lower())
            if not cleaned:
                report.fix(i, f"word text '{word_text}' is all-punctuation — using 'x'")
                cleaned = 'x'
            elif cleaned != word_text.lower():
                report.fix(i, f"word text cleaned: '{word_text}' → '{cleaned}'")
            index_to_word[i] = cleaned

            link_str = next_line() or ""

        pairs = _parse_link_string(link_str.strip(), num_words, i, report)
        raw_links[i] = pairs

    # Build dictionary with full filtering
    dictionary = {}
    for i in range(num_words + 1):
        src = index_to_word.get(i, f"_word{i}")
        pairs = _filter_pairs(
            raw_links.get(i, []),
            cap          = START_PAIRS if i == 0 else MAX_PAIRS,
            rm_singletons= (i == 0),   # strip singleton sentence starters (AMOS files are huge)
            entry_label  = i,
            report       = report,
        )
        successors = _pairs_to_successors(pairs, index_to_word, i, report)
        if i > 0 or successors:
            dictionary[src] = successors

    return dictionary, report, num_words


# --------------------------------------------------------------------------
# Unified loader
# --------------------------------------------------------------------------

def load_any(path):
    """
    Detect format, parse, validate and return (fmt_name, dictionary, report).
    dictionary uses START/__END__ sentinel keys matching niall.py.
    """
    with open(path, "rb") as f:
        raw = f.read()

    fmt = detect_format(raw)

    if fmt == 'v4':
        fmt_name = "v4 binary"
        dictionary, report, num_words = _parse_v4(raw)
    elif fmt == 'v3':
        fmt_name = "v3 binary"
        dictionary, report, num_words = _parse_v3(raw)
    elif fmt == 'amos':
        fmt_name = "AMOS ASCII"
        dictionary, report, num_words = _parse_amos(raw)
    elif fmt == 'json':
        fmt_name = "JSON"
        dictionary, report, num_words = _parse_json(raw)
    elif fmt == 'v_unknown':
        ver = raw[4] if len(raw) > 4 else 0
        _die(f"Unrecognised NIAL binary version {ver}.\n"
             f"       Supported: v3 (use niallconv.c on CP/M to read) and v4.")
    else:
        _die_bad_magic(raw)

    real_count = sum(1 for w in dictionary if w not in (START, END))
    return fmt_name, dictionary, report, real_count


# --------------------------------------------------------------------------
# Output writers
# --------------------------------------------------------------------------

def write_json(dictionary, path):
    """Write dictionary to a niall.py-compatible JSON file."""
    with open(path, "w", encoding="utf-8") as f:
        json.dump({"version": 1, "dictionary": dictionary}, f, indent=2, sort_keys=True)


def write_binary(dictionary, path):
    """
    Write dictionary to a clean v4 binary NIALL.DAT.
    Words sorted alphabetically for reproducible output.
    Pairs capped at MAX_PAIRS/START_PAIRS, keeping most common.
    """
    real_words = sorted(w for w in dictionary if w not in (START, END))
    if len(real_words) > MAX_WORDS - 1:
        print(f"  Warning: {len(real_words)} words exceeds limit of {MAX_WORDS - 1}. "
              f"Truncating to first {MAX_WORDS - 1} alphabetically.")
        real_words = real_words[:MAX_WORDS - 1]

    num_words   = len(real_words)
    word_to_idx = {w: i for i, w in enumerate(real_words, 1)}
    word_to_idx[START] = 0
    word_to_idx[END]   = END_TOKEN

    payload      = bytearray()
    checksum_acc = 0

    def emit(b):
        nonlocal checksum_acc
        payload.extend(b)
        for byte in b:
            checksum_acc += byte
        checksum_acc &= 0xFFFF

    def make_link_record(successors, cap):
        pairs = []
        for dst, cnt in successors.items():
            if dst in word_to_idx:
                pairs.append((word_to_idx[dst], cnt))
        pairs.sort(key=lambda x: x[1], reverse=True)
        pairs = pairs[:cap]
        total = sum(c for _, c in pairs)
        rec   = struct.pack("<H", total)
        rec  += struct.pack("B",  len(pairs))
        for tid, cnt in pairs:
            rec += struct.pack("<H", tid)
            rec += struct.pack("<H", cnt)
        return rec

    for i in range(num_words + 1):
        if i == 0:
            emit(struct.pack("B", 0))
            emit(make_link_record(dictionary.get(START, {}), START_PAIRS))
        else:
            word = real_words[i - 1]
            wlen = min(len(word), MAX_WORD_LEN)
            emit(struct.pack("B", wlen))
            emit(word[:wlen].encode("ascii", errors="replace"))
            emit(make_link_record(dictionary.get(word, {}), MAX_PAIRS))

    header = MAGIC + struct.pack("B", BIN_VERSION) + struct.pack("<H", num_words)
    footer = struct.pack("<H", checksum_acc)

    with open(path, "wb") as f:
        f.write(header)
        f.write(payload)
        f.write(footer)

    return num_words, checksum_acc


# --------------------------------------------------------------------------
# Commands
# --------------------------------------------------------------------------

def _print_separator():
    print("-" * 60)


def cmd_to_json(args):
    src = args[0] if args else "NIALL.DAT"
    dst = args[1] if len(args) > 1 else "niall.json"

    if not os.path.exists(src):
        _die(f"Input file not found: {src}")

    fmt, dictionary, report, real_count = load_any(src)
    print(f"Input : {src}  ({fmt})")
    _print_separator()
    report.print_all()
    _print_separator()
    write_json(dictionary, dst)
    print(f"Output: {dst}  ({real_count} words)")


def cmd_to_bin(args):
    src = args[0] if args else "niall.json"
    dst = args[1] if len(args) > 1 else "NIALL.DAT"

    if not os.path.exists(src):
        _die(f"Input file not found: {src}")

    fmt, dictionary, report, _ = load_any(src)
    print(f"Input : {src}  ({fmt})")
    _print_separator()
    report.print_all()
    _print_separator()
    num_words, checksum = write_binary(dictionary, dst)
    print(f"Output: {dst}  ({num_words} words, checksum {checksum:#06x})")


def cmd_inspect(args):
    src = args[0] if args else "NIALL.DAT"

    if not os.path.exists(src):
        _die(f"Input file not found: {src}")

    base   = os.path.splitext(src)[0]
    dst_bin  = base + "_clean.dat"
    dst_json = base + "_clean.json"

    fmt, dictionary, report, real_count = load_any(src)
    print(f"Input : {src}  ({fmt})")
    print(f"Words : {real_count}")
    _print_separator()
    print("Fixes applied:")
    report.print_all()
    _print_separator()

    num_words, checksum = write_binary(dictionary, dst_bin)
    write_json(dictionary, dst_json)

    print(f"Clean binary : {dst_bin}  ({num_words} words, checksum {checksum:#06x})")
    print(f"Clean JSON   : {dst_json}")


# --------------------------------------------------------------------------
# Helpers
# --------------------------------------------------------------------------

def _die(msg):
    print(f"Error: {msg}", file=sys.stderr)
    sys.exit(1)


def _die_bad_magic(raw):
    if len(raw) > 0 and (chr(raw[0]).isdigit() or chr(raw[0]) == ' '):
        _die("This looks like an AMOS ASCII text save file.\n"
             "       Use: python niallconv.py to-json <file>")
    _die("Unrecognised file format (bad magic bytes).\n"
         "       Supported: AMOS ASCII text, v3 binary, v4 binary, JSON.")


def _usage():
    print(__doc__)
    sys.exit(1)


# --------------------------------------------------------------------------
# Entry point
# --------------------------------------------------------------------------

def main():
    args = sys.argv[1:]
    if not args:
        _usage()

    cmd  = args[0].lower()
    rest = args[1:]

    if cmd == "to-json":
        cmd_to_json(rest)
    elif cmd == "to-bin":
        cmd_to_bin(rest)
    elif cmd == "inspect":
        cmd_inspect(rest)
    else:
        _usage()


if __name__ == "__main__":
    main()

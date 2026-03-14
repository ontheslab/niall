#!/usr/bin/env python3
"""
niall.py - NIALL - Non Intelligent (AMOS) Language Learner
Markov chain chatbot — Python port of the 1990 AMOS BASIC original by Matthew Peck.
Python port by Intangybles.

Usage:
    python niall.py

Save files are JSON (niall.json by default) — human readable, cross-platform.

Word rules (matching AMOS BASIC / C version):
  - Alphanumeric only (a-z, 0-9) — apostrophes and punctuation stripped
  - Commas treated as sentence separators (same as full stops)
  - All input lowercased before learning

Commands:
    #fresh              Clear the dictionary
    #list               Show the dictionary
    #save [file]        Save dictionary to JSON    (default: niall.json)
    #load [file]        Load any NIALL format      (default: niall.json)
    #export [file]      Export to v5 NABU binary   (default: NIALL.DAT)
    #exportcpm [file]   Export to v4 CP/M binary   (default: NIALL.DAT)
    #help               Show this command list
    #quit               Exit
    (any text)          Teach NIALL a sentence and get a reply
"""

import json
import os
import random
import re
import struct
import sys

try:
    import niallconv as _conv
    _CONV_AVAILABLE = True
except ImportError:
    _CONV_AVAILABLE = False

VERSION      = "1.0"
DEFAULT_FILE = "niall.json"
MAX_REPLY    = 100      # max words in a generated reply — stops infinite loops

# Binary export constants — must match niall.c
_MAGIC           = b"NIAL"
_V4_VERSION      = 4;    _V5_VERSION      = 5
_V4_END_TOKEN    = 1000; _V5_END_TOKEN    = 2000
_V4_MAX_WORDS    = 999;  _V5_MAX_WORDS    = 1999
_V4_MAX_PAIRS    = 25;   _V5_MAX_PAIRS    = 63
_V4_START_PAIRS  = 100;  _V5_START_PAIRS  = 255
_MAX_WORD_LEN    = 31

# Sentence boundary markers. Double underscores so they won't
# collide with any real word after clean_word() strips to alphanumerics.
START = "__START__"
END   = "__END__"

# The dictionary: word -> {successor: count, ...}
# e.g. {"my": {"name": 2, "dog": 1}, "__START__": {"my": 3}}
dictionary = {}


# ---------------------------------------------------------------------------
# Core
# ---------------------------------------------------------------------------

def init_dict():
    """Wipe the dictionary completely — same as #fresh."""
    global dictionary
    dictionary = {}


def clean_word(word):
    """
    Strip everything except letters and digits, then lowercase.
    Matches what the AMOS BASIC and C versions do — "it's" becomes
    "its", "TRS-80" becomes "trs80".
    """
    return re.sub(r"[^a-z0-9]", "", word.lower())


def learn_sentence(sentence):
    """
    Walk through the words in a sentence and record each transition.
    For "my name is niall" this:
      START -> my  (+1)
      my    -> name (+1)
      name  -> is  (+1)
      is    -> niall (+1)
      niall -> END (+1)
    If the same transition has been seen before, the count goes up.
    Words that reduce to nothing after cleaning (not real words) are skipped.
    """
    words = [clean_word(w) for w in sentence.split()]
    words = [w for w in words if w]  # ditch anything that cleaned away to nothing
    if not words:
        return

    # Record which word started this sentence
    dictionary.setdefault(START, {})
    dictionary[START][words[0]] = dictionary[START].get(words[0], 0) + 1

    # Record each word-to-word transition through the sentence
    for i in range(len(words) - 1):
        src, dst = words[i], words[i + 1]
        dictionary.setdefault(src, {})
        dictionary[src][dst] = dictionary[src].get(dst, 0) + 1

    # Mark the last word as a valid sentence ender
    last = words[-1]
    dictionary.setdefault(last, {})
    dictionary[last][END] = dictionary[last].get(END, 0) + 1


def weighted_choice(successors):
    """
    Pick a word weighted by count — common transitions come up more often,
    rare ones still get a look-in. Heart of the Markov weighting.
    """
    total = sum(successors.values())
    r = random.randint(1, total)
    for word, count in successors.items():
        r -= count
        if r <= 0:
            return word
    return list(successors.keys())[-1]  # fallback, shouldn't normally be needed


def generate_reply():
    """
    Walk the Markov chain from START, picking each next word by weighted
    chance until END or MAX_REPLY words. Capitalises and adds a full stop.
    """
    if START not in dictionary or not dictionary[START]:
        return "I can't speak yet - empty dictionary."

    words = []
    current = weighted_choice(dictionary[START])
    count = 0

    while current != END and count < MAX_REPLY:
        words.append(current)
        count += 1
        if current not in dictionary or not dictionary[current]:
            # Dead end — word has no recorded successors, stop here
            break
        current = weighted_choice(dictionary[current])

    if not words:
        return "..."

    reply = " ".join(words)
    return reply[0].upper() + reply[1:] + "."


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def do_list():
    """
    Print the dictionary contents — word index, text, total transitions,
    and visible pair count (END transitions are internal and not shown in pairs).
    Sorted alphabetically with the start token always at index 0.
    """
    if not dictionary:
        print("Dictionary is empty.")
        return

    real_words = sorted(w for w in dictionary if w not in (START, END))

    print(f"\n{'Idx':<5} {'Word':<20} {'Total':>6} {'Pairs':>6}")
    print("-" * 42)

    if START in dictionary:
        total = sum(dictionary[START].values())
        pairs = len(dictionary[START])
        print(f"{'0':<5} {'(start)':<20} {total:>6} {pairs:>6}")

    for i, word in enumerate(real_words, 1):
        successors = dictionary[word]
        # Don't count the END transition as a visible pair — it's an internal marker
        visible = {k: v for k, v in successors.items() if k != END}
        total = sum(successors.values())
        pairs = len(visible)
        display = word if len(word) <= 20 else word[:19] + "+"
        print(f"{i:<5} {display:<20} {total:>6} {pairs:>6}")

    print(f"\n{len(real_words)} word(s) in dictionary.")


def do_save(filename):
    """Write the dictionary to a JSON file. Version key included for future format detection."""
    data = {"version": 1, "dictionary": dictionary}
    try:
        with open(filename, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)
        print(f"Saved to {filename}.")
    except OSError as e:
        print(f"Save failed: {e}")


def do_load(filename):
    """
    Load any NIALL format (JSON, v3/v4/v5 binary, AMOS ASCII) via niallconv,
    or plain JSON if niallconv is not available.
    Replaces whatever is currently in memory — same as the C version.
    """
    global dictionary
    if not os.path.exists(filename):
        print(f"File not found: {filename}")
        return
    if _CONV_AVAILABLE:
        try:
            fmt_name, new_dict, report, _ = _conv.load_any(filename)
            if report:
                for line in report:
                    print(f"  {line}")
            dictionary = new_dict
            word_count = sum(1 for w in dictionary if w not in (START, END))
            print(f"Loaded {filename} ({fmt_name}). {word_count} word(s).")
        except Exception as e:
            print(f"Load failed: {e}")
    else:
        # niallconv not available — JSON only
        try:
            with open(filename, "r", encoding="utf-8") as f:
                data = json.load(f)
            if "dictionary" not in data:
                print("Invalid save file.")
                return
            dictionary = data["dictionary"]
            word_count = sum(1 for w in dictionary if w not in (START, END))
            print(f"Loaded {filename}. {word_count} word(s).")
        except (OSError, json.JSONDecodeError) as e:
            print(f"Load failed: {e}")


def do_export(filename, nabu=True):
    """Export dictionary to v5 NABU binary (nabu=True) or v4 CP/M binary (nabu=False)."""
    version     = _V5_VERSION     if nabu else _V4_VERSION
    end_token   = _V5_END_TOKEN   if nabu else _V4_END_TOKEN
    max_words   = _V5_MAX_WORDS   if nabu else _V4_MAX_WORDS
    max_pairs   = _V5_MAX_PAIRS   if nabu else _V4_MAX_PAIRS
    start_pairs = _V5_START_PAIRS if nabu else _V4_START_PAIRS
    fmt_name    = "v5 NABU"       if nabu else "v4 CP/M"

    real_words = sorted(w for w in dictionary if w not in (START, END))
    if len(real_words) > max_words:
        print(f"Warning: {len(real_words)} words exceeds {fmt_name} limit of {max_words}. "
              f"Truncating to first {max_words} alphabetically.")
        real_words = real_words[:max_words]

    num_words   = len(real_words)
    word_to_idx = {w: i for i, w in enumerate(real_words, 1)}
    word_to_idx[START] = 0
    word_to_idx[END]   = end_token

    payload      = bytearray()
    checksum_acc = 0

    def emit(b):
        nonlocal checksum_acc
        payload.extend(b)
        for byte in b:
            checksum_acc += byte
        checksum_acc &= 0xFFFF

    def make_link_record(successors, cap):
        pairs = [(word_to_idx[dst], cnt)
                 for dst, cnt in successors.items() if dst in word_to_idx]
        pairs.sort(key=lambda x: x[1], reverse=True)
        pairs = pairs[:cap]
        total = sum(c for _, c in pairs)
        rec   = struct.pack("<HB", total, len(pairs))
        for tid, cnt in pairs:
            rec += struct.pack("<HH", tid, cnt)
        return rec

    for i in range(num_words + 1):
        if i == 0:
            emit(struct.pack("B", 0))
            emit(make_link_record(dictionary.get(START, {}), start_pairs))
        else:
            word = real_words[i - 1]
            wlen = min(len(word), _MAX_WORD_LEN)
            emit(struct.pack("B", wlen))
            emit(word[:wlen].encode("ascii", errors="replace"))
            emit(make_link_record(dictionary.get(word, {}), max_pairs))

    header = _MAGIC + struct.pack("<BH", version, num_words)
    footer = struct.pack("<H", checksum_acc)

    try:
        with open(filename, "wb") as f:
            f.write(header)
            f.write(payload)
            f.write(footer)
        print(f"Exported {num_words} words to {filename} ({fmt_name}).")
    except OSError as e:
        print(f"Export failed: {e}")


# ---------------------------------------------------------------------------
# Input loop
# ---------------------------------------------------------------------------

def process_input(line):
    """Handle one line — empty <enter> generates a reply, # reads a command, anything else teaches and replies."""
    line = line.strip()

    # Empty input — NIALL just talks, same as the original AMOS behaviour
    if not line:
        print(f"NIALL: {generate_reply()}")
        return

    if line.startswith("#"):
        # Commands are matched on the lowercased version so #SAVE and #save both work.
        # The original line (not lowercased) is used for filenames so paths aren't mangled.
        cmd = line.lower()
        if cmd == "#fresh":
            init_dict()
            print("Dictionary cleared.")
        elif cmd.startswith("#list"):
            do_list()
        elif cmd.startswith("#save"):
            parts = line.split(None, 1)
            do_save(parts[1] if len(parts) > 1 else DEFAULT_FILE)
        elif cmd.startswith("#load"):
            parts = line.split(None, 1)
            do_load(parts[1] if len(parts) > 1 else DEFAULT_FILE)
        elif cmd.startswith("#exportcpm"):
            parts = line.split(None, 1)
            do_export(parts[1] if len(parts) > 1 else "NIALL.DAT", nabu=False)
        elif cmd.startswith("#export"):
            parts = line.split(None, 1)
            do_export(parts[1] if len(parts) > 1 else "NIALL.DAT", nabu=True)
        elif cmd == "#help":
            print("Commands:")
            print("  #fresh              Clear the dictionary")
            print("  #list               Show the dictionary")
            print("  #save [file]        Save to JSON        (default: niall.json)")
            print("  #load [file]        Load any format     (default: niall.json)")
            print("  #export [file]      Export v5 NABU bin  (default: NIALL.DAT)")
            print("  #exportcpm [file]   Export v4 CP/M bin  (default: NIALL.DAT)")
            print("  #quit               Exit")
        elif cmd == "#quit":
            print("Bye.")
            sys.exit(0)
        else:
            print(f"Unknown command: {line}")
        return

    # Split sentences on full stops and commas (commas = sentence separators, matching AMOS/C)
    sentences = re.split(r'[.,]+', line)
    learned = False
    for sentence in sentences:
        sentence = sentence.strip()
        if sentence:
            learn_sentence(sentence)
            learned = True

    if learned:
        print(f"NIALL: {generate_reply()}")


def main():
    print(f"NIALL v{VERSION} — Non Intelligent (AMOS) Language Learner")
    print("Python port by Intangybles of the 1990 AMOS BASIC chatbot by Matthew Peck.")
    print("Type #help for commands.\n")

    while True:
        try:
            line = input("USER: ")
        except (EOFError, KeyboardInterrupt):
            print("\nBye.")
            break
        process_input(line)


if __name__ == "__main__":
    main()

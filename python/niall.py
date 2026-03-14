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
    #fresh          Clear the dictionary
    #list           Show the dictionary
    #save [file]    Save dictionary to JSON  (default: niall.json)
    #load [file]    Load dictionary from JSON (default: niall.json)
    #help           Show this command list
    #quit           Exit
    (any text)      Teach NIALL a sentence and get a reply
"""

import json
import os
import random
import re
import sys

VERSION      = "1.0"
DEFAULT_FILE = "niall.json"
MAX_REPLY    = 100      # max words in a generated reply — stops infinit loops

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
    Read a previously saved JSON file back into the dictionary.
    Replaces whatever is currently in memory — same as the C version.
    """
    global dictionary
    if not os.path.exists(filename):
        print(f"File not found: {filename}")
        return
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
        elif cmd == "#help":
            print("Commands:")
            print("  #fresh          Clear the dictionary")
            print("  #list           Show the dictionary")
            print("  #save [file]    Save to JSON (default: niall.json)")
            print("  #load [file]    Load from JSON (default: niall.json)")
            print("  #quit           Exit")
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

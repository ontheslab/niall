#!/usr/bin/env python3
"""
nialled.py — NIALL Dictionary Editor
Interactive editor for NIALL Markov chain dictionaries.

Loads any NIALL save format (via niallconv.py — must be in the same folder).
All edits are kept in memory until you save or export.

Usage
  python nialled.py [file]

Commands
  list [filter]             List all words (optional substring filter)
  show <word|#>             Show transitions out + incoming links
  chain <word|#>            Walk the most-likely Markov path
  dead                      Words with no outgoing transitions (dead ends)
  orphans                   Words nothing transitions to (unreachable)
  stats                     Dictionary statistics summary
  set <word|#> <to|#> <n>   Set/update a transition count
  del <word|#> <to|#>       Remove a transition
  add <word>                Add a new empty word
  remove <word|#>           Remove a word and all references to it
  rename <old|#> <new>      Rename a word everywhere it appears
  merge <src|#> <dst|#>     Fold src into dst, remove src
  prune <min>               Drop all transitions with count < min
  recount                   Validate all counts
  load [file]               Load any NIALL format (replaces current dict)
  save [file]               Save to JSON
  export [file]             Export to v4 binary (NIALL.DAT)
  help                      Show this list
  quit                      Exit (warns if unsaved changes)
"""

import json
import os
import re
import sys

# ---------------------------------------------------------------------------
# Import niallconv from the same directory for multi-format load/export
# ---------------------------------------------------------------------------

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

try:
    from niallconv import load_any, write_json, write_binary, START, END
    _CONV_AVAILABLE = True
except ImportError:
    _CONV_AVAILABLE = False
    START = "__START__"
    END   = "__END__"

# ---------------------------------------------------------------------------
# State
# ---------------------------------------------------------------------------

dictionary = {}       # word -> {successor: count, ...}
_cur_file  = None     # last loaded / saved path
_dirty     = False    # unsaved changes flag
PAGE_SIZE  = 24       # rows per page in paginated output

# ---------------------------------------------------------------------------
# Word index helpers
# ---------------------------------------------------------------------------

def _real_words():
    """Alphabetically sorted list of real words (not START/END)."""
    return sorted(w for w in dictionary if w not in (START, END))


def _display_index(word):
    """Return the display index for a word (0 = start token, 1..N = real words)."""
    if word == START:
        return 0
    words = _real_words()
    try:
        return words.index(word) + 1
    except ValueError:
        return None


def _resolve(token):
    """
    Resolve a token to a word name.
    Accepts: bare integer, #N, or word text (case-insensitive).
    Returns None and prints an error if not found.
    """
    token = token.strip()
    bare  = token.lstrip('#')
    if bare.isdigit():
        idx = int(bare)
        if idx == 0:
            return START
        words = _real_words()
        if 1 <= idx <= len(words):
            return words[idx - 1]
        print(f"  Index {idx} out of range (0..{len(words)}).")
        return None
    if token in dictionary:
        return token
    lower = token.lower()
    for w in dictionary:
        if w.lower() == lower:
            return w
    print(f"  Word not found: '{token}'")
    return None


def _display_name(word):
    """Human-readable label for a word (including sentinels)."""
    if word == START:
        return "(start)"
    if word == END:
        return "(end)"
    return word


def _incoming(word):
    """List of (src_word, count) that transition to word, sorted by count desc."""
    result = []
    for src, succs in dictionary.items():
        if word in succs:
            result.append((src, succs[word]))
    result.sort(key=lambda x: x[1], reverse=True)
    return result


def _mark_dirty():
    global _dirty
    _dirty = True

# ---------------------------------------------------------------------------
# Paginated output
# ---------------------------------------------------------------------------

def _page(lines):
    """Print a list of lines, pausing every PAGE_SIZE rows."""
    for i, line in enumerate(lines):
        print(line)
        if PAGE_SIZE > 0 and (i + 1) % PAGE_SIZE == 0 and (i + 1) < len(lines):
            try:
                ans = input(f"  -- {i + 1}/{len(lines)} lines -- Enter to continue, q to stop: ").strip().lower()
            except (EOFError, KeyboardInterrupt):
                print()
                break
            if ans == 'q':
                break

# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_list(args):
    filt  = args[0].lower() if args else ""
    words = _real_words()
    lines = []
    lines.append(f"  {'#':<5} {'Word':<24} {'Total':>7} {'Pairs':>6}")
    lines.append("  " + "-" * 46)

    # Start token
    if not filt or filt in "(start)":
        succs = dictionary.get(START, {})
        total = sum(succs.values())
        pairs = len(succs)
        lines.append(f"  {0:<5} {'(start)':<24} {total:>7} {pairs:>6}")

    shown = 0
    for i, word in enumerate(words, 1):
        if filt and filt not in word.lower():
            continue
        succs   = dictionary.get(word, {})
        total   = sum(succs.values())
        pairs   = sum(1 for s in succs if s != END)
        display = word if len(word) <= 24 else word[:23] + "+"
        lines.append(f"  {i:<5} {display:<24} {total:>7} {pairs:>6}")
        shown += 1

    lines.append(f"\n  {shown} word(s)" + (f" matching '{filt}'" if filt else "") + ".")
    _page(lines)


def cmd_show(args):
    if not args:
        print("  Usage: show <word|#>")
        return
    word = _resolve(args[0])
    if word is None:
        return

    succs = dictionary.get(word, {})
    total = sum(succs.values())
    idx   = _display_index(word)

    print(f"\n  Word  : {_display_name(word)}  (#{idx})")
    print(f"  Total : {total} transition(s) across {len(succs)} target(s)\n")

    if succs:
        lines = []
        lines.append(f"  {'#':<5} {'Target':<24} {'Count':>6}  {'Pct':>6}")
        lines.append("  " + "-" * 46)
        for dst, cnt in sorted(succs.items(), key=lambda x: x[1], reverse=True):
            pct  = cnt / total * 100 if total else 0
            tidx = str(_display_index(dst)) if dst != END else "-"
            lines.append(f"  {tidx:<5} {_display_name(dst):<24} {cnt:>6}  {pct:>5.1f}%")
        _page(lines)
    else:
        print("  (no outgoing transitions)")

    inc = _incoming(word)
    if inc:
        preview = ", ".join(f"{_display_name(s)}({c})" for s, c in inc[:8])
        suffix  = f" +{len(inc) - 8} more" if len(inc) > 8 else ""
        print(f"\n  Incoming: {preview}{suffix}")
    else:
        print(f"\n  Incoming: (nothing transitions here)")
    print()


def cmd_chain(args):
    if not args:
        print("  Usage: chain <word|#>")
        return
    word = _resolve(args[0])
    if word is None:
        return

    path    = []
    current = word
    seen    = set()

    while current != END and len(path) < 30:
        if current in seen:
            path.append(f"{_display_name(current)}[loop]")
            break
        seen.add(current)
        path.append(_display_name(current))
        succs = dictionary.get(current, {})
        if not succs:
            break
        current = max(succs.items(), key=lambda x: x[1])[0]

    if current == END:
        path.append("(end)")

    print(f"\n  {' -> '.join(path)}\n")


def cmd_dead(args):
    dead = [w for w in _real_words() if not dictionary.get(w)]
    if not dead:
        print("  No dead ends — all words have outgoing transitions.")
        return
    print(f"  {len(dead)} dead end(s):\n")
    _page([f"  #{_display_index(w):<5} {w}" for w in dead])
    print()


def cmd_orphans(args):
    targets = set()
    for succs in dictionary.values():
        targets.update(succs.keys())
    orphans = [w for w in _real_words() if w not in targets]
    if not orphans:
        print("  No orphans — all words are reachable as transition targets.")
        return
    print(f"  {len(orphans)} orphan(s) — nothing transitions to these words:\n")
    _page([f"  #{_display_index(w):<5} {w}" for w in orphans])
    print()


def cmd_stats(args):
    words   = _real_words()
    n_words = len(words)
    n_pairs = sum(len(s) for s in dictionary.values())
    n_total = sum(sum(s.values()) for s in dictionary.values())
    dead    = sum(1 for w in words if not dictionary.get(w))
    targets = set()
    for succs in dictionary.values():
        targets.update(succs.keys())
    orphans = sum(1 for w in words if w not in targets)

    top = sorted(words, key=lambda w: sum(dictionary.get(w, {}).values()), reverse=True)[:5]

    print(f"\n  Words         : {n_words}")
    print(f"  Unique pairs  : {n_pairs}  (distinct word->word transitions)")
    print(f"  Total count   : {n_total}  (sum of all transition counts)")
    print(f"  Start pairs   : {len(dictionary.get(START, {}))}")
    print(f"  Dead ends     : {dead}  (words with no outgoing transitions)")
    print(f"  Orphans       : {orphans}  (words no transition points to)")
    if top:
        print(f"  Most active   : " +
              ", ".join(f"{w}({sum(dictionary.get(w, {}).values())})" for w in top))
    dirty_flag = "yes (unsaved changes)" if _dirty else "no"
    print(f"  Modified      : {dirty_flag}")
    if _cur_file:
        print(f"  File          : {_cur_file}")
    print()


def cmd_set(args):
    if len(args) < 3:
        print("  Usage: set <word|#> <target|#> <count>")
        return

    src = _resolve(args[0])
    if src is None:
        return

    # Allow 'end' / '(end)' / '__end__' as the target
    if args[1].lower().strip("()_") == "end":
        dst = END
    else:
        # Try to resolve; if not found, treat as a new word name to create
        bare = args[1].lstrip('#')
        if bare.isdigit():
            dst = _resolve(args[1])   # numeric index — must exist
            if dst is None:
                return
        else:
            dst = re.sub(r'[^a-z0-9]', '', args[1].lower())
            if not dst:
                print(f"  Invalid target word '{args[1]}'.")
                return
            if dst not in dictionary:
                dictionary[dst] = {}
                print(f"  Created new word: '{dst}'")

    try:
        count = int(args[2])
    except ValueError:
        print(f"  Count must be an integer, got '{args[2]}'.")
        return
    if count < 0:
        print("  Count must be >= 0.  Use 'del' to remove a transition.")
        return

    if src not in dictionary:
        dictionary[src] = {}

    old = dictionary[src].get(dst, 0)
    if count == 0:
        if dst in dictionary[src]:
            del dictionary[src][dst]
            _mark_dirty()
            print(f"  Removed: {_display_name(src)} -> {_display_name(dst)}  (was {old})")
        else:
            print(f"  No transition {_display_name(src)} -> {_display_name(dst)} to remove.")
        return

    dictionary[src][dst] = count
    _mark_dirty()
    action = "Updated" if old else "Added"
    old_str = f"  (was {old})" if old else ""
    print(f"  {action}: {_display_name(src)} -> {_display_name(dst)} = {count}{old_str}")


def cmd_del(args):
    if len(args) < 2:
        print("  Usage: del <word|#> <target|#>")
        return

    src = _resolve(args[0])
    if src is None:
        return
    if args[1].lower().strip("()_") == "end":
        dst = END
    else:
        dst = _resolve(args[1])
        if dst is None:
            return

    succs = dictionary.get(src, {})
    if dst not in succs:
        print(f"  No transition: {_display_name(src)} -> {_display_name(dst)}")
        return

    old = succs.pop(dst)
    _mark_dirty()
    print(f"  Removed: {_display_name(src)} -> {_display_name(dst)}  (was {old})")


def cmd_add(args):
    if not args:
        print("  Usage: add <word>")
        return
    word = re.sub(r'[^a-z0-9]', '', args[0].lower())
    if not word:
        print("  Word must contain at least one alphanumeric character.")
        return
    if word in dictionary:
        print(f"  '{word}' already exists (#{_display_index(word)}).")
        return
    dictionary[word] = {}
    _mark_dirty()
    print(f"  Added: '{word}'  (no transitions yet — use 'set' to add some)")


def cmd_remove(args):
    if not args:
        print("  Usage: remove <word|#>")
        return
    word = _resolve(args[0])
    if word is None:
        return
    if word == START:
        print("  Cannot remove the start token.")
        return

    refs = [(src, dictionary[src][word])
            for src in dictionary if word in dictionary.get(src, {})]
    for src, _ in refs:
        del dictionary[src][word]
    if word in dictionary:
        del dictionary[word]

    _mark_dirty()
    print(f"  Removed '{word}' and {len(refs)} reference(s) to it.")


def cmd_rename(args):
    if len(args) < 2:
        print("  Usage: rename <old|#> <new>")
        return
    old_word = _resolve(args[0])
    if old_word is None:
        return
    if old_word == START:
        print("  Cannot rename the start token.")
        return
    new_word = re.sub(r'[^a-z0-9]', '', args[1].lower())
    if not new_word:
        print("  New name must contain at least one alphanumeric character.")
        return
    if new_word in dictionary:
        print(f"  '{new_word}' already exists.  Use 'merge' to combine two words.")
        return

    dictionary[new_word] = dictionary.pop(old_word)

    refs = 0
    for succs in dictionary.values():
        if old_word in succs:
            succs[new_word] = succs.pop(old_word)
            refs += 1

    _mark_dirty()
    print(f"  Renamed '{old_word}' -> '{new_word}', updated {refs} reference(s).")


def cmd_merge(args):
    if len(args) < 2:
        print("  Usage: merge <src|#> <dst|#>")
        return
    src = _resolve(args[0])
    dst = _resolve(args[1])
    if src is None or dst is None:
        return
    if src == dst:
        print("  Source and destination are the same word.")
        return
    if src == START:
        print("  Cannot merge the start token into another word.")
        return

    # Merge outgoing transitions from src into dst
    if dst not in dictionary:
        dictionary[dst] = {}
    for target, cnt in dictionary.get(src, {}).items():
        effective = dst if target == src else target   # self-loop → dst
        dictionary[dst][effective] = dictionary[dst].get(effective, 0) + cnt

    # Retarget all incoming references from src to dst
    refs = 0
    for word, succs in dictionary.items():
        if src in succs and word != src:
            cnt = succs.pop(src)
            succs[dst] = succs.get(dst, 0) + cnt
            refs += 1

    del dictionary[src]
    _mark_dirty()
    print(f"  Merged '{_display_name(src)}' into '{_display_name(dst)}', "
          f"retargeted {refs} incoming reference(s).")


def cmd_prune(args):
    if not args:
        print("  Usage: prune <min_count>")
        return
    try:
        threshold = int(args[0])
    except ValueError:
        print(f"  min_count must be an integer, got '{args[0]}'.")
        return
    if threshold < 1:
        print("  min_count must be >= 1.")
        return

    removed = 0
    for succs in dictionary.values():
        to_drop = [dst for dst, cnt in succs.items() if cnt < threshold]
        for dst in to_drop:
            del succs[dst]
            removed += 1

    if removed:
        _mark_dirty()
        print(f"  Pruned {removed} transition(s) with count < {threshold}.")
    else:
        print(f"  Nothing to prune — all transitions have count >= {threshold}.")


def cmd_recount(args):
    issues = 0
    for word, succs in dictionary.items():
        for dst, cnt in list(succs.items()):
            if not isinstance(cnt, int) or cnt <= 0:
                print(f"  Bad count: {_display_name(word)} -> {_display_name(dst)} = {cnt!r}")
                issues += 1
    n = sum(1 for w in dictionary if w not in (START, END))
    if issues == 0:
        print(f"  All counts valid.  ({n} words, {sum(len(s) for s in dictionary.values())} pairs checked)")
    else:
        print(f"  {issues} issue(s) found.  Use 'set' to fix or 'del' to remove bad pairs.")


def cmd_load(args):
    global dictionary, _cur_file, _dirty
    if _dirty:
        try:
            ans = input("  Unsaved changes — load anyway? [y/N] ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            print()
            return
        if ans != 'y':
            return

    path = args[0] if args else (_cur_file or "niall.json")
    if not os.path.exists(path):
        print(f"  File not found: {path}")
        return

    if not _CONV_AVAILABLE:
        # Fallback: JSON only
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
            if "dictionary" not in data:
                print("  Not a valid niall.json — 'dictionary' key missing.")
                return
            dictionary = data["dictionary"]
            _cur_file  = path
            _dirty     = False
            n = sum(1 for w in dictionary if w not in (START, END))
            print(f"  Loaded {path}  ({n} words, JSON)")
        except Exception as e:
            print(f"  Load failed: {e}")
        return

    # Use niallconv for universal format support
    try:
        fmt, new_dict, report, n = load_any(path)
        print(f"  {path}  ({fmt}, {n} words)")
        if report:
            print("  Auto-corrections applied:")
            report.print_all()
            print()
        dictionary = new_dict
        _cur_file  = path
        _dirty     = bool(report)
        if _dirty:
            print("  Dictionary marked as modified (auto-corrections were applied).")
    except SystemExit:
        # load_any calls sys.exit on fatal errors — catch to stay in the REPL
        pass


def cmd_save(args):
    global _cur_file, _dirty
    path = args[0] if args else (_cur_file or "niall.json")
    if not path.lower().endswith(".json"):
        path = os.path.splitext(path)[0] + ".json"
    try:
        if _CONV_AVAILABLE:
            write_json(dictionary, path)
        else:
            with open(path, "w", encoding="utf-8") as f:
                json.dump({"version": 1, "dictionary": dictionary}, f, indent=2, sort_keys=True)
        _cur_file = path
        _dirty    = False
        n = sum(1 for w in dictionary if w not in (START, END))
        print(f"  Saved: {path}  ({n} words)")
    except Exception as e:
        print(f"  Save failed: {e}")


def cmd_export(args):
    if not _CONV_AVAILABLE:
        print("  niallconv.py not found — cannot export binary.")
        return
    path = args[0] if args else "NIALL.DAT"
    if not path.upper().endswith(".DAT"):
        path = os.path.splitext(path)[0] + ".DAT"
    try:
        n, checksum = write_binary(dictionary, path)
        print(f"  Exported: {path}  ({n} words, checksum {checksum:#06x})")
    except Exception as e:
        print(f"  Export failed: {e}")


def cmd_help(args):
    print(__doc__)


# ---------------------------------------------------------------------------
# Command dispatch table
# ---------------------------------------------------------------------------

COMMANDS = {
    "list":    cmd_list,
    "ls":      cmd_list,
    "show":    cmd_show,
    "s":       cmd_show,
    "chain":   cmd_chain,
    "dead":    cmd_dead,
    "orphans": cmd_orphans,
    "stats":   cmd_stats,
    "set":     cmd_set,
    "del":     cmd_del,
    "delete":  cmd_del,
    "add":     cmd_add,
    "remove":  cmd_remove,
    "rm":      cmd_remove,
    "rename":  cmd_rename,
    "merge":   cmd_merge,
    "prune":   cmd_prune,
    "recount": cmd_recount,
    "load":    cmd_load,
    "save":    cmd_save,
    "export":  cmd_export,
    "help":    cmd_help,
    "?":       cmd_help,
}

# ---------------------------------------------------------------------------
# Prompt and main loop
# ---------------------------------------------------------------------------

def _prompt():
    n    = sum(1 for w in dictionary if w not in (START, END))
    flag = "*" if _dirty else " "
    name = os.path.basename(_cur_file) if _cur_file else "no file"
    return f"nialled{flag}[{name} | {n}w]> "


def main():
    print("nialled — NIALL Dictionary Editor  (type 'help' for commands)\n")

    if len(sys.argv) > 1:
        path = sys.argv[1]
        if os.path.exists(path):
            cmd_load([path])
        else:
            print(f"  File not found: {path}  (starting with empty dictionary)")
        print()

    while True:
        try:
            line = input(_prompt()).strip()
        except (EOFError, KeyboardInterrupt):
            print()
            if not _quit_check():
                continue
            break

        if not line:
            continue

        parts = line.split()
        cmd   = parts[0].lower()
        args  = parts[1:]

        if cmd in ("quit", "exit", "q"):
            if _quit_check():
                break
            continue

        if cmd in COMMANDS:
            try:
                COMMANDS[cmd](args)
            except Exception as e:
                print(f"  Error: {e}")
        else:
            matches = sorted(set(c for c in COMMANDS if c.startswith(cmd)))
            if matches:
                print(f"  Unknown command '{cmd}'.  Did you mean: {', '.join(matches)}?")
            else:
                print(f"  Unknown command '{cmd}'.  Type 'help' for a list.")

    print("Bye.")


def _quit_check():
    """Return True if it is safe to quit (no unsaved changes, or user confirms)."""
    global _dirty
    if not _dirty:
        return True
    try:
        ans = input("  Unsaved changes — quit anyway? [y/N] ").strip().lower()
    except (EOFError, KeyboardInterrupt):
        print()
        return True
    return ans == 'y'


if __name__ == "__main__":
    main()

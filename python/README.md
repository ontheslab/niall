# NIALL — Python Port

Python port of the 1990 AMOS BASIC Markov chain chatbot by Matthew Peck.
Faithful to the original algorithm — weighted word transition learning and reply generation — with no external dependencies.

## Requirements

Python 3.6 or later. No packages required — stdlib only.

## Usage

```
python niall.py
```

Type sentences to teach NIALL. It learns word transitions and generates replies by walking the Markov chain.

## Commands

| Command          | Description                              |
|------------------|------------------------------------------|
| `#fresh`         | Clear the dictionary                     |
| `#list`          | Show dictionary with totals and pairs    |
| `#save [file]`   | Save to JSON (default: `niall.json`)     |
| `#load [file]`   | Load from JSON (default: `niall.json`)   |
| `#help`          | Show command list                        |
| `#quit`          | Exit                                     |
| (any text)       | Teach NIALL and get a reply              |

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

There is no direct converter between the Python JSON format and the CP/M v4 binary format — they are intended for different environments.

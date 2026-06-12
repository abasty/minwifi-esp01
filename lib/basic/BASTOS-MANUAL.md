# BASTOS Language Manual

BASTOS is a BASIC dialect designed for the ESP01 board. Programs are made of
numbered lines executed in order; line 0, or no line number, is interpreted
immediately (interactive mode).

```basic
10 PRINT "Hello, World!"
20 PAUSE 1000
30 GOTO 10
```

---

## Program commands

| Command | Description |
|---------|-------------|
| `RUN` | Run from first line |
| `RUN linenumber` | Run from a specific line |
| `RUN "file.bas"` | Load and run ASCII program |
| `RUN "file.bst", linenumber` | Load and run binary program from a specific line |
| `LIST` | List 20 lines from current position |
| `LIST linenum` | List 20 lines starting from `linenum` |
| `LIST linenum, count` | List `count` lines from `linenum` |
| `NEW` | Delete all program lines and vriables |
| `CLEAR` / `END` | Clear variables and stop execution |
| `STOP` | Pause execution |
| `CONT` | Resume after `STOP` |
| `SAVE "file.bas"` | Save program as ASCII |
| `SAVE "file.bst"` | Save program as binary |
| `SAVE "file.var"` | Save variables only |
| `LOAD "file.bas"` | Load ASCII program |
| `LOAD "file.bst"` | Load binary program |
| `LOAD "file.var"` | Load variables only |
| `ERASE "file"` | Delete file |
| `CAT` | List files |
| `FREE` | Display memory usage |
| `RESET` | Reset system |
| `BASTOS` | Display version info and init default screen attributes |

---

## PRINT / INPUT

### PRINT

Prints expressions to the screen, followed by a newline. `?` is a shorthand for `PRINT`.

```basic
PRINT expr [, expr ...]      ' Space between items
PRINT expr ; expr            ' No space between items
PRINT                        ' Print empty line
? expr                       ' Same as PRINT expr
```

Use a trailing `;` to suppress the final newline:

```basic
PRINT "Enter value: ";
INPUT n
```

Numeric expressions and string variables can be mixed freely:

```basic
PRINT "Result: "; a * 2
PRINT "Name: " n$ ", age: " a
```

Position output with `AT line, col`:

```basic
AT 5, 10; "Hello"
```

Redirect all output to a string variable:

```basic
OUTPUT START m$
  CLS
  AT 10, 13; "*** METEOR ***"
OUTPUT STOP
PRINT m$
```

### INPUT

Reads a value from the keyboard and assigns it to a variable.

```basic
INPUT variable
INPUT "prompt", variable
```

- For a numeric variable, expects a number.
- For a string variable (`$`), reads until Enter.
- After input, `VKEY` holds the validation key code.

```basic
10 INPUT "Enter your name: ", n$
20 PRINT "Hello, " n$
```

Non-blocking key read (no wait):

```basic
k$ = INKEY$
IF k$ <> "" THEN PRINT "Pressed: " k$
```

---

## Types, variables, computations

### Types

| Type | Suffix | Storage |
|------|--------|---------|
| Number | none | 32-bit float |
| String | `$` | Variable length |

Number literals can be written in decimal or hexadecimal:

```basic
a = 255
a = 0xff
a = 0x1f
```

String literals support escape sequences:

| Escape | Description |
|--------|-------------|
| `\n` | Newline |
| `\r` | Carriage return |
| `\e` | Escape (0x1B) |
| `\xNN` | Hexadecimal byte |

```basic
a$ = "hello\n"
b$ = "\e[2J"          ' ANSI clear screen
c$ = "\x1b\x41\x42"  ' Escape + 'A' + 'B'
```

### Supported UTF-8 characters (Minitel conversion)

When loading an ASCII `.bas` file, BASTOS converts a limited set of UTF-8
characters into Minitel sequences (SS2 prefix, code `0x19`).

| UTF-8 | Unicode | Generated Minitel sequence |
|-------|---------|----------------------------|
| à | U+00E0 | `\x19Aa` |
| è | U+00E8 | `\x19Ae` |
| ù | U+00F9 | `\x19Au` |
| é | U+00E9 | `\x19Be` |
| â | U+00E2 | `\x19Ca` |
| ê | U+00EA | `\x19Ce` |
| î | U+00EE | `\x19Ci` |
| ô | U+00F4 | `\x19Co` |
| û | U+00FB | `\x19Cu` |
| ä | U+00E4 | `\x19Ha` |
| ë | U+00EB | `\x19He` |
| ï | U+00EF | `\x19Hi` |
| ö | U+00F6 | `\x19Ho` |
| ü | U+00FC | `\x19Hu` |
| ç | U+00E7 | `\x19Kc` |
| Ç | U+00C7 | `\x19KC` |
| ß | U+00DF | `\x19\x7B` |
| £ | U+00A3 | `\x19\x23` |
| § | U+00A7 | `\x19\x27` |
| ° | U+00B0 | `\x19\x30` |
| ± | U+00B1 | `\x19\x31` |
| ÷ | U+00F7 | `\x19\x38` |
| ¼ | U+00BC | `\x19\x34` |
| ½ | U+00BD | `\x19\x35` |
| ¾ | U+00BE | `\x19\x36` |
| Œ | U+0152 | `\x19\x6A` |
| œ | U+0153 | `\x19\x7A` |

Any other UTF-8 characters are not converted and are kept unchanged.

### Variable names

- Numeric: one or more characters, e.g. `x`, `count`, `total`
- String: name ending with `$`, e.g. `name$`, `buf$`
- Keywords are case-insensitive; string content is case-sensitive.

```basic
count = 3.14
name$ = "hello"
```

Assignment with or without `LET`:

```basic
LET x = 42
x = 42
```

### Arrays

Declare with `DIM` before use. Arrays are **1-indexed**.

```basic
DIM a(10)             ' 1-D array of 10 numbers
DIM m(5, 5)           ' 2-D array
DIM s$(10, 25)        ' Array of strings, up to 25 chars each
```

Access:

```basic
a(3) = 99
PRINT a(3)
m(2, 4) = 1.5
s$(1) = "first"
```

### String operations

Parentheses are optional for all functions; use them only for grouping.

| Operation | Syntax | Example |
|-----------|--------|---------|
| Concatenation | `a$ + b$` | `"hello" + " world"` |
| Length | `LEN s$` | `LEN "abc"` → `3` |
| Substring read | `s$(start TO end)` | `a$(11 TO 13)` |
| Substring write | `s$(start TO end) = "..."` | `a$(1 TO 3) = "XYZ"` |
| ASCII code | `CODE s$` | `CODE "A"` → `65` |
| Character | `CHR$ n` | `CHR$ 65` → `"A"` |
| To number | `VAL s$` | `VAL "3.14"` → `3.14` |
| To string | `STR$ n` | `STR$ 42` → `"42"` |
| Find | `INDEX s1$, s2$` | `INDEX "hello", "ll"` → `3` |
| Find from pos | `INDEX s1$, s2$, start` | |
| Repeat | `REP n, s$` | `REP 3, "-"` → `"---"` |

```basic
PRINT LEN a$
PRINT CODE k$
z$ = CHR$ 0
ia$ = CHR$(CODE a$ & 223)   ' parentheses for grouping only
```

Substring indices use the `TO` keyword. `start` defaults to `1`, `end` defaults to `LEN s$`:

```basic
a$ = "Alice and Bob"
PRINT a$(11 TO 13)   ' "Bob"
PRINT a$(TO 5)       ' "Alice"  (start omitted → 1)
PRINT a$(7 TO)       ' "d Bob"  (end omitted → LEN a$)
PRINT a$(TO)         ' whole string
```

### Arithmetic operators

| Operator | Description |
|----------|-------------|
| `*` `/` `%` | Multiply, divide, modulo |
| `+` `-` | Add, subtract |
| `&` | Bitwise AND |
| `\|` | Bitwise OR |

### Comparison operators

| Operator | Meaning |
|----------|---------|
| `=` | Equal |
| `<>` | Not equal |
| `<` `>` | Less / greater than |
| `<=` `>=` | Less / greater or equal |

Result is `1` (true) or `0` (false).

### Logical operators

```basic
IF a > 0 AND b > 0 THEN PRINT "both positive"
IF a = 0 OR b = 0 THEN PRINT "one is zero"
IF NOT a THEN PRINT "a is zero"
```

### Math functions

Parentheses are optional; use them only for grouping sub-expressions.

| Function | Description |
|----------|-------------|
| `ABS x` | Absolute value |
| `INT x` | Truncate to integer |
| `SGN x` | Sign: -1, 0, or 1 |
| `SQR x` | Square root |
| `SIN x` `COS x` `TAN x` | Trigonometry |
| `ASN x` `ACS x` `ATN x` | Inverse trig |
| `EXP x` | e^x |
| `LN x` | Natural logarithm |
| `RND` | Random float 0.0–1.0 |
| `PI` | 3.1415926536 |

```basic
PRINT ABS -5
PRINT SQR 2
PRINT INT(a / b)    ' parentheses for grouping
```

---

## Control structures

### IF / THEN

```basic
IF expression THEN linenumber
IF expression THEN statement
```

```basic
10 INPUT "x: ", x
20 IF x < 0 THEN PRINT "negative"
30 IF x = 0 THEN 10
40 PRINT "positive"
```

### FOR / NEXT

```basic
FOR var = start TO end
FOR var = start TO end STEP step
NEXT var
```

- `var` must be a single letter `A`–`Z`.
- Negative `STEP` counts down.
- Loops may be nested.

```basic
10 FOR i = 1 TO 5
20   PRINT i
30 NEXT i

40 FOR i = 10 TO 1 STEP -1
50   PRINT i
60 NEXT i
```

### GOTO

```basic
GOTO linenumber
```

```basic
10 PRINT "loop"
20 GOTO 10
```

### GOSUB / RETURN

```basic
GOSUB linenumber    ' Call subroutine
RETURN              ' Return to caller
```

Up to 32 nested calls.

```basic
10 GOSUB 1000
20 END

1000 PRINT "In subroutine"
1010 RETURN
```

### PAUSE

```basic
PAUSE milliseconds
```

```basic
PRINT "Wait 2 seconds..."
PAUSE 2000
PRINT "Done"
```

---

## Database

BASTOS provides a simple key/value store organized in numbered sets.

```basic
GET set              ' Returns all keys of the set as a string
GET set, "key"       ' Returns the value associated with a key
PUT set, "key", "value"  ' Store or update a key/value pair
DB LIST n            ' List all entries in set n
DB ERASE n, "key"    ' Delete entry by key from set n
```

Sets are numbered starting from 0. Keys and values are strings. WiFi, Minitel, and FTP connections use specific sets to persist their configuration.

```basic
PUT 1, "city", "Paris"
PRINT GET(1, "city")    ' "Paris"
```

`GET set` returns all keys joined by `\n`. Use `INDEX` to iterate over them:

```basic
10 keys$ = GET 1
20 pos = 1
30 nl = INDEX(keys$, "\n", pos)
40 IF nl = 0 THEN END
50 key$ = keys$(pos TO nl - 1)
60 PRINT key$ " = " GET(1, key$)
70 pos = nl + 1
80 GOTO 30
```

---

## Network

### WiFi

```basic
WIFI SCAN                    ' Scan available access points
WIFI "ssid"                  ' Connect by SSID (prompts for password if needed)
WIFI START "ssid"            ' Same as above (START is the default action)
WIFI n                       ' Connect to scanned network number n
WIFI LIST                    ' List saved networks
WIFI STATUS                  ' Show current connection status
WIFI ERASE "ssid"            ' Remove a saved network
WIFI STOP                    ' Disconnect
```

After `WIFI SCAN`, networks are numbered; use `WIFI n` to connect by index.

### URN format

Network connections are identified by a URN whose parts are separated by `:`:

```
protocol:host:port[:path[:login[:password]]]
```

For `ftp`, `login` defaults to `anonymous` and `password` to `pat@frites.be` if omitted.

| Protocol | Description |
|----------|-------------|
| `tcp` | Raw TCP socket |
| `ws` | WebSocket |
| `ftp` | FTP |

Examples:

```
tcp:go.minipavi.fr:516
ws:3611.re:80:/ws
ftp:abasty-retro.fr:2121:bastos
ftp:files.example.com:21:/pub:myuser:mypassword
ws:mntl.joher.com:2018:/?echo
```

### Minitel

Connect to a Minitel server (Videotex terminal emulation) and save it by name.

```basic
MINITEL "name", "urn"        ' Connect and save under "name"
MINITEL START "name", "urn"  ' Same as above (START is the default action)
MINITEL "name"               ' Reconnect to a saved connection
MINITEL LIST                 ' List saved Minitel connections
MINITEL ERASE "name"         ' Remove a saved connection
```

Examples:

```basic
MINITEL "minipavi", "tcp:go.minipavi.fr:516"
MINITEL "3615", "ws:3615co.de:80:/ws"
MINITEL "3615"               ' reconnect using saved name
```

### FTP

```basic
FTP "name", "urn"            ' Connect and save under "name"
FTP START "name", "urn"      ' Same as above (START is the default action)
FTP "name"                   ' Reconnect to a saved connection
FTP LIST                     ' List saved FTP connections
FTP STATUS                   ' Show current FTP status
FTP PUT "file"               ' Upload a file (same name locally and remotely)
FTP GET "file"               ' Download a file (same name locally and remotely)
FTP CAT                      ' List remote files
FTP ERASE "name"             ' Remove a saved connection
FTP STOP                     ' Disconnect
```

Example session:

```basic
10 FTP "bastos", "ftp:abasty-retro.fr:2121:bastos"
20 FTP STATUS
30 FTP GET "snake.bas"
40 FTP STOP
```

---

## Display and TTY

TTY functions return a string containing the corresponding escape sequence. Used as a statement, they behave as `PRINT ...; ` (output the sequence with no trailing newline). Used as an expression, they can be assigned or embedded in a string:

```basic
CLS                      ' statement: sends clear-screen sequence
c$ = CLS                 ' expression: stores the sequence in c$
PRINT INK(3) "hello"     ' inline: set color then print
m$ = AT(10,13) + "hi"   ' build a string with positioning
```

In Teletext mode (≤1), TTY functions emit Videotex sequences. In VT100 mode (≥2), they emit CSI sequences (`ESC [ ...`).

### Screen

```basic
CLS                  ' Clear screen
CLEOL                ' Clear to end of line
CURSOR n             ' 0=hide, 1=show cursor
BEEP                 ' Sound bell
MODE n               ' Screen mode: ≤1 = 40 cols Teletext, ≥2 = 80 cols VT100
```

### Cursor positioning

```basic
AT line, col; expr
```

Lines and columns are 1-indexed.

### Colors and attributes

```basic
INK color            ' Foreground color (0-7)
PAPER color          ' Background color (0-7); no effect in VT100 mode (≥2)
FLASH n              ' 0=off, 1=blinking
INVERSE n            ' 0=normal, 1=inverted
UNDERLINE n          ' 0=off, 1=underlined
```

In VT100 mode, `INK 7` enables bold/bright; `INK 0`–`6` disables it. `PAPER` has no effect.

### Graphics

```basic
PLOT x, y            ' Set pixel
UNPLOT x, y          ' Clear pixel
TEST(x, y)           ' Returns 1 if pixel set, 0 otherwise
```

### Speed

```basic
SLOW                 ' Normal speed (default)
FAST                 ' High speed
FAST2                ' Higher speed
```

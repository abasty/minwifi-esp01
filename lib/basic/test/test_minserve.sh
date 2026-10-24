#!/bin/bash
# Tests of the Minserve build (bin/bastos-minserve-linux-amd64): runs small
# BASIC programs and checks output, exit code and the state of the disk.
#
# Usage: ./test_minserve.sh [path/to/bastos-minserve-linux-amd64]
#        (or: make test_minserve)

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$(realpath "${1:-$HERE/bin/bastos-minserve-linux-amd64}")"
[[ -x "$BIN" ]] || { echo "$BIN not found: run 'make minserve' first" >&2; exit 2; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
GAME="$WORK/game"
mkdir -p "$GAME/sub"

pass=0
fail=0
ok() { pass=$((pass + 1)); echo "  [PASS] $1"; }
ko() { fail=$((fail + 1)); echo "  [FAIL] $1"; [[ -n "$2" ]] && echo "         $2"; }

# check "description" condition-exit-status [detail]
check() { if [[ "$2" == 0 ]]; then ok "$1"; else ko "$1" "$3"; fi; }

# run_prog DIR PROGRAM [INPUT [DELAY]]
# Runs the program with INPUT sent on its keyboard after DELAY seconds.
# Sets: CODE (exit status), OUT (screen, escape characters removed),
# ERR (standard error), MS (duration in milliseconds).
run_prog() {
    local dir="$1" prog="$2" input="$3" delay="${4:-0.3}"
    local fifo="$WORK/fifo"
    rm -f "$fifo"
    mkfifo "$fifo"
    # Keep the keyboard open: BASTOS stops when its input reaches EOF.
    { sleep 0.1; [[ -n "$input" ]] && { sleep "$delay"; printf '%b' "$input"; }; sleep 6; } >"$fifo" &
    local feeder=$!
    local t0=$(date +%s%N)
    (cd "$dir" && timeout 5 "$BIN" "$prog" <"$fifo" >"$WORK/out" 2>"$WORK/err")
    CODE=$?
    MS=$((($(date +%s%N) - t0) / 1000000))
    kill "$feeder" 2>/dev/null
    wait "$feeder" 2>/dev/null
    rm -f "$fifo"
    OUT="$(tr -d '\033' <"$WORK/out" | tr -c '[:print:]\n' ' ')"
    ERR="$(cat "$WORK/err")"
}

prog() { printf '%b' "$2" >"$GAME/$1"; }

echo "=== Start-up"
(cd "$GAME" && "$BIN" </dev/null >/dev/null 2>"$WORK/err")
check "no argument: exit code 1" $(($? == 1 ? 0 : 1))
check "no argument: usage on stderr" $([[ "$(cat "$WORK/err")" == usage:* ]] && echo 0 || echo 1)

run_prog "$GAME" nope.bas
check "missing program: exit code 1" $((CODE == 1 ? 0 : 1)) "code=$CODE"
check "missing program: message on stderr" $([[ "$ERR" == *"cannot load"* ]] && echo 0 || echo 1) "$ERR"

: >"$GAME/empty.bas"
run_prog "$GAME" empty.bas
check "empty program: exit code 1" $((CODE == 1 ? 0 : 1)) "code=$CODE"

run_prog "$WORK" nodir/x.bas
check "missing directory: exit code 1, message" $([[ $CODE == 1 && "$ERR" == *"cannot enter"* ]] && echo 0 || echo 1) "code=$CODE err=$ERR"

prog hello.bas '10 PRINT "Bonjour"\n20 END\n'
run_prog "$WORK" "game/hello.bas"
check "program given with a directory runs there" $([[ "$OUT" == *Bonjour* && $CODE == 0 ]] && echo 0 || echo 1) "code=$CODE out=$OUT"

echo "=== Run mode only"
for end in 'END' 'STOP' 'RESET' 'GOTO 999'; do
    prog end.bas "10 PRINT \"A\"\n20 $end\n30 PRINT \"never\"\n"
    run_prog "$GAME" end.bas
    check "$end: exits, code 0, no further output" \
        $([[ $CODE == 0 && "$OUT" == *A* && "$OUT" != *never* && "$OUT" != *Ready* && $MS -lt 2000 ]] && echo 0 || echo 1) \
        "code=$CODE ms=$MS out=$OUT"
done

prog err.bas '10 PRINT "A"\n20 LOAD "nope"\n30 PRINT "never"\n'
run_prog "$GAME" err.bas
check "runtime error: exit code 1, message shown" \
    $([[ $CODE == 1 && "$OUT" == *"On line 20"* && "$OUT" != *never* ]] && echo 0 || echo 1) "code=$CODE out=$OUT"

prog input.bas '10 INPUT "nom ",a$\n20 PRINT "salut " a$\n'
run_prog "$GAME" input.bas 'Zoe\r'
check "INPUT reads the keyboard" $([[ "$OUT" == *"salut Zoe"* && $CODE == 0 ]] && echo 0 || echo 1) "out=$OUT"

prog inkey.bas '10 a$=INKEY$\n20 IF a$="" THEN 10\n30 PRINT "touche " a$\n'
run_prog "$GAME" inkey.bas 'x'
check "INKEY\$ reads the keyboard" $([[ "$OUT" == *"touche x"* ]] && echo 0 || echo 1) "out=$OUT"

prog pause.bas '10 PRINT "A"\n20 PAUSE 20\n30 PRINT "B"\n'
run_prog "$GAME" pause.bas
check "PAUSE waits then goes on" $([[ "$OUT" == *A*B* && $CODE == 0 ]] && echo 0 || echo 1) "out=$OUT"

prog cmds.bas '10 NEW\n20 LIST\n30 EDIT 10\n40 BASTOS\n50 CONT\n60 PRINT "ok"\n'
run_prog "$GAME" cmds.bas
check "NEW, LIST, EDIT, BASTOS, CONT have no effect" \
    $([[ "$OUT" == *ok* && "$OUT" != *"PRINT"* && $CODE == 0 ]] && echo 0 || echo 1) "out=$OUT"

echo "=== Read-only disk"
rm -rf "$GAME/sub"; mkdir -p "$GAME/sub"
echo donnees >"$GAME/data.txt"
echo x >"$GAME/sub/x.txt"
prog w.bas '10 SAVE "nouveau"\n20 ERASE "data.txt"\n30 MD "newdir"\n40 RD "sub"\n50 MOVE "data.txt","moved.txt"\n60 PUT 1,"cle","valeur"\n70 PRINT GET(1,"cle")\n80 PRINT "fin"\n'
snapshot() { (cd "$GAME" && find . | sort && md5sum $(find . -type f | sort)); }
before="$(snapshot)"
run_prog "$GAME" w.bas
after="$(snapshot)"
check "SAVE, ERASE, MD, RD, MOVE, PUT: program goes on, code 0" \
    $([[ $CODE == 0 && "$OUT" == *valeur*fin* ]] && echo 0 || echo 1) "code=$CODE out=$OUT"
check "the disk is exactly as before" $([[ "$before" == "$after" ]] && echo 0 || echo 1) "$(diff <(echo "$before") <(echo "$after"))"
check "no autoload.db created" $([[ ! -e "$GAME/autoload.db" ]] && echo 0 || echo 1)

echo "autoexec" >"$GAME/data.txt"
printf '\x05\x00\x01\x02\x03\x04\x05' >"$GAME/autoload.db"
prog db.bas '10 PRINT "[" GET(255) "]"\n20 PRINT "[" GET(1) "]"\n'
run_prog "$GAME" db.bas
check "autoload.db is not read" $([[ "$OUT" == *"[]"*"[]"* ]] && echo 0 || echo 1) "out=$OUT"
rm -f "$GAME/autoload.db"

echo "=== No network"
prog net.bas '10 PRINT "debut"\n20 WIFI SCAN\n30 WIFI "maison"\n40 WIFI 2\n50 WIFI STATUS\n60 WIFI LIST\n70 WIFI ERASE "maison"\n80 WIFI STOP\n90 MINITEL "3615","tcp:127.0.0.1:9"\n100 MINITEL "3615"\n110 MINITEL LIST\n120 MINITEL ERASE "3615"\n130 FTP "b","ftp:127.0.0.1:9:x"\n140 FTP STATUS\n150 FTP GET "a.bas"\n160 FTP PUT "a.bas"\n170 FTP CAT\n180 FTP CAT "*.bas"\n190 FTP LIST\n200 FTP STOP\n210 FTP ERASE "b"\n220 IF 1=1 THEN WIFI SCAN ELSE WIFI STOP\n230 IF 1=2 THEN WIFI SCAN\n240 PRINT "fin"\n'
run_prog "$GAME" net.bas
check "WIFI, MINITEL, FTP do nothing and say nothing" \
    $([[ $CODE == 0 && "$OUT" == *debut*fin* && "$OUT" != *"Host network"* && "$OUT" != *rror* ]] && echo 0 || echo 1) "code=$CODE out=$OUT"
check "... and do not wait (no simulated scan)" $((MS < 1500 ? 0 : 1)) "ms=$MS"

echo "=== Paths stay in the program directory"
mkdir -p "$WORK/outside"
echo "SECRET" >"$WORK/secret.txt"
echo "in-sub" >"$GAME/sub/in.txt"
echo "ok-file" >"$GAME/ok.txt"
ln -sfn /etc "$GAME/link-out"
ln -sfn sub "$GAME/link-in"
ln -sfn ../secret.txt "$GAME/link-secret"
prog esc.bas '10 a$=FILE "ok.txt": PRINT "A=" LEN(a$)\n20 a$=FILE "sub/in.txt": PRINT "B=" LEN(a$)\n30 a$=FILE "link-in/in.txt": PRINT "C=" LEN(a$)\n40 a$=FILE "sub/../ok.txt": PRINT "D=" LEN(a$)\n50 a$=FILE "../secret.txt": PRINT "E=" LEN(a$)\n60 a$=FILE "/etc/hostname": PRINT "F=" LEN(a$)\n70 a$=FILE "link-out/hostname": PRINT "G=" LEN(a$)\n80 a$=FILE "link-secret": PRINT "H=" LEN(a$)\n90 CD "link-out"\n100 PRINT "never"\n'
run_prog "$GAME" esc.bas
check "files inside the directory are readable (also through a link inside)" \
    $([[ "$OUT" == *"A=8"* && "$OUT" == *"B=7"* && "$OUT" == *"C=7"* && "$OUT" == *"D=8"* ]] && echo 0 || echo 1) "out=$OUT"
check "../, absolute path and links leading outside read nothing" \
    $([[ "$OUT" == *"E=0"* && "$OUT" == *"F=0"* && "$OUT" == *"G=0"* && "$OUT" == *"H=0"* ]] && echo 0 || echo 1) "out=$OUT"
check "CD through a link leading outside is an error" \
    $([[ $CODE == 1 && "$OUT" != *never* ]] && echo 0 || echo 1) "code=$CODE out=$OUT"

prog up.bas '10 CD "sub"\n20 CD ".."\n30 PRINT "back"\n40 CD ".."\n50 PRINT "never"\n'
run_prog "$GAME" up.bas
check "CD .. stops at the program directory" \
    $([[ "$OUT" == *back* && "$OUT" != *never* && $CODE == 1 ]] && echo 0 || echo 1) "code=$CODE out=$OUT"

echo "=== SIGINT only reaches BASTOS"
prog loop.bas '10 GOTO 10\n'
cat >"$WORK/sig.sh" <<EOF
cd "$GAME"
sleep 20 & SIB=\$!
mkfifo "$WORK/ff2"; sleep 15 > "$WORK/ff2" & HOLD=\$!
"$BIN" loop.bas < "$WORK/ff2" > /dev/null 2>&1 & BP=\$!
sleep 1; kill -INT \$BP; sleep 1
kill -0 \$SIB 2>/dev/null && echo sibling-alive || echo sibling-dead
kill -0 \$BP 2>/dev/null && echo bastos-alive || echo bastos-stopped
kill \$SIB \$HOLD 2>/dev/null
EOF
sig="$(setsid -w bash "$WORK/sig.sh" 2>/dev/null)"
check "BASTOS stops on SIGINT" $([[ "$sig" == *bastos-stopped* ]] && echo 0 || echo 1) "$sig"
check "a process of the same group survives" $([[ "$sig" == *sibling-alive* ]] && echo 0 || echo 1) "$sig"

echo
echo "=== Results: $pass/$((pass + fail)) tests passed ==="
[[ $fail == 0 ]]

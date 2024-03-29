#!/bin/bash

TMPDIR="$(mktemp -d)"
trap 'rm -rf -- "$TMPDIR"' EXIT

KEYWORDS_FILE=${TMPDIR}/keywords

KEYWORDS_C=keywords.c-static
KEYWORDS_H=keywords.h

# Add keywords after existing ones to preserve load/save compatibility

cat >${KEYWORDS_FILE} <<EOF
abs
acs
asn
atn
bin
chr$
code
cos
exp
int
ln
pi
rnd
sgn
sin
sqr
tan
clear
new
cls
cat
erase
reset
load
save
list
let
print
input
inkey$
run
to
str$
at
ink
paper
and
or
not
if
goto
gosub
return
for
next
step
stop
cont
then
len
rem
cursor
fast
slow
dim
free
rand
pause
tab
scroll
plot
unplot
test
mode
minitel
telnet
peek
poke
usr
EOF

# Do not sort to preserve save/load compatibility
# sort -o ${KEYWORDS_FILE}{,}

function ord {
  printf %d "'$1"
}

echo "#include <stdint.h>" >${KEYWORDS_H}

index=0
while IFS= read -r keyword; do
    u_key=${keyword^^}
    echo ${u_key} >/dev/tty
    u_key=${u_key%$}
    echo "#define TOKEN_KEYWORD_${u_key} ((uint8_t) (${index} | 0b10000000))" >>${KEYWORDS_H}
    ((index++))
done < ${KEYWORDS_FILE}

echo "static const char *keywords =" >${KEYWORDS_C}
index=0
while IFS= read -r keyword; do
    u_key=${keyword^^}
    u_key_last="${u_key: -1}"
    u_key_but_last="${u_key%${u_key_last}}"
    u_key_last_code=$(ord ${u_key_last})
    ((e_key_last_code=${u_key_last_code}+128))
    echo -n "    \"${u_key_but_last}" >>${KEYWORDS_C}
    printf '""\\x%x"' ${e_key_last_code} >>${KEYWORDS_C}
    echo >>${KEYWORDS_C}
    ((index++))
done < ${KEYWORDS_FILE}
echo ";" >>${KEYWORDS_C}

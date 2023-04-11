#!/bin/bash

TMPDIR="$(mktemp -d)"
trap 'rm -rf -- "$TMPDIR"' EXIT

KEYWORDS_FILE=${TMPDIR}/keywords

KEYWORDS_C=keywords.inc
KEYWORDS_H=keywords.h

cat >${KEYWORDS_FILE} <<EOF
free
cats
reset
config
connect
clear
load
save
configopt
EOF

sort -o ${KEYWORDS_FILE}{,}

function ord {
  printf %d "'$1"
}

echo "#include <stdint.h>" >${KEYWORDS_H}

index=0
while IFS= read -r keyword; do
    u_key=${keyword^^}
    echo ${u_key} >/dev/tty
    echo "#define TOKEN_KEYWORD_${u_key} ((uint8_t) (${index} | 0b10000000))" >>${KEYWORDS_H}
    ((index++))
done < ${KEYWORDS_FILE}

echo -n "const char *keywords = \"" >${KEYWORDS_C}
index=0
while IFS= read -r keyword; do
    u_key=${keyword^^}
    u_key_last="${u_key: -1}"
    u_key_but_last="${u_key%${u_key_last}}"
    u_key_last_code=$(ord ${u_key_last})
    ((e_key_last_code=${u_key_last_code}+128))
    echo -n "${u_key_but_last}" >>${KEYWORDS_C}
    printf '""\\x%x""' ${e_key_last_code} >>${KEYWORDS_C}
    ((index++))
done < ${KEYWORDS_FILE}
echo "\";" >>${KEYWORDS_C}

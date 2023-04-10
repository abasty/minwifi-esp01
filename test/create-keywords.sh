#!/bin/bash

TMPDIR="$(mktemp -d)"
trap 'rm -rf -- "$TMPDIR"' EXIT

keywords_file=${TMPDIR}/keywords
cat >${keywords_file} <<EOF
free
cats
reset
config
connect
clear
EOF

sort -o ${keywords_file}{,}

function ord {
  printf %d "'$1"
}

index=0
while IFS= read -r keyword; do
    u_key=${keyword^^}
    echo ${u_key} >/dev/tty
    echo "#define TOKEN_KEYWORD_${u_key} ((uint8_t) (${index}))"
    ((index++))
done < ${keywords_file}

echo

echo -n "const char *keywords = \""
index=0
while IFS= read -r keyword; do
    u_key=${keyword^^}
    u_key_last="${u_key: -1}"
    u_key_but_last="${u_key%${u_key_last}}"
    u_key_last_code=$(ord ${u_key_last})
    ((e_key_last_code=${u_key_last_code}+128))
    echo -n "${u_key_but_last}"
    printf '""\\x%x""' ${e_key_last_code}
    ((index++))
done < ${keywords_file}
echo "\";"

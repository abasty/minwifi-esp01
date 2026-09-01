#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEBSOCAT_URL="https://github.com/vi/websocat/releases/latest/download/websocat.x86_64-unknown-linux-musl"
WEBSOCAT_LOCAL="${SCRIPT_DIR}/websocat"
WS_ADDR="127.0.0.1:1967"

if command -v websocat &> /dev/null; then
    WEBSOCAT="$(command -v websocat)"
elif [[ -x "$WEBSOCAT_LOCAL" ]]; then
    WEBSOCAT="$WEBSOCAT_LOCAL"
else
    echo "websocat introuvable dans le PATH." >&2
    read -r -p "Le télécharger maintenant dans ${SCRIPT_DIR} ? [o/N] " reply
    case "$reply" in
        [oOyY]*)
            echo "Téléchargement de websocat..." >&2
            if command -v curl &> /dev/null; then
                curl -fL -o "$WEBSOCAT_LOCAL" "$WEBSOCAT_URL"
            elif command -v wget &> /dev/null; then
                wget -O "$WEBSOCAT_LOCAL" "$WEBSOCAT_URL"
            else
                echo "Ni curl ni wget disponible. Installe l'un des deux et relance ce script." >&2
                exit 1
            fi
            chmod +x "$WEBSOCAT_LOCAL"
            WEBSOCAT="$WEBSOCAT_LOCAL"
            ;;
        *)
            echo "Annulé. Installe websocat ou relance ce script." >&2
            exit 1
            ;;
    esac
fi

# The binary sits next to this script in a distributed archive, but in the
# dev repo it's still under lib/basic/test/bin/.
if [[ -x "${SCRIPT_DIR}/bastos-linux-amd64" ]]; then
    BASTOS_EXE="${SCRIPT_DIR}/bastos-linux-amd64"
else
    BASTOS_EXE="${SCRIPT_DIR}/lib/basic/test/bin/bastos-linux-amd64"
fi

WS_URL_ENCODED="ws%3A%2F%2F${WS_ADDR/:/%3A}"

echo "BASTOS lancé sur ws://${WS_ADDR}"
echo "Connecte-toi avec minterm : https://abasty.github.io/minterm/?ws=${WS_URL_ENCODED}"

exec "$WEBSOCAT" -v -t -E --no-line ws-l:"${WS_ADDR}" exec:"$BASTOS_EXE"

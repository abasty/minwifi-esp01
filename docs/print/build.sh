#!/bin/bash
# Génère le livret A6 imprimable sur A4 depuis BASTOS-QUICK-START-fr.md
# Résultat : /tmp/bastos-qs/quickstart-book.pdf  (à imprimer recto-verso, bord court)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DOCS_DIR="$(dirname "$SCRIPT_DIR")"
WORK_DIR="/tmp/bastos-qs"
MD_SRC="$DOCS_DIR/BASTOS-QUICK-START-fr.md"

mkdir -p "$WORK_DIR"

echo "=== 1/3  Pré-traitement des diagrammes Mermaid ==="
python3 "$SCRIPT_DIR/preprocess-mermaid.py" "$MD_SRC" "$WORK_DIR" \
  "$SCRIPT_DIR/puppeteer-config.json" \
  > "$WORK_DIR/quickstart.md"

echo "=== 2/3  Markdown → PDF A6 ==="
pandoc "$WORK_DIR/quickstart.md" \
  --resource-path="$DOCS_DIR" \
  -o "$WORK_DIR/quickstart.pdf" \
  --pdf-engine=xelatex \
  -V papersize=a6 \
  -V geometry:margin=10mm \
  -V documentclass=extarticle \
  -V fontsize=8pt \
  -V lang=fr \
  -H "$SCRIPT_DIR/header.tex"

echo "=== 3/3  Imposition livret (4 A6 par A4) ==="
python3 "$SCRIPT_DIR/impose-booklet.py" "$WORK_DIR/quickstart.pdf" "$WORK_DIR/quickstart-book.pdf"

echo ""
echo ">>> Livret prêt : $WORK_DIR/quickstart-book.pdf"
echo "    Imprimer manuellement : rectos (pages impaires), retourner sur le BORD LONG, versos (pages paires)."

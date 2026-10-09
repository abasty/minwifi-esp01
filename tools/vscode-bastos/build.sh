#!/bin/bash
# Package the BASTOS VSCode extension (tools/vscode-bastos) and deploy the
# result to docs/bastos.vsix — the file linked from
# docs/BASTOS-VSCODE-EXTENSION-fr.md for direct download.
#
# Bump "version" in package.json before running this for a new release;
# the script just packages and deploys whatever version is currently set.
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCS_VSIX="${SCRIPT_DIR}/../../docs/bastos.vsix"

log() { echo "[$(date +'%H:%M:%S')] $*"; }
die() { echo "ERROR: $*" >&2; exit 1; }

cd "$SCRIPT_DIR" || die "Failed to change to script directory"

command -v npx &> /dev/null || die "npx not found in PATH (install Node.js)"
command -v node &> /dev/null || die "node not found in PATH (install Node.js)"

NAME="$(node -p "require('./package.json').name")"
VERSION="$(node -p "require('./package.json').version")"
VSIX="${NAME}-${VERSION}.vsix"

log "Packaging ${NAME} ${VERSION}..."
npx --yes @vscode/vsce package || die "vsce package failed"
[[ -f "$VSIX" ]] || die "Expected package not found: $VSIX"
log "✓ Packaged: ${SCRIPT_DIR}/${VSIX}"

cp "$VSIX" "$DOCS_VSIX"
log "✓ Deployed: $DOCS_VSIX"

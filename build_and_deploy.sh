#!/bin/bash
set -e

# === Configuration ===
PIO_PATH="/home/$USER/.platformio/penv/bin/pio"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_SCP_FOLDER="$1"

# === Functions ===
die() {
    echo "ERROR: $*" >&2
    exit 1
}

log() {
    echo "[$(date +'%H:%M:%S')] $*"
}

warn() {
    echo "[$(date +'%H:%M:%S')] WARNING: $*" >&2
}

show_deploy_help() {
    cat <<EOF
To deploy artifacts, provide DEPLOY_SCP_FOLDER:

Usage: $0 [DEPLOY_SCP_FOLDER]

DEPLOY_SCP_FOLDER should be in the format: user@host:/path/to/deploy

Example:
  $0 user@remote:/var/ftp

You can also set DEPLOY_SCP_FOLDER directly in the script (line 8).

EOF
}

check_pio() {
    if [[ ! -x "$PIO_PATH" ]]; then
        # Try to find pio in PATH
        if command -v pio &> /dev/null; then
            PIO_PATH="$(command -v pio)"
            log "Found pio in PATH: $PIO_PATH"
        else
            die "platformio executable not found at $PIO_PATH and not in PATH"
        fi
    fi
}

check_env_exists() {
    local target=$1
    if ! grep -q "^\[env:${target}\]" platformio.ini; then
        die "Environment [env:${target}] not found in platformio.ini"
    fi
}

# Build and deploy a platformio target. The platformio.ini file describes each
# possible target
pio_build_and_deploy() {
    local target=$1
    log "Building and deploying target: $target"

    # 1. Check if [env:${target}] is present in platformio.ini
    check_env_exists "$target"
    log "✓ Environment [$target] found"

    # 2. Build the target with platformio
    log "Building $target..."
    if ! "$PIO_PATH" run -e "$target"; then
        die "Build failed for target $target"
    fi
    log "✓ Build successful for $target"

    # 3. Copy the firmware to the scp folder (only if DEPLOY_SCP_FOLDER is set)
    if [[ -z "$DEPLOY_SCP_FOLDER" ]]; then
        return 0
    fi

    local firmware_src=".pio/build/${target}/firmware.bin"
    if [[ ! -f "$firmware_src" ]]; then
        die "Firmware file not found: $firmware_src"
    fi

    log "Deploying firmware to $DEPLOY_SCP_FOLDER..."
    if ! scp "$firmware_src" "${DEPLOY_SCP_FOLDER}/firmware/${target}_firmware.bin"; then
        die "Failed to deploy firmware for $target"
    fi
    log "✓ Firmware deployed: ${target}_firmware.bin"
}

# === Main ===
cd "$SCRIPT_DIR" || die "Failed to change to script directory"

# Check if DEPLOY_SCP_FOLDER is set
if [[ -z "$DEPLOY_SCP_FOLDER" ]]; then
    warn "DEPLOY_SCP_FOLDER not set - build only, no deployment"
    echo >&2
    show_deploy_help
fi

log "Starting build and deploy process"
if [[ -n "$DEPLOY_SCP_FOLDER" ]]; then
    log "Deploy folder: $DEPLOY_SCP_FOLDER"
fi

# Verify dependencies
check_pio
command -v make &> /dev/null || die "make not found in PATH"

# Check scp only if deployment is needed
if [[ -n "$DEPLOY_SCP_FOLDER" ]]; then
    command -v scp &> /dev/null || die "scp not found in PATH"
fi

# Build and deploy platformio targets
pio_build_and_deploy sonoff
pio_build_and_deploy sonoff-r4

# Build Linux version
log "Building Linux version..."
pushd ./lib/basic/test > /dev/null
make clean
if ! make TERM=MINITEL; then
    popd > /dev/null
    die "Linux version build failed"
fi
popd > /dev/null
log "✓ Linux version built"

# Deploy Linux version (only if DEPLOY_SCP_FOLDER is set)
if [[ -n "$DEPLOY_SCP_FOLDER" ]]; then
    log "Deploying Linux version..."
    if ! scp "lib/basic/test/bin/bastos" "${DEPLOY_SCP_FOLDER}/firmware/"; then
        die "Failed to deploy Linux version"
    fi
    log "✓ Linux version deployed"

    # Deploy BASTOS files
    log "Deploying BASTOS files..."
    if [[ ! -d "disk" ]] || [[ -z "$(ls -A disk 2>/dev/null)" ]]; then
        die "No BASTOS files found in disk/ directory"
    fi

    # Use find -L to follow symlinks and copy real files
    while IFS= read -r -d '' file; do
        rel_path="${file#disk/}"
        if ! scp "$file" "${DEPLOY_SCP_FOLDER}/bastos/${rel_path}"; then
            die "Failed to deploy BASTOS file: $file"
        fi
    done < <(find -L disk -type f -print0)

    log "✓ BASTOS files deployed"
fi

# Deploy documentation files (only if DEPLOY_SCP_FOLDER is set)
if [[ -n "$DEPLOY_SCP_FOLDER" ]]; then
    log "Deploying documentation files..."
    if [[ ! -d "doc" ]] || [[ -z "$(ls -A doc 2>/dev/null)" ]]; then
        die "No documentation files found in doc/ directory"
    fi

    # Use find -L to follow symlinks and copy real files
    while IFS= read -r -d '' file; do
        rel_path="${file#doc/}"
        if ! scp "$file" "${DEPLOY_SCP_FOLDER}/doc/${rel_path}"; then
            die "Failed to deploy documentation file: $file"
        fi
    done < <(find -L doc -type f -print0)

    log "✓ Documentation files deployed"
fi

log "All done!"

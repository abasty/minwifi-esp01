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

# Build a platformio target. The platformio.ini file describes each possible
# target
pio_build() {
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
}

# Collect firmware binaries for a given target into a staging directory.
# For esp8266 targets (sonoff): only firmware.bin is needed.
# For esp32 targets (sonoff-r4): bootloader.bin, partitions.bin, boot_app0.bin
# and firmware.bin are all required.
collect_bins() {
    local target=$1
    local staging=$2

    local build_dir=".pio/build/${target}"
    local target_dir="${staging}/${target}"
    mkdir -p "$target_dir"

    case "$target" in
        sonoff)
            cp "${build_dir}/firmware.bin" "$target_dir/"
            ;;
        sonoff-r4)
            local boot_app0
            boot_app0="$(find "$HOME/.platformio/packages" \
                -path "*/framework-arduinoespressif32/tools/partitions/boot_app0.bin" \
                -print -quit 2>/dev/null)"
            [[ -f "$boot_app0" ]] || die "boot_app0.bin not found in platformio packages"

            cp "${build_dir}/bootloader.bin"  "$target_dir/"
            cp "${build_dir}/partitions.bin"  "$target_dir/"
            cp "${build_dir}/firmware.bin"    "$target_dir/"
            cp "$boot_app0"                   "$target_dir/"
            ;;
        *)
            die "collect_bins: unknown target '$target'"
            ;;
    esac
    log "✓ Binaries collected for $target"
}

# Write the flash.sh script into the staging directory and make it executable.
write_flash_script() {
    local staging=$1

    cat > "${staging}/flash.sh" << 'FLASH_SCRIPT'
#!/bin/bash
# flash.sh — Flash a Sonoff Basic R2/R3 (ESP8266) or R4 (ESP32-C3)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

die() { echo "ERROR: $*" >&2; exit 1; }
log() { echo "[$(date +'%H:%M:%S')] $*"; }

# --- Port detection ---
detect_port() {
    if [[ -n "$PORT" ]]; then
        echo "$PORT"
        return
    fi

    local ports=(/dev/ttyUSB*)
    local count=${#ports[@]}

    if [[ $count -eq 0 ]] || [[ ! -e "${ports[0]}" ]]; then
        die "No /dev/ttyUSB* device found. Connect the adapter and retry,
     or set PORT=/dev/ttyUSBx explicitly."
    fi

    if [[ $count -gt 1 ]]; then
        echo "ERROR: Multiple USB serial ports detected:" >&2
        for p in "${ports[@]}"; do echo "  $p" >&2; done
        die "Please specify the port: PORT=/dev/ttyUSBx $0 [r2|r4]"
    fi

    echo "${ports[0]}"
}

# --- Chip detection via esptool ---
detect_chip() {
    local port=$1
    local chip_line

    command -v esptool &>/dev/null || die "esptool not found in PATH. Install it with: pip install esptool"

    chip_line="$(esptool --port "$port" --before default_reset chip_id 2>&1 \
                 | grep -i "^Chip is" | head -1)"

    if echo "$chip_line" | grep -qi "ESP8266"; then
        echo "esp8266"
    elif echo "$chip_line" | grep -qi "ESP32-C3"; then
        echo "esp32c3"
    else
        echo "unknown"
    fi
}

# --- Flash functions ---
flash_r2() {
    local port=$1
    local fw="${SCRIPT_DIR}/sonoff/firmware.bin"
    [[ -f "$fw" ]] || die "Firmware not found: $fw"

    log "Flashing Sonoff R2/R3 (ESP8266) on $port..."
    esptool \
        --before default_reset --after hard_reset \
        --chip esp8266 \
        --port "$port" --baud 230400 \
        write_flash 0x0 "$fw"
    log "✓ Done."
}

flash_r4() {
    local port=$1
    local dir="${SCRIPT_DIR}/sonoff-r4"
    [[ -f "${dir}/firmware.bin" ]] || die "Firmware not found: ${dir}/firmware.bin"

    log "Flashing Sonoff R4 (ESP32-C3) on $port..."
    esptool \
        --before default_reset --after hard_reset \
        --chip esp32c3 \
        --port "$port" --baud 460800 \
        write_flash -z \
        --flash_mode dio --flash_freq 80m --flash_size 4MB \
        0x0000 "${dir}/bootloader.bin" \
        0x8000 "${dir}/partitions.bin" \
        0xe000 "${dir}/boot_app0.bin" \
        0x10000 "${dir}/firmware.bin"
    log "✓ Done."
}

# --- Main ---
TARGET="${1:-}"
PORT="$(detect_port)"
log "Using port: $PORT"

if [[ -z "$TARGET" ]]; then
    log "No target specified, detecting chip..."
    CHIP="$(detect_chip "$PORT")"
    log "Detected chip: $CHIP"
    case "$CHIP" in
        esp8266)  TARGET="r2" ;;
        esp32c3)  TARGET="r4" ;;
        *)        die "Could not detect chip. Specify target manually: $0 [r2|r4]" ;;
    esac
fi

case "$TARGET" in
    r2|sonoff)    flash_r2 "$PORT" ;;
    r4|sonoff-r4) flash_r4 "$PORT" ;;
    *) die "Unknown target '$TARGET'. Use: $0 [r2|r4]" ;;
esac
FLASH_SCRIPT

    chmod +x "${staging}/flash.sh"
    log "✓ flash.sh written"
}

# Build the combined flash archive after both sonoff targets have been built.
build_flash_archive() {
    local archive="sonoff-flash.tgz"
    local staging
    staging="$(mktemp -d)"
    trap 'rm -rf "$staging"' RETURN

    log "Assembling flash archive..."
    collect_bins sonoff    "$staging"
    collect_bins sonoff-r4 "$staging"
    write_flash_script     "$staging"

    tar -czf "${SCRIPT_DIR}/${archive}" -C "$staging" .
    log "✓ Archive created: ${SCRIPT_DIR}/${archive}"

    if [[ -n "$DEPLOY_SCP_FOLDER" ]]; then
        log "Deploying flash archive..."
        if ! scp "$archive" "${DEPLOY_SCP_FOLDER}/firmware/"; then
            die "Failed to deploy $archive"
        fi
        log "✓ Archive deployed: ${DEPLOY_SCP_FOLDER}/firmware/${archive}"
    fi
}

# Build an archive for a desktop target (linux or windows), bundling the
# bastos binary together with its websocat launch script, then deploy it.
build_desktop_archive() {
    local platform=$1
    local staging
    staging="$(mktemp -d)"
    trap 'rm -rf "$staging"' RETURN

    local archive
    case "$platform" in
        linux)
            archive="bastos-linux-amd64.tar.gz"
            cp "lib/basic/test/bin/bastos-linux-amd64" "$staging/"
            cp "bastos-back-linux.sh" "$staging/"
            chmod +x "${staging}/bastos-linux-amd64" "${staging}/bastos-back-linux.sh"
            tar -czf "${SCRIPT_DIR}/${archive}" -C "$staging" .
            ;;
        windows)
            archive="bastos-windows-amd64.zip"
            cp "lib/basic/test/bin/bastos-windows-amd64.exe" "$staging/"
            cp "bastos-back-windows.bat" "$staging/"
            (cd "$staging" && zip -q "${SCRIPT_DIR}/${archive}" ./*)
            ;;
        *)
            die "build_desktop_archive: unknown platform '$platform'"
            ;;
    esac
    log "✓ Archive created: ${SCRIPT_DIR}/${archive}"

    if [[ -n "$DEPLOY_SCP_FOLDER" ]]; then
        log "Deploying ${archive}..."
        if ! scp "$archive" "${DEPLOY_SCP_FOLDER}/firmware/"; then
            die "Failed to deploy $archive"
        fi
        log "✓ Archive deployed: ${DEPLOY_SCP_FOLDER}/firmware/${archive}"
    fi
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
command -v x86_64-w64-mingw32-gcc &> /dev/null || die "x86_64-w64-mingw32-gcc not found in PATH (install mingw-w64)"
command -v zip &> /dev/null || die "zip not found in PATH"

# Check scp only if deployment is needed
if [[ -n "$DEPLOY_SCP_FOLDER" ]]; then
    command -v scp &> /dev/null || die "scp not found in PATH"
fi

# Build and deploy platformio targets
pio_build sonoff
pio_build sonoff-r4
build_flash_archive

# Build Linux and Windows desktop versions
log "Building Linux version..."
pushd ./lib/basic/test > /dev/null
make clean
if ! make TERM=MINITEL; then
    popd > /dev/null
    die "Linux version build failed"
fi
popd > /dev/null
log "✓ Linux version built"

log "Building Windows version..."
pushd ./lib/basic/test > /dev/null
if ! make windows; then
    popd > /dev/null
    die "Windows version build failed"
fi
popd > /dev/null
log "✓ Windows version built"

build_desktop_archive linux
build_desktop_archive windows

# Deploy BASTOS files (only if DEPLOY_SCP_FOLDER is set)
if [[ -n "$DEPLOY_SCP_FOLDER" ]]; then
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

log "All done!"

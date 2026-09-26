#!/usr/bin/env bash
# Loads newly built flight software onto an assembled CubeSatSim:
#   - CDHDeployment onto the Raspberry Pi Zero 2, over SSH, as the fprime-cdh systemd service
#   - MainSensorBoardDeployment onto the Pico W, over its USB cable to this computer
#
# The Pi Zero cannot program the Pico W (only a UART connects them), so this computer needs both connections.
# Run with --help for usage.

# The project virtual environments are sourced by path at run time
# shellcheck disable=SC1091
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)

PI_TARGET="pi@cubesatsim.local"
PI_PORT=50000
PICO_PORT=""
PICO_REPO="$PROJECT_ROOT/../fprime-amsat-main-board-reference"
ONLY=""
BUILD=0
VERIFY=0
DRY_RUN=0
RESTORE_STOCK=0
KEEP_RELEASES=3

# Stock CubeSatSim services that use the UART, radio, audio, or camera. They are masked, not just disabled, because
# pushbutton.py restarts some of them. pushbutton (reboot and shutdown button) and OliveTin (web UI) keep running.
STOCK_SERVICES="cubesatsim transmit command pacsatsim frequency"
SERVICE=fprime-cdh

# Overridable for testing
SERIAL_BY_ID="${FSW_SERIAL_BY_ID:-/dev/serial/by-id}"
RP2_DISK_GLOB="${FSW_RP2_DISK_GLOB:-/dev/disk/by-id/usb-RPI_RP2*-part1}"
SSH_OPTS=(-o BatchMode=yes -o ConnectTimeout=5)

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]
       $(basename "$0") --pi USER@HOST --restore-stock

Loads the built CDHDeployment (aarch64-linux) onto the Pi Zero 2 and MainSensorBoardDeployment (rpipicow) onto the
Pico W of an assembled CubeSatSim.

Options:
  --pi USER@HOST      Pi Zero 2 SSH target (default: $PI_TARGET)
  --pico-port DEVICE  Pico W serial device (default: detect the Raspberry Pi USB device)
  --pico-repo DIR     fprime-amsat-main-board-reference checkout (default: ../fprime-amsat-main-board-reference)
  --only pi|pico      Load only one board
  --build             Build both deployments before loading them
  --verify            After loading, run tests against both boards through the F' GDS
  --dry-run           Run the checks and print the changes without making them
  --restore-stock     Stop F' on the Pi and restore the stock CubeSatSim services, then exit
  -h, --help          Show this help

First-time setup: ssh-copy-id $PI_TARGET (the Pi user needs sudo).
EOF
}

log() { printf '\n==> %s\n' "$*"; }
warn() { printf 'WARNING: %s\n' "$*" >&2; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

# Run a bash script on the Pi, read from stdin. FSW_REMOTE_LOCAL=1 runs it locally instead, for testing.
remote_script() {
    if [[ -n "${FSW_REMOTE_LOCAL:-}" ]]; then
        bash -s
    else
        ssh "${SSH_OPTS[@]}" "$PI_TARGET" bash -s
    fi
}

# Copy local files into a directory on the Pi
remote_copy() {
    local destination=${*: -1}
    local sources=("${@:1:$#-1}")
    if [[ -n "${FSW_REMOTE_LOCAL:-}" ]]; then
        cp "${sources[@]}" "$destination"
    else
        scp -q "${SSH_OPTS[@]}" "${sources[@]}" "$PI_TARGET:$destination"
    fi
}

# Print a command in dry-run mode, otherwise run it
act() {
    if (( DRY_RUN )); then
        printf '[dry-run] %s\n' "$*"
    else
        "$@"
    fi
}

parse_args() {
    while (( $# )); do
        case "$1" in
            --pi) PI_TARGET=${2:?--pi needs USER@HOST}; shift ;;
            --pico-port) PICO_PORT=${2:?--pico-port needs a device}; shift ;;
            --pico-repo) PICO_REPO=${2:?--pico-repo needs a directory}; shift ;;
            --only)
                ONLY=${2:?--only needs pi or pico}; shift
                [[ "$ONLY" == pi || "$ONLY" == pico ]] || die "--only must be pi or pico"
                ;;
            --build) BUILD=1 ;;
            --verify) VERIFY=1 ;;
            --dry-run) DRY_RUN=1 ;;
            --restore-stock) RESTORE_STOCK=1 ;;
            -h|--help) usage; exit 0 ;;
            *) usage >&2; die "unknown option: $1" ;;
        esac
        shift
    done
}

want() { [[ -z "$ONLY" || "$ONLY" == "$1" ]]; }

# ----------------------------------------------------------------------------------------------------------------------
# Build and preflight
# ----------------------------------------------------------------------------------------------------------------------

build() {
    if want pi; then
        log "Building CDHDeployment for aarch64-linux"
        (cd "$PROJECT_ROOT/CDHDeployment" && . "$PROJECT_ROOT/fprime-venv/bin/activate" && fprime-util build aarch64-linux)
    fi
    if want pico; then
        log "Building MainSensorBoardDeployment for rpipicow"
        (cd "$PICO_REPO/MainSensorBoardDeployment" && . "$PICO_REPO/fprime-venv/bin/activate" && fprime-util build)
    fi
}

# Warn when the newest source file in a deployment is newer than its build output
check_fresh() {
    local artifact=$1 source_dir=$2
    local newer
    newer=$(find "$source_dir" -newer "$artifact" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.fpp' -o -name '*.fppi' \
        -o -name 'CMakeLists.txt' \) -not -path '*/test/*' -print -quit)
    if [[ -n "$newer" ]]; then
        warn "$newer is newer than $(basename "$artifact"). Rebuild, or run with --build."
    fi
}

# Highest GLIBC_x.y version a binary requires
required_glibc() {
    readelf -V "$1" 2>/dev/null | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -V | tail -1
}

preflight_pi() {
    PI_BINARY="$PROJECT_ROOT/build-artifacts/aarch64-linux/CDHDeployment/bin/CDHDeployment"
    PI_DICTIONARY="$PROJECT_ROOT/build-artifacts/aarch64-linux/CDHDeployment/dict/CDHDeploymentTopologyDictionary.json"
    [[ -x "$PI_BINARY" && -f "$PI_DICTIONARY" ]] ||
        die "CDHDeployment aarch64-linux build not found. Build it (see README), or run with --build."
    check_fresh "$PI_BINARY" "$PROJECT_ROOT/CDHDeployment"
    check_fresh "$PI_BINARY" "$PROJECT_ROOT/Components"

    log "Checking the Pi Zero 2 at $PI_TARGET"
    local facts
    if ! facts=$(remote_script <<'EOF'
uname -m
ldd --version 2>&1 | head -1 | grep -o '[0-9]*\.[0-9]*$'
EOF
    ); then
        die "Cannot reach $PI_TARGET over SSH with key authentication. Check the network, then run: ssh-copy-id $PI_TARGET"
    fi
    local arch pi_glibc needed
    arch=$(sed -n 1p <<<"$facts")
    pi_glibc=$(sed -n 2p <<<"$facts")
    [[ "$arch" == aarch64 ]] ||
        die "The Pi runs a $arch OS, but this build is for aarch64. Build for arm-hf-linux instead, or install 64-bit Raspberry Pi OS."
    needed=$(required_glibc "$PI_BINARY")
    if [[ -n "$needed" && -n "$pi_glibc" ]] && [[ $(printf '%s\n%s\n' "$needed" "$pi_glibc" | sort -V | tail -1) != "$pi_glibc" ]]; then
        die "CDHDeployment needs glibc $needed, but the Pi has $pi_glibc. Build with an older cross-compiler (e.g. the Arm GNU Toolchain 10.2 in /opt/toolchains)."
    fi
    echo "Pi OS: $arch, glibc $pi_glibc (binary needs ${needed:-unknown})"
}

# Print the serial devices of connected Raspberry Pi Picos, one per line
find_picos() {
    local link
    for link in "$SERIAL_BY_ID"/*; do
        [[ -e "$link" ]] || continue
        case "$(basename "$link")" in
            *Raspberry_Pi*|*RPI_RP2*|*2e8a*) echo "$link" ;;
        esac
    done
}

preflight_pico() {
    PICO_UF2="$PICO_REPO/build-artifacts/rpipicow/MainSensorBoardDeployment/bin/MainSensorBoardDeployment.elf.uf2"
    [[ -f "$PICO_UF2" ]] ||
        die "Pico W firmware not found at $PICO_UF2. Build it (see the main board README), or run with --build."
    check_fresh "$PICO_UF2" "$PICO_REPO/MainSensorBoardDeployment"

    UF2CONV=$(find ~/.arduino15/packages/rp2040/hardware/rp2040 -path '*/tools/uf2conv.py' 2>/dev/null | sort -V | tail -1)
    [[ -n "$UF2CONV" ]] || die "uf2conv.py not found. Install the rp2040:rp2040 Arduino core (see the main board README)."
    PICO_PYTHON="$PICO_REPO/fprime-venv/bin/python"
    [[ -x "$PICO_PYTHON" ]] || PICO_PYTHON=python3
    "$PICO_PYTHON" -c 'import serial' 2>/dev/null || die "$PICO_PYTHON has no pyserial. Activate the project venv."

    log "Finding the Pico W"
    if [[ -n "$PICO_PORT" ]]; then
        [[ -e "$PICO_PORT" ]] || die "Pico W port $PICO_PORT not found."
    else
        local picos
        picos=$(find_picos)
        case "$(grep -c . <<<"$picos")" in
            0)
                if compgen -G "$RP2_DISK_GLOB" >/dev/null; then
                    echo "Pico W is already in BOOTSEL mode."
                    return
                fi
                die "No Pico W found in $SERIAL_BY_ID. Connect its USB cable to this computer, or pass --pico-port." ;;
            1) PICO_PORT=$picos ;;
            *) die "More than one Pico found; pass --pico-port with one of:"$'\n'"$picos" ;;
        esac
    fi
    echo "Pico W: $PICO_PORT -> $(readlink -f "$PICO_PORT")"
}

# ----------------------------------------------------------------------------------------------------------------------
# Pi Zero 2
# ----------------------------------------------------------------------------------------------------------------------

deploy_pi() {
    local release sha
    sha=$(git -C "$PROJECT_ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)
    git -C "$PROJECT_ROOT" diff --quiet HEAD 2>/dev/null || sha="$sha-dirty"
    release="$(date +%Y%m%d-%H%M%S)-$sha"

    log "Loading CDHDeployment release $release onto $PI_TARGET"
    if (( DRY_RUN )); then
        printf '[dry-run] copy %s and its dictionary to ~/fprime/releases/%s/\n' "$PI_BINARY" "$release"
        printf '[dry-run] stop and mask stock services (first deploy): %s\n' "$STOCK_SERVICES"
        printf '[dry-run] install /etc/systemd/system/%s.service, point ~/fprime/current at the release, restart it\n' "$SERVICE"
        printf '[dry-run] keep the newest %s releases\n' "$KEEP_RELEASES"
        return
    fi

    local base
    base=$(remote_script <<'EOF'
mkdir -p "$HOME/fprime/releases" "$HOME/fprime/data"
printf '%s' "$HOME/fprime"
EOF
    )
    remote_script <<<"mkdir -p $(printf '%q' "$base/releases/$release")"
    remote_copy "$PI_BINARY" "$PI_DICTIONARY" "$base/releases/$release/"

    {
        printf 'BASE=%q RELEASE=%q SERVICE=%q STOCK_SERVICES=%q PORT=%q KEEP=%q SHA=%q\n' \
            "$base" "$release" "$SERVICE" "$STOCK_SERVICES" "$PI_PORT" "$KEEP_RELEASES" "$sha"
        cat <<'EOF'
set -euo pipefail
cd "$BASE"
printf 'release %s\ncommit %s\n' "$RELEASE" "$SHA" > "releases/$RELEASE/VERSION"

# Stop the stock CubeSatSim services once, recording which were enabled so --restore-stock can bring them back
if [[ ! -f stock-services ]]; then
    : > stock-services.new
    for service in $STOCK_SERVICES; do
        if systemctl cat "$service" >/dev/null 2>&1; then
            printf '%s %s\n' "$service" "$(systemctl is-enabled "$service" 2>/dev/null || true)" >> stock-services.new
            sudo systemctl stop "$service"
            sudo systemctl mask "$service"
            echo "Stopped and masked stock service $service"
        fi
    done
    mv stock-services.new stock-services
fi

sudo tee "/etc/systemd/system/$SERVICE.service" > /dev/null <<UNIT
[Unit]
Description=F' CDHDeployment flight software
After=network.target

[Service]
ExecStart=$BASE/current/CDHDeployment -a 0.0.0.0 -p $PORT
WorkingDirectory=$BASE/data
User=$(id -un)
Restart=on-failure
RestartSec=5
# CDHDeployment exits within a few seconds of SIGTERM. If it ever does not, kill it after 10 s rather than the
# default 90 s, so a redeploy is not held up
TimeoutStopSec=10

[Install]
WantedBy=multi-user.target
UNIT
sudo systemctl daemon-reload
sudo systemctl enable "$SERVICE" >/dev/null 2>&1

sudo systemctl stop "$SERVICE" 2>/dev/null || true
ln -sfn "releases/$RELEASE" current
sudo systemctl start "$SERVICE"

# Keep the newest releases, never removing the current one
ls -1dt releases/*/ | tail -n +$((KEEP + 1)) | while read -r old; do
    [[ "$(readlink -f "$old")" == "$(readlink -f current)" ]] || rm -rf "$old"
done

for _ in $(seq 20); do
    if systemctl is-active --quiet "$SERVICE" && ss -ltn "sport = :$PORT" | grep -q LISTEN; then
        echo "$SERVICE is running $RELEASE and listening on port $PORT"
        exit 0
    fi
    sleep 0.5
done
echo "$SERVICE did not start listening on port $PORT. Recent log:" >&2
sudo journalctl -u "$SERVICE" -n 20 --no-pager >&2 || true
exit 1
EOF
    } | remote_script
}

restore_stock() {
    log "Stopping F' and restoring the stock CubeSatSim services on $PI_TARGET"
    if (( DRY_RUN )); then
        printf '[dry-run] disable and stop %s; unmask and restart the services recorded in ~/fprime/stock-services\n' "$SERVICE"
        return
    fi
    {
        printf 'SERVICE=%q\n' "$SERVICE"
        cat <<'EOF'
set -euo pipefail
record="$HOME/fprime/stock-services"
sudo systemctl disable --now "$SERVICE" 2>/dev/null || true
if [[ ! -f "$record" ]]; then
    echo "No stock services were recorded; nothing to restore."
    exit 0
fi
while read -r service state; do
    sudo systemctl unmask "$service"
    if [[ "$state" == enabled ]]; then
        sudo systemctl enable --now "$service"
        echo "Restored $service"
    else
        echo "Unmasked $service (was $state)"
    fi
done < "$record"
rm -f "$record"
EOF
    } | remote_script
}

# ----------------------------------------------------------------------------------------------------------------------
# Pico W
# ----------------------------------------------------------------------------------------------------------------------

flash_pico() {
    log "Flashing the Pico W with $(basename "$PICO_UF2")"
    local tty=""
    [[ -n "$PICO_PORT" ]] && tty=$(readlink -f "$PICO_PORT")

    # uf2conv.py only resets ports named /dev/tty*, so pass the resolved device rather than the by-id link.
    # The 1200-baud reset reboots the running Arduino-Pico firmware into its USB bootloader.
    if [[ -n "$tty" ]] && act "$PICO_PYTHON" "$UF2CONV" --serial "$tty" --family RP2040 --deploy "$PICO_UF2"; then
        :
    else
        (( DRY_RUN )) && return
        echo
        echo "The Pico W did not enter its bootloader on its own."
        echo "Hold its BOOTSEL button, unplug and reconnect its USB cable, then release BOOTSEL."
        echo "Waiting up to 60 s for the RPI-RP2 drive..."
        local waited=0
        until compgen -G "$RP2_DISK_GLOB" >/dev/null; do
            (( waited++ < 60 )) || die "RPI-RP2 drive did not appear."
            sleep 1
        done
        "$PICO_PYTHON" "$UF2CONV" --family RP2040 --deploy "$PICO_UF2"
    fi

    (( DRY_RUN )) && return
    echo "Waiting for the Pico W to restart..."
    local port=""
    for _ in $(seq 30); do
        port=$(find_picos | head -1)
        [[ -n "$port" && -e "$port" ]] && break
        sleep 0.5
    done
    [[ -n "$port" ]] || die "The Pico W did not reappear as a serial device after flashing."
    PICO_PORT=$port
    echo "Pico W is running the new firmware on $PICO_PORT"
}

# ----------------------------------------------------------------------------------------------------------------------
# Verification
# ----------------------------------------------------------------------------------------------------------------------

verify() {
    local status=0
    if want pi; then
        log "Verifying the Pi Zero 2 through the GDS"
        local host=${PI_TARGET#*@}
        (. "$PROJECT_ROOT/fprime-venv/bin/activate" &&
            CDH_TARGET="$host:$PI_PORT" CDH_DICTIONARY="$PI_DICTIONARY" \
            "$PROJECT_ROOT/CDHDeployment/test/int/run-integration-tests.sh" -k "is_streaming or no_op or check_camera") ||
            status=1
    fi
    if want pico; then
        log "Verifying the Pico W through the GDS"
        (. "$PICO_REPO/fprime-venv/bin/activate" &&
            "$PICO_REPO/MainSensorBoardDeployment/test/int/run-hardware-tests.sh" "$(readlink -f "$PICO_PORT")") ||
            status=1
    fi
    return $status
}

main() {
    parse_args "$@"

    if (( RESTORE_STOCK )); then
        restore_stock
        return
    fi

    (( BUILD )) && build
    want pi && preflight_pi
    want pico && preflight_pico

    want pi && deploy_pi
    want pico && flash_pico

    if (( VERIFY && ! DRY_RUN )); then
        verify
    fi
    log "Done"
}

main "$@"

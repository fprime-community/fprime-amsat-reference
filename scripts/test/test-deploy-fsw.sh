#!/usr/bin/env bash
# Tests for deploy-fsw.sh that run without a Pi Zero 2 or Pico W.
#
# The Pi side runs locally (FSW_REMOTE_LOCAL=1) against a scratch home directory, with sudo, systemctl, ss, uname,
# and ldd replaced by stubs. The Pico side uses a scratch /dev/serial/by-id directory. Needs a CDHDeployment
# aarch64-linux build (for the binary and its dictionary).
#
# Usage: scripts/test/test-deploy-fsw.sh
set -euo pipefail

TEST_DIR=$(cd "$(dirname "$0")" && pwd)
DEPLOY="$TEST_DIR/../deploy-fsw.sh"
PROJECT_ROOT=$(cd "$TEST_DIR/../.." && pwd)
BINARY="$PROJECT_ROOT/build-artifacts/aarch64-linux/CDHDeployment/bin/CDHDeployment"
[[ -x "$BINARY" ]] || { echo "Build CDHDeployment for aarch64-linux first." >&2; exit 1; }

SCRATCH=$(mktemp -d "${TMPDIR:-/tmp}/deploy-fsw-test-XXXXXX")
trap 'rm -rf "$SCRATCH"' EXIT
STUBS="$SCRATCH/stubs"
mkdir -p "$STUBS"

# ----------------------------------------------------------------------------------------------------------------------
# Stubs
# ----------------------------------------------------------------------------------------------------------------------

# sudo runs the command without privileges, redirecting writes to /etc/systemd/system into the scratch directory
cat > "$STUBS/sudo" <<'EOF'
#!/bin/bash
if [[ "$1" == tee && "$2" == /etc/systemd/system/* ]]; then
    mkdir -p "$FAKE_ETC"
    exec tee "$FAKE_ETC/$(basename "$2")"
fi
exec "$@"
EOF

# systemctl logs every call. Services listed in $FAKE_INSTALLED exist and are enabled.
cat > "$STUBS/systemctl" <<'EOF'
#!/bin/bash
echo "systemctl $*" >> "$FAKE_LOG"
case "$1" in
    cat) grep -qw "$2" <<<"$FAKE_INSTALLED"; exit $? ;;
    is-enabled) echo enabled ;;
esac
exit 0
EOF

cat > "$STUBS/ss" <<'EOF'
#!/bin/bash
echo "LISTEN 0 1 0.0.0.0:50000 0.0.0.0:*"
EOF

cat > "$STUBS/journalctl" <<'EOF'
#!/bin/bash
echo "(journal)"
EOF

cat > "$STUBS/uname" <<'EOF'
#!/bin/bash
[[ "$1" == -m ]] && { echo "${FAKE_ARCH:-aarch64}"; exit 0; }
exec /usr/bin/uname "$@"
EOF

# ldd reports a very new glibc by default, so the checks pass whichever cross-compiler built the binary
cat > "$STUBS/ldd" <<'EOF'
#!/bin/bash
echo "ldd (GNU libc) ${FAKE_GLIBC:-99.0}"
EOF
chmod +x "$STUBS"/*

export FAKE_ETC="$SCRATCH/etc"
export FAKE_LOG="$SCRATCH/systemctl.log"
export FAKE_INSTALLED="cubesatsim transmit command pushbutton OliveTin"
export FSW_REMOTE_LOCAL=1
export FSW_SERIAL_BY_ID="$SCRATCH/by-id"
export FSW_RP2_DISK_GLOB="$SCRATCH/no-rp2-disk*"

PI_HOME="$SCRATCH/home"
mkdir -p "$PI_HOME" "$FSW_SERIAL_BY_ID"

# Run deploy-fsw.sh with the stubs, capturing output. HOME is the scratch Pi home unless RUN_HOME is set.
run_deploy() {
    HOME="${RUN_HOME:-$PI_HOME}" PATH="$STUBS:$PATH" "$DEPLOY" "$@" > "$SCRATCH/out" 2>&1
}

failures=0
pass() { echo "PASS: $1"; }
fail() {
    echo "FAIL: $1"
    sed 's/^/    /' "$SCRATCH/out"
    failures=$((failures + 1))
}
check() { # description, command...
    local description=$1
    shift
    if "$@"; then pass "$description"; else fail "$description"; fi
}
output_has() { grep -q -- "$1" "$SCRATCH/out"; }
check_rejects() { # description, expected output pattern, deploy-fsw.sh arguments...
    local description=$1 pattern=$2
    shift 2
    if run_deploy "$@"; then
        fail "$description (deploy-fsw.sh succeeded)"
    elif output_has "$pattern"; then
        pass "$description"
    else
        fail "$description (missing: $pattern)"
    fi
}

# ----------------------------------------------------------------------------------------------------------------------
# Pi Zero 2
# ----------------------------------------------------------------------------------------------------------------------

run_deploy --only pi --dry-run
check "dry run describes the Pi install" output_has "\[dry-run\] install /etc/systemd/system/fprime-cdh.service"
check "dry run changes nothing on the Pi" test ! -e "$PI_HOME/fprime"

run_deploy --only pi || true
base="$PI_HOME/fprime"
check "first deploy succeeds" output_has "fprime-cdh is running"
check "release holds the binary, dictionary, and VERSION" \
    bash -c "ls '$base'/releases/*/CDHDeployment '$base'/releases/*/CDHDeploymentTopologyDictionary.json '$base'/releases/*/VERSION >/dev/null"
check "current points at the release" test -x "$base/current/CDHDeployment"
check "data directory exists" test -d "$base/data"
check "installed stock services are recorded" \
    bash -c "[[ \$(cut -d' ' -f1 '$base/stock-services' | tr '\n' ' ') == 'cubesatsim transmit command ' ]]"
check "stock services are stopped and masked" grep -q "systemctl mask transmit" "$FAKE_LOG"
check "pushbutton and OliveTin are left alone" bash -c "! grep -E 'pushbutton|OliveTin' '$FAKE_LOG'"
check "unit runs the current release from the data directory" \
    grep -q "ExecStart=$base/current/CDHDeployment -a 0.0.0.0 -p 50000" "$FAKE_ETC/fprime-cdh.service"
check "unit bounds the stop timeout" grep -q "TimeoutStopSec=10" "$FAKE_ETC/fprime-cdh.service"
check "service is restarted" grep -q "systemctl start fprime-cdh" "$FAKE_LOG"

: > "$FAKE_LOG"
for _ in 1 2 3 4; do
    sleep 1 # release names include the time in seconds
    run_deploy --only pi || fail "repeat deploy"
done
newest=$(find "$base/releases" -mindepth 1 -maxdepth 1 -printf '%T@ %f\n' | sort -n | tail -1 | cut -d' ' -f2)
check "only the newest 3 releases are kept" bash -c "[[ \$(ls '$base/releases' | wc -l) -eq 3 ]]"
check "current points at the newest release" bash -c "[[ \$(readlink '$base/current') == releases/$newest ]]"
check "stock services are only stopped on the first deploy" bash -c "! grep -q 'systemctl mask' '$FAKE_LOG'"

: > "$FAKE_LOG"
run_deploy --restore-stock || true
check "restore disables F'" grep -q "systemctl disable --now fprime-cdh" "$FAKE_LOG"
check "restore unmasks and re-enables recorded services" grep -q "systemctl enable --now transmit" "$FAKE_LOG"
check "restore removes the record" test ! -e "$base/stock-services"

FAKE_ARCH=armv7l check_rejects "32-bit Pi OS is rejected" "Build for arm-hf-linux" --only pi
FAKE_GLIBC=2.10 check_rejects "old glibc is rejected" "needs glibc .*, but the Pi has 2.10" --only pi
FSW_REMOTE_LOCAL="" check_rejects "unreachable Pi is reported" "ssh-copy-id nobody@192.0.2.1" --only pi --pi nobody@192.0.2.1

# ----------------------------------------------------------------------------------------------------------------------
# Pico W
# ----------------------------------------------------------------------------------------------------------------------

check_rejects "missing Pico build is reported" "Pico W firmware not found" --only pico --pico-repo "$SCRATCH/no-such-repo"

PICO_REPO="$SCRATCH/pico-repo"
mkdir -p "$PICO_REPO/build-artifacts/rpipicow/MainSensorBoardDeployment/bin" "$PICO_REPO/MainSensorBoardDeployment"
touch "$PICO_REPO/MainSensorBoardDeployment/Main.cpp"
sleep 1
touch "$PICO_REPO/build-artifacts/rpipicow/MainSensorBoardDeployment/bin/MainSensorBoardDeployment.elf.uf2"
ln -s /dev/null "$FSW_SERIAL_BY_ID/usb-Espressif_USB_JTAG_serial_debug_unit_AC:27-if00"

# The remaining Pico checks need uf2conv.py from the rp2040 core; dry runs never execute it
RUN_HOME="$SCRATCH/pico-home"
mkdir -p "$RUN_HOME/.arduino15/packages/rp2040/hardware/rp2040/9.9.9/tools"
touch "$RUN_HOME/.arduino15/packages/rp2040/hardware/rp2040/9.9.9/tools/uf2conv.py"
check_rejects "no Pico is reported, and other USB serial devices are ignored" "No Pico W found" \
    --only pico --pico-repo "$PICO_REPO" --dry-run

ln -s /dev/null "$FSW_SERIAL_BY_ID/usb-Raspberry_Pi_Pico_W_E6614C311B2F-if00"
run_deploy --only pico --pico-repo "$PICO_REPO" --dry-run || true
check "the Pico is found by its USB name" output_has "Pico W: $FSW_SERIAL_BY_ID/usb-Raspberry_Pi_Pico_W"
check "uf2conv is given the resolved tty, not the by-id link" output_has "uf2conv.py --serial /dev/null --family RP2040 --deploy"

ln -s /dev/null "$FSW_SERIAL_BY_ID/usb-Raspberry_Pi_Pico_W_E6614C311B30-if00"
check_rejects "two Picos are reported" "More than one Pico found" --only pico --pico-repo "$PICO_REPO" --dry-run

echo
if (( failures )); then
    echo "$failures check(s) failed"
    exit 1
fi
echo "All checks passed"

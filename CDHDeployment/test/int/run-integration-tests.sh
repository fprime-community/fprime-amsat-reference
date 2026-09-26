#!/usr/bin/env bash
# Runs the CDHDeployment integration tests against a native build, with fake camera commands on the PATH.
#
# Usage, from the project virtual environment after a native build:
#     CDHDeployment/test/int/run-integration-tests.sh [extra pytest arguments]
#
# The application runs in a scratch directory, which is kept for debugging when a test fails.
#
# To test a deployment that is already running, such as on the Pi Zero 2, set CDH_TARGET=host:port and optionally
# CDH_DICTIONARY. The GDS then connects to it instead of starting the application, and tests that need the fake camera
# are skipped.
set -euo pipefail

TEST_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$TEST_DIR/../../.." && pwd)
ARTIFACTS="$PROJECT_ROOT/build-artifacts/$(uname -s)/CDHDeployment"
DICTIONARY="${CDH_DICTIONARY:-$ARTIFACTS/dict/CDHDeploymentTopologyDictionary.json}"
APP="$ARTIFACTS/bin/CDHDeployment"
TARGET="${CDH_TARGET:-}"

if [[ -n "$TARGET" ]]; then
    if [[ ! -f "$DICTIONARY" ]]; then
        echo "Dictionary $DICTIONARY not found. Set CDH_DICTIONARY to the running deployment's dictionary." >&2
        exit 1
    fi
elif [[ ! -f "$DICTIONARY" || ! -x "$APP" ]]; then
    echo "CDHDeployment native build not found in $ARTIFACTS." >&2
    echo "Build it first: cd CDHDeployment && fprime-util generate && fprime-util build" >&2
    exit 1
fi

WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/cdh-int-XXXXXX")
APP_PID=""
GDS_PID=""

# Job control starts background processes in their own process groups without ignoring SIGINT, as background jobs
# otherwise do in scripts, so they can be stopped cleanly
set -m

if [[ -n "$TARGET" ]]; then
    ADDRESS=${TARGET%:*}
    PORT=${TARGET##*:}
else
    ADDRESS=127.0.0.1
    # A fresh port for each run, so runs never contend for a port
    PORT="${CDH_TEST_PORT:-$(python -c 'import socket; s = socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1])')}"
    # Start the application here rather than through the GDS, which runs it from its bin directory. Its images/ and
    # PrmDb.dat then go to the scratch directory, and the fake camera commands are on its PATH.
    export CDH_APP_DIR="$WORK_DIR"
    (cd "$WORK_DIR" && PATH="$TEST_DIR/fake-camera:$PATH" exec "$APP" -a "$ADDRESS" -p "$PORT") \
        > "$WORK_DIR/app.log" 2>&1 &
    APP_PID=$!
fi

# Stop a background process with a signal, then its whole process group if it has not exited within 10 s
stop_process() {
    local pid=$1 signal=$2
    if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
        kill "-$signal" "$pid"
        for _ in $(seq 20); do
            kill -0 "$pid" 2>/dev/null || break
            sleep 0.5
        done
        kill -KILL -- "-$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
    fi
}

stop_all() {
    stop_process "$GDS_PID" INT
    stop_process "$APP_PID" TERM
}
trap stop_all EXIT

cd "$WORK_DIR"
fprime-gds --gui none --no-app --dictionary "$DICTIONARY" \
    --framing-selection fprime --ip-client --ip-address "$ADDRESS" --ip-port "$PORT" \
    --logs "$WORK_DIR/logs" --file-storage-directory "$WORK_DIR/files" \
    > "$WORK_DIR/gds.out" 2>&1 &
GDS_PID=$!

status=0
python -m pytest "$TEST_DIR" \
    --dictionary "$DICTIONARY" --file-storage-directory "$WORK_DIR/files" --logs "$WORK_DIR/pytest-logs" \
    "$@" || status=$?

stop_all
trap - EXIT
if [[ $status -eq 0 ]]; then
    rm -rf "$WORK_DIR"
else
    echo "Integration tests failed. Application and GDS logs are in $WORK_DIR" >&2
fi
exit $status

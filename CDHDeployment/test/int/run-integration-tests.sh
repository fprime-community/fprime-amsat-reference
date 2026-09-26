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
if [[ -n "$TARGET" ]]; then
    ADDRESS=${TARGET%:*}
    PORT=${TARGET##*:}
    APP_ARGS=(--no-app)
else
    ADDRESS=127.0.0.1
    # A fresh port for each run: the application's TCP server cannot reuse a port still in TIME_WAIT from a previous run
    PORT="${CDH_TEST_PORT:-$(python -c 'import socket; s = socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1])')}"
    APP_ARGS=(--deployment "$ARTIFACTS" --app "$APP")
    export CDH_APP_DIR="$WORK_DIR"
fi
GDS_PID=""

# Job control starts the GDS in its own process group without ignoring SIGINT, as background jobs otherwise do in
# scripts, so it can be stopped cleanly
set -m

stop_gds() {
    if [[ -n "$GDS_PID" ]] && kill -0 "$GDS_PID" 2>/dev/null; then
        # SIGINT lets the GDS shut down cleanly and stop the application, releasing its port
        kill -INT "$GDS_PID"
        for _ in $(seq 20); do
            kill -0 "$GDS_PID" 2>/dev/null || break
            sleep 0.5
        done
        # If it is still running, stop its whole process group
        kill -TERM -- "-$GDS_PID" 2>/dev/null || true
        wait "$GDS_PID" 2>/dev/null || true
    fi
}
trap stop_gds EXIT

cd "$WORK_DIR"
PATH="$TEST_DIR/fake-camera:$PATH" fprime-gds --gui none \
    "${APP_ARGS[@]}" --dictionary "$DICTIONARY" \
    --framing-selection fprime --ip-client --ip-address "$ADDRESS" --ip-port "$PORT" \
    --logs "$WORK_DIR/logs" --file-storage-directory "$WORK_DIR/files" \
    > "$WORK_DIR/gds.out" 2>&1 &
GDS_PID=$!

status=0
python -m pytest "$TEST_DIR" \
    --dictionary "$DICTIONARY" --file-storage-directory "$WORK_DIR/files" --logs "$WORK_DIR/pytest-logs" \
    "$@" || status=$?

stop_gds
trap - EXIT
if [[ $status -eq 0 ]]; then
    rm -rf "$WORK_DIR"
else
    echo "Integration tests failed. Application and GDS logs are in $WORK_DIR" >&2
fi
exit $status

"""test_shutdown.py:

Checks that the native CDHDeployment exits promptly and cleanly on SIGTERM, which is how systemd stops it on the Pi.
With Drv.TcpServer, the application hung if no GDS had connected, and crashed with an assertion after a GDS
disconnected. Build the deployment first (cd CDHDeployment && fprime-util generate && fprime-util build), then run:

    pytest CDHDeployment/test/host
"""

import os
import signal
import socket
import subprocess
import time
from pathlib import Path

import pytest

PROJECT_ROOT = Path(__file__).resolve().parents[3]
EXIT_LIMIT_S = 5  # one 1 s socket retry interval plus margin


@pytest.fixture(scope="module")
def app():
    default = PROJECT_ROOT / "build-artifacts" / "Linux" / "CDHDeployment" / "bin" / "CDHDeployment"
    path = Path(os.environ.get("CDH_APP", default))
    if not path.exists():
        pytest.fail(f"{path} not found. Build CDHDeployment first, or set CDH_APP.")
    return path


def free_port():
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def connect(port, timeout=5):
    """Connect to the application's TCP server, waiting for it to start listening"""
    deadline = time.monotonic() + timeout
    while True:
        try:
            return socket.create_connection(("127.0.0.1", port), timeout=0.5)
        except OSError:
            if time.monotonic() > deadline:
                raise
            time.sleep(0.1)


@pytest.mark.parametrize("gds", ["never connected", "disconnected", "connected"])
def test_sigterm_exits_cleanly(app, tmp_path, gds):
    """SIGTERM stops the application with exit status 0 whether or not a GDS is, or was, connected"""
    port = free_port()
    proc = subprocess.Popen(
        [str(app), "-a", "127.0.0.1", "-p", str(port)],
        cwd=tmp_path,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    client = None
    try:
        if gds == "never connected":
            time.sleep(1.5)
        else:
            client = connect(port)
            if gds == "disconnected":
                client.close()
                client = None
            # Let the application run, sending telemetry, in this state
            time.sleep(1.5)

        proc.send_signal(signal.SIGTERM)
        try:
            status = proc.wait(timeout=EXIT_LIMIT_S)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
            pytest.fail(f"still running {EXIT_LIMIT_S} s after SIGTERM")
        output = proc.stdout.read()
        assert "Assert" not in output, output[-2000:]
        assert status == 0, f"exit status {status}\n{output[-2000:]}"
    finally:
        if client:
            client.close()
        if proc.poll() is None:
            proc.kill()
            proc.wait()

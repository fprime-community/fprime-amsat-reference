"""conftest.py: shared setup for the CDHDeployment integration tests"""

import os
from pathlib import Path

import pytest


@pytest.fixture(scope="session", autouse=True)
def deployment_running(fprime_test_api_session):
    """Wait for the deployment to connect and start sending telemetry before any test runs"""
    if not fprime_test_api_session.await_telemetry_count(1, timeout=30):
        pytest.exit("CDHDeployment did not send telemetry within 30 s. Is the GDS running with --ip-client?")


@pytest.fixture(scope="session")
def app_dir():
    """Working directory of the running CDHDeployment, where it writes images/"""
    path = os.environ.get("CDH_APP_DIR")
    if path is None:
        pytest.skip("CDH_APP_DIR is not set; run through run-integration-tests.sh")
    return Path(path)


@pytest.fixture(scope="session")
def downlink_dir(request):
    """Directory the GDS writes downlinked files to"""
    return Path(request.config.getoption("--file-storage-directory")) / "fprime-downlink"

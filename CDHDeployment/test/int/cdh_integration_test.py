"""cdh_integration_test.py:

Integration tests for the CDHDeployment, run against a live deployment through the F' GDS. Run them with
run-integration-tests.sh, which starts the deployment with fake camera commands on the PATH.
"""

import re
import subprocess
import time
from pathlib import Path

DEPLOYMENT = "CDHDeployment"
COMMANDER = f"{DEPLOYMENT}.cmdDisp"
RATE_GROUPS = ["rateGroup1Hz", "rateGroupHalfHz", "rateGroupQuarterHz"]
TOPOLOGY_CPP = Path(__file__).resolve().parents[2] / "Top" / "CDHDeploymentTopology.cpp"


def names_in(history):
    """Full names of the events or channels in a test history"""
    return [item.template.get_full_name() for item in history.retrieve()]


def ping_table_names():
    """Component names listed in the health pingEntries table"""
    table = TOPOLOGY_CPP.read_text().split("Svc::Health::PingEntry pingEntries[] = {", 1)[1].split("};", 1)[0]
    return re.findall(r"FATAL,\s*\"(\w+)\"", table)


def event_argument(results, event_name, index):
    """Value of one argument of the first event with the given name in send_and_assert_command results"""
    for result in results:
        if result.template.get_full_name() == event_name:
            return result.get_args()[index].val
    raise AssertionError(f"{event_name} not found in command results")


def test_is_streaming(fprime_test_api):
    """Telemetry arrives from the deployment"""
    fprime_test_api.assert_telemetry_count(5, timeout=10)


def test_no_op(fprime_test_api):
    """A NO_OP command is dispatched and completes"""
    fprime_test_api.send_and_assert_command(f"{COMMANDER}.CMD_NO_OP", max_delay=1.0, commander=COMMANDER)


def assert_no_cycle_slips(fprime_test_api):
    """No rate group reported a cycle slip during the test"""
    slips = [name for name in names_in(fprime_test_api.get_telemetry_test_history()) if name.endswith(".RgCycleSlips")]
    assert not slips, f"cycle slips reported: {slips}"


def test_1hz_rate_group(fprime_test_api):
    """systemResources, in the 1 Hz rate group, reports about once per second"""
    window = 6
    time.sleep(window)
    updates = names_in(fprime_test_api.get_telemetry_test_history()).count(f"{DEPLOYMENT}.systemResources.CPU")
    assert window - 2 <= updates <= window + 2, f"{updates} CPU updates in {window} s"
    assert_no_cycle_slips(fprime_test_api)


def test_half_hz_rate_group(fprime_test_api, app_dir):
    """cmdSeq, in the 0.5 Hz rate group, completes a sequence with a 3 s relative wait"""
    sequence = Path(__file__).parent / "wait_sequence.seq"
    binary = "wait_sequence.bin"
    subprocess.run(
        ["fprime-seqgen", "--dictionary", str(fprime_test_api.pipeline.dictionary_path), str(sequence), str(app_dir / binary)],
        check=True,
    )
    noop = fprime_test_api.get_event_pred(f"{COMMANDER}.NoOpReceived")
    start = time.monotonic()
    fprime_test_api.send_and_assert_command(
        f"{DEPLOYMENT}.cmdSeq.CS_RUN", [binary, "BLOCK"], events=[noop, noop], timeout=15, commander=COMMANDER
    )
    elapsed = time.monotonic() - start
    # The wait ends on the first 0.5 Hz cycle after 3 s
    assert 3 <= elapsed <= 7, f"sequence took {elapsed:.1f} s"
    assert_no_cycle_slips(fprime_test_api)


def test_quarter_hz_rate_group_health_detects_hung_camera(fprime_test_api):
    """health, in the 0.25 Hz rate group, warns when the camera stops answering pings

    The camera answers pings on its own thread, which a capture blocks for CAPTURE_DELAY_MS. The first unanswered ping
    is sent within 4 s of the capture starting; health warns 12 s later and declares a FATAL fault 20 s later. An 18 s
    capture therefore always produces the warning and never the fault.
    """
    try:
        fprime_test_api.send_and_assert_command(
            f"{DEPLOYMENT}.camera.CAPTURE_DELAY_MS_PRM_SET", [18000], commander=COMMANDER
        )
        fprime_test_api.send_and_assert_command(
            f"{DEPLOYMENT}.camera.TAKE_PICTURE",
            events=[fprime_test_api.get_event_pred(f"{DEPLOYMENT}.health.HLTH_PING_WARN", ["camera"])],
            timeout=25,
            commander=COMMANDER,
        )
    finally:
        fprime_test_api.send_and_assert_command(
            f"{DEPLOYMENT}.camera.CAPTURE_DELAY_MS_PRM_SET", [1000], commander=COMMANDER
        )
    events = names_in(fprime_test_api.get_event_test_history())
    assert f"{DEPLOYMENT}.health.HLTH_PING_LATE" not in events


def test_ping_table_names_accepted(fprime_test_api):
    """Health accepts every name in the ping table, and rejects a name that is not in it"""
    names = ping_table_names()
    assert "camera" in names
    for name in names:
        fprime_test_api.send_and_assert_command(
            f"{DEPLOYMENT}.health.HLTH_PING_ENABLE", [name, "ENABLED"], commander=COMMANDER
        )
    fprime_test_api.send_and_assert_event(
        f"{DEPLOYMENT}.health.HLTH_PING_ENABLE",
        ["notAComponent", "ENABLED"],
        [fprime_test_api.get_event_pred(f"{COMMANDER}.OpCodeError")],
    )


def test_health_pings_answered(fprime_test_api):
    """No component misses health pings over several ping cycles while the deployment is idle"""
    # Health runs in the 0.25 Hz rate group and warns after 3 missed pings
    assert fprime_test_api.await_event(f"{DEPLOYMENT}.health.HLTH_PING_WARN", timeout=20) is None
    late = [name for name in names_in(fprime_test_api.get_event_test_history()) if ".health.HLTH_PING" in name]
    assert not late, f"health ping problems: {late}"


def test_check_camera(fprime_test_api):
    """CHECK_CAMERA reports the camera listed by rpicam-hello"""
    fprime_test_api.send_and_assert_command(
        f"{DEPLOYMENT}.camera.CHECK_CAMERA",
        events=[fprime_test_api.get_event_pred(f"{DEPLOYMENT}.camera.CameraDetected")],
        commander=COMMANDER,
    )


def take_picture(fprime_test_api, app_dir):
    """Send TAKE_PICTURE, returning the image path reported by the deployment and the image contents"""
    results = fprime_test_api.send_and_assert_command(
        f"{DEPLOYMENT}.camera.TAKE_PICTURE",
        events=[fprime_test_api.get_event_pred(f"{DEPLOYMENT}.camera.PictureTaken")],
        timeout=10,
        commander=COMMANDER,
    )
    path = event_argument(results, f"{DEPLOYMENT}.camera.PictureTaken", 0)
    return path, (app_dir / path).read_text()


def test_take_picture(fprime_test_api, app_dir):
    """TAKE_PICTURE writes an image with the default size and reports it in telemetry"""
    path, contents = take_picture(fprime_test_api, app_dir)
    assert path.startswith("images/img_")
    assert contents == f"FAKE-JPEG -n -o {path} --width 320 --height 256 -t 1000\n"
    fprime_test_api.assert_telemetry(f"{DEPLOYMENT}.camera.LastImageSize", value=len(contents), timeout=5)


def test_image_size_parameters(fprime_test_api, app_dir):
    """Parameter commands change the size passed to rpicam-still"""
    try:
        fprime_test_api.send_and_assert_command(f"{DEPLOYMENT}.camera.IMAGE_WIDTH_PRM_SET", [640], commander=COMMANDER)
        fprime_test_api.send_and_assert_command(f"{DEPLOYMENT}.camera.IMAGE_HEIGHT_PRM_SET", [480], commander=COMMANDER)
        _, contents = take_picture(fprime_test_api, app_dir)
        assert "--width 640 --height 480" in contents
    finally:
        fprime_test_api.send_and_assert_command(f"{DEPLOYMENT}.camera.IMAGE_WIDTH_PRM_SET", [320], commander=COMMANDER)
        fprime_test_api.send_and_assert_command(f"{DEPLOYMENT}.camera.IMAGE_HEIGHT_PRM_SET", [256], commander=COMMANDER)


def test_downlink_picture(fprime_test_api, app_dir, downlink_dir):
    """An image can be downlinked with fileDownlink.SendFile, using the path from the PictureTaken event

    File name arguments are limited to FW_CMD_STRING_MAX_SIZE (40) characters, so the relative path is sent.
    """
    path, contents = take_picture(fprime_test_api, app_dir)
    destination = "downlinked_" + Path(path).name

    fprime_test_api.send_and_assert_command(
        f"{DEPLOYMENT}.fileDownlink.SendFile", [path, destination], timeout=10, commander=COMMANDER
    )
    fprime_test_api.assert_event(f"{DEPLOYMENT}.fileDownlink.FileSent", timeout=15)

    received = downlink_dir / destination
    deadline = time.monotonic() + 10
    while not (received.exists() and received.read_text() == contents) and time.monotonic() < deadline:
        time.sleep(0.2)
    assert received.exists(), f"{received} was not downlinked"
    assert received.read_text() == contents

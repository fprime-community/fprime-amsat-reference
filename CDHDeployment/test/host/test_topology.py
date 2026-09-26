"""test_topology.py:

Checks of the CDHDeployment topology that run against source and build outputs, without running the deployment.
Build the deployment first (cd CDHDeployment && fprime-util generate && fprime-util build), then run:

    pytest CDHDeployment/test/host

Set CDH_BUILD_DIR or CDH_DICTIONARY to check a build other than the default native build.
"""

import json
import os
import re
from fractions import Fraction
from pathlib import Path

import pytest

PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEPLOYMENT_DIR = PROJECT_ROOT / "CDHDeployment"
TOPOLOGY_CPP = DEPLOYMENT_DIR / "Top" / "CDHDeploymentTopology.cpp"
TOPOLOGY_FPP = DEPLOYMENT_DIR / "Top" / "topology.fpp"
MAIN_CPP = DEPLOYMENT_DIR / "Main.cpp"

# IDs at or above this value belong to the MainSensorBoardDeployment on the Pico W
PICO_ID_BASE = 0x10000000


def build_file(env_var, default):
    """Return a build output path, failing with build instructions if it does not exist"""
    path = Path(os.environ.get(env_var, default))
    if not path.exists():
        pytest.fail(f"{path} not found. Build CDHDeployment first, or set {env_var}.")
    return path


@pytest.fixture(scope="module")
def topology_ac():
    build_dir = Path(os.environ.get("CDH_BUILD_DIR", PROJECT_ROOT / "build-fprime-automatic-native"))
    return build_file("CDH_TOPOLOGY_AC", build_dir / "CDHDeployment" / "Top" / "CDHDeploymentTopologyAc.cpp").read_text()


@pytest.fixture(scope="module")
def dictionary():
    default = PROJECT_ROOT / "build-artifacts" / "Linux" / "CDHDeployment" / "dict" / "CDHDeploymentTopologyDictionary.json"
    return json.loads(build_file("CDH_DICTIONARY", default).read_text())


def test_ping_entries_match_health_ports(topology_ac):
    """pingEntries must list components in the order the autocoder assigns health PingSend ports

    Health reports missed pings using the name at the ping port's index, so an out-of-order table blames the wrong
    component.
    """
    ports = re.findall(
        r"health\.set_PingSend_OutputPort\(\s*(\d+),\s*CDHDeployment::(\w+)\.get_[pP]ingIn_InputPort", topology_ac
    )
    assert ports, "no health PingSend connections found in the generated topology"
    port_order = [name for _, name in sorted(ports, key=lambda port: int(port[0]))]

    table = TOPOLOGY_CPP.read_text().split("Svc::Health::PingEntry pingEntries[] = {", 1)[1].split("};", 1)[0]
    entries = re.findall(r"PingEntries::CDHDeployment_(\w+)::WARN,\s*PingEntries::CDHDeployment_(\w+)::FATAL,\s*\"(\w+)\"", table)
    for warn_name, fatal_name, label in entries:
        assert warn_name == fatal_name == label, f"ping entry mixes names: {warn_name}, {fatal_name}, {label}"

    assert [label for _, _, label in entries] == port_order


def test_ids_below_pico_range(dictionary):
    """All IDs stay below the MainSensorBoardDeployment's 0x1xxxxxxx range so the two dictionaries never collide"""
    ids = {
        "command": [command["opcode"] for command in dictionary["commands"]],
        "event": [event["id"] for event in dictionary["events"]],
        "channel": [channel["id"] for channel in dictionary["telemetryChannels"]],
        "parameter": [parameter["id"] for parameter in dictionary["parameters"]],
    }
    for kind, values in ids.items():
        assert values, f"no {kind} IDs in dictionary"
        too_high = [hex(value) for value in values if value >= PICO_ID_BASE]
        assert not too_high, f"{kind} IDs in the Pico range: {too_high}"


def frequency_name(hertz):
    """Name used in rate group instance names for a frequency"""
    names = {Fraction(1, 2): "HalfHz", Fraction(1, 4): "QuarterHz"}
    if hertz in names:
        return names[hertz]
    assert hertz.denominator == 1, f"no naming convention for {hertz} Hz"
    return f"{hertz.numerator}Hz"


def test_rate_group_names_match_rates():
    """Rate group instance names state the rate their divisor produces from the Main.cpp cycle"""
    seconds, microseconds = map(int, re.search(r"startSimulatedCycle\(Fw::TimeInterval\((\d+),\s*(\d+)\)\)", MAIN_CPP.read_text()).groups())
    base_hertz = 1 / (Fraction(seconds) + Fraction(microseconds, 1_000_000))

    divisor_text = re.search(r"DividerSet rateGroupDivisorsSet\{\{(.*?)\}\};", TOPOLOGY_CPP.read_text()).group(1)
    divisors = [int(divisor) for divisor, _ in re.findall(r"\{(\d+),\s*(\d+)\}", divisor_text)]

    topology = TOPOLOGY_FPP.read_text()
    port_names = re.search(r"enum Ports_RateGroups \{(.*?)\}", topology, re.S).group(1).split()
    connected = dict(re.findall(r"rateGroupDriver\.CycleOut\[Ports_RateGroups\.(\w+)\]\s*->\s*(\w+)\.CycleIn", topology))

    assert len(port_names) == len(divisors)
    for port_name, divisor in zip(port_names, divisors):
        expected = "rateGroup" + frequency_name(base_hertz / divisor)
        assert port_name == expected, f"Ports_RateGroups.{port_name} runs at {base_hertz / divisor} Hz"
        assert connected[port_name] == expected, f"{connected[port_name]} runs at {base_hertz / divisor} Hz"

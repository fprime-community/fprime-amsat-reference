""" fprime.fbuild.target_definitions: targets definitions for fprime-util

Defines all the targets for fprime-util. Each target is a singleton that is registered into the list of all targets and
as such, each target need only be instantiated but need not be assigned to anything.

"""

from .gcovr import GcovrTarget
from .target import BuildSystemTarget, DesignatedBuildSystemTarget, TargetScope
from .types import BuildType

#### "build" targets for components, deployments, unittests for both normal and testing builds ####
BuildSystemTarget(
    "",
    mnemonic="build",
    desc="Build components, ports, and deployments",
    scope=TargetScope.BOTH,
)
BuildSystemTarget(
    "ut_exe",
    mnemonic="build",
    desc="Build unittests",
    scope=TargetScope.LOCAL,
    flags={"ut"},
    build_type=BuildType.BUILD_TESTING,
)
BuildSystemTarget(
    "all",
    mnemonic="build",
    desc="Build all components, ports, UTs, and deployments for unittest build",
    scope=TargetScope.GLOBAL,
    flags={"all", "ut"},
    build_type=BuildType.BUILD_TESTING,
)

DesignatedBuildSystemTarget(
    "target",
    mnemonic="build",
    desc="Build a specific CMake target by name",
    scope=TargetScope.GLOBAL,
    flags={"target"},
)

DesignatedBuildSystemTarget(
    "target",
    mnemonic="build",
    desc="Build a specific CMake target by name using the UT build",
    scope=TargetScope.GLOBAL,
    flags={"ut", "target"},
    build_type=BuildType.BUILD_TESTING,
)

#### Check targets ####
check = BuildSystemTarget(
    "check",
    mnemonic="check",
    desc="Build and run unittests",
    build_type=BuildType.BUILD_TESTING,
    scope=TargetScope.BOTH,
)
GcovrTarget(
    check,
    mnemonic="check",
    desc="Build run and calculate coverage of unittests",
    build_type=BuildType.BUILD_TESTING,
    scope=TargetScope.BOTH,
    flags={"coverage"},
)

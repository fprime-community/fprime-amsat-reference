""" fprime.common.utils: defines common utility functions to be used across sub packages

@author thomas-bc
"""

from pathlib import Path


def confirm(msg):
    """Ask user for a yes or no input after displaying the given message"""
    # Loop "forever" intended
    while True:
        confirm_input = input(msg + " (yes/no) [yes]: ")
        # Default to yes when no input
        if confirm_input.lower() in ["y", "yes", ""]:
            return True
        if confirm_input.lower() in ["n", "no"]:
            return False
        print(f"{confirm_input} is invalid.  Please use 'yes' or 'no'")


def replace_contents(filename, what, replacement, count=1):
    """Replace the first instance of what with replacement in filename"""
    changelog = Path(filename).read_text()
    with open(filename, "w") as fh:
        new_file = changelog.replace(what, replacement, count)
        fh.write(new_file)
        return new_file != changelog


def check_path_is_within_fprime_module(path: Path):
    """Check if the current working directory is within an F prime module

    This is done by checking if any line in the path/CMakeLists.txt file starts
    with register_fprime_*

    This is useful to prevent users from running commands in the wrong location.

    Returns:
        bool: True if the path is within an F prime module, False otherwise
    """

    if not Path(path / "CMakeLists.txt").exists():
        return False

    with open(path / "CMakeLists.txt") as cmake_file:
        lines = cmake_file.readlines()

    # Looks for line "register_fprime_*" in CMakeLists.txt
    statement = "register_fprime_"
    for line in lines:
        if line.startswith(statement):
            return True
    return False

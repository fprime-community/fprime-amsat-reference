# Pi Zero 2 Project Configuration
# This excludes baremetal and arduino components that aren't needed for Pi Zero 2

# Only include the USBSoundCard component and CDH deployment
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Components/USBSoundCard/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/CDHDeployment/")

# Explicitly exclude baremetal and arduino components
# These are not needed for Pi Zero 2 deployment
# The fprime-baremetal and fprime-arduino libraries will be ignored 
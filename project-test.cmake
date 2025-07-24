# Test-specific project configuration
# This excludes baremetal components that aren't needed for Pi Zero testing

# Only include the USBSoundCard component and CDH deployment
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Components/USBSoundCard/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/CDHDeployment/")

# Explicitly exclude baremetal and arduino components
# These are not needed for Pi Zero deployment testing 
# This CMake file is intended to register project-wide objects.
# This allows for reuse between deployments, or other projects.

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/MainDeployment/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/CHDDeployment/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-amsat-sim/amsat/Svc/GyroSensor/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-amsat-sim/amsat/Svc/PressureSensor/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-amsat-sim/amsat/Svc/TempSensor/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-amsat-sim/amsat/Svc/COMMS/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-amsat-sim/amsat/Svc/PowerManager/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/fprime-amsat-sim/amsat/Drv/GpioDriver/")

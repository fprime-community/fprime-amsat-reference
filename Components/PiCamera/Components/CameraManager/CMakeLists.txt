####
# FPrime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
# UT_SOURCE_FILES: list of source files for unit tests
#
# More information in the F´ CMake API documentation:
# https://fprime.jpl.nasa.gov/latest/docs/user-manual/build-system/cmake-api/
#
####

register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/CameraManager.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/CameraManager.cpp"
    DEPENDS
        Os
)

set(UT_SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/CameraManager.fpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CameraManagerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CameraManagerTestMain.cpp"
)
set(UT_AUTO_HELPERS ON)
register_fprime_ut()

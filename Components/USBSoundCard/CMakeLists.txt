####
# F Prime CMakeLists.txt:
#
# SOURCES: list of source files (to be compiled)
# AUTOCODER_INPUTS: list of files to be passed to the autocoders
# DEPENDS: list of libraries that this module depends on
#
# More information in the F´ CMake API documentation:
# https://fprime.jpl.nasa.gov/latest/docs/reference/api/cmake/API/
#
####

# Register F Prime component (not library)
register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/USBSoundCard.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/USBSoundCard.cpp"
)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(ALSA alsa)
    if(ALSA_FOUND)
        target_link_libraries(${MODULE_NAME} PRIVATE ${ALSA_LIBRARIES})
        target_include_directories(${MODULE_NAME} PRIVATE ${ALSA_INCLUDE_DIRS})
        target_compile_options(${MODULE_NAME} PRIVATE ${ALSA_CFLAGS_OTHER})
        message(STATUS "Found ALSA via pkg-config")
    else()
        # Fallback to direct linking
        target_link_libraries(${MODULE_NAME} PRIVATE asound)
        message(STATUS "Using direct ALSA linking")
    endif()
else()
    # Fallback to direct linking
    target_link_libraries(${MODULE_NAME} PRIVATE asound)
    message(STATUS "Using direct ALSA linking (no pkg-config)")
endif()

### Unit Tests ###
register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/USBSoundCard.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/unit/USBSoundCardTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/unit/USBSoundCardTester.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/unit/PacketReceiver.cpp"
    DEPENDS
        STest # For rules-based testing
    UT_AUTO_HELPERS
)

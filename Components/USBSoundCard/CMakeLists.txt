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

# Handle ALSA library linking with fallback for cross-compilation
if(CMAKE_CROSSCOMPILING)
    # For cross-compilation, don't link ALSA due to GLIBC version issues
    # ALSA functionality will be loaded dynamically on the Pi
    message(STATUS "Cross-compiling: ALSA functionality stubbed out")
else()
    # For native compilation, try pkg-config first, fall back to direct linking
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ALSA alsa)
        if(ALSA_FOUND)
            target_link_libraries(${MODULE_NAME} ${ALSA_LIBRARIES})
            target_include_directories(${MODULE_NAME} PRIVATE ${ALSA_INCLUDE_DIRS})
            target_compile_options(${MODULE_NAME} PRIVATE ${ALSA_CFLAGS_OTHER})
        else()
            # Fallback to direct linking
            target_link_libraries(${MODULE_NAME} asound)
        endif()
    else()
        # Fallback to direct linking
        target_link_libraries(${MODULE_NAME} asound)
    endif()
endif()

### Unit Tests ###
# register_fprime_ut(
#     AUTOCODER_INPUTS
#         "${CMAKE_CURRENT_LIST_DIR}/USBSoundCard.fpp"
#     SOURCES
#         "${CMAKE_CURRENT_LIST_DIR}/test/ut/USBSoundCardTestMain.cpp"
#         "${CMAKE_CURRENT_LIST_DIR}/test/ut/USBSoundCardTester.cpp"
#     DEPENDS
#         STest # For rules-based testing
#     UT_AUTO_HELPERS
# )
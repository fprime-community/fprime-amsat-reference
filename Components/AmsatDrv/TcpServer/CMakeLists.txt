####
# F' CMakeLists.txt:
#
# AmsatDrv::TcpServer is Drv::TcpServer with fixes for disconnected sends and shutdown. See TcpServerComponentImpl.hpp.
####
register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/TcpServer.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/TcpServerComponentImpl.cpp"
    DEPENDS
        Fw_Logger
        Drv_ByteStreamDriverModel
        Drv_Ip
)

set(UT_SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/TcpServer.fpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/TcpServerTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/TcpServerTester.cpp"
)
set(UT_MOD_DEPS
    STest
    SocketTestHelper
)
set(UT_AUTO_HELPERS ON)
register_fprime_ut()
set(UT_TARGET_NAME "${FPRIME_CURRENT_MODULE}_ut_exe")
if (TARGET "${UT_TARGET_NAME}")
    target_compile_options("${UT_TARGET_NAME}" PRIVATE -Wno-conversion)
endif()

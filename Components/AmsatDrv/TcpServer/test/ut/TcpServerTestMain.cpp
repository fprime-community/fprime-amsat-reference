// ----------------------------------------------------------------------
// TestMain.cpp
// ----------------------------------------------------------------------

#include "TcpServerTester.hpp"

TEST(Nominal, TcpServerBufferDeallocation) {
    AmsatDrv::TcpServerTester tester;
    tester.test_buffer_deallocation();
}

TEST(Nominal, TcpServerBasicMessaging) {
    AmsatDrv::TcpServerTester tester;
    tester.test_basic_messaging();
}

TEST(Nominal, TcpServerBasicReceiveThread) {
    AmsatDrv::TcpServerTester tester;
    tester.test_receive_thread();
}

TEST(Reconnect, TcpServerMultiMessaging) {
    AmsatDrv::TcpServerTester tester;
    tester.test_multiple_messaging();
}

TEST(Reconnect, TcpServerReceiveThreadReconnect) {
    AmsatDrv::TcpServerTester tester;
    tester.test_advanced_reconnect();
}

TEST(AutoConnect, AutoConnectOnSendOff) {
    AmsatDrv::TcpServerTester tester;
    tester.test_no_automatic_send_connection();
}

TEST(AutoConnect, AutoConnectOnRecvOff) {
    AmsatDrv::TcpServerTester tester;
    tester.test_no_automatic_recv_connection();
}


TEST(Shutdown, SendWithoutConnection) {
    AmsatDrv::TcpServerTester tester;
    tester.test_send_without_connection();
}

TEST(Shutdown, SendWhileReadTaskAccepts) {
    AmsatDrv::TcpServerTester tester;
    tester.test_send_while_read_task_accepts();
}

TEST(Shutdown, TerminateWakesAccept) {
    AmsatDrv::TcpServerTester tester;
    tester.test_terminate_wakes_accept();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

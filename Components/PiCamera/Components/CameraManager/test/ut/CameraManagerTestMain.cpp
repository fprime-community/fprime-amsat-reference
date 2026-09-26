// ======================================================================
// \title  CameraManagerTestMain.cpp
// \brief  cpp file for CameraManager component test main function
// ======================================================================

#include "CameraManagerTester.hpp"

TEST(Nominal, ConfigureCreatesDirectory) {
    PiCamera::CameraManagerTester tester;
    tester.testConfigureCreatesDirectory();
}

TEST(Nominal, TakePicture) {
    PiCamera::CameraManagerTester tester;
    tester.testTakePicture();
}

TEST(Nominal, TakePictureUsesParameters) {
    PiCamera::CameraManagerTester tester;
    tester.testTakePictureUsesParameters();
}

TEST(Nominal, CheckCameraDetected) {
    PiCamera::CameraManagerTester tester;
    tester.testCheckCameraDetected();
}

TEST(Nominal, Ping) {
    PiCamera::CameraManagerTester tester;
    tester.testPing();
}

TEST(Nominal, ParameterCommands) {
    PiCamera::CameraManagerTester tester;
    tester.testParameterCommands();
}

TEST(Nominal, CheckCameraLargeOutput) {
    PiCamera::CameraManagerTester tester;
    tester.testCheckCameraLargeOutput();
}

TEST(OffNominal, ConfigureDirectoryError) {
    PiCamera::CameraManagerTester tester;
    tester.testConfigureDirectoryError();
}

TEST(OffNominal, TakePictureKilledBySignal) {
    PiCamera::CameraManagerTester tester;
    tester.testTakePictureKilledBySignal();
}

TEST(OffNominal, CaptureFailedThrottle) {
    PiCamera::CameraManagerTester tester;
    tester.testCaptureFailedThrottle();
}

TEST(OffNominal, CheckCameraExitError) {
    PiCamera::CameraManagerTester tester;
    tester.testCheckCameraExitError();
}

TEST(OffNominal, TakePictureCommandFails) {
    PiCamera::CameraManagerTester tester;
    tester.testTakePictureCommandFails();
}

TEST(OffNominal, TakePictureNoImage) {
    PiCamera::CameraManagerTester tester;
    tester.testTakePictureNoImage();
}

TEST(OffNominal, TakePictureLaunchFails) {
    PiCamera::CameraManagerTester tester;
    tester.testTakePictureLaunchFails();
}

TEST(OffNominal, CheckCameraNotDetected) {
    PiCamera::CameraManagerTester tester;
    tester.testCheckCameraNotDetected();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

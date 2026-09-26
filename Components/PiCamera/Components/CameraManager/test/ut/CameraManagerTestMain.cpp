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

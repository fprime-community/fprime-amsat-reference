// ======================================================================
// \title  CameraManagerTester.cpp
// \brief  cpp file for CameraManager component test harness implementation class
// ======================================================================

#include "CameraManagerTester.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace PiCamera {

namespace {

//! Fake rpicam-still: writes its arguments into the file given with -o
constexpr const char* STILL_WRITES_ARGS =
    "#!/bin/sh\n"
    "out=''\n"
    "prev=''\n"
    "for arg in \"$@\"; do\n"
    "  if [ \"$prev\" = '-o' ]; then out=\"$arg\"; fi\n"
    "  prev=\"$arg\"\n"
    "done\n"
    "printf '%s\\n' \"$*\" > \"$out\"\n";

//! Fake rpicam-still: fails as it does when no camera is connected
constexpr const char* STILL_FAILS =
    "#!/bin/sh\n"
    "echo 'ERROR: *** no cameras available ***' >&2\n"
    "exit 255\n";

//! Fake rpicam-still: exits cleanly without writing an image
constexpr const char* STILL_WRITES_NOTHING =
    "#!/bin/sh\n"
    "exit 0\n";

//! Fake rpicam-hello: one camera connected
constexpr const char* HELLO_CAMERA =
    "#!/bin/sh\n"
    "echo 'Available cameras'\n"
    "echo '-----------------'\n"
    "echo '0 : ov5647 [2592x1944 10-bit GBRG] (/base/soc/i2c0mux/i2c@1/ov5647@36)'\n";

//! Fake rpicam-hello: no camera connected
constexpr const char* HELLO_NO_CAMERA =
    "#!/bin/sh\n"
    "echo 'No cameras available!'\n";

}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

CameraManagerTester ::CameraManagerTester()
    : CameraManagerGTestBase("CameraManagerTester", CameraManagerTester::MAX_HISTORY_SIZE),
      component("CameraManager") {
    this->initComponents();
    this->connectPorts();
    // As in the topology, load parameters at startup. Parameters not set by a test fall back to their defaults.
    this->component.loadParameters();

    char scratchTemplate[] = "/tmp/CameraManagerUt_XXXXXX";
    const char* scratch = mkdtemp(scratchTemplate);
    EXPECT_NE(scratch, nullptr);
    this->m_scratchDirectory = (scratch != nullptr) ? scratch : "/tmp";
    this->m_imageDirectory = this->m_scratchDirectory + "/images";
}

CameraManagerTester ::~CameraManagerTester() {
    const std::string command = "rm -rf '" + this->m_scratchDirectory + "'";
    if (this->m_scratchDirectory.rfind("/tmp/CameraManagerUt_", 0) == 0) {
        (void)system(command.c_str());
    }
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void CameraManagerTester ::testConfigureCreatesDirectory() {
    this->configureComponent(this->writeScript("still", STILL_WRITES_ARGS),
                             this->writeScript("hello", HELLO_CAMERA));
    struct stat info;
    ASSERT_EQ(stat(this->m_imageDirectory.c_str(), &info), 0);
    ASSERT_TRUE(S_ISDIR(info.st_mode));
    ASSERT_EVENTS_ImageDirectoryError_SIZE(0);
}

void CameraManagerTester ::testTakePicture() {
    this->configureComponent(this->writeScript("still", STILL_WRITES_ARGS),
                             this->writeScript("hello", HELLO_CAMERA));
    this->setTestTime(Fw::Time(1234, 0));

    this->takePicture(Fw::CmdResponse::OK);

    const std::string expectedPath = this->m_imageDirectory + "/img_1234_0.jpg";
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_PictureTaken_SIZE(1);
    const std::string arguments = readFile(expectedPath.c_str());
    ASSERT_EQ(arguments, "-n -o " + expectedPath + " --width 320 --height 256 -t 1000\n");
    ASSERT_EVENTS_PictureTaken(0, expectedPath.c_str(), arguments.size());
    ASSERT_TLM_PicturesTaken_SIZE(1);
    ASSERT_TLM_PicturesTaken(0, 1);
    ASSERT_TLM_LastImageSize(0, arguments.size());
    ASSERT_TLM_CaptureFailures_SIZE(0);

    // A second picture in the same second gets a distinct name
    this->clearHistory();
    this->takePicture(Fw::CmdResponse::OK);
    const std::string secondPath = this->m_imageDirectory + "/img_1234_1.jpg";
    ASSERT_EVENTS_PictureTaken_SIZE(1);
    ASSERT_EQ(readFile(secondPath.c_str()), "-n -o " + secondPath + " --width 320 --height 256 -t 1000\n");
    ASSERT_TLM_PicturesTaken(0, 2);
}

void CameraManagerTester ::testTakePictureUsesParameters() {
    this->configureComponent(this->writeScript("still", STILL_WRITES_ARGS),
                             this->writeScript("hello", HELLO_CAMERA));
    this->paramSet_IMAGE_WIDTH(640, Fw::ParamValid::VALID);
    this->paramSet_IMAGE_HEIGHT(480, Fw::ParamValid::VALID);
    this->paramSet_CAPTURE_DELAY_MS(50, Fw::ParamValid::VALID);
    this->component.loadParameters();
    this->setTestTime(Fw::Time(7, 0));

    this->takePicture(Fw::CmdResponse::OK);

    const std::string expectedPath = this->m_imageDirectory + "/img_7_0.jpg";
    ASSERT_EQ(readFile(expectedPath.c_str()), "-n -o " + expectedPath + " --width 640 --height 480 -t 50\n");
}

void CameraManagerTester ::testTakePictureCommandFails() {
    this->configureComponent(this->writeScript("still", STILL_FAILS), this->writeScript("hello", HELLO_CAMERA));
    this->setTestTime(Fw::Time(1, 0));

    this->takePicture(Fw::CmdResponse::EXECUTION_ERROR);

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CaptureFailed_SIZE(1);
    ASSERT_EVENTS_CaptureFailed(0, (this->m_imageDirectory + "/img_1_0.jpg").c_str(), 255);
    ASSERT_TLM_CaptureFailures_SIZE(1);
    ASSERT_TLM_CaptureFailures(0, 1);
    ASSERT_TLM_PicturesTaken_SIZE(0);
}

void CameraManagerTester ::testTakePictureNoImage() {
    this->configureComponent(this->writeScript("still", STILL_WRITES_NOTHING),
                             this->writeScript("hello", HELLO_CAMERA));
    this->setTestTime(Fw::Time(1, 0));

    this->takePicture(Fw::CmdResponse::EXECUTION_ERROR);

    ASSERT_EVENTS_CaptureFailed_SIZE(1);
    ASSERT_EVENTS_CaptureFailed(0, (this->m_imageDirectory + "/img_1_0.jpg").c_str(), 0);
    ASSERT_TLM_CaptureFailures(0, 1);
}

void CameraManagerTester ::testTakePictureLaunchFails() {
    const std::string missing = this->m_scratchDirectory + "/does-not-exist";
    this->configureComponent(missing, this->writeScript("hello", HELLO_CAMERA));

    this->takePicture(Fw::CmdResponse::EXECUTION_ERROR);

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_LaunchFailed_SIZE(1);
    ASSERT_EVENTS_LaunchFailed(0, missing.c_str(), ENOENT);
    ASSERT_TLM_CaptureFailures(0, 1);
}

void CameraManagerTester ::testCheckCameraDetected() {
    this->configureComponent(this->writeScript("still", STILL_WRITES_ARGS),
                             this->writeScript("hello", HELLO_CAMERA));

    this->checkCamera(Fw::CmdResponse::OK);

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CameraDetected_SIZE(1);
}

void CameraManagerTester ::testCheckCameraNotDetected() {
    this->configureComponent(this->writeScript("still", STILL_WRITES_ARGS),
                             this->writeScript("hello", HELLO_NO_CAMERA));

    this->checkCamera(Fw::CmdResponse::OK);

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CameraNotDetected_SIZE(1);

    // A missing rpicam-hello is a launch failure, not a missing camera
    this->clearHistory();
    const std::string missing = this->m_scratchDirectory + "/does-not-exist";
    this->configureComponent(this->writeScript("still", STILL_WRITES_ARGS), missing);
    this->checkCamera(Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_LaunchFailed_SIZE(1);
    ASSERT_EVENTS_LaunchFailed(0, missing.c_str(), ENOENT);
}

void CameraManagerTester ::testPing() {
    this->invoke_to_pingIn(0, 0x1234);
    this->component.doDispatch();
    ASSERT_from_pingOut_SIZE(1);
    ASSERT_from_pingOut(0, 0x1234);
}

// ----------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------

std::string CameraManagerTester ::writeScript(const char* name, const char* body) {
    const std::string path = this->m_scratchDirectory + "/" + name;
    std::ofstream script(path, std::ios::trunc);
    script << body;
    script.close();
    EXPECT_EQ(chmod(path.c_str(), 0755), 0);
    return path;
}

void CameraManagerTester ::configureComponent(const std::string& stillCommand, const std::string& helloCommand) {
    this->component.configure(this->m_imageDirectory.c_str(), stillCommand.c_str(), helloCommand.c_str());
}

void CameraManagerTester ::takePicture(Fw::CmdResponse expected) {
    this->sendCmd_TAKE_PICTURE(0, 10);
    ASSERT_EQ(this->component.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_OK);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, CameraManagerComponentBase::OPCODE_TAKE_PICTURE, 10, expected);
}

void CameraManagerTester ::checkCamera(Fw::CmdResponse expected) {
    this->sendCmd_CHECK_CAMERA(0, 11);
    ASSERT_EQ(this->component.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_OK);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, CameraManagerComponentBase::OPCODE_CHECK_CAMERA, 11, expected);
}

std::string CameraManagerTester ::readFile(const char* path) {
    std::ifstream file(path);
    std::stringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

}  // namespace PiCamera

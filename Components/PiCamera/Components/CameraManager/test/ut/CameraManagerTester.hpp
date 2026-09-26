// ======================================================================
// \title  CameraManagerTester.hpp
// \brief  hpp file for CameraManager component test harness implementation class
// ======================================================================

#ifndef PiCamera_CameraManagerTester_HPP
#define PiCamera_CameraManagerTester_HPP

#include <string>

#include "Components/PiCamera/Components/CameraManager/CameraManager.hpp"
#include "Components/PiCamera/Components/CameraManager/CameraManagerGTestBase.hpp"

namespace PiCamera {

class CameraManagerTester final : public CameraManagerGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object CameraManagerTester, creating a scratch directory for images and fake camera commands
    CameraManagerTester();

    //! Destroy object CameraManagerTester, removing the scratch directory
    ~CameraManagerTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! configure creates the image directory
    void testConfigureCreatesDirectory();

    //! TAKE_PICTURE runs the still command with the default parameters and reports the image
    void testTakePicture();

    //! TAKE_PICTURE passes updated parameters to the still command
    void testTakePictureUsesParameters();

    //! TAKE_PICTURE reports a failure when the still command exits with an error
    void testTakePictureCommandFails();

    //! TAKE_PICTURE reports a failure when the still command exits cleanly without writing an image
    void testTakePictureNoImage();

    //! TAKE_PICTURE reports a launch failure when the still command does not exist
    void testTakePictureLaunchFails();

    //! CHECK_CAMERA reports a detected camera
    void testCheckCameraDetected();

    //! CHECK_CAMERA reports no camera
    void testCheckCameraNotDetected();

    //! pingIn is answered on pingOut
    void testPing();

    //! configure reports an image directory that cannot be created
    void testConfigureDirectoryError();

    //! TAKE_PICTURE reports exit status -1 when the still command is killed by a signal
    void testTakePictureKilledBySignal();

    //! CaptureFailed events are throttled while CaptureFailures telemetry counts every failure
    void testCaptureFailedThrottle();

    //! Parameter set commands change the arguments passed to the still command
    void testParameterCommands();

    //! CHECK_CAMERA reports no camera when rpicam-hello lists cameras but exits with an error
    void testCheckCameraExitError();

    //! CHECK_CAMERA reads large rpicam-hello output without blocking
    void testCheckCameraLargeOutput();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Write an executable shell script to the scratch directory, returning its path
    std::string writeScript(const char* name, const char* body);

    //! Configure the component with fake commands and the scratch image directory
    void configureComponent(const std::string& stillCommand, const std::string& helloCommand);

    //! Send TAKE_PICTURE and dispatch it, checking the command response
    void takePicture(Fw::CmdResponse expected);

    //! Send CHECK_CAMERA and dispatch it, checking the command response
    void checkCamera(Fw::CmdResponse expected);

    //! Read a whole file into a string
    static std::string readFile(const char* path);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    CameraManager component;

    //! Scratch directory holding fake commands and images
    std::string m_scratchDirectory;

    //! Image directory passed to the component
    std::string m_imageDirectory;
};

}  // namespace PiCamera

#endif

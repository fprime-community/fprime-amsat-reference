module PiCamera {
    @ Component capturing still images from the Raspberry Pi camera by running rpicam-still
    active component CameraManager {

        # ----------------------------------------------------------------------
        # Commands
        # ----------------------------------------------------------------------

        @ Capture a still image to a new JPEG file in the image directory
        async command TAKE_PICTURE opcode 0x00

        @ Check whether a camera is connected by running rpicam-hello --list-cameras
        async command CHECK_CAMERA opcode 0x01

        # ----------------------------------------------------------------------
        # Parameters
        # ----------------------------------------------------------------------

        @ Width of captured images in pixels
        param IMAGE_WIDTH: U16 default 320

        @ Height of captured images in pixels
        param IMAGE_HEIGHT: U16 default 256

        @ Time in milliseconds rpicam-still runs the preview pipeline before capturing, letting exposure settle
        param CAPTURE_DELAY_MS: U32 default 1000

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        @ A picture was captured
        event PictureTaken(
            path: string size FileNameStringSize @< Path of the image file
            fileSize: FwSizeType @< Size of the image file in bytes
        ) severity activity high format "Picture saved to {} ({} bytes)"

        @ rpicam-still ran but did not produce an image
        event CaptureFailed(
            path: string size FileNameStringSize @< Path of the requested image file
            exitStatus: I32 @< Exit status of rpicam-still, or -1 if it did not exit normally
        ) severity warning high format "Capture of {} failed: rpicam-still exit status {}" throttle 5

        @ A camera command could not be started
        event LaunchFailed(
            commandName: string size FileNameStringSize @< Command that could not be started
            error: I32 @< Error number from posix_spawnp
        ) severity warning high format "Could not start {}: error {}" throttle 5

        @ The image directory could not be created
        event ImageDirectoryError(
            directory: string size FileNameStringSize @< Image directory
            status: I32 @< Os::FileSystem status
        ) severity warning high format "Could not create image directory {}: status {}"

        @ A camera was found
        event CameraDetected() severity activity high format "Camera detected"

        @ No camera was found
        event CameraNotDetected() severity warning high format "No camera detected"

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Number of pictures captured
        telemetry PicturesTaken: U32

        @ Number of failed capture attempts
        telemetry CaptureFailures: U32

        @ Size in bytes of the most recent image
        telemetry LastImageSize: FwSizeType

        # ----------------------------------------------------------------------
        # Health ping
        # ----------------------------------------------------------------------

        @ Ping input port
        async input port pingIn: Svc.Ping

        @ Ping output port
        output port pingOut: Svc.Ping

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Command receive port
        command recv port CmdDisp

        @ Command registration port
        command reg port CmdReg

        @ Command response port
        command resp port CmdStatus

        @ Event port
        event port Log

        @ Text event port
        text event port LogText

        @ Parameter get port
        param get port PrmGet

        @ Parameter set port
        param set port PrmSet

    }
}

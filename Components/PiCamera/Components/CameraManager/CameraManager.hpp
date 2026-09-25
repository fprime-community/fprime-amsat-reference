// ======================================================================
// \title  CameraManager.hpp
// \brief  hpp file for CameraManager component implementation class
// ======================================================================

#ifndef PiCamera_CameraManager_HPP
#define PiCamera_CameraManager_HPP

#include "Components/PiCamera/Components/CameraManager/CameraManagerComponentAc.hpp"
#include "Fw/Types/FileNameString.hpp"

namespace PiCamera {

class CameraManager final : public CameraManagerComponentBase {
  public:
    //! Default command used to capture still images
    static constexpr const char* DEFAULT_STILL_COMMAND = "rpicam-still";

    //! Default command used to list connected cameras
    static constexpr const char* DEFAULT_HELLO_COMMAND = "rpicam-hello";

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct CameraManager object
    CameraManager(const char* const compName  //!< The component name
    );

    //! Destroy CameraManager object
    ~CameraManager();

    //! Configure the image directory and camera commands, creating the directory if needed
    //!
    //! Commands are found on the PATH. Older Raspberry Pi OS releases name them libcamera-still and libcamera-hello.
    void configure(const char* imageDirectory,                         //!< Directory images are written to
                   const char* stillCommand = DEFAULT_STILL_COMMAND,  //!< Command used to capture still images
                   const char* helloCommand = DEFAULT_HELLO_COMMAND   //!< Command used to list connected cameras
    );

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for pingIn
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command TAKE_PICTURE
    void TAKE_PICTURE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                 U32 cmdSeq            //!< The command sequence number
                                 ) override;

    //! Handler implementation for command CHECK_CAMERA
    void CHECK_CAMERA_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                 U32 cmdSeq            //!< The command sequence number
                                 ) override;

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    Fw::FileNameString m_imageDirectory;  //!< Directory images are written to
    Fw::FileNameString m_stillCommand;    //!< Command used to capture still images
    Fw::FileNameString m_helloCommand;    //!< Command used to list connected cameras
    U32 m_picturesTaken;                  //!< Number of pictures captured
    U32 m_captureFailures;                //!< Number of failed capture attempts
};

}  // namespace PiCamera

#endif

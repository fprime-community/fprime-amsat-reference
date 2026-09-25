// ======================================================================
// \title  CameraManager.cpp
// \brief  cpp file for CameraManager component implementation class
// ======================================================================

#include "Components/PiCamera/Components/CameraManager/CameraManager.hpp"
#include "Os/FileSystem.hpp"

#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>

extern char** environ;

namespace {

//! Text rpicam-hello --list-cameras prints when at least one camera is connected
constexpr const char* CAMERAS_AVAILABLE_TEXT = "Available cameras";

//! Size of the buffer holding rpicam-hello output. Output beyond this is read and discarded.
constexpr size_t HELLO_OUTPUT_SIZE = 1024;

//! Start a command found on the PATH
//!
//! The command's stdout and stderr are written to outputFd, or discarded when outputFd is negative.
//!
//! \return 0 when the command was started, otherwise the posix_spawn error number
int startCommand(char* const argv[], int outputFd, pid_t& pid) {
    posix_spawn_file_actions_t actions;
    int status = posix_spawn_file_actions_init(&actions);
    if (status != 0) {
        return status;
    }
    if (outputFd >= 0) {
        (void)posix_spawn_file_actions_adddup2(&actions, outputFd, STDOUT_FILENO);
        (void)posix_spawn_file_actions_adddup2(&actions, outputFd, STDERR_FILENO);
    } else {
        (void)posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
        (void)posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    }
    status = posix_spawnp(&pid, argv[0], &actions, nullptr, argv, environ);
    (void)posix_spawn_file_actions_destroy(&actions);
    return status;
}

//! Wait for a command started by startCommand, returning its exit status or -1 if it did not exit normally
I32 waitCommand(pid_t pid) {
    int waitStatus = 0;
    pid_t result = 0;
    do {
        result = waitpid(pid, &waitStatus, 0);
    } while (result < 0 && errno == EINTR);
    return (result == pid && WIFEXITED(waitStatus)) ? static_cast<I32>(WEXITSTATUS(waitStatus)) : -1;
}

}  // namespace

namespace PiCamera {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

CameraManager ::CameraManager(const char* const compName)
    : CameraManagerComponentBase(compName),
      m_imageDirectory("."),
      m_stillCommand(DEFAULT_STILL_COMMAND),
      m_helloCommand(DEFAULT_HELLO_COMMAND),
      m_picturesTaken(0),
      m_captureFailures(0) {}

CameraManager ::~CameraManager() {}

void CameraManager ::configure(const char* imageDirectory, const char* stillCommand, const char* helloCommand) {
    FW_ASSERT(imageDirectory != nullptr);
    FW_ASSERT(stillCommand != nullptr);
    FW_ASSERT(helloCommand != nullptr);
    this->m_imageDirectory = imageDirectory;
    this->m_stillCommand = stillCommand;
    this->m_helloCommand = helloCommand;

    Os::FileSystem::Status status = Os::FileSystem::createDirectory(imageDirectory, false);
    if (status != Os::FileSystem::OP_OK) {
        this->log_WARNING_HI_ImageDirectoryError(this->m_imageDirectory, static_cast<I32>(status));
    }
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void CameraManager ::pingIn_handler(FwIndexType portNum, U32 key) {
    this->pingOut_out(0, key);
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void CameraManager ::TAKE_PICTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    Fw::ParamValid valid;
    const U16 width = this->paramGet_IMAGE_WIDTH(valid);
    const U16 height = this->paramGet_IMAGE_HEIGHT(valid);
    const U32 delayMs = this->paramGet_CAPTURE_DELAY_MS(valid);

    // Name images by capture time, with the capture count keeping names unique within a second
    Fw::FileNameString path;
    path.format("%s/img_%" PRIu32 "_%" PRIu32 ".jpg", this->m_imageDirectory.toChar(), this->getTime().getSeconds(),
                this->m_picturesTaken + this->m_captureFailures);

    char widthText[8];
    char heightText[8];
    char delayText[12];
    (void)snprintf(widthText, sizeof(widthText), "%" PRIu16, width);
    (void)snprintf(heightText, sizeof(heightText), "%" PRIu16, height);
    (void)snprintf(delayText, sizeof(delayText), "%" PRIu32, delayMs);

    char* const argv[] = {const_cast<char*>(this->m_stillCommand.toChar()),
                          const_cast<char*>("-n"),
                          const_cast<char*>("-o"),
                          const_cast<char*>(path.toChar()),
                          const_cast<char*>("--width"),
                          widthText,
                          const_cast<char*>("--height"),
                          heightText,
                          const_cast<char*>("-t"),
                          delayText,
                          nullptr};

    // Remove any file left over from an earlier run so a stale image is never reported as new
    (void)unlink(path.toChar());

    pid_t pid = 0;
    const int spawnError = startCommand(argv, -1, pid);
    if (spawnError != 0) {
        this->m_captureFailures++;
        this->tlmWrite_CaptureFailures(this->m_captureFailures);
        this->log_WARNING_HI_LaunchFailed(this->m_stillCommand, static_cast<I32>(spawnError));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    const I32 exitStatus = waitCommand(pid);

    FwSizeType fileSize = 0;
    const bool captured = (exitStatus == 0) &&
                          (Os::FileSystem::getFileSize(path.toChar(), fileSize) == Os::FileSystem::OP_OK) &&
                          (fileSize > 0);
    if (!captured) {
        this->m_captureFailures++;
        this->tlmWrite_CaptureFailures(this->m_captureFailures);
        this->log_WARNING_HI_CaptureFailed(path, exitStatus);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->m_picturesTaken++;
    this->tlmWrite_PicturesTaken(this->m_picturesTaken);
    this->tlmWrite_LastImageSize(fileSize);
    this->log_ACTIVITY_HI_PictureTaken(path, fileSize);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void CameraManager ::CHECK_CAMERA_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    int pipeFds[2];
    if (pipe(pipeFds) != 0) {
        this->log_WARNING_HI_LaunchFailed(this->m_helloCommand, static_cast<I32>(errno));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    // Keep the read end out of the child so the pipe closes when the child exits
    (void)fcntl(pipeFds[0], F_SETFD, FD_CLOEXEC);

    char* const argv[] = {const_cast<char*>(this->m_helloCommand.toChar()), const_cast<char*>("--list-cameras"),
                          nullptr};
    pid_t pid = 0;
    const int spawnError = startCommand(argv, pipeFds[1], pid);
    (void)close(pipeFds[1]);
    if (spawnError != 0) {
        (void)close(pipeFds[0]);
        this->log_WARNING_HI_LaunchFailed(this->m_helloCommand, static_cast<I32>(spawnError));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    // Read all output so the child never blocks on a full pipe, keeping the first HELLO_OUTPUT_SIZE bytes
    char output[HELLO_OUTPUT_SIZE] = {};
    size_t used = 0;
    char discard[256];
    for (;;) {
        char* destination = (used < sizeof(output) - 1) ? output + used : discard;
        size_t space = (used < sizeof(output) - 1) ? sizeof(output) - 1 - used : sizeof(discard);
        ssize_t count = read(pipeFds[0], destination, space);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            break;
        }
        if (destination == output + used) {
            used += static_cast<size_t>(count);
        }
    }
    (void)close(pipeFds[0]);

    const I32 exitStatus = waitCommand(pid);

    if (exitStatus == 0 && strstr(output, CAMERAS_AVAILABLE_TEXT) != nullptr) {
        this->log_ACTIVITY_HI_CameraDetected();
    } else {
        this->log_WARNING_HI_CameraNotDetected();
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace PiCamera

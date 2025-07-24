// ======================================================================
// \title  USBSoundCard.cpp
// \author root
// \brief  cpp file for USBSoundCard component implementation class
// ======================================================================

#include "Components/USBSoundCard/USBSoundCard.hpp"
#include <cmath>
#include <cstring>

#ifdef ALSA_STUBBED
// Stub ALSA functions for cross-compilation
#define SND_PCM_STREAM_CAPTURE 1
#define SND_PCM_FORMAT_S16_LE 2

struct snd_pcm_hw_params_t;

static inline int snd_pcm_open(snd_pcm_t **pcm, const char *name, int stream, int mode) {
    printf("[USB_SOUND] STUB: snd_pcm_open called (ALSA not available during cross-compilation)\n");
    return -1;
}

static inline int snd_pcm_close(snd_pcm_t *pcm) {
    printf("[USB_SOUND] STUB: snd_pcm_close called\n");
    return 0;
}

static inline void snd_pcm_hw_params_alloca(snd_pcm_hw_params_t **ptr) {
    printf("[USB_SOUND] STUB: snd_pcm_hw_params_alloca called\n");
    *ptr = nullptr;
}

static inline int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params) {
    printf("[USB_SOUND] STUB: snd_pcm_hw_params_any called\n");
    return -1;
}

static inline int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, int format) {
    printf("[USB_SOUND] STUB: snd_pcm_hw_params_set_format called\n");
    return -1;
}

static inline int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir) {
    printf("[USB_SOUND] STUB: snd_pcm_hw_params_set_rate_near called\n");
    return -1;
}

static inline int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val) {
    printf("[USB_SOUND] STUB: snd_pcm_hw_params_set_channels called\n");
    return -1;
}

static inline int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params) {
    printf("[USB_SOUND] STUB: snd_pcm_hw_params called\n");
    return -1;
}

static inline int snd_pcm_prepare(snd_pcm_t *pcm) {
    printf("[USB_SOUND] STUB: snd_pcm_prepare called\n");
    return -1;
}

static inline int snd_pcm_readi(snd_pcm_t *pcm, const void *buffer, unsigned long size) {
    printf("[USB_SOUND] STUB: snd_pcm_readi called\n");
    return -1;
}

static inline int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent) {
    printf("[USB_SOUND] STUB: snd_pcm_recover called\n");
    return -1;
}

static inline const char* snd_strerror(int errnum) {
    return "ALSA stubbed out during cross-compilation";
}

#else
#include <alsa/asoundlib.h>
#endif

namespace Components {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

USBSoundCard::USBSoundCard(const char *const compName)
    : USBSoundCardComponentBase(compName),
      m_audioCapturing(false),
      m_transmissionActive(false),
      m_audioDevice(nullptr),
      m_audioBuffer(nullptr),
      m_bufferSize(1024),
      m_framesProcessed(0),
      m_packetsTransmitted(0),
      m_packetSequence(0)
{
    // Initialize audio buffer
    m_audioBuffer = new short[m_bufferSize];
    
    // Initialize transmission buffer
    initializeTransmissionBuffer();
}

USBSoundCard::~USBSoundCard() {
    if (m_audioCapturing) {
        stopAudioCapture();
    }
    delete[] m_audioBuffer;
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void USBSoundCard::START_CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (m_audioCapturing) {
        this->log_WARNING_HI_AUDIO_CAPTURE_ALREADY_STARTED();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    
    if (initializeAudioDevice()) {
        m_audioCapturing = true;
        this->log_ACTIVITY_LO_AUDIO_CAPTURE_STARTED();
        this->tlmWrite_DEVICE_CONNECTED(true);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        this->log_WARNING_HI_DEVICE_DISCONNECTED();
        this->tlmWrite_DEVICE_CONNECTED(false);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
}

void USBSoundCard::STOP_CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!m_audioCapturing) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    
    stopAudioCapture();
    m_audioCapturing = false;
    this->log_ACTIVITY_LO_AUDIO_CAPTURE_STOPPED();
    this->tlmWrite_DEVICE_CONNECTED(false);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void USBSoundCard::START_TRANSMISSION_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (m_transmissionActive) {
        this->log_WARNING_HI_AUDIO_TRANSMISSION_ALREADY_STARTED();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    
    m_transmissionActive = true;
    this->log_ACTIVITY_LO_AUDIO_TRANSMISSION_STARTED();
    this->tlmWrite_TRANSMISSION_ACTIVE(true);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void USBSoundCard::STOP_TRANSMISSION_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!m_transmissionActive) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    
    m_transmissionActive = false;
    this->log_ACTIVITY_LO_AUDIO_TRANSMISSION_STOPPED();
    this->tlmWrite_TRANSMISSION_ACTIVE(false);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void USBSoundCard::SEND_TEST_PACKET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (sendTestPacket()) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
}

// ----------------------------------------------------------------------
// Handler implementations for input ports
// ----------------------------------------------------------------------

void USBSoundCard::run_handler(FwIndexType portNum, U32 context) {
    // This gets called periodically by the rate group
    if (m_audioCapturing) {
        processAudioData();
    }
}

// ----------------------------------------------------------------------
// Private helper methods
// ----------------------------------------------------------------------

bool USBSoundCard::initializeAudioDevice() {
    int err;
    
    printf("[USB_SOUND] Attempting to open USB sound card...\n");
    
    // Try multiple device names for USB sound card
    const char* deviceNames[] = {
        "hw:1,0",                    // Most common USB device
        "plughw:CARD=Device,DEV=0",  // ALSA plugin wrapper
        "hw:CARD=Device,DEV=0",      // Direct hardware access
        "default"                    // System default
    };
    
    for (size_t i = 0; i < sizeof(deviceNames)/sizeof(deviceNames[0]); i++) {
        printf("[USB_SOUND] Trying device: %s\n", deviceNames[i]);
        err = snd_pcm_open(&m_audioDevice, deviceNames[i], SND_PCM_STREAM_CAPTURE, 0);
        if (err >= 0) {
            printf("[USB_SOUND] Successfully opened device: %s\n", deviceNames[i]);
            break;
        }
        printf("[USB_SOUND] Failed to open %s: %s\n", deviceNames[i], snd_strerror(err));
    }
    
    if (err < 0) {
        printf("[USB_SOUND] ERROR: Could not open any audio device\n");
        return false;
    }
    
    // Set hardware parameters
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(m_audioDevice, params);
    
    // Set format to 16-bit signed PCM
    err = snd_pcm_hw_params_set_format(m_audioDevice, params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        printf("[USB_SOUND] ERROR: Cannot set format: %s\n", snd_strerror(err));
        snd_pcm_close(m_audioDevice);
        return false;
    }
    
    // Set sample rate to 44.1kHz
    unsigned int sampleRate = 44100;
    err = snd_pcm_hw_params_set_rate_near(m_audioDevice, params, &sampleRate, 0);
    if (err < 0) {
        printf("[USB_SOUND] ERROR: Cannot set sample rate: %s\n", snd_strerror(err));
        snd_pcm_close(m_audioDevice);
        return false;
    }
    printf("[USB_SOUND] Sample rate set to: %u Hz\n", sampleRate);
    
    // Set to mono (1 channel)
    err = snd_pcm_hw_params_set_channels(m_audioDevice, params, 1);
    if (err < 0) {
        printf("[USB_SOUND] ERROR: Cannot set channels: %s\n", snd_strerror(err));
        snd_pcm_close(m_audioDevice);
        return false;
    }
    
    // Apply parameters
    err = snd_pcm_hw_params(m_audioDevice, params);
    if (err < 0) {
        printf("[USB_SOUND] ERROR: Cannot set hardware parameters: %s\n", snd_strerror(err));
        snd_pcm_close(m_audioDevice);
        return false;
    }
    
    // Prepare the device
    err = snd_pcm_prepare(m_audioDevice);
    if (err < 0) {
        printf("[USB_SOUND] ERROR: Cannot prepare device: %s\n", snd_strerror(err));
        snd_pcm_close(m_audioDevice);
        return false;
    }
    
    printf("[USB_SOUND] Audio device initialized successfully\n");
    return true;
}

void USBSoundCard::stopAudioCapture() {
    if (m_audioDevice) {
        snd_pcm_close(m_audioDevice);
        m_audioDevice = nullptr;
    }
}

void USBSoundCard::processAudioData() {
    if (!m_audioDevice) {
        return;
    }
    
    // Read audio data
    int frames = snd_pcm_readi(m_audioDevice, m_audioBuffer, m_bufferSize);
    
    if (frames < 0) {
        // Handle underrun or other errors
        printf("Audio read error: %s\n", snd_strerror(frames));
        snd_pcm_recover(m_audioDevice, frames, 0);
        return;
    }
    
    if (frames == 0) {
        printf("No audio frames read\n");
        return;
    }
    
    // Calculate audio level (RMS)
    long sum = 0;
    short maxSample = 0;
    for (int i = 0; i < frames; i++) {
        sum += (long)m_audioBuffer[i] * m_audioBuffer[i];
        if (abs(m_audioBuffer[i]) > abs(maxSample)) {
            maxSample = m_audioBuffer[i];
        }
    }
    
    if (frames > 0) {
        double rms = sqrt((double)sum / frames);
        U32 audioLevel = (U32)(rms / 32767.0 * 255.0); // Scale to 0-255
        
        // Send telemetry
        this->tlmWrite_AUDIO_INPUT_LEVEL(audioLevel);
        m_framesProcessed++;
        this->tlmWrite_FRAMES_PROCESSED(m_framesProcessed);
        
        // Transmit audio data if transmission is active
        if (m_transmissionActive) {
            U32 dataSize = frames * sizeof(short);
            if (transmitAudioPacket(m_audioBuffer, dataSize)) {
                // Update transmission telemetry
                Fw::Time currentTime = this->getTime();
                this->tlmWrite_LAST_TRANSMISSION_TIME(currentTime.getSeconds());
            }
        }
        
        // Debug print every 10th call to avoid spam
        static U32 debugCounter = 0;
        if (++debugCounter % 10 == 0) {
            printf("Frames: %d, RMS Level: %u, Max Sample: %d\n", frames, audioLevel, maxSample);
        }
        
        // Log warning if audio level is too high
        if (audioLevel > 200) {
            static U32 lastWarningTime = 0;
            Fw::Time currentTime = this->getTime();
            
            // Only warn once per second to avoid spam
            if (currentTime.getSeconds() > lastWarningTime) {
                this->log_WARNING_LO_AUDIO_LEVEL_HIGH();
                lastWarningTime = currentTime.getSeconds();
            }
        }
    }
}

U32 USBSoundCard::calculateAudioLevel(const short* buffer, int frames) {
    if (frames <= 0) {
        return 0;
    }
    
    long sum = 0;
    for (int i = 0; i < frames; i++) {
        sum += (long)buffer[i] * buffer[i];
    }
    
    double rms = sqrt((double)sum / frames);
    return (U32)(rms / 32767.0 * 255.0); // Scale to 0-255
}

void USBSoundCard::initializeTransmissionBuffer() {
    // Allocate buffer for transmission (audio data + header)
    const U32 maxAudioSize = m_bufferSize * sizeof(short);
    const U32 headerSize = sizeof(U32) * 3; // sequence, timestamp, data size
    const U32 totalSize = headerSize + maxAudioSize;
    
    // Allocate buffer memory
    U8* bufferData = new U8[totalSize];
    m_transmissionBuffer.setData(bufferData);
    m_transmissionBuffer.setSize(totalSize);
}

bool USBSoundCard::transmitAudioPacket(const void* audioData, U32 dataSize) {
    if (!m_transmissionActive) {
        return false;
    }
    
    try {
        // Get current timestamp
        Fw::Time currentTime = this->getTime();
        
        // Prepare packet header
        U8* bufferPtr = m_transmissionBuffer.getData();
        U32* header = reinterpret_cast<U32*>(bufferPtr);
        
        header[0] = m_packetSequence++;  // Sequence number
        header[1] = currentTime.getSeconds();  // Timestamp seconds
        header[2] = dataSize;  // Audio data size
        
        // Copy audio data after header
        U8* audioPtr = bufferPtr + sizeof(U32) * 3;
        std::memcpy(audioPtr, audioData, dataSize);
        
        // Set buffer size to actual data size
        U32 totalPacketSize = sizeof(U32) * 3 + dataSize;
        m_transmissionBuffer.setSize(totalPacketSize);
        
        // Send packet via buffer output port
        this->bufferOut_out(0, m_transmissionBuffer);
        
        // Update telemetry
        m_packetsTransmitted++;
        this->tlmWrite_PACKETS_TRANSMITTED(m_packetsTransmitted);
        
        // Log transmission event
        this->log_ACTIVITY_LO_PACKET_TRANSMITTED();
        
        return true;
        
    } catch (...) {
        this->log_WARNING_HI_TRANSMISSION_ERROR();
        return false;
    }
}

bool USBSoundCard::sendTestPacket() {
    // Create test audio data (sine wave pattern)
    const U32 testDataSize = 256; // 128 samples * 2 bytes per sample
    short testAudioData[128];
    
    // Generate a simple sine wave pattern
    for (int i = 0; i < 128; i++) {
        double frequency = 440.0; // A4 note
        double sampleRate = 44100.0;
        testAudioData[i] = (short)(32767.0 * sin(2.0 * M_PI * frequency * i / sampleRate));
    }
    
    // Transmit test packet
    bool success = transmitAudioPacket(testAudioData, testDataSize);
    
    if (success) {
        printf("[USB_SOUND] Test packet transmitted successfully\n");
    } else {
        printf("[USB_SOUND] Failed to transmit test packet\n");
    }
    
    return success;
}

} // namespace Components
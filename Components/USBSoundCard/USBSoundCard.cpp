// ======================================================================
// \title  USBSoundCard.cpp
// \author root
// \brief  cpp file for USBSoundCard component implementation class
// ======================================================================

#include "Components/USBSoundCard/USBSoundCard.hpp"
#include <alsa/asoundlib.h>
#include <cmath>

namespace Components {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

USBSoundCard::USBSoundCard(const char *const compName)
    : USBSoundCardComponentBase(compName),
      m_audioCapturing(false),
      m_audioDevice(nullptr),
      m_audioBuffer(nullptr),
      m_bufferSize(1024),
      m_framesProcessed(0)
{
    // Initialize audio buffer
    m_audioBuffer = new short[m_bufferSize];
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

} // namespace Components
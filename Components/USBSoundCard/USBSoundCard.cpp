// ======================================================================
// \title  USBSoundCard.cpp
// \author root
// \brief  cpp file for USBSoundCard component implementation class
// ======================================================================

#include "Components/USBSoundCard/USBSoundCard.hpp"
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <cstring>

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
      m_packetSequence(0),
      m_aprsListenSocket(-1),
      m_aprsServerActive(false),
      m_aprsPacketCount(0)
{
    // Initialize audio buffer
    m_audioBuffer = new short[m_bufferSize];
    
    // Initialize transmission buffer
    initializeTransmissionBuffer();
    
    // Initialize APRS server
    initializeAprsServer();
}

USBSoundCard::~USBSoundCard() {
    if (m_audioCapturing) {
        stopAudioCapture();
    }
    
    // Clean up APRS server
    if (m_aprsServerActive && m_aprsListenSocket >= 0) {
        close(m_aprsListenSocket);
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
    
    // Check for APRS connections
    checkAprsConnections();
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

// ----------------------------------------------------------------------
// APRS Telemetry Methods
// ----------------------------------------------------------------------

void USBSoundCard::sendAprsLatitude(F32 latitude) {
    this->tlmWrite_APRS_LATITUDE(latitude);
    printf("[APRS] Latitude updated: %.6f degrees\n", latitude);
}

void USBSoundCard::sendAprsLongitude(F32 longitude) {
    this->tlmWrite_APRS_LONGITUDE(longitude);
    printf("[APRS] Longitude updated: %.6f degrees\n", longitude);
}

void USBSoundCard::sendAprsBattery(F32 voltage) {
    this->tlmWrite_APRS_BATTERY(voltage);
    printf("[APRS] Battery voltage updated: %.2f V\n", voltage);
}

void USBSoundCard::sendAprsTemperature(F32 temperature) {
    this->tlmWrite_APRS_TEMPERATURE(temperature);
    printf("[APRS] Temperature updated: %.1f C\n", temperature);
}

void USBSoundCard::sendAprsSignalStrength(F32 strength) {
    this->tlmWrite_APRS_SIGNAL_STRENGTH(strength);
    printf("[APRS] Signal strength updated: %.1f dBm\n", strength);
}

void USBSoundCard::logAprsPacketReceived(const char* callsign) {
    m_aprsPacketCount++;
    this->tlmWrite_APRS_PACKET_COUNT(m_aprsPacketCount);
    
    // Convert const char* to Fw::LogStringArg for F Prime logging
    Fw::LogStringArg callsignArg(callsign);
    this->log_ACTIVITY_LO_APRS_PACKET_RECEIVED(callsignArg);
    printf("[APRS] Packet #%u received from %s\n", m_aprsPacketCount, callsign);
}

void USBSoundCard::logAprsPositionUpdate(F32 lat, F32 lon) {
    this->log_ACTIVITY_LO_APRS_POSITION_UPDATE(lat, lon);
    printf("[APRS] Position: LAT=%.6f, LON=%.6f\n", lat, lon);
}

void USBSoundCard::logAprsTelemetryUpdate(F32 battery, F32 temp) {
    this->log_ACTIVITY_LO_APRS_TELEMETRY_UPDATE(battery, temp);
    printf("[APRS] Telemetry: BAT=%.1fV, TEMP=%.1fC\n", battery, temp);
}

void USBSoundCard::logAprsParseError(const char* error) {
    // Convert const char* to Fw::LogStringArg for F Prime logging
    Fw::LogStringArg errorArg(error);
    this->log_WARNING_HI_APRS_PARSE_ERROR(errorArg);
    printf("[APRS] Parse error: %s\n", error);
}

// ----------------------------------------------------------------------
// APRS Interface Methods
// ----------------------------------------------------------------------

void USBSoundCard::initializeAprsServer() {
    m_aprsListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_aprsListenSocket < 0) {
        printf("[APRS] Failed to create APRS server socket\n");
        return;
    }
    
    // Set socket to non-blocking
    int flags = fcntl(m_aprsListenSocket, F_GETFL, 0);
    fcntl(m_aprsListenSocket, F_SETFL, flags | O_NONBLOCK);
    
    // Allow address reuse
    int opt = 1;
    setsockopt(m_aprsListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);  // APRS command port
    
    if (bind(m_aprsListenSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        printf("[APRS] Failed to bind APRS server socket to port 8080\n");
        close(m_aprsListenSocket);
        return;
    }
    
    if (listen(m_aprsListenSocket, 3) < 0) {
        printf("[APRS] Failed to listen on APRS server socket\n");
        close(m_aprsListenSocket);
        return;
    }
    
    m_aprsServerActive = true;
    printf("[APRS] APRS command server listening on port 8080\n");
}

void USBSoundCard::checkAprsConnections() {
    if (!m_aprsServerActive) {
        return;
    }
    
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    
    int clientSocket = accept(m_aprsListenSocket, (struct sockaddr*)&address, &addrlen);
    if (clientSocket >= 0) {
        printf("[APRS] APRS client connected from %s\n", inet_ntoa(address.sin_addr));
        
        // Read command from client
        char buffer[1024];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            processAprsCommand(buffer);
        }
        
        close(clientSocket);
    }
}

void USBSoundCard::processAprsCommand(const char* command) {
    printf("[APRS] Received command: %s\n", command);
    
    // Parse APRS telemetry command format:
    // "APRS_TLM LAT=42.123456 LON=-71.123456 BAT=12.6 TEMP=22.1 CALL=AMSAT-11"
    
    if (strncmp(command, "APRS_TLM", 8) == 0) {
        F32 lat = 0, lon = 0, bat = 0, temp = 0;
        char callsign[16] = {0};
        
        const char* latPtr = strstr(command, "LAT=");
        const char* lonPtr = strstr(command, "LON=");
        const char* batPtr = strstr(command, "BAT=");
        const char* tempPtr = strstr(command, "TEMP=");
        const char* callPtr = strstr(command, "CALL=");
        
        if (latPtr) lat = atof(latPtr + 4);
        if (lonPtr) lon = atof(lonPtr + 4);
        if (batPtr) bat = atof(batPtr + 4);
        if (tempPtr) temp = atof(tempPtr + 5);
        if (callPtr) {
            sscanf(callPtr + 5, "%15s", callsign);
        }
        
        // Send telemetry to F Prime
        if (latPtr) sendAprsLatitude(lat);
        if (lonPtr) sendAprsLongitude(lon);
        if (batPtr) sendAprsBattery(bat);
        if (tempPtr) sendAprsTemperature(temp);
        
        // Log events
        if (callPtr) logAprsPacketReceived(callsign);
        if (latPtr && lonPtr) logAprsPositionUpdate(lat, lon);
        if (batPtr && tempPtr) logAprsTelemetryUpdate(bat, temp);
        
        printf("[APRS] Telemetry processed: LAT=%.6f, LON=%.6f, BAT=%.1f, TEMP=%.1f from %s\n",
               lat, lon, bat, temp, callsign);
    }
}

} // namespace Components
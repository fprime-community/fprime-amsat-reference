// ======================================================================
// \title  USBSoundCard.cpp
// \author root
// \brief  cpp file for USBSoundCard component implementation class
// ======================================================================

#include "Components/USBSoundCard/USBSoundCard.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>

namespace Components {

USBSoundCard::USBSoundCard(const char *const compName)
    : USBSoundCardComponentBase(compName),
      m_transmissionActive(false),
      m_packetSequence(0),
      m_packetsTransmitted(0),
      m_direwolfPipe(-1),
      m_direwolfActive(false),
      m_aprsPacketCount(0),
      m_lineBufferPos(0)
{
    initializeTransmissionBuffer();
    initializeDirewolfPipe();
    
    memset(m_lineBuffer, 0, sizeof(m_lineBuffer));
}

USBSoundCard::~USBSoundCard() {
    if (m_direwolfPipe >= 0) {
        close(m_direwolfPipe);
    }
}


void USBSoundCard::START_CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {

    if (m_direwolfActive) {
        this->log_WARNING_HI_AUDIO_CAPTURE_ALREADY_STARTED();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    
    initializeDirewolfPipe();
    
    if (m_direwolfActive) {
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
    if (!m_direwolfActive) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    
    if (m_direwolfPipe >= 0) {
        close(m_direwolfPipe);
        m_direwolfPipe = -1;
    }
    
    m_direwolfActive = false;
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


void USBSoundCard::run_handler(FwIndexType portNum, U32 context) {
    readDirewolfOutput();
}

void USBSoundCard::initializeDirewolfPipe() {
    const char* pipePath = "/tmp/direwolf_output";
    
    struct stat st;
    if (stat(pipePath, &st) != 0) {
        printf("[DIREWOLF] Pipe %s does not exist\n", pipePath);
        m_direwolfActive = false;
        return;
    }
    
    // Open in non-blocking mode
    m_direwolfPipe = open(pipePath, O_RDONLY | O_NONBLOCK);
    
    if (m_direwolfPipe >= 0) {
        m_direwolfActive = true;
        m_lineBufferPos = 0;
        printf("[DIREWOLF] Connected to Direwolf pipe at %s\n", pipePath);
    } else {
        printf("[DIREWOLF] Failed to open pipe: %s (errno=%d)\n", strerror(errno), errno);
        m_direwolfActive = false;
    }
}

void USBSoundCard::readDirewolfOutput() {
    if (!m_direwolfActive) {
        static U32 retryCounter = 0;
        if (++retryCounter % 100 == 0) {
            initializeDirewolfPipe();
        }
        return;
    }
    
    char buffer[1024];
    ssize_t bytesRead = read(m_direwolfPipe, buffer, sizeof(buffer) - 1);
    
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        
        // Process character by character to handle line boundaries
        for (ssize_t i = 0; i < bytesRead; i++) {
            char c = buffer[i];
            
            if (c == '\n' || c == '\r') {
                if (m_lineBufferPos > 0) {
                    m_lineBuffer[m_lineBufferPos] = '\0';
                    parseDirewolfLine(m_lineBuffer);
                    m_lineBufferPos = 0;
                }
            } else {
                if (m_lineBufferPos < sizeof(m_lineBuffer) - 1) {
                    m_lineBuffer[m_lineBufferPos++] = c;
                } else {
                    printf("[DIREWOLF] Line buffer overflow, resetting\n");
                    m_lineBufferPos = 0;
                }
            }
        }
    } else if (bytesRead == 0) {
        close(m_direwolfPipe);
        m_direwolfPipe = -1;
        m_direwolfActive = false;
        printf("[DIREWOLF] Pipe closed, will attempt to reconnect\n");
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("[DIREWOLF] Read error: %s\n", strerror(errno));
            close(m_direwolfPipe);
            m_direwolfPipe = -1;
            m_direwolfActive = false;
        }
    }
}

void USBSoundCard::parseDirewolfLine(const char* line) {
    printf("[DIREWOLF] Received: %s\n", line);
    
    // [0] AMSAT-11>APDIGI:=3902.38N\07704.40W
    // [0.3] AMSAT-11>APDIGI:>MODE=a
    // [0] AMSAT-11>APDIGI:>BAT=12.6 TEMP=22.1
    
    const char* payload = strchr(line, ':');
    if (!payload) {
        return;
    }
    payload++; 
    // Skip the ':'
    
    const char* callsignStart = strchr(line, ']');
    if (callsignStart) {
        callsignStart++; // Skip ']'
        while (*callsignStart == ' ') callsignStart++; // Skip spaces
        
        const char* callsignEnd = strchr(callsignStart, '>');
        if (callsignEnd) {
            char callsign[16] = {0};
            size_t len = callsignEnd - callsignStart;
            if (len < sizeof(callsign)) {
                strncpy(callsign, callsignStart, len);
                callsign[len] = '\0';
                
                logAprsPacketReceived(callsign);
            }
        }
    }
    
    if (strstr(payload, "MODE=") != nullptr) {
        const char* modePtr = strstr(payload, "MODE=");
        if (modePtr[5] != '\0') {
            char mode = modePtr[5];
            handleModeCommand(mode);
        }
        return;
    }
    
    if (payload[0] == '=' || payload[0] == '!' || payload[0] == '/') {
        parseAprsPosition(payload);
        return;
    }
    
    if (payload[0] == '>') {
        parseAprsTelemetry(payload + 1);
        return;
    }
}

void USBSoundCard::parseAprsPosition(const char* position) {
    // APRS position format: =DDMM.MMN\DDDMM.MMW or =DDMM.MMN/DDDMM.MMW
    // Example: =3902.38N\07704.40W
    
    F32 lat = 0, lon = 0;
    
    const char* p = position + 1;
    
    if (strlen(p) < 8) {
        logAprsParseError("Position string too short");
        return;
    }
    
    int latDeg = (p[0] - '0') * 10 + (p[1] - '0');
    float latMin = atof(&p[2]);
    lat = latDeg + (latMin / 60.0);
    
    const char* nsPtr = strchr(p, 'N');
    bool south = false;
    if (!nsPtr) {
        nsPtr = strchr(p, 'S');
        south = true;
    }
    if (south) lat = -lat;
    
    // Parse longitude: DDDMM.MM
    const char* lonStart = strchr(p, '\\');
    if (!lonStart) lonStart = strchr(p, '/');
    
    if (lonStart) {
        lonStart++;
        if (strlen(lonStart) < 9) {
            logAprsParseError("Longitude string too short");
            return;
        }
        
        int lonDeg = (lonStart[0] - '0') * 100 + (lonStart[1] - '0') * 10 + (lonStart[2] - '0');
        float lonMin = atof(&lonStart[3]);
        lon = lonDeg + (lonMin / 60.0);
        
        // Check E/W
        const char* ewPtr = strchr(lonStart, 'W');
        bool west = false;
        if (!ewPtr) {
            ewPtr = strchr(lonStart, 'E');
        } else {
            west = true;
        }
        if (west) lon = -lon;
    }
    
    this->tlmWrite_APRS_LATITUDE(lat);
    printf("[APRS] Latitude updated: %.6f degrees\n", lat);
    this->tlmWrite_APRS_LONGITUDE(lon);
    printf("[APRS] Longitude updated: %.6f degrees\n", lon);
    this->log_ACTIVITY_LO_APRS_POSITION_UPDATE(lat, lon);
    printf("[APRS] Position: LAT=%.6f, LON=%.6f\n", lat, lon);
}

void USBSoundCard::parseAprsTelemetry(const char* telemetry) {
    
    const char* batPtr = strstr(telemetry, "BAT=");
    const char* tempPtr = strstr(telemetry, "TEMP=");
    const char* sigPtr = strstr(telemetry, "SIG=");
    
    F32 bat = 0, temp = 0, sig = 0;
    bool hasBat = false, hasTemp = false, hasSig = false;
    
    if (batPtr) {
        bat = atof(batPtr + 4);
        this->tlmWrite_APRS_BATTERY(bat);
        printf("[APRS] Battery voltage updated: %.2f V\n", bat);
        hasBat = true;
    }
    
    if (tempPtr) {
        temp = atof(tempPtr + 5);
        this->tlmWrite_APRS_TEMPERATURE(temp);
        printf("[APRS] Temperature updated: %.1f C\n", temp);
        hasTemp = true;
    }
    
    if (sigPtr) {
        sig = atof(sigPtr + 4);
        this->tlmWrite_APRS_SIGNAL_STRENGTH(sig);
        printf("[APRS] Signal strength updated: %.1f dBm\n", sig);
        hasSig = true;
    }
    
    if (hasBat && hasTemp) {
        this->log_ACTIVITY_LO_APRS_TELEMETRY_UPDATE(bat, temp);
        printf("[APRS] Telemetry: BAT=%.1fV, TEMP=%.1fC\n", bat, temp);
    }
    
    printf("[APRS] Telemetry parsed: BAT=%.2f TEMP=%.2f SIG=%.2f\n", bat, temp, sig);
}

void USBSoundCard::handleModeCommand(char mode) {
    printf("[APRS] Mode command received: %c\n", mode);

    U32 modeNum = 0;
    const char* modeName = "";
    
    switch(mode) {
        case 'a': modeNum = 1; modeName = "APRS"; break;
        case 'f': modeNum = 2; modeName = "FSK"; break;
        case 'b': modeNum = 3; modeName = "BPSK"; break;
        case 's': modeNum = 4; modeName = "SSTV"; break;
        case 'm': modeNum = 5; modeName = "CW"; break;
        case 'o': modeNum = 10; modeName = "BEACON_TOGGLE"; break;
        case 'e': modeNum = 6; modeName = "REPEATER"; break;
        case 'n': modeNum = 7; modeName = "TX_COMMANDS"; break;
        default:
            printf("[APRS] Unknown mode: %c\n", mode);
            logAprsParseError("Unknown mode command");
            return;
    }
    
    printf("[APRS] Executing mode change to %s (%u)\n", modeName, modeNum);
    Fw::LogStringArg modeArg(modeName);
    
    // TODO: Send command via F Prime output port to other components

}

void USBSoundCard::logAprsPacketReceived(const char* callsign) {
    m_aprsPacketCount++;
    this->tlmWrite_APRS_PACKET_COUNT(m_aprsPacketCount);
    
    Fw::LogStringArg callsignArg(callsign);
    this->log_ACTIVITY_LO_APRS_PACKET_RECEIVED(callsignArg);
    printf("[APRS] Packet #%u received from %s\n", m_aprsPacketCount, callsign);
}

void USBSoundCard::logAprsParseError(const char* error) {
    Fw::LogStringArg errorArg(error);
    this->log_WARNING_HI_APRS_PARSE_ERROR(errorArg);
    printf("[APRS] Parse error: %s\n", error);
}

void USBSoundCard::initializeTransmissionBuffer() {
    const U32 maxAudioSize = 1024 * sizeof(short);
    const U32 headerSize = sizeof(U32) * 3;
    const U32 totalSize = headerSize + maxAudioSize;
    
    U8* bufferData = new U8[totalSize];
    m_transmissionBuffer.setData(bufferData);
    m_transmissionBuffer.setSize(totalSize);
}

bool USBSoundCard::transmitAudioPacket(const void* audioData, U32 dataSize) {
    if (!m_transmissionActive) {
        return false;
    }
    
    try {
        Fw::Time currentTime = this->getTime();
        
        U8* bufferPtr = m_transmissionBuffer.getData();
        U32* header = reinterpret_cast<U32*>(bufferPtr);
        
        header[0] = m_packetSequence++;
        header[1] = currentTime.getSeconds();
        header[2] = dataSize;
        
        U8* audioPtr = bufferPtr + sizeof(U32) * 3;
        std::memcpy(audioPtr, audioData, dataSize);
        
        U32 totalPacketSize = sizeof(U32) * 3 + dataSize;
        m_transmissionBuffer.setSize(totalPacketSize);
        
        this->bufferOut_out(0, m_transmissionBuffer);
        
        m_packetsTransmitted++;
        this->tlmWrite_PACKETS_TRANSMITTED(m_packetsTransmitted);
        this->log_ACTIVITY_LO_PACKET_TRANSMITTED();
        
        return true;
    } catch (...) {
        this->log_WARNING_HI_TRANSMISSION_ERROR();
        return false;
    }
}

bool USBSoundCard::sendTestPacket() {
    const U32 testDataSize = 256;
    short testAudioData[128];
    
    for (int i = 0; i < 128; i++) {
        double frequency = 440.0;
        double sampleRate = 44100.0;
        testAudioData[i] = (short)(32767.0 * sin(2.0 * M_PI * frequency * i / sampleRate));
    }
    
    bool success = transmitAudioPacket(testAudioData, testDataSize);
    
    if (success) {
        printf("[USB_SOUND] Test packet transmitted successfully\n");
    } else {
        printf("[USB_SOUND] Failed to transmit test packet\n");
    }
    
    return success;
}

} // namespace Components
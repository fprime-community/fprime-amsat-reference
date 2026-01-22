// ======================================================================
// \title  USBSoundCard.hpp
// \author root
// \brief  hpp file for USBSoundCard component implementation class
// ======================================================================

#ifndef Components_USBSoundCard_HPP
#define Components_USBSoundCard_HPP

#include "Components/USBSoundCard/USBSoundCardComponentAc.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

namespace Components {

class USBSoundCard final : public USBSoundCardComponentBase {
public:

  USBSoundCard(const char *const compName);

  ~USBSoundCard();


  // (public for testing)

  void sendAprsLatitude(F32 latitude);
  void sendAprsLongitude(F32 longitude);
  void sendAprsBattery(F32 voltage);
  void sendAprsTemperature(F32 temperature);
  void sendAprsSignalStrength(F32 strength);
  void logAprsPacketReceived(const char* callsign);
  void logAprsPositionUpdate(F32 lat, F32 lon);
  void logAprsTelemetryUpdate(F32 battery, F32 temp);
  void logAprsParseError(const char* error);

PRIVATE:

  void START_CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
  void STOP_CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
  void START_TRANSMISSION_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
  void STOP_TRANSMISSION_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
  void SEND_TEST_PACKET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;


  void run_handler(FwIndexType portNum, U32 context) override;

PRIVATE:
  bool m_transmissionActive;
  U32 m_packetsTransmitted;
  Fw::Buffer m_transmissionBuffer;
  U32 m_packetSequence;

  int m_direwolfPipe;
  bool m_direwolfActive;
  char m_lineBuffer[2048];
  U32 m_lineBufferPos;

  U32 m_aprsPacketCount;

  void initializeDirewolfPipe();

  void readDirewolfOutput();

  void parseDirewolfLine(const char* line);

  //(=DDMM.MMN\DDDMM.MMW)
  void parseAprsPosition(const char* position);

  void parseAprsTelemetry(const char* telemetry);

  void handleModeCommand(char mode);

  void initializeTransmissionBuffer();

  bool transmitAudioPacket(const void* audioData, U32 dataSize);

  bool sendTestPacket();
};

} // namespace Components

#endif // Components_USBSoundCard_HPP
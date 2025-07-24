// ======================================================================
// \title  USBSoundCard.hpp
// \author root
// \brief  hpp file for USBSoundCard component implementation class
// ======================================================================

#ifndef Components_USBSoundCard_HPP
#define Components_USBSoundCard_HPP

#include "Components/USBSoundCard/USBSoundCardComponentAc.hpp"

#ifdef ALSA_STUBBED
// Forward declarations for stubbed ALSA
struct _snd_pcm;
typedef struct _snd_pcm snd_pcm_t;
#else
#include <alsa/asoundlib.h>
#endif

namespace Components {

class USBSoundCard final : public USBSoundCardComponentBase {

public:
  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  //! Construct USBSoundCard object
  USBSoundCard(const char *const compName //!< The component name
  );

  //! Destroy USBSoundCard object
  ~USBSoundCard();

PRIVATE:
  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  //! Handler implementation for START_CAPTURE command
  //!
  //! Start audio capture from USB sound card
  void START_CAPTURE_cmdHandler(FwOpcodeType opCode, //!< The opcode
                                U32 cmdSeq           //!< The command sequence number
                                ) override;

  //! Handler implementation for STOP_CAPTURE command
  //!
  //! Stop audio capture from USB sound card
  void STOP_CAPTURE_cmdHandler(FwOpcodeType opCode, //!< The opcode
                               U32 cmdSeq           //!< The command sequence number
                               ) override;

  //! Handler implementation for START_TRANSMISSION command
  //!
  //! Start audio transmission as FPrime packets
  void START_TRANSMISSION_cmdHandler(FwOpcodeType opCode, //!< The opcode
                                     U32 cmdSeq           //!< The command sequence number
                                     ) override;

  //! Handler implementation for STOP_TRANSMISSION command
  //!
  //! Stop audio transmission
  void STOP_TRANSMISSION_cmdHandler(FwOpcodeType opCode, //!< The opcode
                                    U32 cmdSeq           //!< The command sequence number
                                    ) override;

  //! Handler implementation for SEND_TEST_PACKET command
  //!
  //! Send a test packet to verify transmission functionality
  void SEND_TEST_PACKET_cmdHandler(FwOpcodeType opCode, //!< The opcode
                                   U32 cmdSeq           //!< The command sequence number
                                   ) override;

  // ----------------------------------------------------------------------
  // Handler implementations for input ports
  // ----------------------------------------------------------------------

  //! Handler implementation for run port
  //!
  //! Scheduled port for periodic audio processing
  void run_handler(FwIndexType portNum, //!< The port number
                   U32 context          //!< The call context
                   ) override;

PRIVATE:
  // ----------------------------------------------------------------------
  // Private member variables
  // ----------------------------------------------------------------------

  //! Audio capture state
  bool m_audioCapturing;

  //! Audio transmission state
  bool m_transmissionActive;

  //! ALSA PCM device handle
  snd_pcm_t* m_audioDevice;

  //! Audio buffer for storing captured samples
  short* m_audioBuffer;

  //! Size of audio buffer (in samples)
  U32 m_bufferSize;

  //! Frame counter for telemetry
  U32 m_framesProcessed;

  //! Packet counter for telemetry
  U32 m_packetsTransmitted;

  //! Transmission packet buffer
  Fw::Buffer m_transmissionBuffer;

  //! Packet sequence number
  U32 m_packetSequence;

  // ----------------------------------------------------------------------
  // Private helper methods
  // ----------------------------------------------------------------------

  //! Initialize the USB sound card audio device
  //!
  //! \return true if successful, false otherwise
  bool initializeAudioDevice();

  //! Stop audio capture and clean up resources
  void stopAudioCapture();

  //! Process captured audio data
  //!
  //! Called periodically by run_handler to read and process audio
  void processAudioData();

  //! Calculate RMS audio level from buffer
  //!
  //! \param buffer Pointer to audio sample buffer
  //! \param frames Number of frames in buffer
  //! \return Audio level scaled 0-255
  U32 calculateAudioLevel(const short* buffer, int frames);

  //! Format and transmit audio data as FPrime packet
  //!
  //! \param audioData Pointer to audio data
  //! \param dataSize Size of audio data in bytes
  //! \return true if transmission successful, false otherwise
  bool transmitAudioPacket(const void* audioData, U32 dataSize);

  //! Send a test packet to verify transmission functionality
  //!
  //! \return true if transmission successful, false otherwise
  bool sendTestPacket();

  //! Initialize transmission buffer
  void initializeTransmissionBuffer();
};

} // namespace Components

#endif
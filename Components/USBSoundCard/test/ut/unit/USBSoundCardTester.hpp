// ======================================================================
// \title  USBSoundCardTester.hpp
// \author root
// \brief  Header file for USBSoundCard component test harness
// ======================================================================

#ifndef Components_USBSoundCardTester_HPP
#define Components_USBSoundCardTester_HPP

#include "Components/USBSoundCard/USBSoundCard.hpp"
#include "USBSoundCardGTestBase.hpp"
#include "STest/Pick/Pick.hpp"
#include "Fw/Buffer/Buffer.hpp"
#include "Fw/Com/ComBuffer.hpp"
#include <vector>

namespace Components {

class USBSoundCardTester : public USBSoundCardGTestBase {
public:
    //! Test instance constants
    static const U32 TEST_INSTANCE_QUEUE_DEPTH = 100;
    static const U32 TEST_INSTANCE_ID = 0;

    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct USBSoundCardTester object
    USBSoundCardTester();

    //! Destroy USBSoundCardTester object
    ~USBSoundCardTester();

    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! Test basic component initialization
    void testInitialization();

    //! Test audio capture commands
    void testAudioCaptureCommands();

    //! Test transmission commands
    void testTransmissionCommands();

    //! Test packet transmission functionality with F Prime ports
    void testPacketTransmission();

    //! Test test packet functionality
    void testTestPacket();

    //! Test telemetry updates
    void testTelemetryUpdates();

    //! Test event logging
    void testEventLogging();

    //! Test error conditions
    void testErrorConditions();

    //! Test F Prime packet format validation
    void testFPrimePacketFormat();

    //! Test buffer output port functionality
    void testBufferOutputPort();

    //! Test com output port functionality
    void testComOutputPort();

    //! Run all tests
    void runTests();

    // ----------------------------------------------------------------------
    // Port callbacks (for testing output ports)
    // ----------------------------------------------------------------------

    //! Callback for buffer output port
    void bufferOut_handler(FwIndexType portNum, Fw::Buffer& buffer);

    //! Callback for com output port
    void packetOut_handler(FwIndexType portNum, Fw::ComBuffer& comBuffer);

private:
    // ----------------------------------------------------------------------
    // Private helper methods
    // ----------------------------------------------------------------------

    //! Verify telemetry values
    void verifyTelemetry();

    //! Verify event history
    void verifyEvents();

    //! Verify command responses
    void verifyCommandResponses();

    //! Mock time getter
    Fw::Time getTime();

    //! Validate transmitted packet format
    bool validateTransmittedPacket(const Fw::Buffer& buffer);

    //! Validate transmitted com packet format
    bool validateTransmittedComPacket(const Fw::ComBuffer& comBuffer);

    //! Connect test ports
    void connectTestPorts();

    //! Disconnect test ports
    void disconnectTestPorts();

    //! Connect ports (for generated helper)
    void connectPorts();

    //! Initialize components (for generated helper)
    void initComponents();

    // ----------------------------------------------------------------------
    // Private member variables
    // ----------------------------------------------------------------------

    //! The component under test
    USBSoundCard component;

    //! Test time
    Fw::Time m_testTime;

    //! Mock buffer for testing
    Fw::Buffer m_testBuffer;

    //! Test audio data
    short m_testAudioData[128];

    //! Received buffer packets
    std::vector<Fw::Buffer> m_receivedBuffers;

    //! Received com packets
    std::vector<Fw::ComBuffer> m_receivedComPackets;

    //! Expected sequence number for packet validation
    U32 m_expectedSequence;

    //! Test statistics
    struct TestStats {
        U32 totalBufferPackets;
        U32 totalComPackets;
        U32 validBufferPackets;
        U32 validComPackets;
        U32 invalidPackets;
        U32 sequenceErrors;
    } m_testStats;
};

} // namespace Components

#endif 
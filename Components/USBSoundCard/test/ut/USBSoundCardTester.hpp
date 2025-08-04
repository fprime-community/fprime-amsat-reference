// ======================================================================
// \title  USBSoundCardTester.hpp
// \author root
// \brief  Header file for USBSoundCard component test harness
// ======================================================================

#ifndef Components_USBSoundCardTester_HPP
#define Components_USBSoundCardTester_HPP

#include "Components/USBSoundCard/USBSoundCard.hpp"
#include "Fw/Test/UnitTest.hpp"
#include "STest/Pick/Pick.hpp"

namespace Components {

class USBSoundCardTester : public Fw::Test::UnitTest {
public:
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

    //! Test packet transmission functionality
    void testPacketTransmission();

    //! Test test packet functionality
    void testTestPacket();

    //! Test telemetry updates
    void testTelemetryUpdates();

    //! Test event logging
    void testEventLogging();

    //! Test error conditions
    void testErrorConditions();

    //! Run all tests
    void runTests();

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

    // ----------------------------------------------------------------------
    // Private member variables
    // ----------------------------------------------------------------------

    //! The component under test
    USBSoundCard m_component;

    //! Test time
    Fw::Time m_testTime;

    //! Mock buffer for testing
    Fw::Buffer m_testBuffer;

    //! Test audio data
    short m_testAudioData[128];
};

} // namespace Components

#endif 
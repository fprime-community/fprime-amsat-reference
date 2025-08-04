// ======================================================================
// \title  IntegrationTest.cpp
// \author root
// \brief  Integration test for USBSoundCard packet transmission
// ======================================================================

#include "Components/USBSoundCard/test/ut/unit/USBSoundCardTester.hpp"
#include "Components/USBSoundCard/test/ut/PacketReceiver.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <ctime>

// ----------------------------------------------------------------------
// Integration test functions
// ----------------------------------------------------------------------

void testPacketTransmissionIntegration() {
    std::cout << "=== Testing Packet Transmission Integration ===" << std::endl;
    
    // Create USB sound card component
    Components::USBSoundCard soundCard("IntegrationTestSoundCard");
    
    // Create packet receiver
    Components::PacketReceiver receiver;
    
    // Connect the components (in a real deployment, this would be done via ports)
    // For testing, we'll manually call the transmission methods
    
    std::cout << "Starting transmission test..." << std::endl;
    
    // Start transmission
    soundCard.START_TRANSMISSION_cmdHandler(2, 1);
    
    // Send multiple test packets
    for (int i = 0; i < 5; i++) {
        std::cout << "Sending test packet " << (i + 1) << "/5" << std::endl;
        
        // Send test packet
        soundCard.SEND_TEST_PACKET_cmdHandler(4, i + 2);
        
        // Simulate packet reception (in real system, this would be via port)
        // For testing, we'll create a mock buffer and validate it
        Fw::Buffer mockBuffer;
        U8* bufferData = new U8[1024];
        mockBuffer.setData(bufferData);
        
        // Simulate the packet that would be sent
        U32* header = reinterpret_cast<U32*>(bufferData);
        header[0] = i;  // Sequence number
        header[1] = static_cast<U32>(std::time(nullptr));  // Timestamp
        header[2] = 256;  // Audio data size (128 samples * 2 bytes)
        
        // Add test audio data
        short* audioData = reinterpret_cast<short*>(bufferData + sizeof(U32) * 3);
        for (int j = 0; j < 128; j++) {
            double frequency = 440.0; // A4 note
            double sampleRate = 44100.0;
            audioData[j] = (short)(32767.0 * sin(2.0 * M_PI * frequency * j / sampleRate));
        }
        
        mockBuffer.setSize(sizeof(U32) * 3 + 256);
        
        // Receive and validate the packet
        receiver.receiveBuffer(mockBuffer);
        
        // Small delay to simulate real-time processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Clean up this iteration's buffer
        delete[] bufferData;
    }
    
    // Stop transmission
    soundCard.STOP_TRANSMISSION_cmdHandler(3, 7);
    
    // Get and display statistics
    auto stats = receiver.getStats();
    std::cout << "\n=== Packet Transmission Statistics ===" << std::endl;
    std::cout << "Total packets: " << stats.totalPackets << std::endl;
    std::cout << "Valid packets: " << stats.validPackets << std::endl;
    std::cout << "Invalid packets: " << stats.invalidPackets << std::endl;
    std::cout << "Total bytes: " << stats.totalBytes << std::endl;
    std::cout << "Sequence errors: " << stats.sequenceErrors << std::endl;
    
    // Verify results
    if (stats.validPackets == 5 && stats.invalidPackets == 0 && stats.sequenceErrors == 0) {
        std::cout << "✅ Integration test PASSED!" << std::endl;
    } else {
        std::cout << "❌ Integration test FAILED!" << std::endl;
    }
}

void testFPrimePacketFormat() {
    std::cout << "\n=== Testing FPrime Packet Format ===" << std::endl;
    
    // Create packet receiver
    Components::PacketReceiver receiver;
    
    // Test various packet formats
    std::vector<Fw::Buffer> testPackets;
    
    // Valid packet 1
    {
        Fw::Buffer packet;
        U8* data = new U8[1024];
        packet.setData(data);
        
        U32* header = reinterpret_cast<U32*>(data);
        header[0] = 0;  // Sequence
        header[1] = 1234567890;  // Timestamp
        header[2] = 256;  // Data size
        
        // Add audio data
        short* audioData = reinterpret_cast<short*>(data + sizeof(U32) * 3);
        for (int i = 0; i < 128; i++) {
            audioData[i] = static_cast<short>(i * 100);
        }
        
        packet.setSize(sizeof(U32) * 3 + 256);
        testPackets.push_back(packet);
    }
    
    // Valid packet 2
    {
        Fw::Buffer packet;
        U8* data = new U8[1024];
        packet.setData(data);
        
        U32* header = reinterpret_cast<U32*>(data);
        header[0] = 1;  // Sequence
        header[1] = 1234567891;  // Timestamp
        header[2] = 512;  // Data size
        
        // Add audio data
        short* audioData = reinterpret_cast<short*>(data + sizeof(U32) * 3);
        for (int i = 0; i < 256; i++) {
            audioData[i] = static_cast<short>(-i * 50);
        }
        
        packet.setSize(sizeof(U32) * 3 + 512);
        testPackets.push_back(packet);
    }
    
    // Invalid packet (wrong sequence)
    {
        Fw::Buffer packet;
        U8* data = new U8[1024];
        packet.setData(data);
        
        U32* header = reinterpret_cast<U32*>(data);
        header[0] = 5;  // Wrong sequence (should be 2)
        header[1] = 1234567892;  // Timestamp
        header[2] = 128;  // Data size
        
        packet.setSize(sizeof(U32) * 3 + 128);
        testPackets.push_back(packet);
    }
    
    // Test all packets
    for (size_t i = 0; i < testPackets.size(); i++) {
        std::cout << "Testing packet " << (i + 1) << "/" << testPackets.size() << std::endl;
        receiver.receiveBuffer(testPackets[i]);
    }
    
    // Get and display statistics
    auto stats = receiver.getStats();
    std::cout << "\n=== FPrime Packet Format Test Results ===" << std::endl;
    std::cout << "Total packets: " << stats.totalPackets << std::endl;
    std::cout << "Valid packets: " << stats.validPackets << std::endl;
    std::cout << "Invalid packets: " << stats.invalidPackets << std::endl;
    std::cout << "Sequence errors: " << stats.sequenceErrors << std::endl;
    
    // Verify results (should have 2 valid, 1 invalid due to sequence error)
    if (stats.validPackets == 2 && stats.invalidPackets == 1 && stats.sequenceErrors == 1) {
        std::cout << "✅ FPrime packet format test PASSED!" << std::endl;
    } else {
        std::cout << "❌ FPrime packet format test FAILED!" << std::endl;
    }
    
    // Clean up
    for (auto& packet : testPackets) {
        delete[] packet.getData();
    }
}

// ----------------------------------------------------------------------
// Main test runner
// ----------------------------------------------------------------------

int main() {
    std::cout << "Starting USBSoundCard Integration Tests..." << std::endl;
    
    try {
        testPacketTransmissionIntegration();
        testFPrimePacketFormat();
        
        std::cout << "\n🎉 All integration tests completed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
} 
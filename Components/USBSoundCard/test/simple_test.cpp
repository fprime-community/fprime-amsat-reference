// ======================================================================
// \title  simple_test.cpp
// \author root
// \brief  Simple standalone test for USBSoundCard transmission
// ======================================================================

#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>
#include <ctime>

// Simple mock FPrime types for testing
typedef unsigned char U8;
typedef unsigned int U32;
typedef short I16;

struct FwTime {
    U32 seconds;
    U32 useconds;
    
    FwTime(U32 s = 0, U32 us = 0) : seconds(s), useconds(us) {}
    U32 getSeconds() const { return seconds; }
};

struct FwBuffer {
    U8* data;
    U32 size;
    
    FwBuffer() : data(nullptr), size(0) {}
    void setData(U8* d) { data = d; }
    void setSize(U32 s) { size = s; }
    U8* getData() const { return data; }
    U32 getSize() const { return size; }
};

// Mock USB Sound Card component for testing
class MockUSBSoundCard {
private:
    bool m_transmissionActive;
    U32 m_packetSequence;
    FwBuffer m_transmissionBuffer;
    
public:
    MockUSBSoundCard() : m_transmissionActive(false), m_packetSequence(0) {
        // Initialize transmission buffer
        const U32 maxAudioSize = 1024 * sizeof(short);
        const U32 headerSize = sizeof(U32) * 3; // sequence, timestamp, data size
        const U32 totalSize = headerSize + maxAudioSize;
        
        U8* bufferData = new U8[totalSize];
        m_transmissionBuffer.setData(bufferData);
        m_transmissionBuffer.setSize(totalSize);
    }
    
    ~MockUSBSoundCard() {
        delete[] m_transmissionBuffer.getData();
    }
    
    void START_TRANSMISSION() {
        m_transmissionActive = true;
        std::cout << "Transmission started" << std::endl;
    }
    
    void STOP_TRANSMISSION() {
        m_transmissionActive = false;
        std::cout << "Transmission stopped" << std::endl;
    }
    
    bool sendTestPacket() {
        if (!m_transmissionActive) {
            std::cout << "Transmission not active" << std::endl;
            return false;
        }
        
        // Create test audio data (sine wave pattern)
        const U32 testDataSize = 256; // 128 samples * 2 bytes per sample
        short testAudioData[128];
        
        // Generate a simple sine wave pattern
        for (int i = 0; i < 128; i++) {
            double frequency = 440.0; // A4 note
            double sampleRate = 44100.0;
            testAudioData[i] = (short)(32767.0 * sin(2.0 * M_PI * frequency * i / sampleRate));
        }
        
        return transmitAudioPacket(testAudioData, testDataSize);
    }
    
private:
    bool transmitAudioPacket(const void* audioData, U32 dataSize) {
        if (!m_transmissionActive) {
            return false;
        }
        
        try {
            // Get current timestamp
            FwTime currentTime(static_cast<U32>(std::time(nullptr)), 0);
            
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
            
            std::cout << "Packet transmitted: seq=" << header[0] 
                      << ", timestamp=" << header[1] 
                      << ", dataSize=" << header[2] 
                      << ", totalSize=" << totalPacketSize << std::endl;
            
            return true;
            
        } catch (...) {
            std::cout << "Transmission error" << std::endl;
            return false;
        }
    }
};

// Packet receiver for validation
class PacketReceiver {
private:
    U32 m_expectedSequence;
    U32 m_totalPackets;
    U32 m_validPackets;
    U32 m_invalidPackets;
    
public:
    PacketReceiver() : m_expectedSequence(0), m_totalPackets(0), 
                       m_validPackets(0), m_invalidPackets(0) {}
    
    bool validateAudioPacket(const FwBuffer& buffer) {
        // Check minimum size (header: 3 * U32 = 12 bytes)
        if (buffer.getSize() < sizeof(U32) * 3) {
            std::cout << "Packet too small: " << buffer.getSize() << " bytes" << std::endl;
            m_invalidPackets++;
            return false;
        }
        
        // Parse header
        const U8* data = buffer.getData();
        const U32* header = reinterpret_cast<const U32*>(data);
        
        U32 sequence = header[0];
        U32 timestamp = header[1];
        U32 dataSize = header[2];
        
        // Validate sequence number
        if (sequence != m_expectedSequence) {
            std::cout << "Sequence error: expected " << m_expectedSequence 
                      << ", got " << sequence << std::endl;
            m_invalidPackets++;
            m_expectedSequence = sequence + 1; // Update for next packet
            return false;
        }
        
        // Validate data size
        U32 expectedTotalSize = sizeof(U32) * 3 + dataSize;
        if (buffer.getSize() != expectedTotalSize) {
            std::cout << "Size mismatch: expected " << expectedTotalSize 
                      << ", got " << buffer.getSize() << std::endl;
            m_invalidPackets++;
            return false;
        }
        
        // Validate timestamp (basic sanity check)
        if (timestamp > 0xFFFFFFFF) {
            std::cout << "Invalid timestamp: " << timestamp << std::endl;
            m_invalidPackets++;
            return false;
        }
        
        // Validate audio data size (reasonable range)
        if (dataSize == 0 || dataSize > 10000) {
            std::cout << "Invalid audio data size: " << dataSize << std::endl;
            m_invalidPackets++;
            return false;
        }
        
        // Update expected sequence for next packet
        m_expectedSequence = sequence + 1;
        m_validPackets++;
        
        std::cout << "Valid packet: seq=" << sequence 
                  << ", timestamp=" << timestamp 
                  << ", dataSize=" << dataSize 
                  << ", totalSize=" << buffer.getSize() << std::endl;
        
        return true;
    }
    
    void printStats() const {
        std::cout << "\n=== Packet Statistics ===" << std::endl;
        std::cout << "Total packets: " << m_totalPackets << std::endl;
        std::cout << "Valid packets: " << m_validPackets << std::endl;
        std::cout << "Invalid packets: " << m_invalidPackets << std::endl;
    }
};

// Test functions
void testPacketTransmission() {
    std::cout << "=== Testing Packet Transmission ===" << std::endl;
    
    MockUSBSoundCard soundCard;
    PacketReceiver receiver;
    
    // Start transmission
    soundCard.START_TRANSMISSION();
    
    // Send multiple test packets
    for (int i = 0; i < 5; i++) {
        std::cout << "Sending test packet " << (i + 1) << "/5" << std::endl;
        
        if (soundCard.sendTestPacket()) {
            std::cout << "Test packet sent successfully" << std::endl;
        } else {
            std::cout << "Failed to send test packet" << std::endl;
        }
    }
    
    // Stop transmission
    soundCard.STOP_TRANSMISSION();
    
    std::cout << "Packet transmission test completed" << std::endl;
}

void testFPrimePacketFormat() {
    std::cout << "\n=== Testing FPrime Packet Format ===" << std::endl;
    
    PacketReceiver receiver;
    
    // Test various packet formats
    std::vector<FwBuffer> testPackets;
    
    // Valid packet 1
    {
        FwBuffer packet;
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
        FwBuffer packet;
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
        FwBuffer packet;
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
        receiver.validateAudioPacket(testPackets[i]);
    }
    
    receiver.printStats();
    
    // Clean up
    for (auto& packet : testPackets) {
        delete[] packet.getData();
    }
}

int main() {
    std::cout << "Starting USBSoundCard Simple Tests..." << std::endl;
    
    try {
        testPacketTransmission();
        testFPrimePacketFormat();
        
        std::cout << "\n🎉 All simple tests completed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
} 
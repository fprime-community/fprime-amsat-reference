// ======================================================================
// \title  PacketReceiver.cpp
// \author root
// \brief  Implementation file for packet receiver test component
// ======================================================================

#include "PacketReceiver.hpp"
#include <cstring>

namespace Components {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

PacketReceiver::PacketReceiver()
    : m_expectedSequence(0)
{
    // Initialize statistics
    m_stats.totalPackets = 0;
    m_stats.validPackets = 0;
    m_stats.invalidPackets = 0;
    m_stats.totalBytes = 0;
    m_stats.sequenceErrors = 0;
}

PacketReceiver::~PacketReceiver() {
    // Clean up any allocated buffers
    for (auto& buffer : m_receivedBuffers) {
        delete[] buffer.getData();
    }
}

// ----------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------

void PacketReceiver::receiveBuffer(const Fw::Buffer& buffer) {
    // Create a copy of the buffer
    U8* dataCopy = new U8[buffer.getSize()];
    std::memcpy(dataCopy, buffer.getData(), buffer.getSize());
    
    Fw::Buffer bufferCopy;
    bufferCopy.setData(dataCopy);
    bufferCopy.setSize(buffer.getSize());
    
    m_receivedBuffers.push_back(bufferCopy);
    
    // Update statistics
    m_stats.totalPackets++;
    m_stats.totalBytes += buffer.getSize();
    
    // Validate the packet
    if (validateAudioPacket(bufferCopy)) {
        m_stats.validPackets++;
    } else {
        m_stats.invalidPackets++;
    }
}

void PacketReceiver::receiveCom(const Fw::ComBuffer& comBuffer) {
    m_receivedCom.push_back(comBuffer);
    
    // Update statistics
    m_stats.totalPackets++;
    m_stats.totalBytes += comBuffer.getBuffLength();
}

const std::vector<Fw::Buffer>& PacketReceiver::getReceivedBuffers() const {
    return m_receivedBuffers;
}

const std::vector<Fw::ComBuffer>& PacketReceiver::getReceivedCom() const {
    return m_receivedCom;
}

void PacketReceiver::clearReceived() {
    // Clean up buffer memory
    for (auto& buffer : m_receivedBuffers) {
        delete[] buffer.getData();
    }
    
    m_receivedBuffers.clear();
    m_receivedCom.clear();
    
    // Reset statistics
    m_stats.totalPackets = 0;
    m_stats.validPackets = 0;
    m_stats.invalidPackets = 0;
    m_stats.totalBytes = 0;
    m_stats.sequenceErrors = 0;
    m_expectedSequence = 0;
}

bool PacketReceiver::validateAudioPacket(const Fw::Buffer& buffer) {
    // Check minimum size (header: 3 * U32 = 12 bytes)
    if (buffer.getSize() < sizeof(U32) * 3) {
        printf("Packet too small: %lu bytes\n", buffer.getSize());
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
        printf("Sequence error: expected %u, got %u\n", m_expectedSequence, sequence);
        m_stats.sequenceErrors++;
        m_expectedSequence = sequence + 1; // Update for next packet
        return false;
    }
    
    // Validate data size
    U32 expectedTotalSize = sizeof(U32) * 3 + dataSize;
    if (buffer.getSize() != expectedTotalSize) {
        printf("Size mismatch: expected %u, got %lu\n", expectedTotalSize, buffer.getSize());
        return false;
    }
    
    // Validate timestamp (basic sanity check)
    if (timestamp > 0xFFFFFFFF) {
        printf("Invalid timestamp: %u\n", timestamp);
        return false;
    }
    
    // Validate audio data size (reasonable range)
    if (dataSize == 0 || dataSize > 10000) {
        printf("Invalid audio data size: %u\n", dataSize);
        return false;
    }
    
    // Check if audio data is present
    if (dataSize > 0) {
        const U8* audioData = data + sizeof(U32) * 3;
        
        // Basic audio data validation (check for reasonable values)
        const short* samples = reinterpret_cast<const short*>(audioData);
        U32 numSamples = dataSize / sizeof(short);
        
        for (U32 i = 0; i < numSamples && i < 10; i++) { // Check first 10 samples
            if (samples[i] < -32768 || samples[i] > 32767) {
                printf("Invalid audio sample at index %u: %d\n", i, samples[i]);
                return false;
            }
        }
    }
    
    // Update expected sequence for next packet
    m_expectedSequence = sequence + 1;
    
        printf("Valid packet: seq=%u, timestamp=%u, dataSize=%u, totalSize=%lu\n",
           sequence, timestamp, dataSize, buffer.getSize());
    
    return true;
}

PacketReceiver::PacketStats PacketReceiver::getStats() const {
    return m_stats;
}

} // namespace Components 
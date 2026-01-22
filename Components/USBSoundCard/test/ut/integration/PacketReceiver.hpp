// ======================================================================
// \title  PacketReceiver.hpp
// \author root
// \brief  Header file for packet receiver test component
// ======================================================================

#ifndef Components_PacketReceiver_HPP
#define Components_PacketReceiver_HPP

#include "Fw/Obj/SimpleObjRegistry.hpp"
#include "Fw/Buffer/Buffer.hpp"
#include "Fw/Com/ComBuffer.hpp"
#include <vector>

namespace Components {

class PacketReceiver {
public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct PacketReceiver object
    PacketReceiver();

    //! Destroy PacketReceiver object
    ~PacketReceiver();

    // ----------------------------------------------------------------------
    // Public methods
    // ----------------------------------------------------------------------

    //! Receive a buffer packet
    void receiveBuffer(const Fw::Buffer& buffer);

    //! Receive a com packet
    void receiveCom(const Fw::ComBuffer& comBuffer);

    //! Get received packets
    const std::vector<Fw::Buffer>& getReceivedBuffers() const;

    //! Get received com packets
    const std::vector<Fw::ComBuffer>& getReceivedCom() const;

    //! Clear received packets
    void clearReceived();

    //! Validate audio packet format
    bool validateAudioPacket(const Fw::Buffer& buffer);

    //! Get packet statistics
    struct PacketStats {
        U32 totalPackets;
        U32 validPackets;
        U32 invalidPackets;
        U32 totalBytes;
        U32 sequenceErrors;
    };

    PacketStats getStats() const;

private:
    // ----------------------------------------------------------------------
    // Private member variables
    // ----------------------------------------------------------------------

    //! Received buffer packets
    std::vector<Fw::Buffer> m_receivedBuffers;

    //! Received com packets
    std::vector<Fw::ComBuffer> m_receivedCom;

    //! Packet statistics
    PacketStats m_stats;

    //! Expected sequence number
    U32 m_expectedSequence;
};

} // namespace Components

#endif 
// ======================================================================
// \title  TcpServerComponentImpl.cpp
// \author mstarch
// \brief  cpp file for TcpServerComponentImpl component implementation class
//
// Derived from Drv/TcpServer in nasa/fprime 52412392. See TcpServerComponentImpl.hpp for the changes.
//
// \copyright
// Copyright 2009-2020, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include <limits>
#include <Components/AmsatDrv/TcpServer/TcpServerComponentImpl.hpp>
#include <Fw/FPrimeBasicTypes.hpp>
#include "Fw/Types/Assert.hpp"
#include "Fw/Logger/Logger.hpp"

#include <sys/socket.h>

namespace AmsatDrv {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

TcpServerComponentImpl::TcpServerComponentImpl(const char* const compName)
    : TcpServerComponentBase(compName) {}

Drv::SocketIpStatus TcpServerComponentImpl::configure(const char* hostname,
                                                      const U16 port,
                                                      const U32 send_timeout_seconds,
                                                      const U32 send_timeout_microseconds,
                                                      FwSizeType buffer_size) {
    // Check that ensures the configured buffer size fits within the limits fixed-width type, U32
    FW_ASSERT(buffer_size <= std::numeric_limits<U32>::max(), static_cast<FwAssertArgType>(buffer_size));
    m_allocation_size = buffer_size;  // Store the buffer size
    (void)m_socket.configure(hostname, port, send_timeout_seconds, send_timeout_microseconds);
    return startup();
}

TcpServerComponentImpl::~TcpServerComponentImpl() {}

// ----------------------------------------------------------------------
// Implementations for socket read task virtual methods
// ----------------------------------------------------------------------

U16 TcpServerComponentImpl::getListenPort() {
    return m_socket.getListenPort();
}

Drv::IpSocket& TcpServerComponentImpl::getSocketHandler() {
    return m_socket;
}

Fw::Buffer TcpServerComponentImpl::getBuffer() {
    return allocate_out(0, static_cast<U32>(m_allocation_size));
}

void TcpServerComponentImpl::sendBuffer(Fw::Buffer buffer, Drv::SocketIpStatus status) {
    Drv::ByteStreamStatus recvStatus = Drv::ByteStreamStatus::OTHER_ERROR;
    if (status == Drv::SOCK_SUCCESS) {
        recvStatus = Drv::ByteStreamStatus::OP_OK;
    } else if (status == Drv::SOCK_NO_DATA_AVAILABLE) {
        recvStatus = Drv::ByteStreamStatus::RECV_NO_DATA;
    } else {
        recvStatus = Drv::ByteStreamStatus::OTHER_ERROR;
    }
    this->recv_out(0, buffer, recvStatus);
}

void TcpServerComponentImpl::connected() {
    if (isConnected_ready_OutputPort(0)) {
        this->ready_out(0);
    }
}

bool TcpServerComponentImpl::isStarted() {
    Os::ScopeLock scopedLock(this->m_lock);
    return this->m_descriptor.serverFd != -1;
}

Drv::SocketIpStatus TcpServerComponentImpl::startup() {
    Os::ScopeLock scopedLock(this->m_lock);
    Drv::SocketIpStatus status = Drv::SOCK_SUCCESS;
    // Prevent multiple startup attempts
    if (this->m_descriptor.serverFd == -1) {
        status = this->m_socket.startup(this->m_descriptor);
    }
    return status;
}

void TcpServerComponentImpl::terminate() {
    this->stop();
    Os::ScopeLock scopedLock(this->m_lock);
    if (this->m_descriptor.serverFd != -1) {
        // Closing alone does not wake a thread blocked in accept() on Linux; shutting down the socket does
        (void)::shutdown(this->m_descriptor.serverFd, SHUT_RDWR);
        this->m_socket.terminate(this->m_descriptor);
        this->m_descriptor.serverFd = -1;
    }
}

void TcpServerComponentImpl::readLoop() {
    Drv::SocketIpStatus status = Drv::SocketIpStatus::SOCK_NOT_STARTED;
    // Keep trying to reconnect until the status is good, told to stop, or reconnection is turned off
    do {
        status = this->startup();
        if (status != Drv::SOCK_SUCCESS) {
            Fw::Logger::log("[WARNING] Failed to listen on port %hu with status %d\n", this->getListenPort(), status);
            (void)Os::Task::delay(SOCKET_RETRY_INTERVAL);
            continue;
        }
    } while (this->running() && status != Drv::SOCK_SUCCESS && this->m_reopen);
    // If start up was successful then perform normal operations
    if (this->running() && status == Drv::SOCK_SUCCESS) {
        // Perform the nominal read loop
        SocketComponentHelper::readLoop();
    }
    // Terminate the server
    this->terminate();
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------

void TcpServerComponentImpl::send_handler(const FwIndexType portNum, Fw::Buffer& fwBuffer) {
    FW_ASSERT_NO_OVERFLOW(fwBuffer.getSize(), U32);
    this->m_lock.lock();
    const Drv::SocketDescriptor descriptor = this->m_descriptor;
    this->m_lock.unlock();

    // Send only on an established connection. Unlike SocketComponentHelper::send(), never reopen here: that would
    // call accept() on the sending thread while holding this port's lock.
    Drv::ByteStreamStatus returnStatus = Drv::ByteStreamStatus::OTHER_ERROR;
    if (descriptor.fd != -1) {
        const Drv::SocketIpStatus status =
            this->m_socket.send(descriptor, fwBuffer.getData(), static_cast<U32>(fwBuffer.getSize()));
        if (status == Drv::SOCK_SUCCESS) {
            returnStatus = Drv::ByteStreamStatus::OP_OK;
        } else if (status == Drv::SOCK_INTERRUPTED_TRY_AGAIN) {
            returnStatus = Drv::ByteStreamStatus::SEND_RETRY;
        } else if (status == Drv::SOCK_DISCONNECTED) {
            this->closeIfCurrent(descriptor);
        }
    }
    // Return the buffer and status to the caller
    this->sendReturnOut_out(0, fwBuffer, returnStatus);
}

void TcpServerComponentImpl::recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    this->deallocate_out(0, fwBuffer);
}

void TcpServerComponentImpl::closeIfCurrent(const Drv::SocketDescriptor& descriptor) {
    Os::ScopeLock scopedLock(this->m_lock);
    if (this->m_descriptor.fd == descriptor.fd) {
        this->m_socket.close(this->m_descriptor);
        this->m_descriptor.fd = -1;
        this->m_open = OpenState::NOT_OPEN;
    }
}

}  // end namespace AmsatDrv

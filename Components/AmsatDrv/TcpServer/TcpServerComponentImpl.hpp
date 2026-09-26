// ======================================================================
// \title  TcpServerComponentImpl.hpp
// \author mstarch
// \brief  hpp file for TcpServerComponentImpl component implementation class
//
// Derived from Drv/TcpServer in the F' version this project uses (nasa/fprime 52412392), with two fixes:
//
// 1. Sends never open a connection. The F' version reconnects inside send(), which calls accept() on the thread that
//    sends telemetry while it holds this component's port lock. Until a GDS connects, that thread blocks, telemetry
//    backs up, and shutdown waits on it forever. If the read task is already in accept(), the send instead asserts
//    on the invalid socket. Here, sends without a connection return OTHER_ERROR, and only the read task accepts.
// 2. terminate() stops the read task and shuts down the listening socket, waking a read task blocked in accept().
//    Backported from nasa/fprime e3fd78f28.
//
// Replace with Drv.TcpServer after moving to an F' release with equivalent fixes.
//
// \copyright
// Copyright 2009-2020, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef AmsatDrv_TcpServerComponentImpl_HPP
#define AmsatDrv_TcpServerComponentImpl_HPP

#include <config/IpCfg.hpp>
#include <Drv/Ip/IpSocket.hpp>
#include <Drv/Ip/SocketComponentHelper.hpp>
#include <Drv/Ip/TcpServerSocket.hpp>
#include "Components/AmsatDrv/TcpServer/TcpServerComponentAc.hpp"

namespace AmsatDrv {

class TcpServerComponentImpl final : public TcpServerComponentBase, public Drv::SocketComponentHelper {
    friend class TcpServerTester;

  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    /**
     * \brief construct the TcpServer component.
     * \param compName: name of this component
     */
    TcpServerComponentImpl(const char* const compName);

    /**
     * \brief Destroy the component
     */
    ~TcpServerComponentImpl();

    // ----------------------------------------------------------------------
    // Helper methods to start and stop socket
    // ----------------------------------------------------------------------

    /**
     * \brief Configures the TcpServer settings but does not open the connection
     *
     * The TcpServerComponent needs to connect to a remote TCP server. This call configures the hostname, port and
     * send timeouts for that socket connection. This call should be performed on system startup before recv or send
     * are called. Note: hostname must be a dot-notation IP address of the form "x.x.x.x". DNS translation is left up
     * to the user.
     *
     * \param hostname: ip address of remote tcp server in the form x.x.x.x
     * \param port: port of remote tcp server
     * \param send_timeout_seconds: send timeout seconds portion
     * \param send_timeout_microseconds: send timeout microseconds portion. Must be less than 1000000
     * \param buffer_size: size of the buffer to be allocated. Defaults to 1024.
     * \return status of the configure
     */
    Drv::SocketIpStatus configure(const char* hostname,
                                  const U16 port,
                                  const U32 send_timeout_seconds = SOCKET_SEND_TIMEOUT_SECONDS,
                                  const U32 send_timeout_microseconds = SOCKET_SEND_TIMEOUT_MICROSECONDS,
                                  FwSizeType buffer_size = 1024);

    /**
     * \brief is started
     */
    bool isStarted();

    /**
     * \brief startup the server socket for communications
     *
     * Start up the server socket by listening for incoming connections. This must be done before any clients can
     * connect.
     */
    Drv::SocketIpStatus startup();

    /**
     * \brief stop the read task and terminate the server socket
     *
     * Stops the read task, then shuts down and closes the server socket, waking the read task if it is waiting in
     * accept(). Call instead of stop() during teardown, then join().
     */
    void terminate();

    /**
     * \brief get the port being listened on
     *
     * Most useful when listen was configured to use port "0", this will return the port used for listening after a
     * port has been determined. Will return 0 if the connection has not been setup.
     *
     * \return receive port
     */
    U16 getListenPort();

  protected:
    // ----------------------------------------------------------------------
    // Implementations for socket read task virtual methods
    // ----------------------------------------------------------------------

    /**
     * \brief returns a reference to the socket handler
     */
    Drv::IpSocket& getSocketHandler() override;

    /**
     * \brief returns a buffer to fill with data
     *
     * Gets a reference to a buffer to fill with data. This allows the component to determine how to provide a
     * buffer and the socket read task just fills said buffer.
     *
     * \return Fw::Buffer to fill with data
     */
    Fw::Buffer getBuffer() override;

    /**
     * \brief sends a buffer to be filled with data
     *
     * Sends the buffer gotten by getBuffer that has now been filled with data. This is used to delegate to the
     * component how to send back the buffer. Ignores buffers with error status error.
     *
     * \return Fw::Buffer filled with data to send out
     */
    void sendBuffer(Fw::Buffer buffer, Drv::SocketIpStatus status) override;

    /**
     * \brief called when the IPv4 system has been connected
     */
    void connected() override;

    /**
     * \brief read from the socket, overridden to start and terminate the server socket
     */
    void readLoop() override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    /**
     * \brief Send data out of the TcpServer
     *
     * Passing data to this port will send data from the TcpServer to whatever TCP client this component has
     * established a connection with. Without a connection, the buffer is returned with OTHER_ERROR at once; the read
     * task accepts new connections.
     *
     * \param portNum: port number. Unused.
     * \param fwBuffer: buffer containing data to be sent
     */
    void send_handler(const FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    //! Handler implementation for recvReturnIn
    //!
    //! Port receiving back ownership of data sent out on $recv port
    void recvReturnIn_handler(FwIndexType portNum,  //!< The port number
                              Fw::Buffer& fwBuffer  //!< The buffer
                              ) override;

    //! Close the connection after a failed send, unless the read task has already replaced it with a new one
    void closeIfCurrent(const Drv::SocketDescriptor& descriptor);

    Drv::TcpServerSocket m_socket;  //!< Socket implementation

    FwSizeType m_allocation_size;  //!< Member variable to store the buffer size
};

}  // end namespace AmsatDrv

#endif  // end AmsatDrv_TcpServerComponentImpl_HPP

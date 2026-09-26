module AmsatDrv {
    @ TCP server byte stream driver: Drv.TcpServer with fixes for disconnected sends and shutdown
    passive component TcpServer {

        import Drv.ByteStreamDriver

        @ Allocation for received data
        output port allocate: Fw.BufferGet

        @ Deallocation of allocated buffers
        output port deallocate: Fw.BufferSend

    }
}

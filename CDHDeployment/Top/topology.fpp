module CDHDeployment {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

  enum Ports_RateGroups {
    rateGroup1Hz
    rateGroupHalfHz
    rateGroupQuarterHz
  }
  enum Ports_ComPacketQueue {
    EVENTS,
    TELEMETRY
  }
  enum Ports_ComBufferQueue {
    FILE_DOWNLINK
  }

  topology CDHDeployment {

    # ----------------------------------------------------------------------
    # Instances used in the topology
    # ----------------------------------------------------------------------

    instance $health
    instance tlmSend
    instance cmdDisp
    instance cmdSeq
    instance comDriver
    instance comQueue
    instance comStub
    instance fprimeRouter
    instance deframer
    instance frameAccumulator
    instance eventLogger
    instance fatalAdapter
    instance fatalHandler
    instance fileDownlink
    instance fileManager
    instance fileUplink
    instance bufferManager
    instance framer
    instance chronoTime
    instance prmDb
    instance rateGroup1Hz
    instance rateGroupHalfHz
    instance rateGroupQuarterHz
    instance rateGroupDriver
    instance textLogger
    instance systemResources
    instance version
    instance linuxTimer
    instance camera

    # ----------------------------------------------------------------------
    # Pattern graph specifiers
    # ----------------------------------------------------------------------

    command connections instance cmdDisp

    event connections instance eventLogger

    param connections instance prmDb

    telemetry connections instance tlmSend

    text event connections instance textLogger

    time connections instance chronoTime

    health connections instance $health

    # ----------------------------------------------------------------------
    # Telemetry packets
    # ----------------------------------------------------------------------

    include "CDHDeploymentPackets.fppi"


    # ----------------------------------------------------------------------
    # Direct graph specifiers
    # ----------------------------------------------------------------------

    connections Downlink {
      # Inputs to ComQueue (events, telemetry, file)
      eventLogger.PktSend         -> comQueue.comPacketQueueIn[Ports_ComPacketQueue.EVENTS]
      tlmSend.PktSend             -> comQueue.comPacketQueueIn[Ports_ComPacketQueue.TELEMETRY]
      fileDownlink.bufferSendOut  -> comQueue.bufferQueueIn[Ports_ComBufferQueue.FILE_DOWNLINK]
      comQueue.bufferReturnOut[Ports_ComBufferQueue.FILE_DOWNLINK] -> fileDownlink.bufferReturn

      # ComQueue <-> Framer
      comQueue.dataOut   -> framer.dataIn
      framer.dataReturnOut -> comQueue.dataReturnIn
      framer.comStatusOut  -> comQueue.comStatusIn

      # Buffer Management for Framer
      framer.bufferAllocate   -> bufferManager.bufferGetCallee
      framer.bufferDeallocate -> bufferManager.bufferSendIn

      # Framer <-> ComStub
      framer.dataOut        -> comStub.dataIn
      comStub.dataReturnOut -> framer.dataReturnIn
      comStub.comStatusOut  -> framer.comStatusIn

      # ComStub <-> ComDriver
      comStub.drvSendOut      -> comDriver.$send
      comDriver.sendReturnOut -> comStub.drvSendReturnIn
      comDriver.ready         -> comStub.drvConnected
    }

    connections FaultProtection {
      eventLogger.FatalAnnounce -> fatalHandler.FatalReceive
    }

    connections RateGroups {
      # LinuxTimer to drive rate group
      linuxTimer.CycleOut -> rateGroupDriver.CycleIn

      # 1Hz rate group
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1Hz] -> rateGroup1Hz.CycleIn
      rateGroup1Hz.RateGroupMemberOut[0] -> tlmSend.Run
      rateGroup1Hz.RateGroupMemberOut[1] -> fileDownlink.Run
      rateGroup1Hz.RateGroupMemberOut[2] -> systemResources.run
      rateGroup1Hz.RateGroupMemberOut[3] -> comQueue.run

      # 0.5Hz rate group
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroupHalfHz] -> rateGroupHalfHz.CycleIn
      rateGroupHalfHz.RateGroupMemberOut[0] -> cmdSeq.schedIn

      # 0.25Hz rate group
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroupQuarterHz] -> rateGroupQuarterHz.CycleIn
      rateGroupQuarterHz.RateGroupMemberOut[0] -> $health.Run
      rateGroupQuarterHz.RateGroupMemberOut[1] -> bufferManager.schedIn
    }

    connections Sequencer {
      cmdSeq.comCmdOut -> cmdDisp.seqCmdBuff
      cmdDisp.seqCmdStatus -> cmdSeq.cmdResponseIn
    }

    connections Uplink {
      # ComDriver buffer allocations
      comDriver.allocate      -> bufferManager.bufferGetCallee
      comDriver.deallocate    -> bufferManager.bufferSendIn
      # ComDriver <-> ComStub
      comDriver.$recv             -> comStub.drvReceiveIn
      comStub.drvReceiveReturnOut -> comDriver.recvReturnIn
      # ComStub <-> FrameAccumulator
      comStub.dataOut                -> frameAccumulator.dataIn
      frameAccumulator.dataReturnOut -> comStub.dataReturnIn
      # FrameAccumulator buffer allocations
      frameAccumulator.bufferDeallocate -> bufferManager.bufferSendIn
      frameAccumulator.bufferAllocate   -> bufferManager.bufferGetCallee
      # FrameAccumulator <-> Deframer
      frameAccumulator.dataOut  -> deframer.dataIn
      deframer.dataReturnOut    -> frameAccumulator.dataReturnIn
      # Deframer <-> Router
      deframer.dataOut           -> fprimeRouter.dataIn
      fprimeRouter.dataReturnOut -> deframer.dataReturnIn
      # Router buffer allocations
      fprimeRouter.bufferAllocate   -> bufferManager.bufferGetCallee
      fprimeRouter.bufferDeallocate -> bufferManager.bufferSendIn
      # Router <-> CmdDispatcher/FileUplink
      fprimeRouter.commandOut  -> cmdDisp.seqCmdBuff
      cmdDisp.seqCmdStatus     -> fprimeRouter.cmdResponseIn
      fprimeRouter.fileOut     -> fileUplink.bufferSendIn
      fileUplink.bufferSendOut -> fprimeRouter.fileBufferReturnIn
    }

    connections CDHDeployment {
      # Add connections here to user-defined components
    }

  }

}

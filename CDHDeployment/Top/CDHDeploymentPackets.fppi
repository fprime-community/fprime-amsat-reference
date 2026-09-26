telemetry packets CDHDeploymentPackets {

  packet CDH id 1 group 1 {
    cmdDisp.CommandsDispatched
    rateGroup1Hz.RgMaxTime
    rateGroupHalfHz.RgMaxTime
    rateGroupQuarterHz.RgMaxTime
    cmdSeq.CS_LoadCommands
    cmdSeq.CS_CancelCommands
    cmdSeq.CS_CommandsExecuted
    cmdSeq.CS_SequencesCompleted
    fileUplink.FilesReceived
    fileUplink.PacketsReceived
    bufferManager.TotalBuffs
    bufferManager.CurrBuffs
    bufferManager.HiBuffs
    fileDownlink.FilesSent
    fileDownlink.PacketsSent
    fileManager.CommandsExecuted
    comQueue.comQueueDepth
    comQueue.buffQueueDepth
    cmdSeq.CS_CurrentSequence
    # Note: Uncomment when using Svc:TlmPacketizer
    # tlmSend.SendLevel
  }

  packet CDHErrors id 2 group 1 {
    rateGroup1Hz.RgCycleSlips
    rateGroupHalfHz.RgCycleSlips
    rateGroupQuarterHz.RgCycleSlips
    cmdSeq.CS_Errors
    fileUplink.Warnings
    fileDownlink.Warnings
    $health.PingLateWarnings
    fileManager.Errors
    bufferManager.NoBuffs
    bufferManager.EmptyBuffs
    fileManager.Errors
    eventLogger.EventsDropped
    cmdDisp.CommandsDropped
    fileUplink.FilesReceivedFailed
  }

  packet SystemRes1 id 4 group 2 {
    systemResources.MEMORY_TOTAL
    systemResources.MEMORY_USED
    systemResources.NON_VOLATILE_TOTAL
    systemResources.NON_VOLATILE_FREE
  }

  packet SystemRes3 id 5 group 2 {
    systemResources.CPU
    systemResources.CPU_00
    systemResources.CPU_01
    systemResources.CPU_02
    systemResources.CPU_03
    systemResources.CPU_04
    systemResources.CPU_05
    systemResources.CPU_06
    systemResources.CPU_07
    systemResources.CPU_08
    systemResources.CPU_09
    systemResources.CPU_10
    systemResources.CPU_11
    systemResources.CPU_12
    systemResources.CPU_13
    systemResources.CPU_14
    systemResources.CPU_15
  }

  packet Version1 id 6 group 2 {
    version.FrameworkVersion
    version.ProjectVersion
  }

  packet Version_Library1 id 7 group 2 {
    version.LibraryVersion01
    version.LibraryVersion02
  }

  packet Version_Library2 id 8 group 2 {
    version.LibraryVersion03
    version.LibraryVersion04
  }

  packet Version_Library3 id 9 group 2 {
    version.LibraryVersion05
    version.LibraryVersion06
  }

  packet Version_Library4 id 10 group 2 {
    version.LibraryVersion07
    version.LibraryVersion08
  }

  packet Version_Library5 id 11 group 2 {
    version.LibraryVersion09
    version.LibraryVersion10
  }

  packet Version_Custom1 id 12 group 2 {
    version.CustomVersion01
  }

  packet Version_Custom2 id 13 group 2 {
    version.CustomVersion02
  }

  packet Version_Custom3 id 14 group 2 {
    version.CustomVersion03
  }

  packet Version_Custom4 id 15 group 2 {
    version.CustomVersion04
  }

  packet Version_Custom5 id 16 group 2 {
    version.CustomVersion05
  }

  packet Version_Custom6 id 17 group 2 {
    version.CustomVersion06
  }

  packet Version_Custom7 id 18 group 2 {
    version.CustomVersion07
  }

  packet Version_Custom8 id 19 group 2 {
    version.CustomVersion08
  }

  packet Version_Custom9 id 20 group 2 {
    version.CustomVersion09
  }

  packet Version_Custom10 id 21 group 2 {
    version.CustomVersion10
  }

  packet Camera id 22 group 1 {
    camera.PicturesTaken
    camera.CaptureFailures
    camera.LastImageSize
  }

} omit {
  cmdDisp.CommandErrors
}

telemetry packets MainDeploymentPackets {

  packet CDH id 1 group 1 {
    MainDeployment.cmdDisp.CommandsDispatched
    MainDeployment.rateGroup1.RgMaxTime
    MainDeployment.rateGroup2.RgMaxTime
    MainDeployment.rateGroup3.RgMaxTime
    MainDeployment.cmdSeq.CS_LoadCommands
    MainDeployment.cmdSeq.CS_CancelCommands
    MainDeployment.cmdSeq.CS_CommandsExecuted
    MainDeployment.cmdSeq.CS_SequencesCompleted
    MainDeployment.fileUplink.FilesReceived
    MainDeployment.fileUplink.PacketsReceived
    MainDeployment.bufferManager.TotalBuffs
    MainDeployment.bufferManager.CurrBuffs
    MainDeployment.bufferManager.HiBuffs
    MainDeployment.fileDownlink.FilesSent
    MainDeployment.fileDownlink.PacketsSent
    MainDeployment.fileManager.CommandsExecuted
    MainDeployment.comQueue.comQueueDepth
    MainDeployment.comQueue.buffQueueDepth
    # Note: Uncomment when using Svc:TlmPacketizer
    # MainDeployment.tlmSend.SendLevel
  }

  packet CDHErrors id 2 group 1 {
    MainDeployment.rateGroup1.RgCycleSlips
    MainDeployment.rateGroup2.RgCycleSlips
    MainDeployment.rateGroup3.RgCycleSlips
    MainDeployment.cmdSeq.CS_Errors
    MainDeployment.fileUplink.Warnings
    MainDeployment.fileDownlink.Warnings
    MainDeployment.$health.PingLateWarnings
    MainDeployment.fileManager.Errors
    MainDeployment.bufferManager.NoBuffs
    MainDeployment.bufferManager.EmptyBuffs
    MainDeployment.fileManager.Errors
  }

  packet SystemRes1 id 4 group 2 {
    MainDeployment.systemResources.MEMORY_TOTAL
    MainDeployment.systemResources.MEMORY_USED
    MainDeployment.systemResources.NON_VOLATILE_TOTAL
    MainDeployment.systemResources.NON_VOLATILE_FREE
  }

  packet SystemRes3 id 5 group 2 {
    MainDeployment.systemResources.CPU
    MainDeployment.systemResources.CPU_00
    MainDeployment.systemResources.CPU_01
    MainDeployment.systemResources.CPU_02
    MainDeployment.systemResources.CPU_03
    MainDeployment.systemResources.CPU_04
    MainDeployment.systemResources.CPU_05
    MainDeployment.systemResources.CPU_06
    MainDeployment.systemResources.CPU_07
    MainDeployment.systemResources.CPU_08
    MainDeployment.systemResources.CPU_09
    MainDeployment.systemResources.CPU_10
    MainDeployment.systemResources.CPU_11
    MainDeployment.systemResources.CPU_12
    MainDeployment.systemResources.CPU_13
    MainDeployment.systemResources.CPU_14
    MainDeployment.systemResources.CPU_15
  }

  packet Version1 id 6 group 2 {
    MainDeployment.version.FrameworkVersion
    MainDeployment.version.ProjectVersion
  }

  packet Version_Library1 id 7 group 2 {
    MainDeployment.version.LibraryVersion01
    MainDeployment.version.LibraryVersion02
  }

  packet Version_Library2 id 8 group 2 {
    MainDeployment.version.LibraryVersion03
    MainDeployment.version.LibraryVersion04
  }

  packet Version_Library3 id 9 group 2 {
    MainDeployment.version.LibraryVersion05
    MainDeployment.version.LibraryVersion06
  }

  packet Version_Library4 id 10 group 2 {
    MainDeployment.version.LibraryVersion07
    MainDeployment.version.LibraryVersion08
  }

  packet Version_Library5 id 11 group 2 {
    MainDeployment.version.LibraryVersion09
    MainDeployment.version.LibraryVersion10
  }

  packet Version_Custom1 id 12 group 2 {
    MainDeployment.version.CustomVersion01
  }

  packet Version_Custom2 id 13 group 2 {
    MainDeployment.version.CustomVersion02
  }

  packet Version_Custom3 id 14 group 2 {
    MainDeployment.version.CustomVersion03
  }

  packet Version_Custom4 id 15 group 2 {
    MainDeployment.version.CustomVersion04
  }

  packet Version_Custom5 id 16 group 2 {
    MainDeployment.version.CustomVersion05
  }

  packet Version_Custom6 id 17 group 2 {
    MainDeployment.version.CustomVersion06
  }

  packet Version_Custom7 id 18 group 2 {
    MainDeployment.version.CustomVersion07
  }

  packet Version_Custom8 id 19 group 2 {
    MainDeployment.version.CustomVersion08
  }

  packet Version_Custom9 id 20 group 2 {
    MainDeployment.version.CustomVersion09
  }

  packet Version_Custom10 id 21 group 2 {
    MainDeployment.version.CustomVersion10
  }

} omit {
  MainDeployment.cmdDisp.CommandErrors
  MainDeployment.comDriver.BytesRecv
  MainDeployment.comDriver.BytesSent
}

telemetry packets CDHDeploymentPackets {

  packet CDH id 1 group 1 {
    CDHDeployment.cmdDisp.CommandsDispatched
    CDHDeployment.rateGroup1.RgMaxTime
    CDHDeployment.rateGroup2.RgMaxTime
    CDHDeployment.rateGroup3.RgMaxTime
    CDHDeployment.cmdSeq.CS_LoadCommands
    CDHDeployment.cmdSeq.CS_CancelCommands
    CDHDeployment.cmdSeq.CS_CommandsExecuted
    CDHDeployment.cmdSeq.CS_SequencesCompleted
    CDHDeployment.fileUplink.FilesReceived
    CDHDeployment.fileUplink.PacketsReceived
    CDHDeployment.bufferManager.TotalBuffs
    CDHDeployment.bufferManager.CurrBuffs
    CDHDeployment.bufferManager.HiBuffs
    CDHDeployment.fileDownlink.FilesSent
    CDHDeployment.fileDownlink.PacketsSent
    CDHDeployment.fileManager.CommandsExecuted
    CDHDeployment.comQueue.comQueueDepth
    CDHDeployment.comQueue.buffQueueDepth
    # USB Sound Card telemetry
    CDHDeployment.usbSoundCard.AUDIO_INPUT_LEVEL
    CDHDeployment.usbSoundCard.DEVICE_CONNECTED
    CDHDeployment.usbSoundCard.FRAMES_PROCESSED
    CDHDeployment.usbSoundCard.AUDIO_PEAK_LEVEL
    # Note: Uncomment when using Svc:TlmPacketizer
    # CDHDeployment.tlmSend.SendLevel
  }

  packet CDHErrors id 2 group 1 {
    CDHDeployment.rateGroup1.RgCycleSlips
    CDHDeployment.rateGroup2.RgCycleSlips
    CDHDeployment.rateGroup3.RgCycleSlips
    CDHDeployment.cmdSeq.CS_Errors
    CDHDeployment.fileUplink.Warnings
    CDHDeployment.fileDownlink.Warnings
    CDHDeployment.$health.PingLateWarnings
    CDHDeployment.fileManager.Errors
    CDHDeployment.bufferManager.NoBuffs
    CDHDeployment.bufferManager.EmptyBuffs
    CDHDeployment.fileManager.Errors
  }

  packet SystemRes1 id 4 group 2 {
    CDHDeployment.systemResources.MEMORY_TOTAL
    CDHDeployment.systemResources.MEMORY_USED
    CDHDeployment.systemResources.NON_VOLATILE_TOTAL
    CDHDeployment.systemResources.NON_VOLATILE_FREE
  }

  packet SystemRes3 id 5 group 2 {
    CDHDeployment.systemResources.CPU
    CDHDeployment.systemResources.CPU_00
    CDHDeployment.systemResources.CPU_01
    CDHDeployment.systemResources.CPU_02
    CDHDeployment.systemResources.CPU_03
    CDHDeployment.systemResources.CPU_04
    CDHDeployment.systemResources.CPU_05
    CDHDeployment.systemResources.CPU_06
    CDHDeployment.systemResources.CPU_07
    CDHDeployment.systemResources.CPU_08
    CDHDeployment.systemResources.CPU_09
    CDHDeployment.systemResources.CPU_10
    CDHDeployment.systemResources.CPU_11
    CDHDeployment.systemResources.CPU_12
    CDHDeployment.systemResources.CPU_13
    CDHDeployment.systemResources.CPU_14
    CDHDeployment.systemResources.CPU_15
  }

  packet Version1 id 6 group 2 {
    CDHDeployment.version.FrameworkVersion
    CDHDeployment.version.ProjectVersion
  }

  packet Version_Library1 id 7 group 2 {
    CDHDeployment.version.LibraryVersion01
    CDHDeployment.version.LibraryVersion02
  }

  packet Version_Library2 id 8 group 2 {
    CDHDeployment.version.LibraryVersion03
    CDHDeployment.version.LibraryVersion04
  }

  packet Version_Library3 id 9 group 2 {
    CDHDeployment.version.LibraryVersion05
    CDHDeployment.version.LibraryVersion06
  }

  packet Version_Library4 id 10 group 2 {
    CDHDeployment.version.LibraryVersion07
    CDHDeployment.version.LibraryVersion08
  }

  packet Version_Library5 id 11 group 2 {
    CDHDeployment.version.LibraryVersion09
    CDHDeployment.version.LibraryVersion10
  }

  packet Version_Custom1 id 12 group 2 {
    CDHDeployment.version.CustomVersion01
  }

  packet Version_Custom2 id 13 group 2 {
    CDHDeployment.version.CustomVersion02
  }

  packet Version_Custom3 id 14 group 2 {
    CDHDeployment.version.CustomVersion03
  }

  packet Version_Custom4 id 15 group 2 {
    CDHDeployment.version.CustomVersion04
  }

  packet Version_Custom5 id 16 group 2 {
    CDHDeployment.version.CustomVersion05
  }

  packet Version_Custom6 id 17 group 2 {
    CDHDeployment.version.CustomVersion06
  }

  packet Version_Custom7 id 18 group 2 {
    CDHDeployment.version.CustomVersion07
  }

  packet Version_Custom8 id 19 group 2 {
    CDHDeployment.version.CustomVersion08
  }

  packet Version_Custom9 id 20 group 2 {
    CDHDeployment.version.CustomVersion09
  }

  packet Version_Custom10 id 21 group 2 {
    CDHDeployment.version.CustomVersion10
  }

} omit {
  CDHDeployment.cmdDisp.CommandErrors
}
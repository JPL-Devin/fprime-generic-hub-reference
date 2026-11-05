module FprimeGenericHubReference {
module DeploymentA {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

    enum Ports_RateGroups {
      rateGroup1
      rateGroup2
      rateGroup3
    }

  deployment topology DeploymentA {

    # ----------------------------------------------------------------------
    # Subtopology imports
    # ----------------------------------------------------------------------
      import CdhCore.Subtopology
      import ComCcsds.Subtopology
      import DataProducts.Subtopology
      import FileHandling.Subtopology
    
    # ----------------------------------------------------------------------
    # Instances used in the topology
    # ----------------------------------------------------------------------
      instance a_rateGroup1
      instance a_rateGroup2
      instance a_rateGroup3
      instance a_cmdSeq
      instance a_comDriver
      instance a_rateGroupDriver
      instance systemResources
      instance linuxTimer
      instance a_cmdSplitter
      instance a_hub
      instance a_hubComDriver
      instance a_hubByteStreamAdapter
      instance a_hubBufferManager
      instance a_hubComDriverBufferManager
      instance a_hubComQueue
      instance a_hubComStub
      instance a_comp
      instance c_comp

    # ----------------------------------------------------------------------
    # Pattern graph specifiers
    # ----------------------------------------------------------------------

    
    command connections instance CdhCore.cmdDisp
    
    event connections instance CdhCore.events
    
    telemetry connections instance CdhCore.tlmSend
    
    text event connections instance CdhCore.textLogger
    
    health connections instance CdhCore.$health
    

    # ----------------------------------------------------------------------
    # Direct graph specifiers
    # ----------------------------------------------------------------------

    connections ComCcsds_CdhCore {
      # Core events and telemetry to communication queue
      CdhCore.events.PktSend -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.EVENTS]
      CdhCore.tlmSend.PktSend -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.TELEMETRY]

      # Router to Command Dispatcher
      ComCcsds.fprimeRouter.commandOut -> a_cmdSplitter.CmdBuff
    
      a_cmdSplitter.LocalCmd[0] -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus -> a_cmdSplitter.seqCmdStatus
      #a_cmdSplitter.forwardSeqCmdStatus -> ComCcsds.fprimeRouter.cmdResponseIn
      a_hub.cmdRespOut -> ComCcsds.fprimeRouter.cmdResponseIn
      a_cmdSplitter.forwardSeqCmdStatus -> ComCcsds.fprimeRouter.cmdResponseIn

    }

    connections Communications {
      # ComDriver buffer allocations
      a_comDriver.allocate      -> ComCcsds.commsBufferManager.bufferGetCallee
      a_comDriver.deallocate    -> ComCcsds.commsBufferManager.bufferSendIn
      
      # ComDriver <-> ComStub (Uplink)
      a_comDriver.$recv                    -> ComCcsds.comStub.drvReceiveIn
      ComCcsds.comStub.drvReceiveReturnOut -> a_comDriver.recvReturnIn
      
      # ComStub <-> ComDriver (Downlink)
      ComCcsds.comStub.drvSendOut  -> a_comDriver.$send
      a_comDriver.ready            -> ComCcsds.comStub.drvConnected
    }

    connections RateGroups {
      # LinuxTimer to drive rate group
      linuxTimer.CycleOut -> a_rateGroupDriver.CycleIn

      # Rate group 1
      a_rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1] -> a_rateGroup1.CycleIn
      a_rateGroup1.RateGroupMemberOut[0] -> CdhCore.tlmSend.Run
      a_rateGroup1.RateGroupMemberOut[1] -> FileHandling.fileDownlink.Run
      a_rateGroup1.RateGroupMemberOut[2] -> systemResources.run
      a_rateGroup1.RateGroupMemberOut[3] -> ComCcsds.comQueue.run
      a_rateGroup1.RateGroupMemberOut[4] -> ComCcsds.aggregator.timeout
      a_rateGroup1.RateGroupMemberOut[5] -> a_hubComQueue.run

      # Rate group 2
      a_rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup2] -> a_rateGroup2.CycleIn
      a_rateGroup2.RateGroupMemberOut[0] -> a_cmdSeq.schedIn

      # Rate group 3
      a_rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup3] -> a_rateGroup3.CycleIn
      a_rateGroup3.RateGroupMemberOut[0] -> CdhCore.$health.Run
      a_rateGroup3.RateGroupMemberOut[1] -> ComCcsds.commsBufferManager.schedIn
      a_rateGroup3.RateGroupMemberOut[2] -> DataProducts.dpBufferManager.schedIn
      a_rateGroup3.RateGroupMemberOut[3] -> DataProducts.dpWriter.schedIn
      a_rateGroup3.RateGroupMemberOut[4] -> a_hubComDriverBufferManager.schedIn
      a_rateGroup3.RateGroupMemberOut[5] -> a_hubBufferManager.schedIn
      a_rateGroup3.RateGroupMemberOut[6] -> a_hub.schedIn
    }

    connections send_hub {
      a_hub.eventOut -> a_comp.eventIn
      a_hub.tlmOut   -> a_comp.telemIn

      a_hub.serialOut -> a_comp.serIn
      a_hub.bufferOut -> a_comp.bufIn
      a_comp.bufInRet -> a_hub.bufferOutReturn
    }

    connections recv_hub {
      a_hubComDriver.$recv -> a_hubByteStreamAdapter.fromByteStreamDriver
      
      a_hubByteStreamAdapter.bufferOut -> a_hub.fromBufferDriver
      a_hub.fromBufferDriverReturn     -> a_hubByteStreamAdapter.bufferOutReturn

      a_comp.eventOut -> a_hub.eventIn
      a_comp.telemOut -> a_hub.tlmIn

      a_comp.serOut   -> a_hub.serialIn
      a_comp.opBuff   -> a_hub.bufferIn
      a_hub.bufferInReturn -> a_comp.bufRet

    }

    connections hub {
      # Send path: hub -> ComQueue -> ComStub -> driver (recommended hub wiring, see Svc/GenericHub/docs/sdd.md)
      a_hub.toBufferDriver             -> a_hubComQueue.bufferQueueIn[0]
      a_hubComQueue.bufferReturnOut[0] -> a_hub.toBufferDriverReturn
      a_hubComQueue.dataOut            -> a_hubComStub.dataIn
      a_hubComStub.dataReturnOut       -> a_hubComQueue.dataReturnIn
      a_hubComStub.comStatusOut        -> a_hubComQueue.comStatusIn
      a_hubComStub.drvSendOut          -> a_hubComDriver.$send
      a_hubComDriver.ready             -> a_hubComStub.drvConnected

      # Receive path keeps the byte-stream adapter (see recv_hub)
      a_hubByteStreamAdapter.fromByteStreamDriverReturn -> a_hubComDriver.recvReturnIn

      # Separate pools: the hub sends from a_hubBufferManager, the driver receives into a_hubComDriverBufferManager
      a_hub.allocate   -> a_hubBufferManager.bufferGetCallee
      a_hub.deallocate -> a_hubBufferManager.bufferSendIn
      a_hubComDriver.allocate   -> a_hubComDriverBufferManager.bufferGetCallee
      a_hubComDriver.deallocate -> a_hubComDriverBufferManager.bufferSendIn

      a_cmdSplitter.RemoteCmd -> a_hub.cmdDispIn
      #a_hub.bufferOut[1] -> a_cmdSplitter.seqCmdStatus[1]
    }

    connections DeploymentA {
      # Add here connections to user-defined components
    }

  }

}
}

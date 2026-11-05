module FprimeGenericHubReference {
  module DeploymentB {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

    enum Ports_RateGroups {
      rateGroup1
      rateGroup2
      rateGroup3
    }

  deployment topology DeploymentB {

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
      instance b_rateGroup1
      instance b_rateGroup2
      instance b_rateGroup3
      instance b_cmdSeq
      instance b_comDriver
      instance b_rateGroupDriver
      instance systemResources
      instance linuxTimer
      instance b_cmdSplitter
      instance b_hub
      instance b_hubComDriver
      instance b_hubByteStreamAdapter
      instance b_hubComDriverBufferManager
      instance b_hubComQueue
      instance b_hubComStub
      instance b_bufferManager
      instance b_comp
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
      
      #b_hub.eventOut -> CdhCore.events.LogRecv

      # Router to Command Dispatcher
      #ComCcsds.fprimeRouter.commandOut -> CdhCore.cmdDisp.seqCmdBuff
      #CdhCore.cmdDisp.seqCmdStatus -> ComCcsds.fprimeRouter.cmdResponseIn
    }

    connections RateGroups {
      # LinuxTimer to drive rate group
      linuxTimer.CycleOut -> b_rateGroupDriver.CycleIn

      # Rate group 1
      b_rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1] -> b_rateGroup1.CycleIn
      b_rateGroup1.RateGroupMemberOut[0] -> CdhCore.tlmSend.Run
      b_rateGroup1.RateGroupMemberOut[1] -> FileHandling.fileDownlink.Run
      b_rateGroup1.RateGroupMemberOut[2] -> systemResources.run
      b_rateGroup1.RateGroupMemberOut[3] -> ComCcsds.comQueue.run
      b_rateGroup1.RateGroupMemberOut[4] -> ComCcsds.aggregator.timeout
      b_rateGroup1.RateGroupMemberOut[5] -> b_hubComQueue.run

      # Rate group 2
      b_rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup2] -> b_rateGroup2.CycleIn
      b_rateGroup2.RateGroupMemberOut[0] -> b_cmdSeq.schedIn

      # Rate group 3
      b_rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup3] -> b_rateGroup3.CycleIn
      b_rateGroup3.RateGroupMemberOut[0] -> CdhCore.$health.Run
      b_rateGroup3.RateGroupMemberOut[1] -> ComCcsds.commsBufferManager.schedIn
      b_rateGroup3.RateGroupMemberOut[2] -> DataProducts.dpBufferManager.schedIn
      b_rateGroup3.RateGroupMemberOut[3] -> DataProducts.dpWriter.schedIn
      b_rateGroup3.RateGroupMemberOut[4] -> DataProducts.dpMgr.schedIn
      b_rateGroup3.RateGroupMemberOut[5] -> b_bufferManager.schedIn
      b_rateGroup3.RateGroupMemberOut[6] -> b_hub.schedIn
      b_rateGroup3.RateGroupMemberOut[7] -> b_hubComDriverBufferManager.schedIn
    }


    connections send_hub {
      b_hub.eventOut   -> b_comp.eventIn
      b_hub.tlmOut     -> b_comp.telemIn

      b_hub.serialOut  -> b_comp.serIn
      b_hub.bufferOut  -> b_comp.bufIn
      b_hub.cmdDispOut -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus -> b_hub.cmdRespIn
    }

    connections recv_hub {
      b_hubComDriver.$recv -> b_hubByteStreamAdapter.fromByteStreamDriver
      
      b_hubByteStreamAdapter.bufferOut -> b_hub.fromBufferDriver
      b_hub.fromBufferDriverReturn     -> b_hubByteStreamAdapter.bufferOutReturn

      b_comp.serOut   -> b_hub.serialIn
      b_comp.telemOut -> b_hub.tlmIn
      b_comp.eventOut -> b_hub.eventIn
      b_comp.bufOut   -> b_hub.bufferIn
      b_hub.bufferInReturn -> b_comp.bufRet
    }

    connections hub {
      # Send path: hub -> ComQueue -> ComStub -> driver (recommended hub wiring, see Svc/GenericHub/docs/sdd.md)
      b_hub.toBufferDriver             -> b_hubComQueue.bufferQueueIn[0]
      b_hubComQueue.bufferReturnOut[0] -> b_hub.toBufferDriverReturn
      b_hubComQueue.dataOut            -> b_hubComStub.dataIn
      b_hubComStub.dataReturnOut       -> b_hubComQueue.dataReturnIn
      b_hubComStub.comStatusOut        -> b_hubComQueue.comStatusIn
      b_hubComStub.drvSendOut          -> b_hubComDriver.$send
      b_hubComDriver.ready             -> b_hubComStub.drvConnected

      # Receive path keeps the byte-stream adapter (see recv_hub)
      b_hubByteStreamAdapter.fromByteStreamDriverReturn -> b_hubComDriver.recvReturnIn

      # Separate pools: the hub sends from b_bufferManager, the driver receives into b_hubComDriverBufferManager
      b_hub.allocate   -> b_bufferManager.bufferGetCallee
      b_hub.deallocate -> b_bufferManager.bufferSendIn
      b_hubComDriver.allocate   -> b_hubComDriverBufferManager.bufferGetCallee
      b_hubComDriver.deallocate -> b_hubComDriverBufferManager.bufferSendIn

      #b_hub.bufferInReturn -> b_comp.bufRet
      b_comp.bufInRet      -> b_hub.bufferOutReturn
    }

    connections DeploymentB {
      # Add here connections to user-defined components
    }
  }

}
}

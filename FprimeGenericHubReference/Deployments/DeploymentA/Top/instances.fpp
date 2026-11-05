module FprimeGenericHubReference {
module DeploymentA {

  # ----------------------------------------------------------------------
  # Defaults
  # ----------------------------------------------------------------------

  module Default {
    constant QUEUE_SIZE = 10
    constant STACK_SIZE = 64 * 1024
    @ Hub send pool size (buffers); also the hub ComQueue depth
    constant HUB_BUFFER_COUNT = 30
    @ Hub ComQueue instance queue: the pool's buffers + 8 in-flight status/command/rate-group messages
    constant HUB_QUEUE_SIZE = HUB_BUFFER_COUNT + 8
  }

  # ----------------------------------------------------------------------
  # Active component instances
  # ----------------------------------------------------------------------

  instance a_rateGroup1: Svc.ActiveRateGroup base id 0x0200 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 120

  instance a_rateGroup2: Svc.ActiveRateGroup base id 0x0300 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 119

  instance a_rateGroup3: Svc.ActiveRateGroup base id 0x0400 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 118

  instance a_cmdSeq: Svc.CmdSequencer base id 0x0600 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 100

  instance a_comQueue: Svc.ComQueue base id 0x0700 \
      queue size Default.QUEUE_SIZE \
      stack size Default.STACK_SIZE \
      priority 100 

  instance a_comp: ComponentA base id 0x10017500 \
      queue size Default.QUEUE_SIZE \
      stack size Default.STACK_SIZE \
      priority 100  

  instance c_comp: ComponentC base id 0x11017500 \
      queue size Default.QUEUE_SIZE \
      stack size Default.STACK_SIZE \
      priority 100  


# comment in Svc.TlmChan or Svc.TlmPacketizer
  # depending on which form of telemetry downlink
  # you wish to use

  instance a_tlmSend: Svc.TlmChan base id 0x0C00 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 97


  # ----------------------------------------------------------------------
  # Queued component instances
  # ----------------------------------------------------------------------

  instance $health: Svc.Health base id 0x1000 \
    queue size 25

  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  @ Communications driver. May be swapped with other com drivers like UART or TCP
  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  instance chronoTime: Svc.ChronoTime base id 0x10010000

  instance a_comDriver: Drv.TcpServer base id 0x10011000

  instance a_rateGroupDriver: Svc.RateGroupDriver base id 0x10012000

  instance systemResources: Svc.SystemResources base id 0x10013000

  instance linuxTimer: Svc.LinuxTimer base id 0x10014000

  instance a_hubBufferManager: Svc.BufferManager base id 0x10015400

  instance a_posixTime: Svc.PosixTime base id 0x10015600

  instance a_textLogger: Svc.PassiveTextLogger base id 0x10015800

  instance a_systemResources: Svc.SystemResources base id 0x10015A00

  instance a_cmdSplitter: Svc.CmdSplitter base id 0x10016600

  instance a_hub: Svc.GenericHub base id 0x10017000
  
  instance a_hubComDriver: Drv.Udp base id 0x10017100

  instance a_hubByteStreamAdapter: Drv.ByteStreamBufferAdapter base id 0x10017200

  @ Queues the hub's outgoing buffers off the producers' threads (recommended hub wiring)
  instance a_hubComQueue: Svc.ComQueue base id 0x10017300 \
    queue size Default.HUB_QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 100

  instance a_hubComStub: Svc.ComStub base id 0x10017400

  @ Receive-side pool of the hub driver, kept separate from the hub's own pool
  instance a_hubComDriverBufferManager: Svc.BufferManager base id 0x10015C00

}
}
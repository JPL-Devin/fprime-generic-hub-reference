// ======================================================================
// \title  DeploymentATopology.cpp
// \brief cpp file containing the topology instantiation code
//
// ======================================================================
// Provides access to autocoded functions
//#include <FprimeGenericHubReference/Deployments/DeploymentA/Top/DeploymentATopologyAc.hpp>
#include <FprimeGenericHubReference/Deployments/DeploymentA/Top/DeploymentATopologyAc.hpp>
// Necessary project-specified types
#include <Fw/Types/MallocAllocator.hpp>
#include <Svc/ComQueue/ComQueue.hpp>
#include <Svc/RateGroupDriver/RateGroupDriver.hpp>
#include <Svc/ActiveRateGroup/ActiveRateGroup.hpp>
#include <Svc/Health/Health.hpp>
#include <FprimeGenericHubReference/Deployments/DeploymentA/Top/DeploymentATopologyDefs.hpp>

// Used for 1Hz synthetic cycling
#include <Os/Mutex.hpp>

// Allows easy reference to objects in FPP/autocoder required namespaces
using namespace FprimeGenericHubReference::DeploymentA;
namespace FprimeGenericHubReference {
// The reference topology uses a malloc-based allocator for components that need to allocate memory during the
// initialization phase.
Fw::MallocAllocator mallocator_1;
Fw::MallocAllocator mallocator_2;

Svc::ComQueue::QueueConfigurationTable configurationTable;

// The reference topology divides the incoming clock signal (1Hz) into sub-signals: 1Hz, 1/2Hz, and 1/4Hz with 0 offset
Svc::RateGroupDriver::DividerSet rateGroupDivisorsSet{{{1, 0}, {2, 0}, {4, 0}}};

// Rate groups may supply a context token to each of the attached children whose purpose is set by the project. The
// reference topology sets each token to zero as these contexts are unused in this project.
U32 rateGroup1Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup2Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup3Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};

const char* REMOTE_HUB_IP_ADDRESS = "127.0.0.1";
const U32 REMOTE_HUB_SEND_PORT = 50556;
const U32 REMOTE_HUB_RECV_PORT = 50555;

// A number of constants are needed for construction of the topology. These are specified here.
enum TopologyConstants {
    CMD_SEQ_BUFFER_SIZE = 5 * 1024,
    FILE_DOWNLINK_TIMEOUT = 1000,
    FILE_DOWNLINK_COOLDOWN = 1000,
    FILE_DOWNLINK_CYCLE_TIME = 1000,
    FILE_DOWNLINK_FILE_QUEUE_DEPTH = 10,
    HEALTH_WATCHDOG_CODE = 0x123,
    COMM_PRIORITY = 100,
    // bufferManager constants
    COM_DRIVER_BUFFER_SIZE = 3000,
    COM_DRIVER_BUFFER_COUNT = Default::HUB_BUFFER_COUNT,
    BUFFER_MANAGER_ID = 200,
    HUB_COM_DRIVER_BUFFER_MANAGER_ID = 201,
    // Hub ComQueue depth: at least the hub buffer count, so the hub stays the only drop point
    HUB_QUEUE_DEPTH = COM_DRIVER_BUFFER_COUNT
};

/**
 * \brief configure/setup components in project-specific way
 *
 * This is a *helper* function which configures/sets up each component requiring project specific input. This includes
 * allocating resources, passing-in arguments, etc. This function may be inlined into the topology setup function if
 * desired, but is extracted here for clarity.
 */
void configureTopology() {
    // Command sequencer needs to allocate memory to hold contents of command sequences
    a_cmdSeq.allocateBuffer(0, mallocator_1, CMD_SEQ_BUFFER_SIZE);

    Svc::BufferManager::BufferBins buffMgrBins_1;
    memset(&buffMgrBins_1, 0, sizeof(buffMgrBins_1));
    buffMgrBins_1.bins[0].bufferSize = COM_DRIVER_BUFFER_SIZE;
    buffMgrBins_1.bins[0].numBuffers = COM_DRIVER_BUFFER_COUNT;
    a_hubBufferManager.setup(BUFFER_MANAGER_ID, 0, mallocator_1, buffMgrBins_1);
    a_hubComDriverBufferManager.setup(HUB_COM_DRIVER_BUFFER_MANAGER_ID, 0, mallocator_1, buffMgrBins_1);

    // Hub ComQueue: only the buffer queue (hub frames) is used; the Fw::Com ports stay disabled (depth 0)
    configurationTable.entries[Svc::ComQueue::COM_PORT_COUNT].depth = HUB_QUEUE_DEPTH;
    configurationTable.entries[Svc::ComQueue::COM_PORT_COUNT].priority = 0;
    a_hubComQueue.configure(configurationTable, 0, mallocator_1);

    // Rate group driver needs a divisor list
    a_rateGroupDriver.configure(rateGroupDivisorsSet);

    // Rate groups require context arrays.
    a_rateGroup1.configure(rateGroup1Context, FW_NUM_ARRAY_ELEMENTS(rateGroup1Context));
    a_rateGroup2.configure(rateGroup2Context, FW_NUM_ARRAY_ELEMENTS(rateGroup2Context));
    a_rateGroup3.configure(rateGroup3Context, FW_NUM_ARRAY_ELEMENTS(rateGroup3Context));

}


namespace DeploymentA {


// Public functions for use in main program are namespaced with deployment name DeploymentA
void setupTopology(const TopologyState& state) {
    // Autocoded initialization. Function provided by autocoder.
    initComponents(state);
    // Autocoded id setup. Function provided by autocoder.
    setBaseIds();
    // Autocoded connection wiring. Function provided by autocoder.
    connectComponents();
    // Autocoded command registration. Function provided by autocoder.
    regCommands();
    // Autocoded configuration. Function provided by autocoder.
    configComponents(state);
    // Project-specific component configuration. Function provided above. May be inlined, if desired.
    configureTopology();
    // Autocoded parameter loading. Function provided by autocoder.
    loadParameters();
    // Autocoded task kick-off (active components). Function provided by autocoder.
    startTasks(state);
    // Initialize socket communication if and only if there is a valid specification
    if (state.hostname != nullptr && state.port != 0) {
        Os::TaskString name("ReceiveTask");
        // Uplink is configured for receive so a socket task is started
        a_comDriver.configure(state.hostname, state.port);
        a_comDriver.start(name, COMM_PRIORITY, Default::STACK_SIZE);
    }
    
    //a_hubComDriver.configure("0.0.0.0", 50555);
    a_hubComDriver.configureSend(REMOTE_HUB_IP_ADDRESS, REMOTE_HUB_SEND_PORT);
    a_hubComDriver.configureRecv("0.0.0.0", REMOTE_HUB_RECV_PORT);
    //a_cmdSplitter.configure(0x10000);
    a_cmdSplitter.configure(0x11000000);
    Os::TaskString hubName("hub");
    a_hubComDriver.start(hubName, COMM_PRIORITY, Default::STACK_SIZE);
}

// Variables used for cycle simulation
Os::Mutex cycleLock;
volatile bool cycleFlag = true;

void startRateGroups(const Fw::TimeInterval& interval) {
    // This timer drives the fundamental tick rate of the system.
    // Svc::RateGroupDriver will divide this down to the slower rate groups.
    // This call will block until the stopRateGroups() call is made.
    // For this Linux demo, that call is made from a signal handler.
    linuxTimer.startTimer(interval);
}

void stopRateGroups() {
    linuxTimer.quit();
}

void teardownTopology(const TopologyState& state) {
    // Autocoded (active component) task clean-up. Functions provided by topology autocoder.
    stopTasks(state);
    freeThreads(state);

    // Other task clean-up.
    a_comDriver.stop();
    (void)a_comDriver.join();
    a_hubComDriver.stop();
    (void)a_hubComDriver.join();
    
    // Resource deallocation
    a_cmdSeq.deallocateBuffer(mallocator_1);

    tearDownComponents(state);
    a_hubComQueue.cleanup();
    a_hubBufferManager.cleanup();
    a_hubComDriverBufferManager.cleanup();
}
};  // namespace DeploymentA
}

// ======================================================================
// \title  DeploymentBTopology.cpp
// \brief cpp file containing the topology instantiation code
//
// ======================================================================
// Provides access to autocoded functions
//#include <FprimeGenericHubReference/Deployments/DeploymentB/Top/DeploymentBTopologyAc.hpp>
#include <FprimeGenericHubReference/Deployments/DeploymentB/Top/DeploymentBTopologyAc.hpp>
// Necessary project-specified types
#include <Fw/Types/MallocAllocator.hpp>
#include <Svc/ComQueue/ComQueue.hpp>
#include <Svc/RateGroupDriver/RateGroupDriver.hpp>
#include <Svc/ActiveRateGroup/ActiveRateGroup.hpp>
#include <Svc/Health/Health.hpp>
#include <FprimeGenericHubReference/Deployments/DeploymentB/Top/DeploymentBTopologyDefs.hpp>

// Used for 1Hz synthetic cycling
#include <Os/Mutex.hpp>
#include <cstdio>

// Allows easy reference to objects in FPP/autocoder required namespaces
using namespace FprimeGenericHubReference::DeploymentB;
namespace FprimeGenericHubReference {

// The reference topology uses a malloc-based allocator for components that need to allocate memory during the
// initialization phase.
Fw::MallocAllocator mallocator;

Svc::ComQueue::QueueConfigurationTable configurationTable;

// The reference topology divides the incoming clock signal (1Hz) into sub-signals: 1Hz, 1/2Hz, and 1/4Hz with 0 offset
Svc::RateGroupDriver::DividerSet rateGroupDivisorsSet{{{1, 0}, {2, 0}, {4, 0}}};

// Rate groups may supply a context token to each of the attached children whose purpose is set by the project. The
// reference topology sets each token to zero as these contexts are unused in this project.
U32 rateGroup1Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup2Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup3Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};

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
    COM_DRIVER_BUFFER_COUNT = 30,
    BUFFER_MANAGER_ID = 200,
    HUB_COM_DRIVER_BUFFER_MANAGER_ID = 201,
    // Hub ComQueue depth: at least the hub buffer count, so the hub stays the only drop point
    HUB_QUEUE_DEPTH = COM_DRIVER_BUFFER_COUNT
};

// Hub Constants
const char* REMOTE_HUB_IP_ADDRESS = "127.0.0.1";
const U32 REMOTE_HUB_SEND_PORT = 50555;
const U32 REMOTE_HUB_RECV_PORT = 50556;

/**
 * \brief configure/setup components in project-specific way
 *
 * This is a *helper* function which configures/sets up each component requiring project specific input. This includes
 * allocating resources, passing-in arguments, etc. This function may be inlined into the topology setup function if
 * desired, but is extracted here for clarity.
 */
void configureTopology() {
    // Command sequencer needs to allocate memory to hold contents of command sequences
    b_cmdSeq.allocateBuffer(0, mallocator, CMD_SEQ_BUFFER_SIZE);

    Svc::BufferManager::BufferBins upBuffMgrBins;
    memset(&upBuffMgrBins, 0, sizeof(upBuffMgrBins));
    upBuffMgrBins.bins[0].bufferSize = COM_DRIVER_BUFFER_SIZE;
    upBuffMgrBins.bins[0].numBuffers = COM_DRIVER_BUFFER_COUNT;

    b_bufferManager.setup(BUFFER_MANAGER_ID, 0, mallocator, upBuffMgrBins);
    b_hubComDriverBufferManager.setup(HUB_COM_DRIVER_BUFFER_MANAGER_ID, 0, mallocator, upBuffMgrBins);

    // Rate group driver needs a divisor list
    b_rateGroupDriver.configure(rateGroupDivisorsSet);

    // Rate groups require context arrays.
    b_rateGroup1.configure(rateGroup1Context, FW_NUM_ARRAY_ELEMENTS(rateGroup1Context));
    b_rateGroup2.configure(rateGroup2Context, FW_NUM_ARRAY_ELEMENTS(rateGroup2Context));
    b_rateGroup3.configure(rateGroup3Context, FW_NUM_ARRAY_ELEMENTS(rateGroup3Context));

    // Hub ComQueue: only the buffer queue (hub frames) is used; the Fw::Com ports stay disabled (depth 0)
    configurationTable.entries[Svc::ComQueue::COM_PORT_COUNT].depth = HUB_QUEUE_DEPTH;
    configurationTable.entries[Svc::ComQueue::COM_PORT_COUNT].priority = 0;
    b_hubComQueue.configure(configurationTable, 0, mallocator);
}

namespace DeploymentB {


// Public functions for use in main program are namespaced with deployment name DeploymentB
void setupTopology(const TopologyState& state) {
    // Autocoded initialization. Function provided by autocoder.
    initComponents(state);
    // Autocoded id setup. Function provided by autocoder.
    setBaseIds();
    // Autocoded connection wiring. Function provided by autocoder.
    printf("Calling connectComponents\n");
    connectComponents();
    // Autocoded command registration. Function provided by autocoder.
    regCommands();
    // Autocoded configuration. Function provided by autocoder.
    configComponents(state);

    // Project-specific component configuration. Function provided above. May be inlined, if desired.
    configureTopology();
    // Autocoded parameter loading. Function provided by autocoder.
    // loadParameters();
    // Autocoded task kick-off (active components). Function provided by autocoder.
    startTasks(state);

    // Initialize hub socket communication
    Os::TaskString name("HubTask");

    // Configured for receive so a socket task is started
    //b_hubComDriver.configure(REMOTE_HUB_IP_ADDRESS, REMOTE_HUB_PORT);
    b_hubComDriver.configureSend(REMOTE_HUB_IP_ADDRESS, REMOTE_HUB_SEND_PORT);
    b_hubComDriver.configureRecv("0.0.0.0", REMOTE_HUB_RECV_PORT);
    b_hubComDriver.start(name, COMM_PRIORITY, Default::STACK_SIZE);

    //b_cmdSplitter.configure(0x10000);
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
    b_hubComDriver.stop();
    (void)b_hubComDriver.join();
    
    // Resource deallocation
    b_cmdSeq.deallocateBuffer(mallocator);
    b_hubComQueue.cleanup();
    b_bufferManager.cleanup();
    b_hubComDriverBufferManager.cleanup();
}
};  // namespace DeploymentB
}

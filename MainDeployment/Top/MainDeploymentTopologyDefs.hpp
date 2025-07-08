// ======================================================================
// \title  MainDeploymentTopologyDefs.hpp
// \brief required header file containing the required definitions for the topology autocoder
//
// ======================================================================
#ifndef MAINDEPLOYMENT_MAINDEPLOYMENTTOPOLOGYDEFS_HPP
#define MAINDEPLOYMENT_MAINDEPLOYMENTTOPOLOGYDEFS_HPP

#include "Fw/Types/MallocAllocator.hpp"
#include "MainDeployment/Top/FppConstantsAc.hpp"
#include "Svc/FramingProtocol/FprimeProtocol.hpp"
#include "Svc/Health/Health.hpp"

// Definitions are placed within a namespace named after the deployment
namespace MainDeployment {

/**
 * \brief required type definition to carry state
 *
 * The topology autocoder requires an object that carries state with the name `MainDeployment::TopologyState`. Only the type
 * definition is required by the autocoder and the contents of this object are otherwise opaque to the autocoder. The contents are entirely up
 * to the definition of the project. Here, they are derived from command line inputs.
 */
struct TopologyState {
    const CHAR* uartDevice;
    U32 baudRate;
};

/**
 * \brief required ping constants
 *
 * The topology autocoder requires a WARN and FATAL constant definition for each component that supports the health-ping
 * interface. These are expressed as enum constants placed in a namespace named for the component instance. These
 * are all placed in the PingEntries namespace.
 *
 * Each constant specifies how many missed pings are allowed before a WARNING_HI/FATAL event is triggered. In the
 * following example, the health component will emit a WARNING_HI event if the component instance cmdDisp does not
 * respond for 3 pings and will FATAL if responses are not received after a total of 5 pings.
 *
 * ```c++
 * namespace PingEntries {
 * namespace cmdDisp {
 *     enum { WARN = 3, FATAL = 5 };
 * }
 * }
 * ```
 */
namespace PingEntries {
namespace MainDeployment_blockDrv {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_tlmSend {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_cmdDisp {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_cmdSeq {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_eventLogger {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_fileDownlink {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_fileManager {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_fileUplink {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_prmDb {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_rateGroup1 {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_rateGroup2 {
enum { WARN = 3, FATAL = 5 };
}
namespace MainDeployment_rateGroup3 {
enum { WARN = 3, FATAL = 5 };
}
}  // namespace PingEntries
}  // namespace MainDeployment
#endif

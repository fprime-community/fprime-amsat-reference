// ======================================================================
// \title  Main.cpp
// \brief main program for the F' application.
//
// ======================================================================
// Used to access topology functions
#include <MainDeployment/Top/MainDeploymentTopologyAc.hpp>
#include <MainDeployment/Top/MainDeploymentTopology.hpp>
// Used for Task Runner
#include <fprime-baremetal/Os/TaskRunner/TaskRunner.hpp>

// Used for logging
#include <Fw/Logger/Logger.hpp>
#include <Arduino/Os/Console.hpp>

/**
 * \brief setup the program
 */
void setup()
{
    // Setup Serial
    Serial.begin(115200); //Uart Comm and logging
    static_cast<Os::Arduino::StreamConsoleHandle*>(Os::Console::getSingleton().getHandle())->setStreamHandler(Serial);

    delay(1000);
    Fw::Logger::log("Program Started\n");

    // Object for communicating state to the reference topology
    MainDeployment::TopologyState inputs;
    inputs.uartDevice = "/dev/null";
    inputs.baudRate = 115200;

    // Setup, cycle, and teardown topology
    MainDeployment::setupTopology(inputs);
}

void loop() {
#ifdef USE_BASIC_TIMER
    rateDriver.cycle();
#endif
    Os::Baremetal::TaskRunner::getSingleton().run();
}

#ifndef _H_SST_CINNAMON_MEMORY_UNIT
#define _H_SST_CINNAMON_MEMORY_UNIT

#include <queue>
#include <list>

#include <sst/core/component.h>
#include "sst/core/interfaces/stdMem.h"
#include "chiplet.h"
#include "instruction.h"
#include "utils/utils.h"

namespace SST {
namespace Cinnamon {

class CinnamonMemoryUnit {
    
    CinnamonCPU * cpu;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonMemoryInstruction>> loadQueue;
    std::list<std::shared_ptr<CinnamonMemoryInstruction>> storeQueue;
    std::shared_ptr<CinnamonMemoryInstruction> busyWith;
    Interfaces::StandardMem *memory; // Interface to Memory
    Interfaces::StandardMem::Request::id_t outstandingRequestID;
    SST::Cycle_t issuedAtCycle;
    std::uint16_t cyclesToCompletion;
    bool responseReceived;

    SST::Cycle_t busyCycles;
    SST::Cycle_t totalCycles;

    public:

    // CinnamonMemoryUnit(Interfaces::StandardMem * memory);
    CinnamonMemoryUnit(CinnamonCPU * cpu, const uint32_t outputLevel, Interfaces::StandardMem * memory);
    std::shared_ptr<PhysicalRegister> findLoadAlias(Interfaces::StandardMem::Addr addr);
    std::shared_ptr<PhysicalRegister> findStoreAlias(Interfaces::StandardMem::Addr addr, bool quashAliasingStore);
    void addToLoadQueue(std::shared_ptr<CinnamonMemoryInstruction>);
    void addToStoreQueue(std::shared_ptr<CinnamonMemoryInstruction>);
    void operateQueue(SST::Cycle_t currentCycle, std::list<std::shared_ptr<CinnamonMemoryInstruction>> & queue, const std::string & queueName);
    void executeCycleBegin(SST::Cycle_t currentCycle);
    void executeCycleEnd(SST::Cycle_t currentCycle);
    void init(unsigned int phase);
    void setup();
    void handleResponse(SST::Interfaces::StandardMem::Request *ev);
    void handleVectorLoad(SST::Cycle_t currentCycle, Interfaces::StandardMem::Addr addr, std::size_t size);
    void handleVectorStore(SST::Cycle_t currentCycle, Interfaces::StandardMem::Addr addr, std::size_t size );
    void handleScalarLoad(SST::Cycle_t currentCycle, Interfaces::StandardMem::Addr addr, std::size_t size);
    bool okayToFinish();

};
} // Namespace Cinnamon
} // Namespace SST



#endif
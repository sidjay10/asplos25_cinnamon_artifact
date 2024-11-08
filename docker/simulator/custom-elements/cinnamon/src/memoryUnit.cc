#include "sst/core/interfaces/stdMem.h"
#include "chiplet.h"
#include "CPU.h"
#include "memoryUnit.h"

namespace SST {
namespace Cinnamon {

// CinnamonCPU::CinnamonMemoryUnit::CinnamonMemoryUnit(Interfaces::StandardMem * memory) : memory(memory), busyWith(nullptr), cyclesToCompletion(0) {};
CinnamonMemoryUnit::CinnamonMemoryUnit(CinnamonChiplet * pe, CinnamonCPU * cpu, const uint32_t outputLevel, Interfaces::StandardMem * memory, size_t requestWidth) : pe(pe), cpu(cpu), memory(memory), requestWidth(requestWidth) {
	output = std::make_shared<SST::Output>(SST::Output("CinnamonMemoryUnit[@p:@l]: ", outputLevel, 0, SST::Output::STDOUT));
    // Interfaces::StandardMem * memory = cpu->loadUserSubComponent<Interfaces::StandardMem>("memory", ComponentInfo::SHARE_NONE, time, new Interfaces::StandardMem::Handler<CinnamonMemoryUnit>(this, &CinnamonMemoryUnit::handleResponse) );
    // if ( !memory ) {
    //     output->fatal(CALL_INFO, -1, "Unable to load memoryInterface subcomponent\n");
    // }
};

void CinnamonMemoryUnit::init(unsigned int phase) {
    memory->init(phase);
}

void CinnamonMemoryUnit::setup() {
    memory->setup();
    auto lineSize = memory->getLineSize();
	std::cout << "LineSize: " << lineSize << "\n";
}

void CinnamonMemoryUnit::addToLoadQueue(std::shared_ptr<CinnamonMemoryInstruction> instruction){
    loadQueue.emplace_back(instruction);
}

void CinnamonMemoryUnit::addToStoreQueue(std::shared_ptr<CinnamonMemoryInstruction> instruction){
    storeQueue.emplace_back(instruction);
}

std::shared_ptr<PhysicalRegister> CinnamonMemoryUnit::findStoreAlias(Interfaces::StandardMem::Addr addr, bool quashAliasingStore){
    std::shared_ptr<PhysicalRegister> aliasPhyReg = nullptr;
    using OpCode = CinnamonInstruction::OpCode;
    auto it = storeQueue.rbegin();
    for(; it != storeQueue.rend();){
        std::shared_ptr<CinnamonMemoryInstruction> instruction = *it;
        if(instruction->getAddr() == addr){
            aliasPhyReg = instruction->getPhyReg();
            output->verbose(CALL_INFO, 4, 0, "%s: Found Store Alias for addr %" PRIx64 ": %s.\n",
                        pe->getName().c_str(), addr, instruction->getString().c_str());

            // Subsequent loads to the same address can quash aliasing spills
            // Subsequent stores/spills to the same address can quash aliasing spills. These
            // Instructions must set quashAliasingStore to be true
            if(quashAliasingStore || instruction->getOpCode() == OpCode::Spill ){
                instruction->quash();
                // instruction->setExecutionComplete();
                it = decltype(it)(storeQueue.erase( std::next(it).base()));
                output->verbose(CALL_INFO, 4, 0, "%s: Quashing Store Alias for addr %" PRIx64 ": %s.\n",
                            pe->getName().c_str(), addr, instruction->getString().c_str());
            }
            return aliasPhyReg;
        } 
        it++;
    }

    return aliasPhyReg;
}

std::shared_ptr<PhysicalRegister> CinnamonMemoryUnit::findLoadAlias(Interfaces::StandardMem::Addr addr){

    std::shared_ptr<PhysicalRegister> aliasPhyReg = nullptr;
    auto it = loadQueue.rbegin();
    for(; it != loadQueue.rend(); it++){
        std::shared_ptr<CinnamonMemoryInstruction> instruction = *it;
        if(instruction->getAddr() == addr){
            aliasPhyReg = instruction->getPhyReg();
            output->verbose(CALL_INFO, 4, 0, "%s: Found Load Alias for addr %" PRIx64 ": %s.\n",
                        pe->getName().c_str(), addr, instruction->getString().c_str());
            return aliasPhyReg;
        }
    }
    return aliasPhyReg;
}

bool CinnamonMemoryUnit::operateQueue(SST::Cycle_t currentCycle, std::list<std::shared_ptr<CinnamonMemoryInstruction>> & queue, const std::string & queueName){
    for(size_t i = 0; i < NumConcurrentRequests; i++){
    if(memRequest[i].busyWith == nullptr ){
        if(queue.empty()){
            return false; // nothing to do
        }
        auto it = queue.begin();
        for(; it != queue.end();){
            std::shared_ptr<CinnamonMemoryInstruction> instruction = *it;
            if(instruction->isQuashed() == true){
                assert(0);
                // instruction->setExecutionComplete();
                // instruction->quash();
                // it = storeQueue.erase(it);
            }
            if(instruction->allOperandsReady() == true){

                output->verbose(CALL_INFO, 4, 0, "%s: %lu Issuing Instruction: %s\n", pe->getName().c_str(), currentCycle, instruction->getString().c_str() );
                using OpCode = CinnamonInstruction::OpCode;
                OpCode op = instruction->getOpCode();
                if(op == OpCode::LoadV){
                    handleVectorLoad(currentCycle,i,instruction->getAddr(),instruction->getSize());
                } else if(op == OpCode::LoadS){
                    handleScalarLoad(currentCycle,instruction->getAddr(),instruction->getSize());
                } else if(op == OpCode::Store){
                    handleVectorStore(currentCycle,i,instruction->getAddr(),instruction->getSize());
                } else if(op == OpCode::Spill){
                    handleVectorStore(currentCycle,i,instruction->getAddr(),instruction->getSize());
                } else {
                    assert(0);
                }

                memRequest[i].issuedAtCycle = currentCycle;
                memRequest[i].busyWith = instruction;
                memRequest[i].responseReceived = false;
                it = queue.erase(it);
                break;
            } else {
                it++;
                output->verbose(CALL_INFO, 4, 0, "%s: %lu: %s Waiting for values to be ready. Skipping Ahead: %s\n",
                            pe->getName().c_str(), currentCycle, queueName.c_str(), instruction->getString().c_str());
            }
        }
        if(it == queue.end()){
                    output->verbose(CALL_INFO, 4, 0, "%s: %lu %s No Instrutctions Ready to be Issued:\n",
                                pe->getName().c_str(), currentCycle, queueName.c_str());
                    return false;
        }
    }
    }
    return true;

}

void CinnamonMemoryUnit::executeCycleBegin(SST::Cycle_t currentCycle) {
    bool busy = false;
    operateQueue(currentCycle,loadQueue,"loadQueue");
    operateQueue(currentCycle,storeQueue,"storeQueue");
    stats_.totalCycles++;
    for(size_t i = 0; i < NumConcurrentRequests; i++){
        if(memRequest[i].busyWith != nullptr){
            busy = true;
        }
    }
    if(busy) {
        stats_.busyCycles++;
        stats_.busyCyclesWindow++;
    }

    if(currentCycle % 100000 == 0){
        output->verbose(CALL_INFO, 1, 0, "%s:Heartbeat @ %" PRId64 "00K cycles. Memory Util Cycles: %" PRIu64 "\n",pe->getName().c_str(),currentCycle/(100000),stats_.busyCyclesWindow);
        output->flush();
        stats_.busyCyclesWindow = 0;
    }
}

void CinnamonMemoryUnit::executeCycleEnd(SST::Cycle_t currentCycle) {

    for(size_t i = 0; i < NumConcurrentRequests; i++){
        if(memRequest[i].responseReceived){
            memRequest[i].busyWith->setExecutionComplete();
            output->verbose(CALL_INFO, 3, 0, "%s: [Time: %" PRIu64 "] Completed Instruction: %s\n",
                        pe->getName().c_str(), currentCycle, memRequest[i].busyWith->getString().c_str());
            memRequest[i].busyWith = nullptr;
            memRequest[i].responseReceived = false;
        }
    }
}

void CinnamonMemoryUnit::handleResponse(SST::Interfaces::StandardMem::Request *response_ptr){
	std::unique_ptr<Interfaces::StandardMem::Request> response(response_ptr);
    // std::map<uint64_t, SimTime_t>::iterator i = requests.find(response->getID());
    auto it = outstandingRequestID.find(response->getID());
    MemRequest * memReq = nullptr;
    if ( it == outstandingRequestID.end() ) {
        output->fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", response->getID());
    } else {
        memReq = it->second;
        memReq->bytesProcessed += requestWidth;
        outstandingRequestID.erase(it);
        SimTime_t et = cpu->getCurrentSimTime() - memReq->issuedAtCycle;
        output->verbose(CALL_INFO, 5, 0, "%s: Received Response: %lu [Time: %" PRIu64 "] [%zu outstanding requests]\n",
                    pe->getName().c_str(), response->getID(), et, loadQueue.size() + storeQueue.size());
    }
    if(memReq->bytesProcessed >= memReq->requestSize){
        memReq->responseReceived = true;
        SimTime_t et = cpu->getCurrentSimTime() - memReq->issuedAtCycle;
        stats_.totalLatency += et;
        if(et > stats_.maxLatency) {
            stats_.maxLatency = et;
        }
        output->verbose(CALL_INFO, 3, 0, "%s: Received Response: [Time: %" PRIu64 "] [%zu outstanding requests]\n",
                    pe->getName().c_str(), et, loadQueue.size() + storeQueue.size());
    }
}

void CinnamonMemoryUnit::handleVectorLoad(SST::Cycle_t currentCycle, size_t memRequestIndex, Interfaces::StandardMem::Addr addr, std::size_t size ) {

    auto memRequestPtr = &(memRequest[memRequestIndex]);
    for(size_t i = 0; i < size; i+= requestWidth){
        auto request = std::make_unique<Interfaces::StandardMem::Read>(addr + i, requestWidth);
        outstandingRequestID[request->getID()] = memRequestPtr;
        output->verbose(CALL_INFO, 5, 0, "%s: %lu Issued Read for address 0x%" PRIx64 "\n",
                            pe->getName().c_str(), currentCycle, addr + i);
        memory->send(request.release());
    }
    memRequestPtr->requestSize = size;
    memRequestPtr->bytesProcessed = 0;
    stats_.loadsIssued++;

}

void CinnamonMemoryUnit::handleVectorStore(SST::Cycle_t currentCycle, size_t memRequestIndex, Interfaces::StandardMem::Addr addr, std::size_t size ) {

    auto memRequestPtr = &(memRequest[memRequestIndex]);
    for(size_t i = 0; i < size; i+=requestWidth){
        std::vector<uint8_t> data;
        auto request = std::make_unique<Interfaces::StandardMem::Write>(addr + i, requestWidth, data);
        outstandingRequestID[request->getID()] = memRequestPtr;
        output->verbose(CALL_INFO, 5, 0, "%s: %lu Issued Write for address 0x%" PRIx64 "\n",
                            pe->getName().c_str(), currentCycle, addr + i);
        memory->send(request.release());
    }
    memRequestPtr->requestSize = size;
    memRequestPtr->bytesProcessed = 0;
    stats_.storesIssued++;

}

void CinnamonMemoryUnit::handleScalarLoad(SST::Cycle_t currentCycle, Interfaces::StandardMem::Addr addr, std::size_t size ) {
    assert(0);
	// size_t scalarSize = (1024* 28) / 8; // 3.5 KB
	// auto request = std::make_unique<Interfaces::StandardMem::Read>(addr, size);
    // outstandingRequestID.insert(request->getID());
	// output->verbose(CALL_INFO, 4, 0, "%s: %lu Issued Read for address 0x%" PRIx64 "\n",
	// 					pe->getName().c_str(), currentCycle, addr);
	// memory->send(request.release());

}

bool CinnamonMemoryUnit::okayToFinish() {
    if(!loadQueue.empty() || !storeQueue.empty()){
        return false;
    } else {
        for(size_t i = 0; i < NumConcurrentRequests; i++){
            if(memRequest[i].busyWith != nullptr) {
                return false;
            }
        }
    }
    return true;
}

std::string CinnamonMemoryUnit::printStats() {
    std::stringstream s;
    s << "Memory Unit\n";
    s << "\tTotal Cycles: " << stats_.totalCycles << "\n";
    s << "\tBusy Cycles: " << stats_.busyCycles << "\n";
    double utilisation = ((100.0) * stats_.busyCycles)/stats_.totalCycles;
    s << "\tUtilisation %: " << utilisation << "\n";
    s << "\tLoad Executed : " << stats_.loadsIssued << "\n";
    s << "\tStores Executed: " << stats_.storesIssued << "\n";
    s << "\tMax Latency: " << stats_.maxLatency << "\n";
    double avgLatency = double(stats_.totalLatency) / (stats_.loadsIssued + stats_.storesIssued);
    s << "\tAverage Latency: " << avgLatency << "\n";
    return s.str();
}


} // Namespace Cinnamon
} // Namespace SST
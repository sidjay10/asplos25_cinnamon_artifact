#include "network.h"
#include "CPU.h"

namespace SST {
namespace Cinnamon {

CinnamonNetwork::CinnamonNetwork(ComponentId_t id, Params &params, CinnamonCPU * cpu, size_t numChiplets) : cpu(cpu), numChiplets(numChiplets), SubComponent(id) {

    const uint32_t output_level = (uint32_t)params.find<uint32_t>("verbose", 0);
    output = std::make_shared<SST::Output>(SST::Output("CinnamonNetwork[@p:@l]: ", output_level, 0, SST::Output::STDOUT));

    hops = params.find<uint32_t>("hops","2");
    auto linkBW = params.find<UnitAlgebra>("linkBW");
    UnitAlgebra limbSize("224KB"); 
    UnitAlgebra outputClock = linkBW / limbSize;

    std::string port_name_base("chiplet_port_");
    for(size_t chipletID = 0; chipletID < numChiplets; chipletID++){
        std::string port_name = port_name_base + std::to_string(chipletID);
        output->verbose(CALL_INFO, 1, 0, "Configured Link: %s\n", port_name.c_str());
        auto link = configureLink(port_name, new Event::Handler<CinnamonNetwork,int>(this,&CinnamonNetwork::handleInput,chipletID));
        if ( !link) {
            output->fatal(CALL_INFO, -1, "Unable to load chiplet Link for port : %s\n",port_name.c_str());
        }
        chipletLinks.push_back(link);
        auto outputTimingLink = configureSelfLink("output_timing_" + std::to_string(chipletID), outputClock,
                new Event::Handler<CinnamonNetwork,int>(this,&CinnamonNetwork::handleOutput,chipletID));
        // auto outputTimingLink = configureSelfLink("output_timing_" + std::to_string(chipletID),"1GHz",
        //         new Event::Handler<CinnamonNetwork,int>(this,&CinnamonNetwork::handleOutput,chipletID));
        // TimeConverter* outputTC = getTimeConverter(outputClock);
        // outputTimingLink->setDefaultTimeBase(outputTC);
        outputTiming.push_back(outputTimingLink);
    }

    // TimeConverter *time = registerClock(outputClock, new Clock::Handler<CinnamonNetwork>(this, &CinnamonNetwork::bufferTick));
    outputBWBuffer.resize(numChiplets);

}
CinnamonNetwork::~CinnamonNetwork() {}

void CinnamonNetwork::init(unsigned int phase) {}
void CinnamonNetwork::setup() {}
void CinnamonNetwork::finish() {}

bool CinnamonNetwork::tryRegisterSync(size_t ChipletID, uint64_t syncID, uint64_t syncSize, OpType op, bool sendReply /* Does the network need to send you a value */, bool recvValue /* Are you sending a value to the network*/) {
    std::unique_lock lock(mtx);
    if(syncOps.find(syncID) == syncOps.end()) {
        if(syncOps.size() > 1){
            output->verbose(CALL_INFO, 4, 0, "Registered Sync for syncID = %ld. Sync op size: \n", syncID);
        }
        auto syncOp = SyncOperation(syncID, syncSize, op);
        syncOp.incrementReadyCount(ChipletID);
        output->verbose(CALL_INFO, 4, 0, "Registered Sync for syncID = %ld\n", syncID);
        if(recvValue) {
            syncOp.incrementInputsPending();
        }
        if(sendReply) {
            syncOp.incrementOutputsPending();
            if(op == OpType::Agg) {
                syncOp.setAggregationDestination(ChipletID);
                // assert(aggregateDestination == -1);
                // aggregateDestination = ChipletID;
            } else if(op == OpType::Brc){
                syncOp.addBroadcastDestination(ChipletID);
            }
        }
        syncOps[syncID] = std::move(syncOp);
        return true;
    } else {
        auto & syncOp = syncOps.at(syncID);
        if (op != syncOp.operation()) {
            throw std::invalid_argument("Registered operation does not match expected operation");
        }
        if (syncSize != syncOp.syncSize()) {
            throw std::invalid_argument("Registered syncSize does not match expected syncSize");
        }
        syncOp.incrementReadyCount(ChipletID);
        if(recvValue) {
            syncOp.incrementInputsPending();
        }
        if(sendReply) {
            syncOp.incrementOutputsPending();
            if(op == OpType::Agg) {
                syncOp.setAggregationDestination(ChipletID);
            } else if(op == OpType::Brc){
                syncOp.addBroadcastDestination(ChipletID);
            }
        }
        output->verbose(CALL_INFO, 4, 0, "Increment readyCount to %ld for syncID = %ld\n", syncOp.readyCount(),syncID);
        return true;
    }
    return false;
}

bool CinnamonNetwork::networkReady(uint64_t syncID) const {
    std::shared_lock lock(mtx);
    if (syncOps.find(syncID) == syncOps.end()) {
        return false;
    } 
    const auto & syncOp = syncOps.at(syncID);
    assert(syncOp.inputsPending() >= 0);
    assert(syncOp.outputsPending() >= 0);
    bool ready = syncOp.ready();
    auto operation = syncOp.operation();
    if(ready){
        if(operation == OpType::Brc) {
            assert(syncOp.inputsPending() == 1);
        } else if(operation == OpType::Agg) {
            assert(syncOp.outputsPending() == 1);
            assert(syncOp.aggregationDestination() != -1);
        }
        // syncOp.computeRoute();
    }
    return ready;
}

void CinnamonNetwork::handleInput(SST::Event * ev, int portID){
    std::unique_ptr<CinnamonNetworkEvent> networkEvent(static_cast<CinnamonNetworkEvent *>(ev));
    auto syncID = networkEvent->syncID();
    if(syncOps.find(syncID) == syncOps.end()){
        output->fatal(CALL_INFO, -1, "%s: %lu Received Spurious Incoming With mismatching syncID: %lu\n",getName().c_str(),cpu->getCurrentSimTime(), networkEvent->syncID());
        return;
    }

    auto & syncOp = syncOps.at(syncID);
    // if(!syncID.has_value()){
    //     output->fatal(CALL_INFO, -1, "%s: %lu Received Spurious Incoming\n",getName().c_str(),cpu->getCurrentSimTime());
    // }
    // if(networkEvent->syncID() != syncID.value()){
    //     output->fatal(CALL_INFO, -1, "%s: %lu Received Spurious Incoming With mismatching syncID. Expected: %lu, Got: %lu\n",getName().c_str(),cpu->getCurrentSimTime(), networkEvent->syncID(),syncID.value());
    // }
    syncOp.decrementInputsPending();
    assert(syncOp.inputsPending() >= 0);
    // inputsPending--;
    // assert(inputsPending >= 0);
    output->verbose(CALL_INFO, 1, 4, "%s: %lu Received Incoming with syncID : %lu\n", getName().c_str(), cpu->getCurrentSimCycle(), networkEvent->syncID());
    if(syncOp.inputsPending() == 0) {
        auto operation = syncOp.operation();
        if(operation == OpType::Brc){
            for(auto &i : syncOp.broadcastDestinations()){
                if(i == portID){
                    continue;
                }
                // auto responseEvent = std::make_unique<CinnamonNetworkEvent>(syncID);
                // TODO: Make buffers here that handle the latency of ops
                auto outputBWBufferEntry = CinnamonNetworkOutputBWEntry(syncID,224*1024);
                outputBWBuffer[i].push_back(outputBWBufferEntry);
                // outputTiming[i]->send(1,responseEvent.release());
            }
        } else if(operation == OpType::Agg) {
                // auto responseEvent = std::make_unique<CinnamonNetworkEvent>(syncID);
                auto aggregationDestination = syncOp.aggregationDestination();
                assert(aggregationDestination != -1);
                // outputTiming[aggregationDestination]->send(1,responseEvent.release());

                auto outputBWBufferEntry = CinnamonNetworkOutputBWEntry(syncID,224*1024);
                outputBWBuffer[aggregationDestination].push_back(outputBWBufferEntry);
        } else {
            throw std::runtime_error("Unimplement Network Operation");
        }
    }
    // completeOperation();
    // output->verbose(CALL_INFO, -1, "%s: %lu Received Spurious Incoming\n",getName().c_str(),cpu->getCurrentSimTime());
}

void CinnamonNetwork::handleOutput(SST::Event * ev, int portID){
    std::unique_ptr<CinnamonNetworkEvent> networkEvent(static_cast<CinnamonNetworkEvent *>(ev));
    auto syncID = networkEvent->syncID();
    if(syncOps.find(syncID) == syncOps.end()){
        output->fatal(CALL_INFO, -1, "%s: %lu Received Spurious Incoming With mismatching syncID: %lu\n",getName().c_str(),cpu->getCurrentSimTime(), networkEvent->syncID());
        return;
    }

    auto & syncOp = syncOps.at(syncID);

    output->verbose(CALL_INFO, 4, 0, "%s: %lu Outputing syncID : %lu to chiplet: %d\n", getName().c_str(), cpu->getCurrentSimCycle(), networkEvent->syncID(),portID);
    auto responseEvent = std::make_unique<CinnamonNetworkEvent>(networkEvent->syncID());

    auto hops_ = syncOp.computeHops();
    /* -1 because we already counted the latency once while receiving*/
    chipletLinks[portID]->send(hops_ - 1 /*Latency */, responseEvent.release());
    syncOp.decrementOutputsPending();
    if(syncOp.inputsPending() == 0 && syncOp.outputsPending() == 0){
        completeOperation(syncID);
    }
    assert(!outputBWBuffer[portID].empty());
    auto & bufferEntry = outputBWBuffer[portID].front();
    assert(bufferEntry.inFlight == true);
    outputBWBuffer[portID].pop_front();

    
}

void CinnamonNetwork::completeOperation(uint64_t syncID) {
    std::unique_lock lock(mtx);
    auto & syncOp = syncOps.at(syncID);
    assert(syncOp.inputsPending() == 0);
    assert(syncOp.outputsPending() == 0);
    output->verbose(CALL_INFO, 3, 0, "Completed Operation for syncID = %ld\n", syncID);
    syncOps.erase(syncID);
}

bool CinnamonNetwork::tick(SST::Cycle_t cycle) {
    for(auto & [k,v]: syncOps){
        if(v.ready()){
            stats_.busyCycles++;
            stats_.busyCyclesWindow++;
            break;
        }
    }

    if(cycle % 100000 == 0){
        output->verbose(CALL_INFO, 1, 0, "%s:Heartbeat @ %" PRIu64 " 00K cycles. Network Util Cycles: %" PRIu64 "\n",getName().c_str(),cycle/(100000),stats_.busyCyclesWindow);
        output->flush();
        stats_.busyCyclesWindow = 0;
    }
    stats_.totalCycles++;
    bufferTick(cycle);
    return true;
}

bool CinnamonNetwork::bufferTick(SST::Cycle_t cycle) {
    for(size_t i = 0; i < numChiplets; i++){
        if(outputBWBuffer[i].empty()){
            continue;
        }
        auto & bufferEntry = outputBWBuffer[i].front();
        if(bufferEntry.inFlight) {
            continue;
        }
        auto packet = std::make_unique<CinnamonNetworkEvent>(bufferEntry.syncID);
        outputTiming[i]->send(1,packet.release());
        bufferEntry.inFlight = true;
    }
    return true;
}



#if 0
bool CinnamonNetwork::tryRegisterSync(size_t ChipletID, uint64_t id, uint64_t syncSize, OpType op, bool sendReply /* Does the network need to send you a value */, bool recvValue /* Are you sending a value to the network*/) {
    std::unique_lock lock(mtx);
    if (!syncID.has_value()) {
        readyCount = 1;
        syncID = id;
        operation = op;
        inputsPending = 0;
        outputsPending = 0;
        output->verbose(CALL_INFO, 4, 0, "Registered Sync for syncID = %ld\n", syncID.value());
        if(recvValue) {
            inputsPending++;
        }
        if(sendReply) {
            outputsPending++;
            if(op == OpType::Agg) {
                assert(aggregateDestination == -1);
                aggregateDestination = ChipletID;
            }
        }
        return true;
    } else if (syncID.value() == id) {
        if (op != operation) {
            throw std::invalid_argument("Registered operation does not match expected operation");
        }
        readyCount++;
        if(recvValue) {
            inputsPending++;
        }
        if(sendReply) {
            outputsPending++;
            if(op == OpType::Agg) {
                assert(aggregateDestination == -1);
                aggregateDestination = ChipletID;
            }
        }
        output->verbose(CALL_INFO, 4, 0, "Increment readyCount to %ld for syncID = %ld\n", readyCount,syncID.value());
        return true;
    }
    return false;
}
#endif

#if 0
bool CinnamonNetwork::networkReady(uint64_t id) const {
    std::shared_lock lock(mtx);
    if (!syncID.has_value()) {
        return false;
    }
    if (syncID.value() != id) {
        return false;
    }
    assert(inputsPending >= 0);
    assert(outputsPending >= 0);
    bool ready = (readyCount == numChiplets);
    if(ready){
        if(operation == OpType::Brc) {
            assert(inputsPending == 1);
        } else if(operation == OpType::Agg) {
            assert(outputsPending == 1);
            assert(aggregateDestination != -1);
        }
    }
    return ready;
}
#endif

#if 0
void CinnamonNetwork::handleInput(SST::Event * ev, int portID){
    std::unique_ptr<CinnamonNetworkEvent> networkEvent(static_cast<CinnamonNetworkEvent *>(ev));
    if(!syncID.has_value()){
        output->fatal(CALL_INFO, -1, "%s: %lu Received Spurious Incoming\n",getName().c_str(),cpu->getCurrentSimTime());
    }
    if(networkEvent->syncID() != syncID.value()){
        output->fatal(CALL_INFO, -1, "%s: %lu Received Spurious Incoming With mismatching syncID. Expected: %lu, Got: %lu\n",getName().c_str(),cpu->getCurrentSimTime(), networkEvent->syncID(),syncID.value());
    }
    inputsPending--;
    assert(inputsPending >= 0);
    output->verbose(CALL_INFO, 4, 2, "%s: %lu Received Incoming with syncID : %lu\n", getName().c_str(), cpu->getCurrentSimCycle(), networkEvent->syncID());
    if(inputsPending == 0) {
        if(operation == OpType::Brc){
            for(size_t i = 0; i < numChiplets; i++){
                if(i == portID){
                    continue;
                }
                auto responseEvent = std::make_unique<CinnamonNetworkEvent>(syncID.value());
                outputTiming[i]->send(1,responseEvent.release());
                // outputsPending++;
                // chipletLinks[i]->send(responseEvent.release());
            }
        } else if(operation == OpType::Agg) {
                auto responseEvent = std::make_unique<CinnamonNetworkEvent>(syncID.value());
                assert(aggregateDestination != -1);
                outputTiming[aggregateDestination]->send(1,responseEvent.release());
        } else {
            throw std::runtime_error("Unimplement Network Operation");
        }
    }
    // completeOperation();
    // output->verbose(CALL_INFO, -1, "%s: %lu Received Spurious Incoming\n",getName().c_str(),cpu->getCurrentSimTime());
}
#endif

#if 0
void CinnamonNetwork::handleOutput(SST::Event * ev, int portID){
    std::unique_ptr<CinnamonNetworkEvent> networkEvent(static_cast<CinnamonNetworkEvent *>(ev));
    if(!syncID.has_value()){
        output->fatal(CALL_INFO, -1, "%s: %lu Received Spurious Incoming\n",getName().c_str(),cpu->getCurrentSimTime());
    }
    if(networkEvent->syncID() != syncID.value()){
        output->fatal(CALL_INFO, -1, "%s: %lu Received Spurious Incoming With mismatching syncID. Expected: %lu, Got: %lu\n",getName().c_str(),cpu->getCurrentSimTime(), networkEvent->syncID(),syncID.value());
    }
    output->verbose(CALL_INFO, 1, 4, "%s: %lu Outputing syncID : %lu to chiplet: %d\n", getName().c_str(), cpu->getCurrentSimCycle(), networkEvent->syncID(),portID);
    auto responseEvent = std::make_unique<CinnamonNetworkEvent>(networkEvent->syncID());
    // chipletLinks[portID]->send(hops /*Latency */, responseEvent.release());

    /* -1 because we already counted the latency once while receiving*/
    chipletLinks[portID]->send(hops - 1 /*Latency */, responseEvent.release());
    outputsPending--;
    if(inputsPending == 0 && outputsPending == 0){
        completeOperation();
    }
    // output->verbose(CALL_INFO, -1, "%s: %lu Received Spurious Incoming\n",getName().c_str(),cpu->getCurrentSimTime());
}
#endif

#if 0
void CinnamonNetwork::completeOperation() {
    std::unique_lock lock(mtx);
    assert(inputsPending == 0);
    assert(outputsPending == 0);
    output->verbose(CALL_INFO, 3, 0, "Completed Operation for syncID = %ld\n", syncID.value());
    syncID.reset();
    readyCount == 0;
    inputsPending = 0;
    outputsPending = 0;
    aggregateDestination = -1;
}
#endif

#if 0
bool CinnamonNetwork::tick(SST::Cycle_t cycle) {
    if(syncID.has_value()) {
        stats_.busyCycles++;
    }
    stats_.totalCycles++;
    return true;
}
#endif

std::string CinnamonNetwork::printStats() const {

    std::stringstream s;
    s << "Network Unit: \n";
    s << "\tTotal Cycles: " << stats_.totalCycles << "\n";
    s << "\tBusy Cycles: " << stats_.busyCycles << "\n";
    double utilisation = ((100.0) * stats_.busyCycles)/stats_.totalCycles;
    s << "\tUtilisation %: " << utilisation << "\n";
    return s.str();
}


} // namespace Cinnamon
} // namespace SST
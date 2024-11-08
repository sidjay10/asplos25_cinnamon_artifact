#include <queue>
#include <cmath>

#include "sst/core/component.h"
#include "sst/core/event.h"
#include "sst/core/link.h"
#include "sst/core/output.h"
#include "sst/core/params.h"
#include "sst/core/sst_types.h"
// #include "sst/core/interfaces/simpleMem.h"
#include "sst/core/interfaces/stdMem.h"

#include "readers/reader.h"
#include "CPU.h"
#include "utils/utils.h"

// using namespace SST;
// using namespace SST::Interfaces;


namespace SST {
namespace Cinnamon {

uint64_t VEC_DEPTH = 64;

CinnamonCPU::CinnamonCPU(ComponentId_t id, Params &params) : Component(id) {

    const uint32_t output_level = (uint32_t)params.find<uint32_t>("verbose", 0);
    output = std::make_shared<SST::Output>(SST::Output("Cinnamon[@p:@l]: ", output_level, 0, SST::Output::STDOUT));

    std::string prosClock = params.find<std::string>("clock", "1GHz");
    // Register the clock
    TimeConverter *time = registerClock(prosClock, new Clock::Handler<CinnamonCPU>(this, &CinnamonCPU::tick));

    numChiplets = params.find<size_t>("num_chiplets", "1");

    VEC_DEPTH = params.find<uint64_t>("vec_depth", "64");

    output->verbose(CALL_INFO, 1, 0, "Configured Cinnamon VecDepth %lu\n",VEC_DEPTH); 
    output->verbose(CALL_INFO, 1, 0, "Configured Cinnamon clock for %s\n", prosClock.c_str());

    // // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
    
    latency_.Mul = 5;
    latency_.Add = 1;
    latency_.Evg = 200;

    latency_.Mod = 6 + VEC_DEPTH*(16-1);
    latency_.Rsv = 9 + VEC_DEPTH*(16-1);

    
    latency_.NTT_butterfly = 6;
    latency_.Rot_one_stage = std::log2l(256);

    latency_.NTT_one_stage = std::log2l(256)*latency_.NTT_butterfly; // 48 cycles
    latency_.Transpose = VEC_DEPTH + std::log2l(VEC_DEPTH);
    latency_.NTT = latency_.NTT_one_stage + latency_.Mul + latency_.Transpose + latency_.NTT_one_stage;
    latency_.Rot = latency_.Rot_one_stage + latency_.Transpose + latency_.Rot_one_stage + latency_.Transpose;

    latency_.Bcu_read = latency_.Mul * std::ceil(std::log2l(13)) + VEC_DEPTH*(2-1);
    latency_.Bcu_write = 1;

    // Interfaces::StandardMem *memory = loadUserSubComponent<Interfaces::StandardMem>("memory", ComponentInfo::SHARE_NONE, time, new Interfaces::StandardMem::Handler<CinnamonChiplet>(chiplet.get(), &CinnamonChiplet::handleResponse));
    // if (!memory) {
    //     output->fatal(CALL_INFO, -1, "Unable to load memoryInterface subcomponent\n");
    // }

    network.reset(loadUserSubComponent<CinnamonNetwork>("network", ComponentInfo::SHARE_NONE,this,numChiplets));
    if (!network) {
        output->fatal(CALL_INFO, -1, "Unable to load Cinnamon Network\n");
    }

    for(size_t chipletID = 0; chipletID < numChiplets; chipletID++){
        std::unique_ptr<CinnamonChiplet> chiplet(loadUserSubComponent<CinnamonChiplet>("chiplet_" +std::to_string(chipletID), ComponentInfo::SHARE_NONE, this,network.get(),chipletID));
        if (!chiplet) {
            output->fatal(CALL_INFO, -1, "Unable to load chiplet_%ld\n",chipletID);
        }
        chiplets.push_back(std::move(chiplet));
    }

    output->verbose(CALL_INFO, 1, 0, "Cinnamon configuration completed successfully.\n");
}

void CinnamonCPU::init(unsigned int phase) {
    for(auto & chiplet: chiplets){
        chiplet->init(phase);
    }
}

void CinnamonCPU::setup() {
    for(auto & chiplet: chiplets){
        chiplet->setup();
    }
}

void CinnamonCPU::finish() {
    for(auto & chiplet: chiplets){
        chiplet->finish();
    }
	output->output("------------------------------------------------------------------------\n");
	output->output("%s",network->printStats().c_str());
	output->output("------------------------------------------------------------------------\n");
	output->output("Finished \n");
}

bool CinnamonCPU::tick(SST::Cycle_t cycle) {
    bool retval = true;
    for(auto &chiplet: chiplets){
        retval &= chiplet->tick(cycle);
    }
    network->tick(cycle);
    if(retval){
        primaryComponentOKToEndSim();
        return true;
    }
    return false;
}

} // Namespace Cinnamon
} // Namespace SST

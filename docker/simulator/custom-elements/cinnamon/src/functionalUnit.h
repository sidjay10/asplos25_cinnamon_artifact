#ifndef _H_SST_CINNAMON_FUNCTIONALUNIT
#define _H_SST_CINNAMON_FUNCTIONALUNIT

#include <queue>
#include <list>

#include <sst/core/component.h>
#include "sst/core/interfaces/stdMem.h"
#include "instruction.h"
#include "utils/utils.h"

#include "network.h"
#include "latency.h"


namespace SST {
namespace Cinnamon {

using CinnamonInstructionInterval = Utils::Interval<std::shared_ptr<CinnamonInstruction>>;
using CinnamonFuDisjointIntervalSet = Utils::DisjointIntervalSet<std::shared_ptr<CinnamonInstruction>>;

class CinnamonFunctionalUnit {

    CinnamonChiplet * pe;
    std::shared_ptr<SST::Output> output;
    std::string name;
    // std::queue<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    CinnamonFuDisjointIntervalSet reservations;
    std::list<std::pair<std::shared_ptr<CinnamonInstruction>,SST::Cycle_t>> busyWith;
    std::list<std::pair<std::shared_ptr<CinnamonInstruction>,SST::Cycle_t>> inProcess;
    // SST::Cycle_t issuedAtCycle;
    std::uint16_t latency;
    // std::uint16_t unitBusyCycles;
    // std::uint8_t numUnits;
    SST::Cycle_t consumingCycles = 0;
    std::uint16_t vecDepth = 0;

    struct Stats {
        SST::Cycle_t busyCycles = 0;
        SST::Cycle_t busyCyclesWindow = 0;
        SST::Cycle_t issueCycles = 0;
        SST::Cycle_t issueCyclesWindow = 0;
        SST::Cycle_t totalCycles = 0;
    } stats_;

    public:

    // CinnamonMemoryUnit(Interfaces::StandardMem * memory);
    CinnamonFunctionalUnit(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const uint16_t latency, const uint16_t vecDepth);
    // void addToQueue(std::shared_ptr<CinnamonInstruction>);
    void executeCycleBegin(SST::Cycle_t currentCycle);
    void executeCycleEnd(SST::Cycle_t currentCycle);
    bool okayToFinish();
    bool isIntervalReservable(const CinnamonInstructionInterval & );
    void addReservation(const CinnamonInstructionInterval & interval);
    std::string printStats();

};

class CinnamonBaseConversionUnit {

    CinnamonChiplet * pe;
    std::shared_ptr<SST::Output> output;
    BaseConversionRegister::PhysicalID_t phyID;
    std::string name;
    // std::queue<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    std::optional<std::shared_ptr<CinnamonBciInstruction>> busyWith;
    // SST::Cycle_t issuedAtCycle;
    std::uint16_t latency;
    // std::uint16_t unitBusyCycles;
    // std::uint8_t numUnits;

    public:

    CinnamonBaseConversionUnit(CinnamonChiplet * pe, const BaseConversionRegister::PhysicalID_t phyID, const std::string & name, const uint32_t outputLevel, const uint16_t latency);
    void executeCycleBegin(SST::Cycle_t currentCycle);
    void executeCycleEnd(SST::Cycle_t currentCycle);
    bool okayToFinish();
    // void assignInstruction(std::shared_ptr<CinnamonBciInstruction> instruction);
    bool isBusy() const;
    void initInstruction(SST::Cycle_t currentCycle, std::shared_ptr<CinnamonBciInstruction> instruction);

};


class CinnamonInstructionQueue {

    public:
        virtual void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) = 0;
        virtual void tick(SST::Cycle_t currentCycle) = 0;
        virtual bool okayToFinish() = 0;
        virtual ~CinnamonInstructionQueue() = default; 
    protected:
        using FuVector = std::vector<std::shared_ptr<CinnamonFunctionalUnit>>;
        int QUEUE_EMPTY = 0;
};

class CinnamonAddQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    std::vector<std::shared_ptr<CinnamonFunctionalUnit>> addUnits;

    public:

        CinnamonAddQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & mulUnits);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonMulQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    std::vector<std::shared_ptr<CinnamonFunctionalUnit>> mulUnits;

    public:

        CinnamonMulQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & mulUnits);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonEvgQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector evgUnits;

    public:

        CinnamonEvgQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & evgUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};


class CinnamonRotQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector rotUnits;
    FuVector transposeUnits;
    uint32_t halfRotLatency;
    uint32_t transposeLatency;

    public:

        CinnamonRotQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & rotUnits, const FuVector & transposeUnits);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonNttQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector bcReadUnits;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector bcWriteUnits;

    public:
        CinnamonNttQueue() = delete;
        CinnamonNttQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & bcReadUnits, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & bcWriteUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonSuDQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector bcReadUnits;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector addUnits;
    FuVector mulUnits;

    public:

        CinnamonSuDQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & bcReadUnits, const FuVector & addUnits, const FuVector & mulUnits, const FuVector & nttUnits, const FuVector & transposeUnits);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonBciQueue: public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonBciInstruction>> instructionQueue;
    std::vector<std::shared_ptr<CinnamonBaseConversionUnit>> baseConversionUnits;

    public:

        CinnamonBciQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const std::vector<std::shared_ptr<CinnamonBaseConversionUnit>> & baseConversionUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonBcwQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector bcWriteUnits;

    public:

        CinnamonBcwQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & bcWriteUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};
class CinnamonPl1Queue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector bcWriteUnits;

    public:

        CinnamonPl1Queue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & bcWriteUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonPl2Queue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector mulUnits;
    FuVector bcWriteUnits;

    public:

        CinnamonPl2Queue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & mulUnits, const FuVector & bcWriteUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonPl3Queue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector mulUnits;
    FuVector bcWriteUnits;

    public:

        CinnamonPl3Queue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & mulUnits, const FuVector & bcWriteUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonPl4Queue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector bcReadUnits;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector mulUnits;
    FuVector addUnits;

    public:

        CinnamonPl4Queue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & mulUnits, const FuVector & addUnits, const FuVector & bcReadUnits);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonRsvQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector rsvUnits;

    public:

        CinnamonRsvQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & rsvUnits);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonModQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    const Latency & latency;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector modUnits;

    public:

        CinnamonModQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const Latency & latency, const FuVector & modUnits);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonDisQueue : public CinnamonInstructionQueue {
    CinnamonNetwork * network;
    Link * networkLink;
    CinnamonChiplet * pe;
    CinnamonCPU * cpu;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;

    bool syncRegistered;
    std::shared_ptr<CinnamonDisInstruction> busyWith;
        
    void handle_dis(std::shared_ptr<CinnamonDisInstruction> & instruction);
    void handle_joi(std::shared_ptr<CinnamonDisInstruction> & instruction);
    void handle_incoming(SST::Event * ev);

    struct Stats {
        SST::Cycle_t totalCycles = 0;
        SST::Cycle_t busyCycles = 0;
        SST::Cycle_t waitingForNetworkCycles = 0;

    } stats_;

    public:

        CinnamonDisQueue(CinnamonChiplet * pe, CinnamonCPU * cpu, const std::string & name, const uint32_t outputLevel, CinnamonNetwork * network, Link * networkLink);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;


        // TODO: Add destructor 
};




} // Namespace Cinnamon
} // Namespace SST



#endif
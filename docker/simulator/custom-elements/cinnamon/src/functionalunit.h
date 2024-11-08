#ifndef _H_SST_CINNAMON_FUNCTIONALUNIT
#define _H_SST_CINNAMON_FUNCTIONALUNIT

#include <queue>
#include <list>

#include <sst/core/component.h>
#include "sst/core/interfaces/stdMem.h"
#include "chiplet.h"
#include "instruction.h"
#include "utils/utils.h"

namespace SST {
namespace Cinnamon {

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
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    std::vector<std::shared_ptr<CinnamonFunctionalUnit>> addUnits;

    public:

        CinnamonAddQueue(CinnamonChiplet * pe, const std::string & name, std::vector<std::shared_ptr<CinnamonFunctionalUnit>> & addUnits, const uint32_t outputLevel);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonMulQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    std::vector<std::shared_ptr<CinnamonFunctionalUnit>> mulUnits;

    public:

        CinnamonMulQueue(CinnamonChiplet * pe, const std::string & name, std::vector<std::shared_ptr<CinnamonFunctionalUnit>> & mulUnits, const uint32_t outputLevel);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonEvgQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector evgUnits;

    public:

        CinnamonEvgQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const FuVector & evgUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};


class CinnamonRotQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector rotUnits;
    FuVector transposeUnits;
    uint32_t halfRotLatency;
    uint32_t transposeLatency;

    public:

        CinnamonRotQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const FuVector & rotUnits, const FuVector & transposeUnits, const uint32_t rotLatency, const uint32_t transposeLatency);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonNttQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;

    public:

        CinnamonNttQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const FuVector & nttUnits, const FuVector & transposeUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonSuDQueue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector addUnits;
    FuVector mulUnits;

    public:

        CinnamonSuDQueue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const FuVector & addUnits, const FuVector & mulUnits, const FuVector & nttUnits, const FuVector & transposeUnits);
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

class CinnamonPl1Queue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector bcWriteUnits;

    public:

        CinnamonPl1Queue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & bcWriteUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonPl2Queue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector mulUnits;
    FuVector bcWriteUnits;

    public:

        CinnamonPl2Queue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & mulUnits, const FuVector & bcWriteUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonPl3Queue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector mulUnits;
    FuVector bcWriteUnits;

    public:

        CinnamonPl3Queue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & mulUnits, const FuVector & bcWriteUnits );
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonPl4Queue : public CinnamonInstructionQueue {
    CinnamonChiplet * pe;
    std::string name;
    std::shared_ptr<SST::Output> output;
    std::list<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    FuVector nttUnits;
    FuVector transposeUnits;
    FuVector mulUnits;
    FuVector addUnits;

    public:

        CinnamonPl4Queue(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const FuVector & nttUnits, const FuVector & transposeUnits, const FuVector & mulUnits, const FuVector & addUnits);
        void addToInstructionQueue(std::shared_ptr<CinnamonInstruction> instruction) override;
        void tick(SST::Cycle_t currentCycle) override;
        bool okayToFinish() override;

        // TODO: Add destructor 
};

class CinnamonFunctionalUnit {

    CinnamonChiplet * pe;
    std::shared_ptr<SST::Output> output;
    std::string name;
    // std::queue<std::shared_ptr<CinnamonInstruction>> instructionQueue;
    CinnamonFuDisjointIntervalSet reservations;
    std::list<std::pair<std::shared_ptr<CinnamonInstruction>,SST::Cycle_t>> busyWith;
    // SST::Cycle_t issuedAtCycle;
    std::uint16_t latency;
    // std::uint16_t unitBusyCycles;
    // std::uint8_t numUnits;
    SST::Cycle_t busyCycles;
    SST::Cycle_t totalCycles;

    public:

    // CinnamonMemoryUnit(Interfaces::StandardMem * memory);
    CinnamonFunctionalUnit(CinnamonChiplet * pe, const std::string & name, const uint32_t outputLevel, const uint16_t latency);
    // void addToQueue(std::shared_ptr<CinnamonInstruction>);
    void executeCycleBegin(SST::Cycle_t currentCycle);
    void executeCycleEnd(SST::Cycle_t currentCycle);
    bool okayToFinish();
    bool isIntervalReservable(const CinnamonInstructionInterval & );
    void addReservation(const CinnamonInstructionInterval & interval);

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


} // Namespace Cinnamon
} // Namespace SST



#endif
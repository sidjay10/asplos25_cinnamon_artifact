#ifndef CINNAMON_CHIPLET_H
#define CINNAMON_CHIPLET_H

#include <queue>

#include "sst/core/output.h"
#include "sst/core/component.h"
#include "sst/core/params.h"
#include "sst/core/event.h"
#include "sst/core/sst_types.h"
#include "sst/core/link.h"
// #include "sst/core/interfaces/simpleMem.h"
#include "sst/core/interfaces/stdMem.h"
#include <sst/core/subcomponent.h>

#include "readers/textreader.h"
#include "utils/utils.h"

#include "physicalRegister.h"
#include "baseConversionRegister.h"
// #include "instruction.h"
// #include "functionalUnit.h"
// #include "memoryUnit.h"


// using namespace SST;
// using namespace SST::Interfaces;

namespace SST {
namespace Cinnamon {


class CinnamonCPU;
class CinnamonNetwork;
  // class BaseConversionRegister;
  // class PhysicalRegister;

  class CinnamonInstruction;
  class CinnamonMemoryInstruction;
  class CinnamonBinOpInstruction;
  class CinnamonUnOpInstruction;
  class CinnamonEvgInstruction;
  class CinnamonNttInstruction;
  class CinnamonInttInstruction;
  class CinnamonNoOpInstruction;
  class CinnamonSuDInstruction;
  class CinnamonBciInstruction;
  class CinnamonBcReadInstruction;
  class CinnamonBcWriteInstruction;
  class CinnamonPl1Instruction;
  class CinnamonPl2Instruction;
  class CinnamonPl3Instruction;
  class CinnamonPl4Instruction;
  // class CinnamonRegisterFileReadUnit;
  // class CinnamonRegisterFileWriteUnit;

  class CinnamonMemoryUnit;
  class CinnamonFunctionalUnit;
  class CinnamonBaseConversionUnit;

  class CinnamonInstructionQueue;
  class CinnamonAddQueue;
  class CinnamonMulQueue;
  class CinnamonRotQueue;
  class CinnamonEvgQueue;
  class CinnamonNttQueue;
  class CinnamonSuDQueue;
  class CinnamonBciQueue;
  class CinnamonPl1Queue;
  class CinnamonPl2Queue;
  class CinnamonPl3Queue;
  class CinnamonPl4Queue;
  class CinnamonRsvQueue;


class CinnamonChiplet : public SubComponent {
public:
  CinnamonChiplet(ComponentId_t id, Params &params, CinnamonCPU * cpu, CinnamonNetwork * network, uint32_t chipletID);
  ~CinnamonChiplet();

  void init(unsigned int phase);
  void setup();
  void finish();

  auto chipletID() const {
    return chipletID_;
  }

  void addBusyCyclesWindow(SST::Cycle_t val) {
    stats_.busyCyclesWindow += val;
  }

  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Cinnamon::CinnamonChiplet, CinnamonCPU * , CinnamonNetwork * , uint32_t)

  SST_ELI_REGISTER_SUBCOMPONENT(
      CinnamonChiplet,
      "cinnamon",
      "Chiplet",
      SST_ELI_ELEMENT_VERSION(1, 0, 0),
      "Cinnamon Chiplet",
      SST::Cinnamon::CinnamonChiplet);

  SST_ELI_DOCUMENT_PORTS(
      {"memory_link", "Link to the memory hierarchy (e.g., HBM)", {"memHierarchy.memEvent", ""}},
      {"cinnamon_network_port", "Link to the memory hierarchy (e.g., HBM)", {"memHierarchy.memEvent", ""}}
      )

  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
      {"memory", "Interface to the memory hierarchy (e.g., cache)", "SST::Interfaces::StandardMem"},
      {"reader", "Trace Reader to use to", "SST::CinnamonCPU::TraceReader"})


private:
  CinnamonChiplet();                       // Serialization only
  CinnamonChiplet(const CinnamonChiplet &);       // Do not impl.
  void operator=(const CinnamonChiplet &); // Do not impl.

  void handleResponse(SST::Interfaces::StandardMem::Request *ev);
  bool tick(Cycle_t);

  std::uint16_t numVectorRegs = 1024;
  // std::uint16_t numVectorRegs = 1170;
  std::uint16_t numScalarRegs = 64;
  std::uint16_t numBcuVRegs = 64;

  std::uint16_t numMulUnits = 5;
  std::uint16_t numAddUnits = 5;
  std::uint16_t numNTTUnits = 2;
  std::uint16_t numRotUnits = 1;
  std::uint16_t numTraUnits = 3;
  std::uint16_t numBcuUnits = 2;
  std::uint16_t numBcuBuffs = 2;
  std::uint16_t numEvgUnits = 1;


  std::vector<std::shared_ptr<PhysicalRegister>> vectorRegisters;
  std::vector<std::shared_ptr<PhysicalRegister>> scalarRegisters;
  std::vector<std::shared_ptr<BaseConversionRegister>> baseConversionVirtualRegisters;

  std::map<std::uint16_t,PhysicalRegisterID_t> vectorRegisterRenameMap;
  std::map<std::uint16_t,PhysicalRegisterID_t> scalarRegisterRenameMap;
  std::map<std::uint16_t,BaseConversionRegister::VirtualID_t> baseConversionVirtualRegisterRenameMap;
  std::map<std::string,SST::Interfaces::StandardMem::Addr> termToAddressMap;
  uint64_t numTerms = 0;

  std::queue<PhysicalRegisterID_t> freeVectorRegisters; 
  std::queue<PhysicalRegisterID_t> freeScalarRegisters; 
  std::queue<BaseConversionRegister::VirtualID_t> freeBaseConversionVirtualRegisters; 

  std::shared_ptr<CinnamonParsedInstruction> fetchedInstruction; 

  bool canMapToPhysicalRegister(const CinnamonParsedValueType & val);
  std::shared_ptr<PhysicalRegister> mapToPhysicalRegister(const CinnamonParsedValueType & val);
  std::shared_ptr<PhysicalRegister> getMappedPhysicalRegister(const CinnamonParsedValueType & val);
  void mapSrcToDest(const CinnamonParsedVectorReg & dest, const CinnamonParsedVectorReg & src );
  std::shared_ptr<BaseConversionRegister> mapToBaseConversionVirtualRegister(const CinnamonParsedBcuInitReg & val);
  std::shared_ptr<BaseConversionRegister> getMappedBaseConversionVirtualRegister(const CinnamonParsedBcuReg & val);
  bool dispatchMemoryInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> &  instruction);
  bool dispatchEvgInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchBinOpInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchUnOpInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchNttInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchSuDInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchBciInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchBcwInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchPl1Instruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchPl2Instruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchPl3Instruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchPl4Instruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchMovInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchRsvInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchModInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchDisInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);
  bool dispatchJoiInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction);



  std::unique_ptr<CinnamonMemoryUnit> memoryUnit;
  std::vector<std::shared_ptr<CinnamonFunctionalUnit>> functionalUnits;
  std::vector<std::shared_ptr<CinnamonBaseConversionUnit>> baseConversionUnits;

  std::unique_ptr<CinnamonInstructionQueue> addQueue;
  std::unique_ptr<CinnamonInstructionQueue> mulQueue;
  std::unique_ptr<CinnamonInstructionQueue> rotQueue;
  std::unique_ptr<CinnamonInstructionQueue> evgQueue;
  std::unique_ptr<CinnamonInstructionQueue> nttQueue;
  std::unique_ptr<CinnamonInstructionQueue> sudQueue;
  std::unique_ptr<CinnamonInstructionQueue> bciQueue;
  std::unique_ptr<CinnamonInstructionQueue> bcwQueue;
  std::unique_ptr<CinnamonInstructionQueue> pl1Queue;
  std::unique_ptr<CinnamonInstructionQueue> pl2Queue;
  std::unique_ptr<CinnamonInstructionQueue> pl3Queue;
  std::unique_ptr<CinnamonInstructionQueue> pl4Queue;
  std::unique_ptr<CinnamonInstructionQueue> rsvQueue;
  std::unique_ptr<CinnamonInstructionQueue> modQueue;
  std::unique_ptr<CinnamonInstructionQueue> disQueue;
  // std::unique_ptr<CinnamonInstructionQueue> joiQueue;
  
  void dummyHandler(SST::Event * ev) { };


  std::shared_ptr<SST::Output> output;
  std::unique_ptr<SST::Cinnamon::CinnamonTraceReader> reader;

  uint64_t numInstructions;

  CinnamonCPU * cpu;
  uint32_t chipletID_;
  CinnamonNetwork * network;
  SST::Link * networkLink;
  friend class CinnamonCPU;
  friend class PhysicalRegister;
  friend class BaseConversionRegister;

  struct Stats {
      SST::Cycle_t busyCyclesWindow = 0;
      uint64_t vectorRegisterReads = 0;
      uint64_t vectorRegisterWrites = 0;
  } stats_;

  struct Config {
    bool usePRNG = true;
  } config;

};


} // Namespace Cinnamon
} // Namespace SST

#endif /* CINNAMON_CHIPLET_H */

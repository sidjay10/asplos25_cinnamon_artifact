#include "sst/core/sst_config.h"
#include <sst/core/unitAlgebra.h>

#include "chiplet.h"
#include "memoryUnit.h"
#include "functionalUnit.h"
#include "CPU.h"
#include <algorithm>


namespace SST {
namespace Cinnamon {
/**
 * @brief Cinnamon Constructor
 * 
 * Read and initialize simulator parameters (e.g. clock speed) from setting file (trace-*.py)
 */
CinnamonChiplet::CinnamonChiplet(ComponentId_t id, Params &params, CinnamonCPU * cpu, CinnamonNetwork * network, uint32_t chipletID): cpu(cpu), network(network) , chipletID_(chipletID), numInstructions(0), SubComponent(id) {
// CinnamonChiplet::CinnamonChiplet(uint32_t chipletID, const uint32_t output_level, Interfaces::StandardMem * memory){
	
    const uint32_t output_level = (uint32_t)params.find<uint32_t>("verbose", 0);
	output = std::make_shared<SST::Output>(SST::Output("Cinnamon[@p:@l]: ", output_level, 0, SST::Output::STDOUT));

    numVectorRegs = params.find<uint16_t>("numVectorRegs", 1024);

    numAddUnits = params.find<uint16_t>("numAddUnits", 5);
    numMulUnits = params.find<uint16_t>("numMulUnits", 5);
    numNTTUnits = params.find<uint16_t>("numNttUnits", 2);
    numRotUnits = params.find<uint16_t>("numRotUnits", 1);
    numTraUnits = params.find<uint16_t>("numTraUnits", 2);
    numBcuUnits = params.find<uint16_t>("numBcuUnits", 2);
    numBcuBuffs = params.find<uint16_t>("numBcuBuffs", 2);
    numEvgUnits = params.find<uint16_t>("numEvgUnits", 1);
    config.usePRNG = params.find<bool>("usePRNG", true);

	output->verbose(CALL_INFO, 1, 0, "Use PRNG: %s\n", config.usePRNG ? "true" : "false");


	// Load Reader the new way
	// reader = std::make_unique<CinnamonTextTraceReader>(output,"instructions");
	reader.reset(loadUserSubComponent<CinnamonTraceReader>("reader",
	 ComponentInfo::SHARE_NONE, 
	 output));

	if (reader == nullptr) {
	    output->fatal(CALL_INFO, -1, "%s, Fatal: Failed to load reader 2 module\n", getName().c_str());
	}

    Event::Handler<CinnamonChiplet>* dummy_handler = new Event::Handler<CinnamonChiplet>(this,&CinnamonChiplet::dummyHandler);
    std::string port_name("cinnamon_network_port");
    networkLink = configureLink(port_name, 0, dummy_handler/* We will set this later */);
    if ( !networkLink) {
        output->fatal(CALL_INFO, -1, "Unable to load networkLink\n");
    }

	// std::string prosClock = params.find<std::string>("clock", "1GHz");
	// // Register the clock
	// TimeConverter* time = registerClock(prosClock, new Clock::Handler<CinnamonChiplet>(this, &CinnamonChiplet::tick));

	// output->verbose(CALL_INFO, 1, 0, "Configured Cinnamon clock for %s\n", prosClock.c_str());

	// // tell the simulator not to end without us
  	// registerAsPrimaryComponent();
  	// primaryComponentDoNotEndSim();

	auto & latency = cpu->latency();

    Interfaces::StandardMem * memory = loadUserSubComponent<Interfaces::StandardMem>("memory", ComponentInfo::SHARE_NONE, nullptr, new Interfaces::StandardMem::Handler<CinnamonChiplet>(this, &CinnamonChiplet::handleResponse));
    if ( !memory ) {
        output->fatal(CALL_INFO, -1, "Unable to load memoryInterface subcomponent\n");
    }

	auto requestWidth = params.find<size_t>("memoryRequestWidth", 1024);
	memoryUnit = std::make_unique<CinnamonMemoryUnit>(this,cpu,output_level,memory,requestWidth);
	// functionalUnit = std::make_unique<CinnamonFunctionalUnit>(this,output_level,2);
	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> addUnits;
	for(int i = 0; i < numAddUnits; i ++){
		auto fu = std::make_shared<CinnamonFunctionalUnit>(this, "addFU" + std::to_string(i),output_level,latency.Add,VEC_DEPTH);
		addUnits.push_back(fu);
		functionalUnits.push_back(fu);
	}
	addQueue = std::make_unique<CinnamonAddQueue>(this,"addQueue",output_level,latency,addUnits);

	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> mulUnits;
	for(int i = 0; i < numMulUnits; i ++){
		auto fu = std::make_shared<CinnamonFunctionalUnit>(this, "mulFU" + std::to_string(i),output_level,latency.Mul,VEC_DEPTH);
		mulUnits.push_back(fu);
		functionalUnits.push_back(fu);
	}
	mulQueue = std::make_unique<CinnamonMulQueue>(this,"mulQueue",output_level,latency,mulUnits);

	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> bcWriteUnits;
	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> bcReadUnits;

	for(int i = 0; i < numBcuBuffs; i ++){
		auto bcu = std::make_shared<CinnamonBaseConversionUnit>(this,i, "bcu" + std::to_string(i),output_level,latency.Bcu_read);
		baseConversionUnits.push_back(bcu);
		auto bcw = std::make_shared<CinnamonFunctionalUnit>(this, "bcWrite" + std::to_string(i),output_level,latency.Bcu_write,VEC_DEPTH);
		functionalUnits.push_back(bcw);
		bcWriteUnits.push_back(bcw);
	}

	for(int i = 0; i < numBcuUnits; i ++){
		auto bcr = std::make_shared<CinnamonFunctionalUnit>(this, "bcRead" + std::to_string(i),output_level,latency.Bcu_read,VEC_DEPTH*2);
		functionalUnits.push_back(bcr);
		bcReadUnits.push_back(bcr);
	}

	bciQueue = std::make_unique<CinnamonBciQueue>(this,"bciQueue",output_level,baseConversionUnits);

	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> nttUnits;
	for(int i = 0; i < numNTTUnits; i ++){
		auto fu = std::make_shared<CinnamonFunctionalUnit>(this, "nttFU" + std::to_string(i),output_level,latency.NTT,VEC_DEPTH);
		nttUnits.push_back(fu);
		functionalUnits.push_back(fu);
	}
	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> transposeUnits;
	for(int i = 0; i < numTraUnits; i ++){
		auto fu = std::make_shared<CinnamonFunctionalUnit>(this, "traFU" + std::to_string(i),output_level,latency.Transpose,VEC_DEPTH);
		transposeUnits.push_back(fu);
		functionalUnits.push_back(fu);
	}

	nttQueue = std::make_unique<CinnamonNttQueue>(this,"nttQueue",output_level,latency,bcReadUnits,nttUnits,transposeUnits,bcWriteUnits);
	sudQueue = std::make_unique<CinnamonSuDQueue>(this,"sudQueue",output_level,latency,bcReadUnits,addUnits,mulUnits,nttUnits,transposeUnits);

	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> rotateUnits;
	for(int i = 0; i < numRotUnits; i ++){
		auto fu = std::make_shared<CinnamonFunctionalUnit>(this, "rotFU" + std::to_string(i),output_level,latency.Rot,VEC_DEPTH);
		rotateUnits.push_back(fu);
		functionalUnits.push_back(fu);
	}

	rotQueue = std::make_unique<CinnamonRotQueue>(this,"rotQueue",output_level,latency,rotateUnits,transposeUnits);
	
	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> evgUnits;
	for(int i = 0; i < numEvgUnits; i ++){
		auto fu = std::make_shared<CinnamonFunctionalUnit>(this, "evgFU" + std::to_string(i),output_level,latency.Evg,VEC_DEPTH);
		evgUnits.push_back(fu);
		functionalUnits.push_back(fu);
	}
	evgQueue = std::make_unique<CinnamonEvgQueue>(this,"evgQueue",output_level,latency,evgUnits);


	bcwQueue = std::make_unique<CinnamonBcwQueue>(this,"bcwQueue",output_level,latency,bcWriteUnits);
	pl1Queue = std::make_unique<CinnamonPl1Queue>(this,"pl1Queue",output_level,latency,nttUnits,transposeUnits,bcWriteUnits);
	// pl2Queue = std::make_unique<CinnamonPl2Queue>(this,"pl2Queue",output_level,latency,nttUnits,transposeUnits,mulUnits,bcWriteUnits);
	// pl3Queue = std::make_unique<CinnamonPl3Queue>(this,"pl3Queue",output_level,latency,nttUnits,transposeUnits,mulUnits,bcWriteUnits);
	// pl4Queue = std::make_unique<CinnamonPl4Queue>(this,"pl4Queue",output_level,latency,nttUnits,transposeUnits,mulUnits,addUnits,bcReadUnits);

	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> rsvUnits;
	for(int i = 0; i < 1; i ++){
		auto rsv = std::make_shared<CinnamonFunctionalUnit>(this, "rsv" + std::to_string(i),output_level,latency.Rsv,VEC_DEPTH*16);
		functionalUnits.push_back(rsv);
		rsvUnits.push_back(rsv);
	}
	
	rsvQueue = std::make_unique<CinnamonRsvQueue>(this,"rsvQueue",output_level,latency,rsvUnits);

	std::vector<std::shared_ptr<CinnamonFunctionalUnit>> modUnits;
	for(int i = 0; i < 1; i ++){
		auto mod = std::make_shared<CinnamonFunctionalUnit>(this, "mod" + std::to_string(i),output_level,latency.Mod,VEC_DEPTH*16);
		functionalUnits.push_back(mod);
		modUnits.push_back(mod);
	}

	modQueue = std::make_unique<CinnamonModQueue>(this,"modQueue",output_level,latency,modUnits);

	disQueue = std::make_unique<CinnamonDisQueue>(this,cpu,"disQueue",output_level,network,networkLink);

	for(int i = 0; i < numVectorRegs; i++){
		auto vreg = std::make_shared<PhysicalRegister>(this,PhysicalRegister::PhysicalRegister_t::Vector,i);
		vectorRegisters.push_back(vreg);
		freeVectorRegisters.push(i);
	}

	for(int i = 0; i < numScalarRegs; i++){
		auto sreg = std::make_shared<PhysicalRegister>(this,PhysicalRegister::PhysicalRegister_t::Scalar,i);
		scalarRegisters.push_back(sreg);
		freeScalarRegisters.push(i);
	}

	for(int i = 0; i < numBcuVRegs; i++){
		auto breg = std::make_shared<BaseConversionRegister>(this,i);
		baseConversionVirtualRegisters.push_back(breg);
		freeBaseConversionVirtualRegisters.push(i);
	}



	output->verbose(CALL_INFO, 1, 0, "Cinnamon configuration completed successfully.\n");
}


void CinnamonChiplet::handleResponse(Interfaces::StandardMem::Request * response_ptr) {
	memoryUnit->handleResponse(response_ptr);
}


CinnamonChiplet::~CinnamonChiplet() {}

/**
 * @brief Cinnamon Initializer 
 * 
 * Initialize other subcomponents 
 */
void CinnamonChiplet::init(unsigned int phase) {
    memoryUnit->init(phase);
}

void CinnamonChiplet::setup() {
    memoryUnit->setup();
}

void CinnamonChiplet::finish() {
	const uint64_t nanoSeconds = cpu->getCurrentSimTimeNano();

	output->output("\n");
	output->output("------------------------------------------------------------------------\n");
	output->output("Chiplet: %" PRIu32 " Statistics:\n",chipletID_);

	output->output("------------------------------------------------------------------------\n");
	output->output("- Completed at:                          %" PRIu64 " ns\n", nanoSeconds);
	output->output("------------------------------------------------------------------------\n");
	output->output("%s",memoryUnit->printStats().c_str());
	output->output("------------------------------------------------------------------------\n");
	for(auto &fu: functionalUnits) {
		output->output("%s",fu->printStats().c_str());
		output->output("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
	}
	std::stringstream s;
    s << "Register File:\n";
	s << "\tVector Register Reads : " << stats_.vectorRegisterReads << "\n";
	s << "\tVector Register Writes: " << stats_.vectorRegisterWrites << "\n";
	output->output("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
	output->output("%s",s.str().c_str());
	output->output("------------------------------------------------------------------------\n");
}

bool CinnamonChiplet::canMapToPhysicalRegister(const CinnamonParsedValueType & val){

	bool mappable = false;
	std::visit(overloaded{[](auto &arg )
						  { },
						  [&](const CinnamonParsedVectorReg &arg)
						  { 
							auto it = vectorRegisterRenameMap.find(arg.id);
							if(freeVectorRegisters.empty() == false){
								mappable = true;
								return;
							}
							// if(it != vectorRegisterRenameMap.end()){
							// 	if(vectorRegisters.at(it->second)->numReferences() == 1){
							// 		mappable = true;
							// 	}
							// }
						  },
						  [&](const CinnamonParsedScalarReg &arg)
						  { 
							auto it = scalarRegisterRenameMap.find(arg.id);
							if(freeScalarRegisters.empty() == false){
								mappable = true;
								return;
							}
							// if(it != scalarRegisterRenameMap.end()){
							// 	if(scalarRegisters.at(it->second)->numReferences() == 1){
							// 		mappable = true;
							// 	}
							// }
							
						}}, val);
	return mappable;
}


std::shared_ptr<PhysicalRegister> CinnamonChiplet::mapToPhysicalRegister(const CinnamonParsedValueType & val){

	std::shared_ptr<PhysicalRegister> mappedRegister = nullptr;
	std::visit(overloaded{[](auto &arg )
						  { },
						  [&](const CinnamonParsedVectorReg &arg)
						  { 
							auto it = vectorRegisterRenameMap.find(arg.id);
							if(it != vectorRegisterRenameMap.end()){
								vectorRegisters.at(it->second)->decReference();
								// vectorRegisters.at(it->second)->unsetMapped();
							}
							assert(!freeVectorRegisters.empty());
							auto freeVRegID = freeVectorRegisters.front(); 
							freeVectorRegisters.pop();
							vectorRegisterRenameMap[arg.id] = freeVRegID;
							mappedRegister = vectorRegisters.at(vectorRegisterRenameMap.at(arg.id)); 
							// mappedRegister->setMapped(arg.id);
							mappedRegister->incReference();
							stats_.vectorRegisterWrites++;
						  },
						  [&](const CinnamonParsedScalarReg &arg)
						  { 
							auto it = scalarRegisterRenameMap.find(arg.id);
							if(it != scalarRegisterRenameMap.end()){
								scalarRegisters.at(it->second)->decReference();
								// vectorRegisters.at(it->second)->unsetMapped();
							}
							assert(!freeScalarRegisters.empty());
							auto freeSRegID = freeScalarRegisters.front(); 
							freeScalarRegisters.pop();
							scalarRegisterRenameMap[arg.id] = freeSRegID;
							mappedRegister = scalarRegisters.at(scalarRegisterRenameMap.at(arg.id)); 
							// mappedRegister->setMapped(arg.id);
							mappedRegister->incReference();
							
						}}, val);
	// mappedRegister->incReference();
	return mappedRegister;

}

std::shared_ptr<PhysicalRegister> CinnamonChiplet::getMappedPhysicalRegister(const CinnamonParsedValueType & val) {
	std::shared_ptr<PhysicalRegister> mappedRegister = nullptr;
	std::visit(overloaded{[](auto &arg )
						  { },
						  [&](const CinnamonParsedVectorReg &arg)
						  { 
							mappedRegister = vectorRegisters.at(vectorRegisterRenameMap.at(arg.id)); 
						 	if(arg.dead){
								// mappedRegister->unsetMapped();
								mappedRegister->decReference();
								vectorRegisterRenameMap.erase(arg.id);
							} 
							stats_.vectorRegisterReads++;
						  },
						  [&](const CinnamonParsedScalarReg &arg)
						  {
							mappedRegister = scalarRegisters.at(scalarRegisterRenameMap.at(arg.id));
							if(arg.dead){
								// mappedRegister->unsetMapped();
								mappedRegister->decReference();
								scalarRegisterRenameMap.erase(arg.id);
							} 
						  }},
			   val);
	return mappedRegister;
}

void CinnamonChiplet::mapSrcToDest(const CinnamonParsedVectorReg & dest, const CinnamonParsedVectorReg & src ) {

	auto it = vectorRegisterRenameMap.find(dest.id);
	if(it != vectorRegisterRenameMap.end()){
		vectorRegisters.at(it->second)->decReference();
		// vectorRegisters.at(it->second)->unsetMapped();
		vectorRegisterRenameMap.erase(dest.id);
	}
	// assert(vectorRegisters.at(it->second)->numReferences() == 0);
	vectorRegisterRenameMap[dest.id] = vectorRegisterRenameMap.at(src.id);
	vectorRegisters.at(vectorRegisterRenameMap.at(dest.id))->incReference();
}

std::shared_ptr<BaseConversionRegister> CinnamonChiplet::mapToBaseConversionVirtualRegister(const CinnamonParsedBcuInitReg & val){

	std::shared_ptr<BaseConversionRegister> mappedRegister = nullptr;
	auto it = baseConversionVirtualRegisterRenameMap.find(val.bcuId);
	if(it != baseConversionVirtualRegisterRenameMap.end()){
		baseConversionVirtualRegisters.at(it->second)->decReference();
	}
	assert(!freeBaseConversionVirtualRegisters.empty());
	auto freeBcuVirtRegID = freeBaseConversionVirtualRegisters.front(); 
	freeBaseConversionVirtualRegisters.pop();
	baseConversionVirtualRegisterRenameMap[val.bcuId] = freeBcuVirtRegID;
	mappedRegister = baseConversionVirtualRegisters.at(baseConversionVirtualRegisterRenameMap.at(val.bcuId));
	mappedRegister->incReference();
	return mappedRegister;

}

std::shared_ptr<BaseConversionRegister> CinnamonChiplet::getMappedBaseConversionVirtualRegister(const CinnamonParsedBcuReg & val) {
	std::shared_ptr<BaseConversionRegister> mappedRegister = baseConversionVirtualRegisters[baseConversionVirtualRegisterRenameMap[val.bcuId]];
	return mappedRegister;
}


bool CinnamonChiplet::dispatchMemoryInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	std::size_t limbSize = (64 * 1024 * 28) / 8; //224 KB
	std::size_t scalarSize = (2048 * 28) / 8; //7KB
	auto & dests = instruction->dests;
	std::shared_ptr<PhysicalRegister> destReg = nullptr;
	assert(dests.size() == 1);
	auto &op = instruction->opCode;
	std::size_t size = 0;

	SST::Interfaces::StandardMem::Addr addr = 0;
	auto & srcs = instruction->srcs;
	assert(srcs.size() == 1);
	auto term = std::get<CinnamonParsedTerm>(srcs[0]);
	auto it = termToAddressMap.find(term.term);
	if(it == termToAddressMap.end()){
		addr = numTerms;
		addr *= limbSize;
		termToAddressMap[term.term] = addr;
		output->verbose(CALL_INFO, 3, 0, "%s: [Time: %lu] Mapping Term %s to Address : %" PRIx64 "\n", getName().c_str(), currentCycle, term.term.c_str(), addr);
		numTerms++;
	} else {
		addr = it->second;
	}
	// if(term.free_from_mem){
	// 	termToAddressMap.erase(term.term);
	// }

	if(op == OpCode::Store) {
		auto aliasPhyReg = memoryUnit->findStoreAlias(addr,true /* Quash aliasing store since it is being overwritten */);
		destReg = getMappedPhysicalRegister(dests[0]);
		destReg->incReference();
		size = limbSize;
		auto dispatchInstruction  = std::make_shared<CinnamonMemoryInstruction>(op,destReg,addr,size);
		memoryUnit->addToStoreQueue(dispatchInstruction);
		output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );
	} else if(op == OpCode::Spill) {
		auto aliasPhyReg = memoryUnit->findStoreAlias(addr,false/* Don't quash aliasing store since this spill itself might get quashed. However quash aliasing spills */);
		destReg = getMappedPhysicalRegister(dests[0]);
		destReg->incReference();
		size = limbSize;
		auto dispatchInstruction  = std::make_shared<CinnamonMemoryInstruction>(op,destReg,addr,size);
		memoryUnit->addToStoreQueue(dispatchInstruction);
		output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );
	} else if(op == OpCode::LoadV){
		auto aliasPhyReg = memoryUnit->findStoreAlias(addr,false /*Don't quash pending stores. Only spills will be quashed */);
		size = limbSize;
		if(aliasPhyReg != nullptr){
			auto arg = std::get<CinnamonParsedVectorReg>(dests[0]);
			auto it = vectorRegisterRenameMap.find(arg.id);
			if(it != vectorRegisterRenameMap.end()){
				vectorRegisters.at(it->second)->decReference();
				// vectorRegisters.at(it->second)->unsetMapped();
				// vectorRegisterRenameMap.erase(arg.id);
			}
			vectorRegisterRenameMap[arg.id] = aliasPhyReg->getID();
			destReg = aliasPhyReg;
			destReg->incReference();
			return true;
		}
		aliasPhyReg = memoryUnit->findLoadAlias(addr);
		if(aliasPhyReg != nullptr){
			auto arg = std::get<CinnamonParsedVectorReg>(dests[0]);
			auto it = vectorRegisterRenameMap.find(arg.id);
			if(it != vectorRegisterRenameMap.end()){
				vectorRegisters.at(it->second)->decReference();
				// vectorRegisters.at(it->second)->unsetMapped();
			}
			vectorRegisterRenameMap[arg.id] = aliasPhyReg->getID();
			destReg = aliasPhyReg;
			destReg->incReference();
			// destReg->setMapped();
			return true;
		}
		if(canMapToPhysicalRegister(dests[0]) == false){
			return false;
		}
		destReg = mapToPhysicalRegister(dests[0]);
		destReg->incReference();
		auto dispatchInstruction  = std::make_shared<CinnamonMemoryInstruction>(op,destReg,addr,size);
		memoryUnit->addToLoadQueue(dispatchInstruction);
		output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );
	} else if(op == OpCode::LoadS) {

		auto aliasPhyReg = memoryUnit->findLoadAlias(addr);
		if(aliasPhyReg != nullptr){
			auto arg = std::get<CinnamonParsedScalarReg>(dests[0]);
			auto it = scalarRegisterRenameMap.find(arg.id);
			if(it != scalarRegisterRenameMap.end()){
				scalarRegisters.at(it->second)->decReference();
				// scalarRegisters.at(it->second)->unsetMapped();
			}
			scalarRegisterRenameMap[arg.id] = aliasPhyReg->getID();
			destReg = aliasPhyReg;
			destReg->incReference();
			return true;
		}
		// if(freeScalarRegisters.size() < 1){
		// 	return false;
		// }
		if(canMapToPhysicalRegister(dests[0]) == false){
			return false;
		}
		destReg = mapToPhysicalRegister(dests[0]);
		destReg->incReference();
		size = scalarSize;
		auto dispatchInstruction  = std::make_shared<CinnamonMemoryInstruction>(op,destReg,addr,size);

		// Scalar loads don't take any time
		dispatchInstruction->setExecutionComplete();
		// memoryUnit->addToLoadQueue(dispatchInstruction);
		// output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );
		// return true;
	}

	return true;

}


bool CinnamonChiplet::dispatchBinOpInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	// TODO: Change to get Freeable registers
	// If the existing registers are mapped then we unmap and reuse
	// else get any two registers that 
	// if(freeVectorRegisters.size() < dests.size()){
	// 	return false;
	// }
	if(canMapToPhysicalRegister(dests[0]) == false){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;


	auto & srcs = instruction->srcs;
	assert(srcs.size() == 2);

	std::shared_ptr<PhysicalRegister> destReg = mapToPhysicalRegister(dests[0]);
	destReg->incReference();

	std::shared_ptr<PhysicalRegister> src1Reg = getMappedPhysicalRegister(srcs[0]);
	src1Reg->incReference();
	std::shared_ptr<PhysicalRegister> src2Reg = getMappedPhysicalRegister(srcs[1]);
	src2Reg->incReference();


	auto dispatchInstruction  = std::make_shared<CinnamonBinOpInstruction>(op,destReg,src1Reg,src2Reg,baseIndex);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::Add:
		case OpCode::Sub:
			addQueue->addToInstructionQueue(dispatchInstruction);
			break;
		case OpCode::Mul:
			mulQueue->addToInstructionQueue(dispatchInstruction);
			break;

	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchUnOpInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	// TODO: Change to get Freeable registers
	// If the existing registers are mapped then we unmap and reuse
	// else get any two registers that 
	// if(freeVectorRegisters.size() < dests.size()){
	// 	return false;
	// }
	if(canMapToPhysicalRegister(dests[0]) == false){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;


	auto & srcs = instruction->srcs;
	assert(srcs.size() == 1);

	std::shared_ptr<PhysicalRegister> destReg = mapToPhysicalRegister(dests[0]);
	destReg->incReference();

	std::shared_ptr<PhysicalRegister> src1Reg = getMappedPhysicalRegister(srcs[0]);
	src1Reg->incReference();

	std::shared_ptr<CinnamonUnOpInstruction> dispatchInstruction;
	if(op == OpCode::Rot){
		auto rotIndex = instruction->rotIndex;
		dispatchInstruction  = std::make_shared<CinnamonUnOpInstruction>(op,rotIndex.value(),destReg,src1Reg,baseIndex);
	} else {
		dispatchInstruction  = std::make_shared<CinnamonUnOpInstruction>(op,destReg,src1Reg,baseIndex);
	}
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::Int:
			nttQueue->addToInstructionQueue(dispatchInstruction);
			break;
		case OpCode::Neg:
			addQueue->addToInstructionQueue(dispatchInstruction);
			break;
		case OpCode::Rot:
		case OpCode::Con:
			rotQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchEvgInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	// TODO: Change to get Freeable registers
	// If the existing registers are mapped then we unmap and reuse
	// else get any two registers that 
	// if(freeVectorRegisters.size() < dests.size()){
	// 	return false;
	// }
	if(canMapToPhysicalRegister(dests[0]) == false){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;

	std::shared_ptr<PhysicalRegister> destReg = mapToPhysicalRegister(dests[0]);
	destReg->incReference();

	auto dispatchInstruction  = std::make_shared<CinnamonEvgInstruction>(op,destReg,baseIndex);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::EvkGen:
			evgQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchNttInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	// TODO: Change to get Freeable registers
	// If the existing registers are mapped then we unmap and reuse
	// else get any two registers that 
	// if(freeVectorRegisters.size() < dests.size()){
	// 	return false;
	// }
	if(canMapToPhysicalRegister(dests[0]) == false){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;


	auto & srcs = instruction->srcs;
	assert(srcs.size() == 1);


	std::shared_ptr<PhysicalRegister> destReg = mapToPhysicalRegister(dests[0]);
	destReg->incReference();

	std::shared_ptr<CinnamonNttInstruction> dispatchInstruction = nullptr;
	std::visit(overloaded{[](auto &arg )
						  { assert(0); },
						  [&](CinnamonParsedVectorReg &arg)
						  { 
							std::shared_ptr<PhysicalRegister> src1Reg = getMappedPhysicalRegister(srcs[0]);
							src1Reg->incReference();
							dispatchInstruction  = std::make_shared<CinnamonNttInstruction>(op,destReg,src1Reg,baseIndex);
						  },
						  [&](CinnamonParsedBcuReg &arg)
						  { 
							std::shared_ptr<BaseConversionRegister> src1BcuVirtReg = getMappedBaseConversionVirtualRegister(arg);
							src1BcuVirtReg->incReference();
							dispatchInstruction  = std::make_shared<CinnamonNttInstruction>(op,destReg,src1BcuVirtReg,baseIndex);
						}}, srcs[0]);
	switch(op){
		case OpCode::Ntt:
			nttQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchSuDInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	// TODO: Change to get Freeable registers
	// If the existing registers are mapped then we unmap and reuse
	// else get any two registers that 
	// if(freeVectorRegisters.size() < dests.size()){
	// 	return false;
	// }
	if(canMapToPhysicalRegister(dests[0]) == false){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;


	auto & srcs = instruction->srcs;
	assert(srcs.size() == 2);

	std::shared_ptr<PhysicalRegister> destReg = mapToPhysicalRegister(dests[0]);
	destReg->incReference();

	std::shared_ptr<PhysicalRegister> src1Reg = getMappedPhysicalRegister(srcs[0]);
	src1Reg->incReference();

	std::shared_ptr<CinnamonSuDInstruction> dispatchInstruction = nullptr;
	std::visit(overloaded{[](auto &arg )
						  { assert(0); },
						  [&](CinnamonParsedVectorReg &arg)
						  { 
							std::shared_ptr<PhysicalRegister> src2Reg = getMappedPhysicalRegister(srcs[1]);
							src2Reg->incReference();
							dispatchInstruction  = std::make_shared<CinnamonSuDInstruction>(op,destReg,src1Reg,src2Reg,baseIndex);
						  },
						  [&](CinnamonParsedBcuReg &arg)
						  { 
							std::shared_ptr<BaseConversionRegister> src2BcuVirtReg = getMappedBaseConversionVirtualRegister(arg);
							src2BcuVirtReg->incReference();
							dispatchInstruction  = std::make_shared<CinnamonSuDInstruction>(op,destReg,src1Reg,src2BcuVirtReg,baseIndex);
						}}, srcs[1]);
	// std::shared_ptr<PhysicalRegister> src2Reg = getMappedPhysicalRegister(srcs[1]);
	// src2Reg->incReference();


	// auto dispatchInstruction  = std::make_shared<CinnamonSuDInstruction>(op,destReg,src1Reg,src2Reg,baseIndex);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::SuD:
			sudQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchBciInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	if(freeBaseConversionVirtualRegisters.size() < dests.size()){
		return false;
	}
	auto &op = instruction->opCode;
	CinnamonParsedBcuInitReg dest = std::move(std::get<CinnamonParsedBcuInitReg>(dests[0]));

	std::shared_ptr<BaseConversionRegister> destBcuVirtReg = mapToBaseConversionVirtualRegister(dest);
	destBcuVirtReg->incReference();
	destBcuVirtReg->setReadsRemaining(dest.numReads);
	destBcuVirtReg->setWritesRemaining(dest.numWrites);

	auto dispatchInstruction  = std::make_shared<CinnamonBciInstruction>(op,destBcuVirtReg);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::Bci:
			bciQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchPl1Instruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	if(freeBaseConversionVirtualRegisters.size() < dests.size()){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;
	CinnamonParsedBcuReg dest = std::move(std::get<CinnamonParsedBcuReg>(dests[0]));

	std::shared_ptr<BaseConversionRegister> destBcuVirtReg = getMappedBaseConversionVirtualRegister(dest);
	destBcuVirtReg->incReference();

	auto & srcs = instruction->srcs;
	assert(srcs.size() == 1);

	std::shared_ptr<PhysicalRegister> src1Reg = getMappedPhysicalRegister(srcs[0]);
	src1Reg->incReference();

	auto dispatchInstruction  = std::make_shared<CinnamonPl1Instruction>(op,destBcuVirtReg,src1Reg,baseIndex);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::Pl1:
			pl1Queue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchBcwInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	if(freeBaseConversionVirtualRegisters.size() < dests.size()){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;
	CinnamonParsedBcuReg dest = std::move(std::get<CinnamonParsedBcuReg>(dests[0]));

	std::shared_ptr<BaseConversionRegister> destBcuVirtReg = getMappedBaseConversionVirtualRegister(dest);
	destBcuVirtReg->incReference();

	auto & srcs = instruction->srcs;
	assert(srcs.size() == 1);

	std::shared_ptr<PhysicalRegister> src1Reg = getMappedPhysicalRegister(srcs[0]);
	src1Reg->incReference();

	auto dispatchInstruction  = std::make_shared<CinnamonBcwInstruction>(op,destBcuVirtReg,src1Reg,baseIndex);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::BcW:
			bcwQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

#if 0
bool CinnamonChiplet::dispatchPl2Instruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 2);
	if(freeBaseConversionVirtualRegisters.size() < 1){
		return false;
	}
	// if(freeVectorRegisters.size() < 1){
	// 	return false;
	// }
	if(canMapToPhysicalRegister(dests[1]) == false){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;
	CinnamonParsedBcuReg dest1 = std::move(std::get<CinnamonParsedBcuReg>(dests[0]));

	std::shared_ptr<BaseConversionRegister> destBcuVirtReg = getMappedBaseConversionVirtualRegister(dest1);
	destBcuVirtReg->incReference();

	std::shared_ptr<PhysicalRegister> dest2Reg = mapToPhysicalRegister(dests[1]);
	dest2Reg->incReference();

	auto & srcs = instruction->srcs;
	assert(srcs.size() == 2);

	CinnamonParsedBcuReg src1 = std::move(std::get<CinnamonParsedBcuReg>(srcs[0]));
	std::shared_ptr<BaseConversionRegister> srcBcuVirtReg = getMappedBaseConversionVirtualRegister(src1);
	srcBcuVirtReg->incReference();
	std::shared_ptr<PhysicalRegister> src2Reg = getMappedPhysicalRegister(srcs[1]);
	src2Reg->incReference();

	auto dispatchInstruction  = std::make_shared<CinnamonPl2Instruction>(op,destBcuVirtReg,dest2Reg,srcBcuVirtReg,src1.id.value(),src2Reg,baseIndex);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::Pl2:
			pl2Queue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchPl3Instruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	if(freeBaseConversionVirtualRegisters.size() < 1){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;
	CinnamonParsedBcuReg dest1 = std::move(std::get<CinnamonParsedBcuReg>(dests[0]));

	std::shared_ptr<BaseConversionRegister> destBcuVirtReg = getMappedBaseConversionVirtualRegister(dest1);
	destBcuVirtReg->incReference();

	auto & srcs = instruction->srcs;
	assert(srcs.size() == 2);

	std::shared_ptr<PhysicalRegister> src1Reg = getMappedPhysicalRegister(srcs[0]);
	src1Reg->incReference();
	std::shared_ptr<PhysicalRegister> src2Reg = getMappedPhysicalRegister(srcs[1]);
	src2Reg->incReference();

	auto dispatchInstruction  = std::make_shared<CinnamonPl3Instruction>(op,destBcuVirtReg,src1Reg,src2Reg,baseIndex);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::Pl3:
			pl3Queue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchPl4Instruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);
	// if(freeVectorRegisters.size() < 1){
	// 	return false;
	// }
	if(canMapToPhysicalRegister(dests[0]) == false){
		return false;
	}
	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;

	std::shared_ptr<PhysicalRegister> destReg = mapToPhysicalRegister(dests[0]);
	destReg->incReference();

	auto & srcs = instruction->srcs;
	assert(srcs.size() == 3);

	CinnamonParsedBcuReg src1 = std::move(std::get<CinnamonParsedBcuReg>(srcs[0]));
	std::shared_ptr<BaseConversionRegister> src1BcuVirtReg = getMappedBaseConversionVirtualRegister(src1);
	src1BcuVirtReg->incReference();

	std::shared_ptr<PhysicalRegister> src2Reg = getMappedPhysicalRegister(srcs[1]);
	src2Reg->incReference();
	std::shared_ptr<PhysicalRegister> src3Reg = getMappedPhysicalRegister(srcs[2]);
	src3Reg->incReference();

	auto dispatchInstruction  = std::make_shared<CinnamonPl4Instruction>(op,destReg,src1BcuVirtReg,src1.id.value(),src2Reg,src3Reg,baseIndex);
	// functionalUnit->addToQueue(dispatchInstruction);
	switch(op){
		case OpCode::Pl4:
			pl4Queue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}
#endif

bool CinnamonChiplet::dispatchMovInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;

	auto & dests = instruction->dests;
	assert(dests.size() == 1);

	auto & srcs = instruction->srcs;
	assert(srcs.size() == 1);

	auto dest = std::get<CinnamonParsedVectorReg>(dests[0]);	
	auto src = std::get<CinnamonParsedVectorReg>(srcs[0]);	

	mapSrcToDest(dest,src);

	return true;

}
bool CinnamonChiplet::dispatchRsvInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;
	auto & dests = instruction->dests;
	auto & srcs = instruction->srcs;
	assert(srcs.size() <= 1);

	if(op == OpCode::Rsi) {
		if(freeVectorRegisters.size() < dests.size()){
			return false;
		}
	}
	
	std::shared_ptr<PhysicalRegister> src1Reg;;
	std::vector<std::shared_ptr<PhysicalRegister>> destRegs;;
	if(srcs.size() == 1){
		src1Reg = getMappedPhysicalRegister(srcs[0]);
		src1Reg->incReference();
	}

	if(op == OpCode::Rsi){
		for(auto & dest: dests){
			std::shared_ptr<PhysicalRegister> destReg = mapToPhysicalRegister(dest);
			destReg->incReference();
			destRegs.push_back(destReg);
		}
	} else {
		for(auto & dest: dests){
			std::shared_ptr<PhysicalRegister> destReg = getMappedPhysicalRegister(dest);
			destReg->incReference();
			destRegs.push_back(destReg);
		}
	}

	auto dispatchInstruction  = std::make_shared<CinnamonRsvInstruction>(op,destRegs,src1Reg,baseIndex);

	switch(op){
		case OpCode::Rsv:
		case OpCode::Rsi:
			rsvQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchModInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto &op = instruction->opCode;
	auto baseIndex = instruction->baseIndex;
	auto & dests = instruction->dests;
	auto & srcs = instruction->srcs;
	assert(dests.size() == 1);
	assert(srcs.size() >= 1);

	if(canMapToPhysicalRegister(dests[0]) == false){
		return false;
	}
	
	std::shared_ptr<PhysicalRegister> destReg;;
	std::vector<std::shared_ptr<PhysicalRegister>> srcRegs;;

	destReg = mapToPhysicalRegister(dests[0]);
	destReg->incReference();

	for(auto & src: srcs){
		std::shared_ptr<PhysicalRegister> srcReg = getMappedPhysicalRegister(src);
		srcReg->incReference();
		srcRegs.push_back(srcReg);
	}

	auto dispatchInstruction  = std::make_shared<CinnamonModInstruction>(op,destReg,srcRegs,baseIndex);

	switch(op){
		case OpCode::Mod:
			modQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchDisInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto &op = instruction->opCode;
	auto &syncID = instruction->syncID;
	assert(syncID.has_value());
	auto &syncSize = instruction->syncSize;
	assert(syncSize.has_value());
	auto & dests = instruction->dests;
	auto & srcs = instruction->srcs;

	
	std::shared_ptr<PhysicalRegister> destReg;;
	std::shared_ptr<PhysicalRegister> srcReg;;

	if(op == OpCode::Rcv) {
		assert(dests.size() == 1);
		if(canMapToPhysicalRegister(dests[0]) == false){
			return false;
		}
		destReg = mapToPhysicalRegister(dests[0]);
		destReg->incReference();
	} else if(op == OpCode::Dis) {
		assert(srcs.size() == 1);
		srcReg = getMappedPhysicalRegister(srcs[0]);
		srcReg->incReference();
	}

	auto dispatchInstruction  = std::make_shared<CinnamonDisInstruction>(op,destReg,srcReg,syncID.value(),syncSize.value());

	switch(op){
		case OpCode::Dis:
		case OpCode::Rcv:
			disQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}

bool CinnamonChiplet::dispatchJoiInstruction(SST::Cycle_t currentCycle, const std::shared_ptr<CinnamonParsedInstruction> & instruction){
	using OpCode = CinnamonInstructionOpCode;

	auto &op = instruction->opCode;
	auto &syncID = instruction->syncID;
	assert(syncID.has_value());
	auto &syncSize = instruction->syncSize;
	assert(syncSize.has_value());
	auto &baseIndex = instruction->baseIndex;
	auto & dests = instruction->dests;
	auto & srcs = instruction->srcs;

	
	std::shared_ptr<PhysicalRegister> destReg;;
	std::shared_ptr<PhysicalRegister> srcReg;;

	assert(dests.size() <= 1);

	if(dests.size() == 1){
		if(canMapToPhysicalRegister(dests[0]) == false){
			return false;
		}
		destReg = mapToPhysicalRegister(dests[0]);
		destReg->incReference();
	} 
	if(srcs.size() == 1){
		srcReg = getMappedPhysicalRegister(srcs[0]);
		srcReg->incReference();
	}
	// assert(srcs.size() == 1);

	auto dispatchInstruction  = std::make_shared<CinnamonDisInstruction>(op,destReg,srcReg,syncID.value(),syncSize.value(),baseIndex);

	switch(op){
		case OpCode::Joi:
			disQueue->addToInstructionQueue(dispatchInstruction);
			break;
	}

	output->verbose(CALL_INFO, 3, 0, "%s: %lu Dispatching Instruction: %s\n", getName().c_str(), currentCycle, dispatchInstruction->getString().c_str() );

	return true;

}



/**
 * @brief CPU tick 
 * 
 * Simulates a clock cycle
 */
bool CinnamonChiplet::tick(SST::Cycle_t currentCycle) {

	bool traceCompleted = false;
	bool dispatched = false;

	if(fetchedInstruction == nullptr){
		fetchedInstruction = reader->readNextInstruction(0);
		numInstructions++;
	}

	while(fetchedInstruction){
		using OpCode = CinnamonInstructionOpCode;
		switch(fetchedInstruction->opCode){
			case OpCode::LoadV:
			case OpCode::LoadS:
			case OpCode::Store:
			case OpCode::Spill:
				dispatched = dispatchMemoryInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Add:
			case OpCode::Sub:
			case OpCode::Mul:
				dispatched = dispatchBinOpInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Ntt:
				dispatched = dispatchNttInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Int:
			case OpCode::Neg:
			case OpCode::Rot:
			case OpCode::Con:
				dispatched = dispatchUnOpInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::EvkGen:
				if(config.usePRNG) {
					dispatched = dispatchEvgInstruction(currentCycle, fetchedInstruction);
				} else {
					// If PRNG is not to be used, turn the evkGen instruction into a load
					fetchedInstruction->opCode = OpCode::LoadV;
					dispatched = dispatchMemoryInstruction(currentCycle, fetchedInstruction);
				}
				break;
			case OpCode::SuD:
				dispatched = dispatchSuDInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Bci:
				dispatched = dispatchBciInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::BcW:
				dispatched = dispatchBcwInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Pl1:
				dispatched = dispatchPl1Instruction(currentCycle, fetchedInstruction);
				break;
			// case OpCode::Pl2:
			// 	dispatched = dispatchPl2Instruction(currentCycle, fetchedInstruction);
			// 	break;
			// case OpCode::Pl3:
			// 	dispatched = dispatchPl3Instruction(currentCycle, fetchedInstruction);
			// 	break;
			// case OpCode::Pl4:
			// 	dispatched = dispatchPl4Instruction(currentCycle, fetchedInstruction);
			// 	break;
			case OpCode::Mov:
				dispatched = dispatchMovInstruction(currentCycle, fetchedInstruction);
				// dispatched = dispatchRotInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Rsi:
			case OpCode::Rsv:
				dispatched = dispatchRsvInstruction(currentCycle, fetchedInstruction);
				// dispatched = dispatchRotInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Mod:
				dispatched = dispatchModInstruction(currentCycle, fetchedInstruction);
				// dispatched = dispatchRotInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Rcv:
			case OpCode::Dis:
				dispatched = dispatchDisInstruction(currentCycle, fetchedInstruction);
				break;
			case OpCode::Joi:
				dispatched = dispatchJoiInstruction(currentCycle, fetchedInstruction);
				break;
			default:
				throw std::invalid_argument("Unhandled OpCode: " + getOpCodeString(fetchedInstruction->opCode));
		}

		if(!dispatched){
			break;
		} else {
			fetchedInstruction = reader->readNextInstruction(0);
			numInstructions++;
			if(numInstructions % 100000 == 0) {
				uint64_t mils = numInstructions / 1000000;
				uint64_t hundredKs = (numInstructions - mils*1000000) / 100000;
				output->verbose(CALL_INFO, 1, 0, "%s:Heartbeat @  %lu.%lu M instructions %" PRIu64 " cycles\n",getName().c_str(),mils,hundredKs,currentCycle);
				output->flush();
			}
		}
	}

	if(currentCycle % 1000000 == 0) {
		uint64_t mils = numInstructions / 1000000;
		uint64_t hundredKs = (numInstructions - mils*1000000) / 100000;
		output->verbose(CALL_INFO, 1, 0, "%s:Heartbeat @ %" PRIu64 " M cycles %lu.%lu M instructions\n",getName().c_str(),currentCycle/(1000000),mils,hundredKs);
		output->flush();
	}

    if(currentCycle % 100000 == 0){
        output->verbose(CALL_INFO, 1, 0, "%s:Heartbeat @ %" PRIu64 " 00K cycles. Compute Util Cycles: %" PRIu64 "\n",getName().c_str(),currentCycle/(100000),stats_.busyCyclesWindow);
        output->flush();
        stats_.busyCyclesWindow = 0;
    }

	if(!fetchedInstruction){
		traceCompleted = true;
	}

	addQueue->tick(currentCycle);
	mulQueue->tick(currentCycle);
	rotQueue->tick(currentCycle);
	evgQueue->tick(currentCycle);
	nttQueue->tick(currentCycle);
	sudQueue->tick(currentCycle);
	bciQueue->tick(currentCycle);
	bcwQueue->tick(currentCycle);
	pl1Queue->tick(currentCycle);
	// pl2Queue->tick(currentCycle);
	// pl3Queue->tick(currentCycle);
	// pl4Queue->tick(currentCycle);
	rsvQueue->tick(currentCycle);
	modQueue->tick(currentCycle);
	disQueue->tick(currentCycle);

	memoryUnit->executeCycleBegin(currentCycle);
	for(int i = 0; i < functionalUnits.size(); i++){
		functionalUnits.at(i)->executeCycleBegin(currentCycle);
	}
	for(int i = 0; i < baseConversionUnits.size(); i++){
		baseConversionUnits.at(i)->executeCycleBegin(currentCycle);
	}
	memoryUnit->executeCycleEnd(currentCycle);
	for(int i = 0; i < functionalUnits.size(); i++){
		functionalUnits.at(i)->executeCycleEnd(currentCycle);
	}
	for(int i = 0; i < baseConversionUnits.size(); i++){
		baseConversionUnits.at(i)->executeCycleEnd(currentCycle);
	}
	// fetchRequest(currentCycle);
	// issueRequest(currentCycle);
	// executeRequest(currentCycle);
	// commitRequest(currentCycle);

	// // End condition:
	// if (fetchBuffer->isEmpty() &&
	// 	issueBuffer->getLoadQueue()->isEmpty() && 
	// 	issueBuffer->getStoreQueue()->isEmpty() && 
	// 	computeAdd->getInstrQueue()->isEmpty() && 
	// 	computeMult->getInstrQueue()->isEmpty()) {

    if ( traceCompleted ) {

		bool okayToFinish = memoryUnit->okayToFinish();
		for(int i = 0; i < functionalUnits.size(); i++){
			okayToFinish = okayToFinish && functionalUnits[i]->okayToFinish();
		}
		okayToFinish = okayToFinish && addQueue->okayToFinish();
		okayToFinish = okayToFinish && mulQueue->okayToFinish();
		okayToFinish = okayToFinish && rotQueue->okayToFinish();
		okayToFinish = okayToFinish && evgQueue->okayToFinish();
		okayToFinish = okayToFinish && nttQueue->okayToFinish();
		okayToFinish = okayToFinish && sudQueue->okayToFinish();
		okayToFinish = okayToFinish && bciQueue->okayToFinish();
		okayToFinish = okayToFinish && bcwQueue->okayToFinish();
		okayToFinish = okayToFinish && pl1Queue->okayToFinish();
		// okayToFinish = okayToFinish && pl2Queue->okayToFinish();
		// okayToFinish = okayToFinish && pl3Queue->okayToFinish();
		// okayToFinish = okayToFinish && pl4Queue->okayToFinish();
		okayToFinish = okayToFinish && rsvQueue->okayToFinish();
        okayToFinish = okayToFinish && modQueue->okayToFinish();
		okayToFinish = okayToFinish && disQueue->okayToFinish();
		if(okayToFinish){
			output->verbose(CALL_INFO, 1, 0, "CinnamonChiplet: Test Completed Successfuly\n");
			// primaryComponentOKToEndSim();
			return true;    // Turn our clock off while we wait for any other CPUs to end
		}
    }
		
	// }

	// // TODO: Measure number of outstanding instructions 

	// // Keep simulation ticking, we have more work to do if we reach here
	return false;
}

//   std::string getName() {
// 	return "chiplet";
//   } 

} // Namespace Cinnamon
} // Namespace SST



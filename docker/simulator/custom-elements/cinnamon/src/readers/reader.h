#ifndef _H_SST_CINNAMON_READER
#define _H_SST_CINNAMON_READER

#include "opcode.h"

#include<memory>
#include<variant>
#include<optional>

namespace SST {
namespace Cinnamon {

struct CinnamonParsedVectorReg {
	std::uint16_t id;
	bool dead;
	CinnamonParsedVectorReg(const std::uint16_t id, bool dead) : id(id), dead(dead) {}
};

struct CinnamonParsedScalarReg {
	std::uint16_t id;
	bool dead;
	CinnamonParsedScalarReg(const std::uint16_t id, bool dead) : id(id), dead(dead) {}
};

struct CinnamonParsedBcuInitReg {
	std::uint8_t bcuId;
	std::uint8_t numWrites;
	std::uint8_t numReads;
	CinnamonParsedBcuInitReg(const std::uint8_t bcuId, const std::uint8_t numWrites, const std::uint8_t numReads) : bcuId(bcuId) , numWrites(numWrites), numReads(numReads) {}
};

struct CinnamonParsedBcuReg {
	std::uint8_t bcuId;
	std::optional<std::uint16_t> id;
	CinnamonParsedBcuReg(const std::uint8_t bcuId) : bcuId(bcuId) {}
	CinnamonParsedBcuReg(const std::uint8_t bcuId, const std::uint16_t id) : bcuId(bcuId) , id(id) {}
};

struct CinnamonParsedTerm {
	std::string term;
	bool free_from_mem;
	CinnamonParsedTerm(const std::string && term, bool free_from_mem) : term(term), free_from_mem(free_from_mem) {}
};

using CinnamonParsedValueType = std::variant<CinnamonParsedVectorReg,CinnamonParsedScalarReg,CinnamonParsedBcuReg,CinnamonParsedBcuInitReg,CinnamonParsedTerm>;

struct CinnamonParsedInstruction {
		using OpCode = Cinnamon::CinnamonInstructionOpCode;
		OpCode opCode;
		std::uint16_t baseIndex;
		std::optional<std::uint64_t> syncID;
		std::optional<std::uint64_t> syncSize;
		std::optional<std::int32_t> rotIndex;
		std::vector<CinnamonParsedValueType> srcs;
		std::vector<CinnamonParsedValueType> dests;
		// CinnamonParsedInstruction(const OpCode opCode ) : opCode(opCode) {}
		CinnamonParsedInstruction(const OpCode opCode, std::optional<const std::int32_t> rotIndex, const std::uint16_t baseIndex, const std::vector<CinnamonParsedValueType> && dests, const std::vector<CinnamonParsedValueType> && srcs ) : opCode(opCode), rotIndex(rotIndex), baseIndex(baseIndex), dests(dests), srcs(srcs) {}
		CinnamonParsedInstruction(const OpCode opCode, const std::uint16_t baseIndex, const std::vector<CinnamonParsedValueType> && dests, const std::vector<CinnamonParsedValueType> && srcs ) : opCode(opCode), baseIndex(baseIndex), dests(dests), srcs(srcs) {}
		CinnamonParsedInstruction(const OpCode opCode, const std::uint16_t baseIndex, std::optional<const std::uint32_t> syncID, std::optional<const std::uint32_t> syncSize, const std::vector<CinnamonParsedValueType> && dests, const std::vector<CinnamonParsedValueType> && srcs ) : opCode(opCode), syncID(syncID), syncSize(syncSize), baseIndex(baseIndex), dests(dests), srcs(srcs) {}
};

class CinnamonTraceReader : public SubComponent {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Cinnamon::CinnamonTraceReader, std::shared_ptr<SST::Output>)

	CinnamonTraceReader( ComponentId_t id, Params& params) : SubComponent(id) {
		// output = out;
	}

	~CinnamonTraceReader() { };
	virtual std::unique_ptr<CinnamonParsedInstruction> readNextInstruction(uint64_t instrId) = 0;

protected:

};
	
};


}
	
#endif	

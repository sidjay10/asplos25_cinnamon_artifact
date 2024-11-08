#ifndef _H_SST_CINNAMON_TEXT_READER
#define _H_SST_CINNAMON_TEXT_READER

#include <fstream>
#include <regex>
#include "reader.h"

using namespace SST::Cinnamon;

namespace SST {
namespace Cinnamon {

class CinnamonTextTraceReader : public CinnamonTraceReader {

public:
	CinnamonTextTraceReader( ComponentId_t id, Params& params, std::shared_ptr<SST::Output> out);
	~CinnamonTextTraceReader();
	//CinnamonInstr* readNextInstr(uint64_t nextInstr, std::map<uint64_t, CinnamonInstr*>* regToInstr);
	// virtual std::unique_ptr<CinnamonInstruction> readNextInstruction(uint64_t instrId) override;
	virtual std::unique_ptr<CinnamonParsedInstruction> readNextInstruction(uint64_t instrId) override;
	// bool readNextInstr();

	SST_ELI_REGISTER_SUBCOMPONENT(
		CinnamonTextTraceReader,
		"cinnamon",
		"CinnamonTextTraceReader",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Text Trace Reader",
		SST::Cinnamon::CinnamonTraceReader
	)

	SST_ELI_DOCUMENT_PARAMS(
		{ "file", "Sets the file for the trace reader to use", "" }
	)

private:
	using OpCode = CinnamonInstructionOpCode;
	std::string traceFileName;
	std::ifstream traceInputFile;
	std::shared_ptr<SST::Output> output;
	std::regex rsi_regex = std::regex("\\{(r[0-9]+, )*(r[0-9]+)\\}");
	std::unique_ptr<CinnamonParsedInstruction> handle_rsi(const std::string & instruction);
	std::regex rsv_regex = std::regex("\\{(.*)}: (r[0-9]+(\\[X\\])?): \\[(.*)\\] \\| ([0-9]+)");
	std::unique_ptr<CinnamonParsedInstruction> handle_rsv(const std::string & instruction);
	std::regex mod_regex = std::regex("(r[0-9]+(\\[X\\])?): \\{(.*)} \\| ([0-9]+)");
	std::unique_ptr<CinnamonParsedInstruction> handle_mod(const std::string & instruction);
	std::regex rcv_regex = std::regex("@ ([0-9]+):([0-9]+) (r[0-9]+(\\[X\\])?):");
	std::unique_ptr<CinnamonParsedInstruction> handle_rcv(const std::string & instruction);
	std::regex dis_regex = std::regex("@ ([0-9]+):([0-9]+) : (r[0-9]+(\\[X\\])?)");
	std::unique_ptr<CinnamonParsedInstruction> handle_dis(const std::string & instruction);
	std::regex joi_regex = std::regex("@ ([0-9]+):([0-9]+) (r[0-9]+(\\[X\\])?)?: (r[0-9]+(\\[X\\])?)? \\| ([0-9]+)");
	std::unique_ptr<CinnamonParsedInstruction> handle_joi(const std::string & instruction);

};

CinnamonParsedValueType parseValue(std::string && value_str);

} // namespace Cinnamon
} // namespace SST

#endif

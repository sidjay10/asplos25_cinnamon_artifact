// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst/core/sst_config.h"
#include "textreader.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <optional>

namespace SST {
namespace Cinnamon {

CinnamonTextTraceReader::CinnamonTextTraceReader( ComponentId_t id, Params& params, std::shared_ptr<SST::Output> out ) :
	output(out), CinnamonTraceReader(id, params) {

	traceFileName = params.find<std::string>("file", "");
	std::cout << "Trace file: " << traceFileName.c_str() << std::endl;

	traceInputFile.open(traceFileName,std::ios::in);

	if(output == nullptr){
		std::cout << "Output is nullptr\n";
	}

	if(!traceInputFile.is_open()) {
            output->fatal(CALL_INFO, -1, "%s, Fatal: Unable to open file: %s in text reader.\n",
                    getName().c_str(), traceFileName.c_str());
	}

	std::string line;
	if( getline (traceInputFile,line) ) {};

}

CinnamonTextTraceReader::~CinnamonTextTraceReader() {
	traceInputFile.close();
}

//CinnamonInstr* CinnamonTextTraceReader::readNextInstr(uint64_t instrId, std::map<uint64_t, CinnamonInstr*>* regToInstr) {
// int CinnamonTextTraceReader::readNextInstr() {
// std::unique_ptr<CinnamonInstruction> CinnamonTextTraceReader::readNextInstruction(uint64_t instrId) {

SST::Cinnamon::CinnamonParsedValueType parseValue(std::string && value_str){
	if(value_str.at(0) == 'r'){
		bool dead = false;
		auto pos = value_str.find("[");
		if(pos != std::string::npos){
			value_str = value_str.substr(0,pos);
			dead = true;
		}
		value_str[0] = '0';
		return CinnamonParsedVectorReg(std::stoi(value_str),dead);
	} else if (value_str.at(0) == 's'){
		bool dead = false;
		auto pos = value_str.find("[");
		if(pos != std::string::npos){
			value_str = value_str.substr(0,pos);
			dead = true;
		}
		value_str[0] = '0';
		return CinnamonParsedScalarReg(std::stoi(value_str),dead);
	} else if (value_str.at(0) == 'B'){
		std::uint8_t bcuId = -1;
		std::uint16_t id = -1;
		value_str[0] = '0';
		bcuId = std::stoi(value_str);
		return CinnamonParsedBcuReg(bcuId,id);
	} else if (value_str.at(0) == 'b'){
		std::uint8_t bcuId = -1;
		std::uint16_t id = -1;
		
		size_t lbPos = value_str.find('{');
		size_t rbPos = value_str.find('}');
		if(lbPos == std::string::npos){
			value_str[0] = '0';
			bcuId = std::stoi(value_str);
		} else {
			bcuId = std::stoi(value_str.substr(1,lbPos));
			id = std::stoi(value_str.substr(lbPos+1,rbPos-1));
		}
		return CinnamonParsedBcuReg(bcuId,id);
	}
	
	throw std::invalid_argument("Invalid value: " + value_str);
}
std::unique_ptr<CinnamonParsedInstruction> CinnamonTextTraceReader::handle_rsi(const std::string &instruction) {
    std::smatch match;
	std::vector<CinnamonParsedValueType> dests;
	std::vector<CinnamonParsedValueType> srcs;
    if (std::regex_search(instruction.begin(), instruction.end(), match, rsi_regex)) {

        for (auto i = 1; i < match.size(); i += 1) {
            dests.push_back(parseValue(match[i]));
        }
    } else {
        output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
                      getName().c_str(), instruction.c_str());
    }

    auto cinnamonInstruction = std::make_unique<CinnamonParsedInstruction>(OpCode::Rsi, -1, std::move(dests), std::move(srcs));
	return std::move(cinnamonInstruction);
}

std::unique_ptr<CinnamonParsedInstruction> CinnamonTextTraceReader::handle_rsv(const std::string &instruction) {
    std::smatch match;
	std::vector<CinnamonParsedValueType> dests;
	std::vector<CinnamonParsedValueType> srcs;
    if (std::regex_search(instruction.begin(), instruction.end(), match, rsv_regex)) {

		srcs.push_back(parseValue(match[2]));
		auto baseIndex = std::stoi(match[5]);

		size_t pos = 0;
		auto destsStr = match[1].str();
		while((pos = destsStr.find(",") )!= std::string::npos){
			dests.push_back(parseValue(destsStr.substr(0,pos)));
			destsStr.erase(0,pos+2);
		}
		dests.push_back(parseValue(std::move(destsStr)));

		auto cinnamonInstruction = std::make_unique<CinnamonParsedInstruction>(OpCode::Rsv, baseIndex, std::move(dests), std::move(srcs));
		return std::move(cinnamonInstruction);
    } else {
        output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
                      getName().c_str(), instruction.c_str());
    }

	return nullptr;

}

std::unique_ptr<CinnamonParsedInstruction> CinnamonTextTraceReader::handle_mod(const std::string &instruction) {
    std::smatch match;
	std::vector<CinnamonParsedValueType> dests;
	std::vector<CinnamonParsedValueType> srcs;
    if (std::regex_search(instruction.begin(), instruction.end(), match, mod_regex)) {

		dests.push_back(parseValue(match[1]));
		auto baseIndex = std::stoi(match[4]);

		size_t pos = 0;
		auto srcsStr = match[3].str();
		while((pos = srcsStr.find(",") )!= std::string::npos){
			srcs.push_back(parseValue(srcsStr.substr(0,pos)));
			srcsStr.erase(0,pos+2);
		}
		srcs.push_back(parseValue(std::move(srcsStr)));

		auto cinnamonInstruction = std::make_unique<CinnamonParsedInstruction>(OpCode::Mod, baseIndex, std::move(dests), std::move(srcs));
		return std::move(cinnamonInstruction);
    } else {
        output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
                      getName().c_str(), instruction.c_str());
    }

	return nullptr;

}

std::unique_ptr<CinnamonParsedInstruction> CinnamonTextTraceReader::handle_rcv(const std::string &instruction) {
    std::smatch match;
	std::vector<CinnamonParsedValueType> dests;
	std::vector<CinnamonParsedValueType> srcs;
    if (std::regex_search(instruction.begin(), instruction.end(), match, rcv_regex)) {

		dests.push_back(parseValue(match[3]));
		std::optional<uint64_t>  syncID = std::stoull(match[1]);
		std::optional<uint64_t>  syncSize = std::stoull(match[2]);
		auto baseIndex = -1;

		auto cinnamonInstruction = std::make_unique<CinnamonParsedInstruction>(OpCode::Rcv, baseIndex, syncID, syncSize, std::move(dests), std::move(srcs));
		return std::move(cinnamonInstruction);
    } else {
        output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
                      getName().c_str(), instruction.c_str());
    }

	return nullptr;

}

std::unique_ptr<CinnamonParsedInstruction> CinnamonTextTraceReader::handle_dis(const std::string &instruction) {
    std::smatch match;
	std::vector<CinnamonParsedValueType> dests;
	std::vector<CinnamonParsedValueType> srcs;
    if (std::regex_search(instruction.begin(), instruction.end(), match, dis_regex)) {

		srcs.push_back(parseValue(match[3]));
		std::optional<uint64_t>  syncID = std::stoull(match[1]);
		std::optional<uint64_t>  syncSize = std::stoull(match[2]);
		auto baseIndex = -1;

		auto cinnamonInstruction = std::make_unique<CinnamonParsedInstruction>(OpCode::Dis, baseIndex, syncID, syncSize, std::move(dests), std::move(srcs));
		return std::move(cinnamonInstruction);
    } else {
        output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
                      getName().c_str(), instruction.c_str());
    }

	return nullptr;

}

std::unique_ptr<CinnamonParsedInstruction> CinnamonTextTraceReader::handle_joi(const std::string &instruction) {
    std::smatch match;
	std::vector<CinnamonParsedValueType> dests;
	std::vector<CinnamonParsedValueType> srcs;
    if (std::regex_search(instruction.begin(), instruction.end(), match, joi_regex)) {

		std::optional<uint64_t>  syncID = std::stoull(match[1]);
		std::optional<uint64_t>  syncSize = std::stoull(match[2]);
		if(match[3].length() != 0){
			dests.push_back(parseValue(match[3]));
		}
		auto baseIndex = std::stoi(match[7]);
		if(match[5].length() != 0){
			srcs.push_back(parseValue(match[5]));
		}
		// srcs.push_back(parseValue(match[4]));

		auto cinnamonInstruction = std::make_unique<CinnamonParsedInstruction>(OpCode::Joi, baseIndex, syncID, syncSize, std::move(dests), std::move(srcs));
		return std::move(cinnamonInstruction);
    } else {
        output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
                      getName().c_str(), instruction.c_str());
    }

	return nullptr;

}

std::unique_ptr<CinnamonParsedInstruction> CinnamonTextTraceReader::readNextInstruction(uint64_t instrId) {
	std::string line;
	if( getline (traceInputFile,line) ) {
    //   std::cout << line << '\n';
			size_t pos = std::string::npos;
			std::string instruction_string = line;
			pos = line.find(" ");
			assert(pos != std::string::npos);
			auto op = line.substr(0, pos);
			line.erase(0,pos+1);
			std::optional<std::int32_t> rotIndex;
			if(op == "rot"){
				pos = line.find(" ");
				assert(pos != std::string::npos);
				rotIndex = std::stoi(line.substr(0, pos));
				line.erase(0,pos+1);
			}

			if(op == "rsi") {
				return handle_rsi(line);
			} else if(op == "rsv") {
				return handle_rsv(line);
			} else if(op == "mod") {
				return handle_mod(line);
			} else if(op == "rcv") {
				return handle_rcv(line);
			} else if(op == "dis") {
				return handle_dis(line);
			} else if(op == "joi") {
				return handle_joi(line);
			}
			pos = line.find("|");
			std::uint32_t baseIndex = -1;
			if(pos != std::string::npos){
				baseIndex = std::stoi(line.substr(pos+1,std::string::npos));
				line.erase(pos-1,std::string::npos);
			}
			pos = line.find(":");
			assert(pos != std::string::npos);
			auto dests_str = line.substr(0,pos);
			line.erase(0,pos+2);

			OpCode opCode = OpCode::NUM_OPCODES;
			std::vector<CinnamonParsedValueType> dests,srcs;

			if(op == "bci"){
				opCode = OpCode::Bci;
				dests_str[0] = '0';
				// auto dest = CinnamonParsedVectorReg(std::stoi(dests_str));
				std::size_t lBracePos, rBracePos;
				lBracePos = line.find("[");
				rBracePos= line.find("]");
				std::string outBases = line.substr(lBracePos + 1,rBracePos - 1);
				line = line.erase(0,rBracePos + 1);
				lBracePos = line.find("[");
				rBracePos= line.find("]");
				std::string inBases = line.substr(lBracePos + 1,rBracePos - 1);
				std::uint8_t numInBases = 0, numOutBases = 0;
				std::size_t pos;
				while((pos = outBases.find("," )) != std::string::npos){
					outBases.erase(0,pos+2);
					numOutBases++;
				}
				numOutBases++;
				while((pos = inBases.find("," )) != std::string::npos){
					inBases.erase(0,pos+2);
					numInBases++;
				}
				numInBases++;
				CinnamonParsedBcuInitReg bcuInitReg(std::stoi(dests_str),numInBases,numOutBases);
				dests = {bcuInitReg};
				return std::make_unique<CinnamonParsedInstruction>(opCode,rotIndex,baseIndex,std::move(dests),std::move(srcs));
			}
			std::string srcs_str = std::move(line);
			if(op == "load" || op == "loas" || op == "store" || op == "evg" || op == "spill" ){
				if(op == "load"){
					opCode = OpCode::LoadV; 
				} else if (op == "loas"){
					opCode = OpCode::LoadS; 
				} else if (op == "store"){
					opCode = OpCode::Store; 
				} else if (op == "spill"){
					opCode = OpCode::Spill; 
				} else if (op == "evg") {
					opCode = OpCode::EvkGen;
				} else {
					assert(0 && "unreachable");
				}
				auto pos = dests_str.find(",");
				if(pos != std::string::npos){
					output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
							getName().c_str(), instruction_string.c_str());
				}
				pos = srcs_str.find(",");
				if(pos != std::string::npos){
					output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
							getName().c_str(), instruction_string.c_str());
				}
				if(opCode == OpCode::LoadS){
					if(dests_str.at(0) != 's'){
						output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
								getName().c_str(), instruction_string.c_str());
					}
					if(srcs_str.at(0) != 's'){
						output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
								getName().c_str(), instruction_string.c_str());
					}
				} else {
					if(dests_str.at(0) != 'r'){
						output->fatal(CALL_INFO, -1, "%s, Fatal: Invalid instruction %s in text reader.\n",
								getName().c_str(), instruction_string.c_str());
					}
				}
				// dests_str[0] = '0';
				// auto dest = CinnamonParsedVectorReg(std::stoi(dests_str),false);
				auto dest = parseValue(std::move(dests_str));
				dests = {dest};
				pos = srcs_str.find("{F}");
				bool free_from_mem = false;
				if(pos != std::string::npos){
					srcs_str = srcs_str.substr(0,pos);
					free_from_mem = true;
				}
				auto src = CinnamonParsedTerm(std::move(srcs_str),free_from_mem);
				srcs = {src};
			} else {
				if(op == "add"){
					opCode = OpCode::Add; 
				} else if(op == "ads"){
					opCode = OpCode::Add; 
				} else if( op == "sub") {
					opCode = OpCode::Sub; 
				} else if( op == "sus") {
					opCode = OpCode::Sub; 
				} else if( op == "neg") {
					opCode = OpCode::Neg; 
				} else if( op == "mul") {
					opCode = OpCode::Mul; 
				} else if( op == "mup") {
					opCode = OpCode::Mul; 
				} else if(op == "mus"){
					opCode = OpCode::Mul; 
				} else if( op == "int" ){
					opCode = OpCode::Int;
				} else if( op == "ntt" ){
					opCode = OpCode::Ntt;
				} else if( op == "sud" ){
					opCode = OpCode::SuD;
				} else if (op == "bcw") {
					opCode = OpCode::BcW;
				} else if (op == "pl1") {
					opCode = OpCode::Pl1;
				} else if (op == "pl2") {
					opCode = OpCode::Pl2;
				} else if (op == "pl3") {
					opCode = OpCode::Pl3;
				} else if (op == "pl4") {
					opCode = OpCode::Pl4;
				} else if (op == "rot") {
					opCode = OpCode::Rot;
				} else if (op == "mov") {
					opCode = OpCode::Mov;
				} else if (op == "con") {
					opCode = OpCode::Con;
				} else {
					throw std::invalid_argument("Invalid opCode Parse: " + op);

				}
				
				while((pos = dests_str.find("," )) != std::string::npos){
					dests.push_back(parseValue(dests_str.substr(0,pos)));
					dests_str.erase(0,pos+2);
				}
				dests.push_back(parseValue(dests_str.substr(0,pos)));
				while((pos = srcs_str.find("," )) != std::string::npos){
					srcs.push_back(parseValue(srcs_str.substr(0,pos)));
					srcs_str.erase(0,pos+2);
				}
				srcs.push_back(parseValue(srcs_str.substr(0,pos)));
			}

			auto instruction = std::make_unique<CinnamonParsedInstruction>(opCode,rotIndex,baseIndex,std::move(dests),std::move(srcs));

	  return instruction;
    }
	return nullptr;
}

} // namespace Cinnamon
} // namespace SST

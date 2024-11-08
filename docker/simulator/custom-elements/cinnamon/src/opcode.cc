#include "opcode.h"
#include <string>

namespace SST {
namespace Cinnamon {


std::string getOpCodeString(CinnamonInstructionOpCode opCode)
{
    std::string str;
    using OpCode = CinnamonInstructionOpCode;
    switch (opCode)
    {
    case OpCode::LoadV:
        str = "LoadV";
        break;
    case OpCode::LoadS:
        str = "LoadS";
        break;
    case OpCode::Store:
        str = "Store";
        break;
    case OpCode::Spill:
        str = "Spill";
        break;
    case OpCode::EvkGen:
        str = "EvkGen";
        break;
    case OpCode::Dis:
        str = "Dis";
        break;
    case OpCode::Rcv:
        str = "Rcv";
        break;
    case OpCode::Joi:
        str = "Joi";
        break;
    case OpCode::Add:
        str = "Add";
        break;
    case OpCode::Sub:
        str = "Sub";
        break;
    case OpCode::Neg:
        str = "Neg";
        break;
    case OpCode::Mul:
        str = "Mul";
        break;
    case OpCode::Div:
        str = "Div";
        break;
    case OpCode::Rot:
        str = "Rot";
        break;
    case OpCode::Con:
        str = "Con";
        break;
    case OpCode::Ntt:
        str = "Ntt";
        break;
    case OpCode::Int:
        str = "Int";
        break;
    case OpCode::Mov:
        str = "Mov";
        break;
    case OpCode::Pl1:
        str = "Pl1";
        break;
    case OpCode::Pl2:
        str = "Pl2";
        break;
    case OpCode::Pl3:
        str = "Pl3";
        break;
    case OpCode::Pl4:
        str = "Pl4";
        break;
    case OpCode::SuD:
        str = "SuD";
        break;
    case OpCode::Bci:
        str = "Bci";
        break;
    case OpCode::BcW:
        str = "BcW";
        break;
    case OpCode::BcR:
        str = "BcR";
        break;
    case OpCode::Nop:
        str = "Nop";
        break;
    case OpCode::Rsi:
        str = "Rsi";
        break;
    case OpCode::Rsv:
        str = "Rsv";
        break;
    case OpCode::Mod:
        str = "Mod";
        break;
    default:
        throw std::invalid_argument("Invalid OpCode : " + std::to_string((uint32_t) opCode));
    }
    return str;
}

} // Namespace Cinnamon
} // Namespace SST
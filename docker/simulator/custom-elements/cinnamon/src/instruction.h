#ifndef _H_SST_CINNAMON_INSTR
#define _H_SST_CINNAMON_INSTR

#include <sst/core/component.h>
#include "sst/core/interfaces/stdMem.h"
#include "opcode.h"
#include "physicalRegister.h"
#include "baseConversionRegister.h"

#include <variant>


namespace SST {
namespace Cinnamon {

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

class CinnamonInstruction {
	public:
    using OpCode = CinnamonInstructionOpCode;
    using LimbID_t = std::uint16_t;
	
    CinnamonInstruction(OpCode opCode): opCode(opCode) {};
	CinnamonInstruction::OpCode getOpCode() const { return opCode; }

    virtual bool allOperandsReady() const = 0;
    virtual void setExecutionComplete() = 0;
    virtual std::string getString() const = 0;
    virtual ~CinnamonInstruction() = default;
	protected:
	OpCode opCode;

};



class CinnamonMemoryInstruction : public CinnamonInstruction {
    
    std::shared_ptr<PhysicalRegister> phyReg;
    Interfaces::StandardMem::Addr addr;
    std::size_t size;
    bool quashed;

    public:

    CinnamonMemoryInstruction() = delete;
    CinnamonMemoryInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & phyReg, const Interfaces::StandardMem::Addr addr, std::size_t size) : phyReg(phyReg), addr(addr), size(size), quashed(false), CinnamonInstruction(opCode) {
        switch(opCode) {
            case OpCode::LoadV:
            case OpCode::LoadS:
            case OpCode::Store:
            case OpCode::Spill:
                break;
            default:
                throw std::invalid_argument("Invalid Memory Instruction with OpCode : " + getOpCodeString(opCode));
        }
    }; 

    bool allOperandsReady() const override {
        if(opCode != OpCode::Store || opCode != OpCode::Spill){
            return true;
        }

        return phyReg->getValueReady();
    }

    void setExecutionComplete() override {
        if(opCode != OpCode::Store || opCode != OpCode::Spill){
            phyReg->setValueReady(true);
        }
        phyReg->decReference();
        // phyReg->addToFreeListIfFree();
    }

    std::shared_ptr<PhysicalRegister> getPhyReg() {
        return phyReg;
    }

    Interfaces::StandardMem::Addr getAddr() const {
        return addr;
    }

    std::size_t getSize() const {
        return size;
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << phyReg->getString() << " : 0x" << std::hex << addr << std::dec;
        return s.str();
    }

    void quash(){
        quashed = true;
        phyReg->decReference();
    }

    bool isQuashed() const {
        return quashed;
    }

};

class CinnamonBinOpInstruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    std::shared_ptr<PhysicalRegister> src1, src2;
    LimbID_t limb;
    public:
    CinnamonBinOpInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const std::shared_ptr<PhysicalRegister> & src2, const LimbID_t limb) : dest(dest), src1(src1), src2(src2), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Add:
            case OpCode::Sub:
            case OpCode::Mul:
            break;
            default:
                throw std::invalid_argument("Invalid BinOp Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return src1->getValueReady() && src2->getValueReady() ;
    }

    void setExecutionComplete() override {
        src1->decReference();
        src2->decReference();
        dest->setValueReady(true);
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString() << " : " << src1->getString() << ", " << src2->getString() << " | " << limb;
        return s.str();
    }
};

class CinnamonBcReadInstruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    std::shared_ptr<BaseConversionRegister> src1;
    LimbID_t limb;
    public:
    CinnamonBcReadInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<BaseConversionRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::BcR:
            break;
            default:
                throw std::invalid_argument("Invalid BcR Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return src1->getValueReady();
        // TODO: Load these operands too
    }

    void setExecutionComplete() override {
        src1->executeRead();
        src1->decReference();
        dest->setValueReady(true);
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString() << " : " << src1->getString() << " | " << limb;
        return s.str();
    }

};

class CinnamonBcWriteInstruction: public CinnamonInstruction {

    std::shared_ptr<BaseConversionRegister> dest;
    std::shared_ptr<PhysicalRegister> src1;
    LimbID_t limb;
    public:
    CinnamonBcWriteInstruction(const OpCode opCode, const std::shared_ptr<BaseConversionRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::BcW:
            break;
            default:
                throw std::invalid_argument("Invalid BcW Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return src1->getValueReady();
        // TODO: Load these operands too
    }

    void setExecutionComplete() override {
        src1->decReference();
        dest->executeWrite();
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString() << " : " << src1->getString() << " | " << limb;
        return s.str();
    }

};


// class CinnamonBcReadInstruction : public CinnamonInstruction;
class CinnamonNttInstruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    std::optional<std::shared_ptr<PhysicalRegister>> dest2;
    std::variant<std::shared_ptr<PhysicalRegister>,std::shared_ptr<BaseConversionRegister>> src1;
    LimbID_t limb;
    public:
    CinnamonNttInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Ntt:
            break;
            default:
                throw std::invalid_argument("Invalid Ntt Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    CinnamonNttInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<BaseConversionRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Ntt:
            break;
            default:
                throw std::invalid_argument("Invalid Ntt Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    CinnamonNttInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & dest2, const std::shared_ptr<BaseConversionRegister> & src1, const LimbID_t limb) : dest(dest), dest2(dest2), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Ntt:
            break;
            default:
                throw std::invalid_argument("Invalid Ntt Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        bool ready = false;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                ready = arg->getValueReady(); 
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                 ready = arg->getValueReady(); 
                            }
                        }, src1);
        return ready;
        }

    void setExecutionComplete() override {
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                arg->executeRead();
                                arg->decReference(); 
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                arg->decReference(); 
                            }
                        }, src1);
        dest->setValueReady(true);
        dest->decReference();
        if(dest2.has_value()){
            dest2.value()->setValueReady(true);
            dest2.value()->decReference();
        }
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode);
        s << " " << dest->getString();
        if(dest2.has_value()){
            s << ", " << dest2.value()->getString();
        }
        s << " : ";
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                s << arg->getString(); 
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                s << arg->getString(); 
                            }
                        }, src1);
         s << " | " << limb;
        return s.str();
    }

    bool hasBcSrc() const {
        bool bcSrc = false;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                bcSrc = true;
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                bcSrc = false;
                            }
                        }, src1);
        return bcSrc;
    }

    BaseConversionRegister::PhysicalID_t getBcSrcPhyID() const {
        BaseConversionRegister::PhysicalID_t id = -1;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                id = arg->getPhyID();
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                            }
                        }, src1);
        return id;
    }

   std::vector<std::shared_ptr<CinnamonInstruction>> splitInstruction() const {

        std::vector<std::shared_ptr<CinnamonInstruction>> split;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 

                                auto fwReg1 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 5); // XXX: Change This
                                auto bcReadInstruction = std::make_shared<CinnamonBcReadInstruction>(OpCode::BcR,fwReg1,arg,limb);
                                fwReg1->incReference();
                                auto nttInstruction = std::make_shared<CinnamonNttInstruction>(OpCode::Ntt,dest,fwReg1,limb);
                                fwReg1->incReference();
                                split.push_back(bcReadInstruction);
                                split.push_back(nttInstruction);
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                auto nttInstruction = std::make_shared<CinnamonNttInstruction>(OpCode::Ntt,dest,arg,limb);
                                split.push_back(nttInstruction);
                            }
                        }, src1);
        return split;
    }

};

class CinnamonInttInstruction : public CinnamonInstruction {

    std::variant<std::shared_ptr<PhysicalRegister>,std::shared_ptr<BaseConversionRegister>> dest;
    std::shared_ptr<PhysicalRegister> src1;
    LimbID_t limb;
    public:
    CinnamonInttInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Int:
            break;
            default:
                throw std::invalid_argument("Invalid Int Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    CinnamonInttInstruction(const OpCode opCode,  const std::shared_ptr<BaseConversionRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Int:
            break;
            default:
                throw std::invalid_argument("Invalid Ntt Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        bool ready = false;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                ready = arg->hasPhysicalID(); 
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                 ready = true; 
                            }
                        }, dest);
        return ready && src1->getValueReady();
        }

    void setExecutionComplete() override {
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                arg->executeWrite();
                                arg->decReference(); 
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                arg->setValueReady(true);
                                arg->decReference(); 
                            }
                        }, dest);
        src1->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " "; 
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                s << arg->getString(); 
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                s << arg->getString(); 
                            }
                        }, dest);
        s << " : " << src1->getString() << " | " << limb;
        return s.str();
    }

   bool hasBcDest() const {
        bool bcDest = false;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                bcDest = true;
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                bcDest = false;
                            }
                        }, dest);
        return bcDest;
    }

    BaseConversionRegister::PhysicalID_t getBcDestPhyID() const {
        BaseConversionRegister::PhysicalID_t id = -1;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                id = arg->getPhyID();
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                            }
                        }, dest);
        return id;
    }

   std::vector<std::shared_ptr<CinnamonInstruction>> splitInstruction() const {

        std::vector<std::shared_ptr<CinnamonInstruction>> split;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 

                                auto fwReg1 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 5); // XXX: Change This
                                auto inttInstruction = std::make_shared<CinnamonInttInstruction>(OpCode::Int,fwReg1,src1,limb);
                                fwReg1->incReference();
                                auto bcWriteInstruction = std::make_shared<CinnamonBcWriteInstruction>(OpCode::BcW,arg,fwReg1,limb);
                                fwReg1->incReference();
                                split.push_back(inttInstruction);
                                split.push_back(bcWriteInstruction);
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                auto inttInstruction = std::make_shared<CinnamonInttInstruction>(OpCode::Int,arg,src1,limb);
                                split.push_back(inttInstruction);
                            }
                        }, dest);
        return split;
    }

};

class CinnamonUnOpInstruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    std::shared_ptr<PhysicalRegister> src1;
    std::int32_t rotIndex;
    LimbID_t limb;
    public:
    CinnamonUnOpInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Neg:
            case OpCode::Ntt:
            case OpCode::Int:
            case OpCode::Con:
            case OpCode::Div:
            break;
            default:
                throw std::invalid_argument("Invalid UnOp Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    CinnamonUnOpInstruction(const OpCode opCode, const std::int32_t rotIndex, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), rotIndex(rotIndex), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Rot:
            break;
            default:
                throw std::invalid_argument("Invalid BinOp Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return src1->getValueReady();
    }

    void setExecutionComplete() override {
        src1->decReference();
        dest->setValueReady(true);
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode);
        if(opCode == OpCode::Rot){
            s << " " << rotIndex;
        }
        s << " " << dest->getString() << " : " << src1->getString() << " | " << limb;
        return s.str();
    }
};

class CinnamonNoOpInstruction : public CinnamonInstruction {

    public:
    CinnamonNoOpInstruction() :CinnamonInstruction(OpCode::Nop) {
    };

    bool allOperandsReady() const override {
        return true;
    }

    void setExecutionComplete() override {
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode);
        return s.str();
    }
};

class CinnamonEvgInstruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    LimbID_t limb;
    public:

    CinnamonEvgInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const LimbID_t limb) : dest(dest), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::EvkGen:
            break;
            default:
                throw std::invalid_argument("Invalid BinOp Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return true;
    }

    void setExecutionComplete() override {
        dest->setValueReady(true);
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode);
        s << " " << dest->getString() << " : " << " | " << limb;
        return s.str();
    }
};


class CinnamonSuDInstruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    std::shared_ptr<PhysicalRegister> src1;
    std::variant<std::shared_ptr<PhysicalRegister>,std::shared_ptr<BaseConversionRegister>> src2;
    LimbID_t limb;
    public:
    CinnamonSuDInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const std::shared_ptr<PhysicalRegister> & src2, const LimbID_t limb) : dest(dest), src1(src1), src2(src2), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::SuD:
            break;
            default:
                throw std::invalid_argument("Invalid SuD Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };
    CinnamonSuDInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const std::shared_ptr<BaseConversionRegister> & src2, const LimbID_t limb) : dest(dest), src1(src1), src2(src2), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::SuD:
            break;
            default:
                throw std::invalid_argument("Invalid SuD Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        bool ready = false;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                ready = arg->getValueReady(); 
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                 ready = arg->getValueReady(); 
                            }
                        }, src2);
        return ready && src1->getValueReady();
    }

    void setExecutionComplete() override {
        src1->decReference();
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                arg->executeRead();
                                arg->decReference(); 
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                arg->decReference(); 
                            }
                        }, src2);
        dest->setValueReady(true);
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString() << " : " << src1->getString() << ", "; 
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                s << arg->getString();
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                s << arg->getString();
                            }
                        }, src2);
        s << " | " << limb;
        return s.str();
    }

    bool hasBcSrc() const {
        bool bcSrc = false;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                bcSrc = true;
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                                bcSrc = false;
                            }
                        }, src2);
        return bcSrc;
    }

    BaseConversionRegister::PhysicalID_t getBcSrcPhyID() const {
        BaseConversionRegister::PhysicalID_t id = -1;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                id = arg->getPhyID();
                            },
                            [&](const std::shared_ptr<PhysicalRegister>&arg){
                            }
                        }, src2);
        return id;
    }

    std::vector<std::shared_ptr<CinnamonInstruction>> splitInstruction() const {

        std::vector<std::shared_ptr<CinnamonInstruction>> split;
        auto fwReg1 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 5); // XXX: Change This
        // std::shared_ptr<CinnamonNttInstruction> nttInstruction = nullptr;
        std::visit(overloaded{
                            [&](const std::shared_ptr<BaseConversionRegister >&arg){ 
                                    auto fwReg0 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 7); // XXX: Change This
                                    auto bcReadInstruction = std::make_shared<CinnamonBcReadInstruction>(OpCode::BcR,fwReg0,arg,limb);
                                    fwReg0->incReference();
                                    auto nttInstruction = std::make_shared<CinnamonNttInstruction>(OpCode::Ntt,fwReg1,fwReg0,limb);
                                    fwReg0->incReference();
                                    fwReg1->incReference();
                                    split.push_back(bcReadInstruction);
                                    split.push_back(nttInstruction);
                                },
                                [&](const std::shared_ptr<PhysicalRegister>&arg){
                                    auto nttInstruction = std::make_shared<CinnamonNttInstruction>(OpCode::Ntt,fwReg1,arg,limb);
                                    fwReg1->incReference();
                                    split.push_back(nttInstruction);
                            }
                        }, src2);
        auto fwReg2 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 5); // XXX: Change This
        auto subInstruction = std::make_shared<CinnamonBinOpInstruction>(OpCode::Sub,fwReg2,src1,fwReg1,limb);
        fwReg1->incReference();
        fwReg2->incReference();
        split.push_back(subInstruction);
        auto divInstruction = std::make_shared<CinnamonUnOpInstruction>(OpCode::Div,dest,fwReg2,limb);
        fwReg2->incReference();
        split.push_back(divInstruction);
        return split;
        
    }
};

class CinnamonBciInstruction : public CinnamonInstruction {

    std::shared_ptr<BaseConversionRegister> dest;
    LimbID_t baseConversionLimbID;
    public:
    CinnamonBciInstruction(const OpCode opCode, const std::shared_ptr<BaseConversionRegister> & dest) : dest(dest), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Bci:
            break;
            default:
                throw std::invalid_argument("Invalid Bci Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return true; // Operands of the Base Conversion Init instruction are always ready
        // TODO: Load these operands too
    }

    void setExecutionComplete() override {
        dest->resetPhyID();
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString();
        return s.str();
    }

    void setPhyiscalBaseConversionRegister(const BaseConversionRegister::PhysicalID_t phyID){
        dest->setPhyID(phyID);
    }

    bool isCompleted() const {
        return dest->isCompleted();
    }

};

class CinnamonBcwInstruction : public CinnamonInstruction {

    std::shared_ptr<BaseConversionRegister> dest;
    std::shared_ptr<PhysicalRegister> src1;
    LimbID_t limb;
    public:
    CinnamonBcwInstruction(const OpCode opCode, const std::shared_ptr<BaseConversionRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::BcW:
            break;
            default:
                throw std::invalid_argument("Invalid Bcw Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return dest->hasPhysicalID() && src1->getValueReady();
    }

    void setExecutionComplete() override {
        src1->decReference();
        dest->executeWrite();
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString() << " : " << src1->getString() << " | " << limb;
        return s.str();
    }

    BaseConversionRegister::PhysicalID_t getBcDestPhyID() const {
        return dest->getPhyID();
    }

};

class CinnamonPl1Instruction : public CinnamonInstruction {

    std::shared_ptr<BaseConversionRegister> dest;
    std::shared_ptr<PhysicalRegister> src1;
    LimbID_t limb;
    public:
    CinnamonPl1Instruction(const OpCode opCode, const std::shared_ptr<BaseConversionRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dest(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Pl1:
            break;
            default:
                throw std::invalid_argument("Invalid Pl1 Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return dest->hasPhysicalID() && src1->getValueReady();
    }

    void setExecutionComplete() override {
        src1->decReference();
        dest->executeWrite();
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString() << " : " << src1->getString() << " | " << limb;
        return s.str();
    }

    BaseConversionRegister::PhysicalID_t getBcDestPhyID() const {
        return dest->getPhyID();
    }

    std::vector<std::shared_ptr<CinnamonInstruction>> splitInstruction() const {

        auto fwReg1 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto inttInstruction = std::make_shared<CinnamonInttInstruction>(OpCode::Int,fwReg1,src1,limb);
        fwReg1->incReference();
        
        auto bcwInstruction = std::make_shared<CinnamonBcWriteInstruction>(OpCode::BcW,dest,fwReg1,limb);
        fwReg1->incReference();
        
        return std::vector<std::shared_ptr<CinnamonInstruction>>{inttInstruction,bcwInstruction};
    }

};

class CinnamonPl2Instruction : public CinnamonInstruction {

    std::shared_ptr<BaseConversionRegister> dest1;
    std::shared_ptr<PhysicalRegister> dest2;
    std::shared_ptr<BaseConversionRegister> src1;
    std::shared_ptr<PhysicalRegister> src2;
    LimbID_t limb;
    LimbID_t baseConversionLimbID;
    public:
    CinnamonPl2Instruction(const OpCode opCode, const std::shared_ptr<BaseConversionRegister> & dest1, const std::shared_ptr<PhysicalRegister> & dest2, const std::shared_ptr<BaseConversionRegister> & src1, LimbID_t baseConversionLimbId, const std::shared_ptr<PhysicalRegister> & src2, const LimbID_t limb) : dest1(dest1), dest2(dest2), src1(src1), baseConversionLimbID(baseConversionLimbID), src2(src2), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Pl2:
            break;
            default:
                throw std::invalid_argument("Invalid Pl2 Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return dest1->hasPhysicalID() && src1->getValueReady() && src2->getValueReady();
    }

    void setExecutionComplete() override {
        src1->executeRead();
        src1->decReference();

        src2->decReference();

        dest1->executeWrite();
        dest1->decReference();

        dest2->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest1->getString() << "," << dest2->getString() << " : " << src1->getString() << " | " << limb;
        return s.str();
    }

    BaseConversionRegister::PhysicalID_t getBcDestPhyID() const {
        return dest1->getPhyID();
    }

    std::vector<std::shared_ptr<CinnamonInstruction>> splitInstruction() const {

        auto fwReg1 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto nttInstruction = std::make_shared<CinnamonNttInstruction>(OpCode::Ntt,fwReg1,dest2,src1,limb);
        fwReg1->incReference();
        
        auto fwReg2 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto mulInstruction = std::make_shared<CinnamonBinOpInstruction>(OpCode::Mul,fwReg2,fwReg1,src2,limb);
        fwReg1->incReference();
        fwReg2->incReference();

        auto fwReg3 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto inttInstruction = std::make_shared<CinnamonInttInstruction>(OpCode::Int,fwReg3,fwReg2,limb);
        fwReg2->incReference();
        fwReg3->incReference();

        auto bcwInstruction = std::make_shared<CinnamonBcWriteInstruction>(OpCode::BcW,dest1,fwReg3,limb);
        fwReg3->incReference();
        
        return std::vector<std::shared_ptr<CinnamonInstruction>>{nttInstruction,mulInstruction,inttInstruction,bcwInstruction};
    }

};

class CinnamonPl3Instruction : public CinnamonInstruction {

    std::shared_ptr<BaseConversionRegister> dest;
    std::shared_ptr<PhysicalRegister> src1;
    std::shared_ptr<PhysicalRegister> src2;
    LimbID_t limb;
    LimbID_t baseConversionLimbID;
    public:
     CinnamonPl3Instruction(const OpCode opCode, const std::shared_ptr<BaseConversionRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const std::shared_ptr<PhysicalRegister> & src2, const LimbID_t limb) : dest(dest), src1(src1), src2(src2), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Pl3:
            break;
            default:
                throw std::invalid_argument("Invalid Pl2 Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return dest->hasPhysicalID() && src1->getValueReady() && src2->getValueReady();
    }

    void setExecutionComplete() override {
        src1->decReference();
        src2->decReference();
        dest->executeWrite();
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString() << " : " << src1->getString() << "," << src2->getString() <<  " | " << limb;
        return s.str();
    }

    BaseConversionRegister::PhysicalID_t getBcDestPhyID() const {
        return dest->getPhyID();
    }

    std::vector<std::shared_ptr<CinnamonInstruction>> splitInstruction() const {

        auto fwReg1 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto mulInstruction = std::make_shared<CinnamonBinOpInstruction>(OpCode::Mul,fwReg1,src1,src2,limb);
        fwReg1->incReference();

        auto fwReg2 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto inttInstruction = std::make_shared<CinnamonInttInstruction>(OpCode::Int,fwReg2,fwReg1,limb);
        fwReg1->incReference();
        fwReg2->incReference();

        auto bcwInstruction = std::make_shared<CinnamonBcWriteInstruction>(OpCode::BcW,dest,fwReg2,limb);
        fwReg2->incReference();
        
        return std::vector<std::shared_ptr<CinnamonInstruction>>{mulInstruction,inttInstruction,bcwInstruction};
    }

};

class CinnamonPl4Instruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    std::shared_ptr<BaseConversionRegister> src1;
    std::shared_ptr<PhysicalRegister> src2;
    std::shared_ptr<PhysicalRegister> src3;
    LimbID_t limb;
    LimbID_t baseConversionLimbID;
    public:
    CinnamonPl4Instruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<BaseConversionRegister> & src1, LimbID_t baseConversionLimbId, const std::shared_ptr<PhysicalRegister> & src2, const std::shared_ptr<PhysicalRegister> & src3, const LimbID_t limb) : dest(dest), src1(src1), baseConversionLimbID(baseConversionLimbID), src2(src2), src3(src3), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Pl4:
            break;
            default:
                throw std::invalid_argument("Invalid Pl4 Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        return src1->getValueReady() && src2->getValueReady() && src3->getValueReady();
    }

    void setExecutionComplete() override {
        src1->executeRead();
        src1->decReference();

        src2->decReference();
        src3->decReference();

        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " " << dest->getString() << " : " << src1->getString() << ", " << src2->getString() << ", " << src3->getString() <<" | " << limb;
        return s.str();
    }

    std::vector<std::shared_ptr<CinnamonInstruction>> splitInstruction() const {

        auto fwReg1 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto bcrInstruction = std::make_shared<CinnamonBcReadInstruction>(OpCode::BcR,fwReg1,src1,limb);
        fwReg1->incReference();

        auto fwReg2 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto nttInstruction = std::make_shared<CinnamonNttInstruction>(OpCode::Ntt,fwReg2,fwReg1,limb);
        fwReg1->incReference();
        fwReg2->incReference();
        
        auto fwReg3 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto mulInstruction = std::make_shared<CinnamonBinOpInstruction>(OpCode::Mul,fwReg3,src2,src3,limb);
        fwReg3->incReference();

        auto fwReg4 = std::make_shared<PhysicalRegister>(PhysicalRegister::PhysicalRegister_t::Forwarding, 1); // XXX: Change This
        auto subInstruction = std::make_shared<CinnamonBinOpInstruction>(OpCode::Sub,fwReg4,fwReg3,fwReg2,limb);
        fwReg2->incReference();
        fwReg3->incReference();
        fwReg4->incReference();

        auto divInstruction = std::make_shared<CinnamonUnOpInstruction>(OpCode::Div,dest,fwReg4,limb);
        fwReg4->incReference();
        
        return std::vector<std::shared_ptr<CinnamonInstruction>>{bcrInstruction, nttInstruction, mulInstruction,subInstruction,divInstruction};
    }

    BaseConversionRegister::PhysicalID_t getBcSrcPhyID() const {
        return src1->getPhyID();
    }

};

class CinnamonRsvInstruction : public CinnamonInstruction {

    std::vector<std::shared_ptr<PhysicalRegister>> dests;
    std::shared_ptr<PhysicalRegister> src1;
    LimbID_t limb;
    public:
    CinnamonRsvInstruction(const OpCode opCode, const std::vector<std::shared_ptr<PhysicalRegister>> & dest, const std::shared_ptr<PhysicalRegister> & src1, const LimbID_t limb) : dests(dest), src1(src1), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Rsi:
            case OpCode::Rsv:
            break;
            default:
                throw std::invalid_argument("Invalid Rsv Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        if(src1 == nullptr){
            return true;
        }
        return src1->getValueReady(); 
    }

    void setExecutionComplete() override {
        if(src1){
            src1->decReference();
        }
        for(auto &dest: dests){
            dest->decReference();
        }

        if(opCode == OpCode::Rsi){
            return;
        }

        // How do we know that all the writes of the resolve instruction have been 
        // completed?

        // HACK:: Set one destination ready on each Rsv instruction
        // This hack works since we know the number of writes is equal to the number of dests
        // so, we ensure that one write sets one destintation to ready. Then all dests will be
        // ready when all writes are complete.

        for(auto &dest: dests){
            if(dest->getValueReady() != true){
                dest->setValueReady(true);
                return;
            }
        }
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " {";
        for(auto &dest: dests){
            s << dest->getString() << ", ";
        }
        s << "} : ";
        if(src1 != nullptr) {
            s << src1->getString();
        }
        if(opCode != OpCode::Rsi){
            s << " | " << limb;
        }
        return s.str();
    }

};

class CinnamonModInstruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    std::vector<std::shared_ptr<PhysicalRegister>> srcs;
    LimbID_t limb;
    public:
    CinnamonModInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::vector<std::shared_ptr<PhysicalRegister>> & srcs, const LimbID_t limb) : dest(dest), srcs(srcs), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Mod:
            break;
            default:
                throw std::invalid_argument("Invalid Mod Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        for(auto &src: srcs){
            if(src->getValueReady() == false){
                return false;
            }
        }
        return true;
    }

    void setExecutionComplete() override {
        for(auto &src: srcs){
            src->decReference();
        }

        dest->setValueReady(true);
        dest->decReference();
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode) << " {";
        s << dest->getString() << ":";
        s << "{";
        for(auto &src: srcs) {
            s << src->getString() << ", ";
        }
        s << "} | " << limb;
        return s.str();
    }

};

class CinnamonDisInstruction : public CinnamonInstruction {

    std::shared_ptr<PhysicalRegister> dest;
    std::shared_ptr<PhysicalRegister> src1;
    std::uint64_t syncID_;
    std::uint64_t syncSize_;
    std::optional<LimbID_t> limb;
    public:
    CinnamonDisInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const uint64_t syncID, const uint64_t syncSize) : dest(dest), src1(src1), syncID_(syncID), syncSize_(syncSize), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Rcv:
            case OpCode::Dis:
            break;
            default:
                throw std::invalid_argument("Invalid Dis Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    CinnamonDisInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const uint64_t syncID, const uint64_t syncSize, const LimbID_t limb) : dest(dest), src1(src1), syncID_(syncID), syncSize_(syncSize), limb(limb), CinnamonInstruction(opCode) {
        switch(opCode){ 
            case OpCode::Joi:
            break;
            default:
                throw std::invalid_argument("Invalid Dis Instruction with OpCode : " + getOpCodeString(opCode));
        }
    };

    bool allOperandsReady() const override {
        if(opCode == OpCode::Dis){
            return src1->getValueReady();
        } else if(opCode == OpCode::Joi){
            if(src1 != nullptr){
                return src1->getValueReady();
            } else {
                return true;
            }
        } 
        
        return true;
    }

    void setExecutionComplete() override {
        if(dest != nullptr ){
            dest->setValueReady(true);
            dest->decReference();
        }
        if(src1 != nullptr) {
            src1->decReference();
        }
    }

    std::string getString() const override {
        std::stringstream s;
        s << getOpCodeString(opCode);
        s << " @ " << syncID_ << ":" << syncSize_ << " ";
        if(dest){
            s << dest->getString();
        }
        s << " : ";
        if(src1){
            s << src1->getString();
        }
        if(limb.has_value()){
            s << " | " << limb.value();
        }
        return s.str();
    }

    auto syncID() const {
        return syncID_;
    }

    auto syncSize() const {
        return syncSize_;
    }

    bool hasSource() const {
        return src1 != nullptr;
    }
    
    bool hasDest() const {
        return dest != nullptr;
    }
};

// class CinnamonJoiInstruction : public CinnamonInstruction {

//     std::shared_ptr<PhysicalRegister> dest;
//     std::shared_ptr<PhysicalRegister> src1;
//     std::uint64_t syncID;
//     LimbID_t limb;
//     public:
//     CinnamonJoiInstruction(const OpCode opCode, const std::shared_ptr<PhysicalRegister> & dest, const std::shared_ptr<PhysicalRegister> & src1, const uint64_t syncID, const LimbID_t limb) : dest(dest), src1(src1), syncID(syncID), limb(limb), CinnamonInstruction(opCode) {
//         switch(opCode){ 
//             case OpCode::Joi:
//             break;
//             default:
//                 throw std::invalid_argument("Invalid Joi Instruction with OpCode : " + getOpCodeString(opCode));
//         }
//     };

//     bool allOperandsReady() const override {
//         return src1->getValueReady();
//     }

//     void setExecutionComplete() override {
//         if(dest){
//             dest->setValueReady(true);
//             dest->decReference();
//         } 
//         src1->decReference();
//     }

//     std::string getString() const override {
//         std::stringstream s;
//         s << getOpCodeString(opCode);
//         s << " @ " << syncID << " ";
//         if(dest){
//             s << dest->getString();
//         }
//         s << " : " << src1->getString() << " | " << limb;
//         return s.str();
//     }
// };




#if 0

// TODO: Add NTT instruction
typedef enum {
	LOAD,
	STORE,
	ADD,
    MULT
} CinnamonInstructionOpCode;

std::string opToString( int op );

typedef enum {
    NONE, 
    FETCH,
    ISSUE,
    EXECUTE,
    COMMIT
} CinnamonInstructionState;

CinnamonInstructionOpCode parseOpCode(std::string opCode);

class CinnamonInstruction {
    protected:
        uint64_t id_;
        const CinnamonInstructionOpCode op_;
        CinnamonInstructionState state_;
       unsigned virtualAddress;
       unsigned physicalAddress;

    public:
        CinnamonInstruction(uint64_t id, const CinnamonInstructionOpCode op) :
		    id_(id), op_(op), state_(NONE), virtualAddress(0), physicalAddress(0) {
        }

        uint64_t id();
        uint64_t issuedAtCycle();
        CinnamonInstructionOpCode op() const;
        //unsigned getVAddr() const;

        void setState(CinnamonInstructionState);
        CinnamonInstructionState state(); 

        // bool operator ==(CinnamonInstr* instr) {
		// 	return cycles == instr->cycles && op == instr->op;
		// }
};

#if 0
class CinnamonLoadInstr : public CinnamonInstr {
    protected: 
        uint64_t src;
        uint64_t dest;
        uint64_t size;

    public: 
        CinnamonLoadInstr(
            uint64_t id,  
            uint64_t cycles, 
            uint64_t src,
            uint64_t dest,
            uint64_t size) : 
            src(src),
            dest(dest),
            size(size),
            CinnamonInstr(id, cycles, LOAD) {
        }

        uint64_t getSrc();
        uint64_t getDest();
        uint64_t getSize();

        bool operator ==(CinnamonInstr* instr) {
            try {
                CinnamonLoadInstr* loadInstr = static_cast<CinnamonLoadInstr*>(instr);
                return src == loadInstr->src && 
                        dest == loadInstr->dest && 
                        size == loadInstr->size && 
                        cycles == loadInstr->cycles &&
                        op == loadInstr->op;
            } catch(...) {
                return false; 
            }
        }
};

class CinnamonStoreInstr : public CinnamonInstr {
    protected: 
        uint64_t src;
        uint64_t dest;
        uint64_t size;

    public: 
        CinnamonStoreInstr(
            uint64_t id,  
            uint64_t cycles, 
            uint64_t src,
            uint64_t dest,
            uint64_t size) : 
            src(src),
            dest(dest),
            size(size),
            CinnamonInstr(id, cycles, STORE) {
        }

        uint64_t getSrc();
        uint64_t getDest();
        uint64_t getSize();

        bool operator ==(CinnamonInstr* instr) {
            try {
                CinnamonStoreInstr* storeInstr = static_cast<CinnamonStoreInstr*>(instr);
                return src == storeInstr->src && 
                        dest == storeInstr->dest && 
                        size == storeInstr->size && 
                        cycles == storeInstr->cycles &&
                        op == storeInstr->op;
            } catch(...) {
                return false; 
            }
        }
};

#endif

class CinnamonAddInstruction : public CinnamonInstruction {
    protected: 
        uint64_t src1_;
        uint64_t src2_;
        uint64_t dest_;
//        uint64_t lifetime;

    public: 
        CinnamonAddInstruction(
            uint64_t id,  
            uint64_t src1,
            uint64_t src2,
            uint64_t dest) : 
            src1_(src1),
            src2_(src2),
            dest_(dest),
            CinnamonInstruction(id, ADD) {
        }
        
        uint64_t src1();
        uint64_t src2();
        uint64_t dest();

//        void setLifetime(uint64_t lifetime);
//        uint64_t getLifetime();

        // bool operator ==(CinnamonInstr* instr) {
        //     try {
        //         CinnamonAddInstr* addInstr = static_cast<CinnamonAddInstr*>(instr);
        //         return src1 == addInstr->src1 && 
        //                 src2 == addInstr->src2 && 
        //                 dest == addInstr->dest && 
        //                 cycles == addInstr->cycles &&
        //                 op == addInstr->op;
        //     } catch (...) {
        //         return false; 
        //     }
        // }
};

#if 0
class CinnamonMultInstr : public CinnamonInstr {
    protected: 
        uint64_t src1;
        uint64_t src2;
        uint64_t dest;
        uint64_t lifetime;

    public: 
        CinnamonMultInstr(
            uint64_t id,  
            uint64_t cycles, 
            uint64_t src1,
            uint64_t src2,
            uint64_t dest) : 
            src1(src1),
            src2(src2),
            dest(dest),
            CinnamonInstr(id, cycles, ADD) {
        }
        
        uint64_t getSrc1();
        uint64_t getSrc2();
        uint64_t getDest();

        void setLifetime(uint64_t lifetime);
        uint64_t getLifetime();

        bool operator ==(CinnamonInstr* instr) {
            try {
                CinnamonMultInstr* addInstr = static_cast<CinnamonMultInstr*>(instr);
                return src1 == addInstr->src1 && 
                        src2 == addInstr->src2 && 
                        dest == addInstr->dest && 
                        cycles == addInstr->cycles &&
                        op == addInstr->op;
            } catch (...) {
                return false; 
            }
        }
};
#endif
#endif

} // Namespace Cinnamon
} // Namespace SST
#endif
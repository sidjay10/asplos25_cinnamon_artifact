#ifndef CINNAMON_PHYSICAL_REGISTER_H
#define CINNAMON_PHYSICAL_REGISTER_H

#include <cassert>
#include <optional>
#include <sstream>
#include <string>


namespace SST {
namespace Cinnamon {

class CinnamonChiplet;

using PhysicalRegisterID_t = std::uint16_t;
class PhysicalRegister {
public:
    enum class PhysicalRegister_t {
        Vector,
        Scalar,
        Forwarding
    };

private:
    CinnamonChiplet *pe;
    PhysicalRegister_t type;
    PhysicalRegisterID_t id;
    bool valueReady;
    bool isFree;
    // bool mapped;
    std::optional<std::uint16_t> mappedVirtualReg;
    std::int16_t references;

    void addToFreeListIfFree();

public:
    PhysicalRegister(CinnamonChiplet *pe, const PhysicalRegister_t type, const std::uint16_t id) : pe(pe), type(type), id(id), valueReady(false), isFree(true), references(0){};
    PhysicalRegister(const PhysicalRegister_t type, const std::uint16_t id) : pe(nullptr), type(type), id(id), valueReady(false), isFree(true), references(0) { assert(type == PhysicalRegister_t::Forwarding); };
    void setValueReady(bool b) { valueReady = b; }
    bool getValueReady() const { return valueReady; }
    PhysicalRegisterID_t getID() const { return id; }

    std::int16_t numReferences() const {
        return references;
    }

    void incReference() {
        references++;
        // isFree = false;
    }

    void decReference() {
        references--;
        assert(references >= 0);
        addToFreeListIfFree();
        // if(!isFree && references == 0){
        //   isFree = true;
        //   valueReady = false;
        // }
    }

    std::string getString() const {
        std::stringstream s;
        if (type == PhysicalRegister_t::Vector) {
            s << "R";
        } else if (type == PhysicalRegister_t::Scalar) {
            s << "S";
        } else if (type == PhysicalRegister_t::Forwarding) {
            s << "F";
        } else {
            assert(0 && "Invalid PhysicalRegister_t");
        }
        s << id;
        return s.str();
    }
};

} // namespace Cinnamon
} // namespace SST
#endif // CINNAMON_PHYSICAL_REGISTER_H
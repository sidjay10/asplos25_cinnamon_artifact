#include <cassert>
#include <optional>
#include <sstream>
#include <string>

#include "physicalRegister.h"
#include "chiplet.h"

// class CinnamonChiplet;

namespace SST {
namespace Cinnamon {

void PhysicalRegister::addToFreeListIfFree() {
    // if(mappedVirtualReg.has_value()){
    //   return;
    // }
    if (references > 0) {
        return;
    }
    if (type == PhysicalRegister_t::Vector) {
        pe->freeVectorRegisters.push(id);
    } else if (type == PhysicalRegister_t::Scalar) {
        pe->freeScalarRegisters.push(id);
    } else if (type == PhysicalRegister_t::Forwarding) {
        ;
    } else {
        throw std::runtime_error("Invalid PhysicalRegister_t");
    }
}

} // namespace Cinnamon
} // namespace SST
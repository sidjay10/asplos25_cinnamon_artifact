#include <cassert>
#include <optional>
#include <sstream>
#include <string>

#include "baseConversionRegister.h"
#include "chiplet.h"

namespace SST {
namespace Cinnamon {

void BaseConversionRegister::executeWrite() {
    assert(phyID.has_value());
    writesRemaining--;
    assert(writesRemaining >= 0);
    if (writesRemaining == 0) {
        valueReady = true;
    }
}

void BaseConversionRegister::executeRead() {
    assert(phyID.has_value());
    assert(valueReady);
    readsRemaining--;
    assert(readsRemaining >= 0);
}

void BaseConversionRegister::decReference() {
    references--;
    assert(references >= 0);
    if (references == 0) {
        assert(writesRemaining == 0);
        assert(readsRemaining == 0);
        pe->freeBaseConversionVirtualRegisters.push(virtID);
        phyID.reset();
        writesRemaining = 0;
        readsRemaining = 0;
        valueReady = false;
    }
}

} // namespace Cinnamon
} // namespace SST
#ifndef CINNAMON_BASECONVERSION_REGISTER_H
#define CINNAMON_BASECONVERSION_REGISTER_H

#include <cassert>
#include <optional>
#include <sstream>
#include <string>


namespace SST {
namespace Cinnamon{

class CinnamonChiplet;

class BaseConversionRegister {
public:
    using VirtualID_t = std::uint16_t;
    using PhysicalID_t = std::uint16_t;

private:
    CinnamonChiplet *pe;
    VirtualID_t virtID;
    std::optional<PhysicalID_t> phyID;
    std::int16_t writesRemaining;
    std::int16_t readsRemaining;
    bool valueReady;
    std::uint16_t references;

public:
    BaseConversionRegister(CinnamonChiplet *pe, const VirtualID_t virtID) : pe(pe), virtID(virtID), readsRemaining(0), writesRemaining(0), valueReady(false), references(0){};
    void setValueReady(bool b) { valueReady = b; }
    bool getValueReady() const { return valueReady; }
    VirtualID_t getVirtID() const { return virtID; }
    PhysicalID_t getPhyID() const { return phyID.value(); }

    bool hasPhysicalID() const { return phyID.has_value(); }

    void setPhyID(const PhysicalID_t id) {
        phyID = id;
    }

    void setReadsRemaining(const std::int16_t val) {
        readsRemaining = val;
    }

    void setWritesRemaining(const std::int16_t val) {
        writesRemaining = val;
    }

    void executeWrite();

    void executeRead();

    void incReference() {
        references++;
        // isFree = false;
    }

    void decReference();

    bool isCompleted() const {
        return readsRemaining == 0 && writesRemaining == 0;
    }

    void resetPhyID() {
        phyID.reset();
    }

    std::string getString() const {
        std::stringstream s;
        if (phyID.has_value()) {
            s << "B" << phyID.value();
        } else {
            s << "BV" << virtID;
        }
        return s.str();
    }
};
} // namespace Cinnamon
} // namespace SST
#endif // CINNAMON_BASECONVERSION_REGISTER_H
#ifndef _CINNAMON_LATENCY_H
#define _CINNAMON_LATENCY_H

#include <sst/core/clock.h>

namespace SST {
namespace Cinnamon {
struct Latency {
    SST::Cycle_t Add;
    SST::Cycle_t Mul;
    SST::Cycle_t Rsv;
    SST::Cycle_t Mod;
    SST::Cycle_t Evg;
    SST::Cycle_t NTT_butterfly;
    SST::Cycle_t NTT_one_stage;
    SST::Cycle_t NTT;
    SST::Cycle_t Transpose;
    SST::Cycle_t Rot_one_stage;
    SST::Cycle_t Rot;
    SST::Cycle_t Bcu_write;
    SST::Cycle_t Bcu_read;

};
} // namespace Cinnamon

} // namespace SST

#endif //_CINNAMON_LATENCY_H
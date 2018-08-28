#ifndef CROWN_CORE_STAKEMINER_H
#define CROWN_CORE_STAKEMINER_H

#include <cstdint>

class Kernel;
class uint256;

bool SearchTimeSpan(Kernel& kernel, uint64_t nTimeStart, uint64_t nTimeEnd, const uint256& nTarget);

#endif //CROWN_CORE_STAKEMINER_H

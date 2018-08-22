#ifndef CROWN_CORE_STAKEMINER_H
#define CROWN_CORE_STAKEMINER_H

#include <cstdint>
#include "../uint256.h"
#include "../arith_uint256.h"

class CKernel;

bool SearchTimeSpan(CKernel& kernel, uint64_t nTimeStart, uint64_t nTimeEnd, const uint256& nTarget);

#endif //CROWN_CORE_STAKEMINER_H

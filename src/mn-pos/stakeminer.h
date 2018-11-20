#ifndef CROWN_CORE_STAKEMINER_H
#define CROWN_CORE_STAKEMINER_H

#include <cstdint>

class CBlock;
class Kernel;
class uint256;

bool SearchTimeSpan(Kernel& kernel, uint32_t nTimeStart, uint32_t nTimeEnd, const uint256& nTarget);
bool SignBlock(CBlock* pblock);
#endif //CROWN_CORE_STAKEMINER_H

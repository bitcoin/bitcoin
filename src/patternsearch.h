//#include "bignum.h"
#include "uint256.h"

namespace patternsearch {
    std::vector< std::pair<uint32_t,uint32_t> > pattern_search( uint256 midHash, char* scratchpad, int totalThreads );
    bool pattern_verify( uint256 midHash, uint32_t a, uint32_t b );
}


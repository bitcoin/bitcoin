#include "omnicore/convert.h"

#include <cmath>
#include <stdint.h>

namespace mastercore
{
    
uint64_t rounduint64(long double ld)
{
    return static_cast<uint64_t>(roundl(fabsl(ld)));
}

} // namespace mastercore

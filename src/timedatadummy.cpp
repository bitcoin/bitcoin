#include <stdint.h>

// needed when linking transaction.cpp, since we are not going to pull real GetAdjustedTime from timedata.cpp
int64_t GetAdjustedTime()
{
    return 0;
}

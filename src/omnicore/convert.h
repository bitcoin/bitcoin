#ifndef BITCOIN_OMNICORE_CONVERT_H
#define BITCOIN_OMNICORE_CONVERT_H

#include <stdint.h>

namespace mastercore
{

/**
 * Converts numbers to 64 bit wide unsigned integer whereby
 * any signedness is ignored. If absolute value of the number
 * is greater or equal than .5, then the result is rounded
 * up and down otherwise.
 */
uint64_t rounduint64(long double);
}


#endif // BITCOIN_OMNICORE_CONVERT_H

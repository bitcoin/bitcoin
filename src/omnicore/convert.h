#ifndef OMNICORE_CONVERT_H
#define OMNICORE_CONVERT_H

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

/**
 * Swaps byte order on little-endian systems and does nothing 
 * otherwise. swapByteOrder cycles on LE systems.
 */
void swapByteOrder16(uint16_t&);
void swapByteOrder32(uint32_t&);
void swapByteOrder64(uint64_t&);

}


#endif // OMNICORE_CONVERT_H

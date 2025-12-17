#ifndef SIMPLICITY_TAPTWEAK_H
#define SIMPLICITY_TAPTWEAK_H

#include "frame.h"

/* This is a generic taptweak jet implementation parameterized by the tag used in the hash.
 * It is designed to be specialized to implement slightly different taptweak operations for Bitcoin and Elements.
 *
 * PUBKEY * TWO^256 |- PUBKEY
 *
 * Precondition: unsigned char[tagLen] tag
 */
bool simplicity_generic_taptweak(frameItem* dst, frameItem *src, const unsigned char *tag, size_t tagLen);

#endif

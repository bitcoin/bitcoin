// Copyright (c) 2016-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_INTERFACES_H
#define BITCOIN_CONSENSUS_INTERFACES_H

#include <stdint.h>

typedef const unsigned char* (*GetHashFn)(const void* indexObject);
typedef const void* (*GetAncestorFn)(const void* indexObject, int64_t height);
typedef int64_t (*GetHeightFn)(const void* indexObject);
typedef int32_t (*GetVersionFn)(const void* indexObject);
typedef uint32_t (*GetTimeFn)(const void* indexObject);
typedef uint32_t (*GetBitsFn)(const void* indexObject);

// Potential optimizations:

/**
 * Some implementations may chose to store a pointer to the previous
 * block instead of calling AncestorFn, trading memory for
 * validation speed.
 */
typedef const void* (*GetPrevFn)(const void* indexObject);
/**
 * Some implementations may chose to cache the Median Time Past.
 */
typedef int64_t (*GetMedianTimeFn)(const void* indexObject);
/**
 * While not using this, it is assumed that the caller - who is
 * responsible for all the new allocations - will free all the memory
 * (or not) of the things that have been newly created in memory (or
 * not) after the call to the exposed libbitcoinconsenus function.
 * This function is mostly here to document the fact that some storage
 * optimizations are only possible if there's a fast signaling from
 * libbitcoinconsenus when data resources that have been asked for as
 * part of the validation are no longer needed. 
 */
typedef void (*IndexDeallocatorFn)(void* indexObject);


/**
 * Collection of function pointers to interface with block index storage.
 */
struct BlockIndexInterface
{
    GetHashFn GetHash;
    GetAncestorFn GetAncestor;
    GetHeightFn GetHeight;
    GetVersionFn GetVersion;
    GetTimeFn GetTime;
    GetBitsFn GetBits;
    /**
     * Just for optimization: If this is set to NULL, Ancestor() and
     * Height() will be called instead.
     */
    GetPrevFn GetPrev;
    /**
     * Just for optimization: If this is set to NULL, Prev() and
     * Time() will be called instead.
     */
    GetMedianTimeFn GetMedianTime;
    //! TODO This is mostly here for discussion, but not used yet.
    IndexDeallocatorFn DeleteIndex;
};

#endif // BITCOIN_CONSENSUS_INTERFACES_H

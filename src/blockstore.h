#ifndef BITCOIN_BLOCKSTORE_H
#define BITCOIN_BLOCKSTORE_H

// This API is considered stable ONLY for existing bitcoin codebases,
// any futher uses are not yet supported.
// This API is subject to change dramatically overnight, do not
// depend on it for anything.

#include <boost/signals2/signal.hpp>
#include <queue>

#include "sync.h"

class CBlockStoreSignalTable
{
public:

};

class CBlockStore
{
private:
    CBlockStoreSignalTable sigtable;
public:
//Register methods

//Blockchain access methods

};

#endif

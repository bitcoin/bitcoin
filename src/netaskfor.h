// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/** Inventory request management */
#ifndef BITCOIN_NETASKFOR_H
#define BITCOIN_NETASKFOR_H

#include "protocol.h"
#include "sync.h"

#include <map>
#include <set>

class CNode;
class CNodeSignals;

namespace NetAskFor
{
/** Retry ask for inventory item to a different node if node doesn't
 * respond with data within this time (in ms)
 */
const int64_t REQUEST_TIMEOUT = 2 * 60 * 1000;

/** The maximum number of inv requests that can be in flight from a node */
static const unsigned int MAX_SETASKFOR_SZ = 10000;

/** Register node signals for inventory request handling */
void RegisterNodeSignals(CNodeSignals& nodeSignals);

/** Unregister node signals for inventory request handling */
void UnregisterNodeSignals(CNodeSignals& nodeSignals);

/** Notify that a node owns a certain inventory item,
 * also initiate a inventory item request if none exists yet for it.
 */
void AskFor(CNode *node, const CInv &inv);

/** Forget an inventory item request. This must be called from the
 *  'tx' message handler.
 */
void Completed(CNode *node, const CInv &inv);

};

#endif // BITCOIN_NETASKFOR

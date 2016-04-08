// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERNANCE_BRAIN_H
#define GOVERNANCE_BRAIN_H

//todo: which of these do we need?
#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include "governance-types.h"
#include "governance-vote.h"
#include <boost/lexical_cast.hpp>
#include "init.h"
#include <univalue.h>

class CGovernanceBrain
{
private:

public:

    bool IsPlacementValid(GovernanceType childType, GovernanceType parentType);
};

#endif
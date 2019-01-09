// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_SPECIALTX_COMMON_H
#define CROWN_PLATFORM_SPECIALTX_COMMON_H

#include "primitives/transaction.h"

class SpecTxMemPoolHandler;

class SpecTxMemPoolHandlerRegistrator
{
public:
    virtual bool RegisterHandler(TxType specialTxType, SpecTxMemPoolHandler * handler) = 0;
    virtual void UnregisterHandler(TxType specialTxType) = 0;
    virtual ~SpecTxMemPoolHandlerRegistrator() = default;
};

class SpecTxMemPoolHandler
{
public:
    virtual bool RegisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator) = 0;
    virtual void UnregisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator) = 0;
    virtual bool ExistsTxConflict(const CTransaction & tx) const = 0;
    virtual bool AddMemPoolTx(const CTransaction & tx) = 0;
    virtual void RemoveMemPoolTx(const CTransaction & tx) = 0;
    virtual uint256 GetConflictTxIfExists(const CTransaction & tx) = 0;
    virtual ~SpecTxMemPoolHandler() = default;
};

#endif // CROWN_PLATFORM_SPECIALTX_COMMON_H

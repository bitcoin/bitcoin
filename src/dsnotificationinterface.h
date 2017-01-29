// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DSNOTIFICATIONINTERFACE_H
#define BITCOIN_DSNOTIFICATIONINTERFACE_H

#include "validationinterface.h"

class CDSNotificationInterface : public CValidationInterface
{
public:
    // virtual CDSNotificationInterface();
    CDSNotificationInterface();
    virtual ~CDSNotificationInterface();

protected:
    // CValidationInterface
    void UpdatedBlockTip(const CBlockIndex *pindex);
    void SyncTransaction(const CTransaction &tx, const CBlock *pblock);

private:
};

#endif // BITCOIN_DSNOTIFICATIONINTERFACE_H

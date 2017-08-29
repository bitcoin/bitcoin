// Copyright (c) 2015 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_DSNOTIFICATIONINTERFACE_H
#define SYSCOIN_DSNOTIFICATIONINTERFACE_H

#include "validationinterface.h"

class CDSNotificationInterface : public CValidationInterface
{
public:
    // virtual CDSNotificationInterface();
    CDSNotificationInterface();
    virtual ~CDSNotificationInterface();

protected:
    // CValidationInterface
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload);
    void SyncTransaction(const CTransaction &tx, const CBlock *pblock);

private:
};

#endif // SYSCOIN_DSNOTIFICATIONINTERFACE_H

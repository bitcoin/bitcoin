// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WATCHDOG_H
#define BITCOIN_WATCHDOG_H

#include <primitives/block.h>
#include <stdint.h>

class CWatchdog
{
public:
    ~CWatchdog();
    CWatchdog();
    //! Launch scan of block issuance or network anomalies
    //! This is periodically called by scheduler.
    void ScanAnomalies();
    //! Log header to detect block issuance anomalies.
    void LogHeader(const std::vector<CBlockHeader>& block);
private:
    //! Last time header has been logged.
    int64_t nLastHeader;
};

#endif

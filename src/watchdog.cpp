// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <watchdog.h>

#include <logging.h>
#include <util/time.h>
#include <watchdoginterface.h>

CWatchdog::CWatchdog()
{
    nLastHeader = 0;
}

CWatchdog::~CWatchdog() {}

void CWatchdog::ScanAnomalies()
{
    LogPrint(BCLog::ALTSTACK, "Block header anomalie detected - notifying subscribers\n");
    GetWatchSignals().BlockHeaderAnomalie();
}

void CWatchdog::LogHeader(const std::vector<CBlockHeader>& block)
{
    nLastHeader = GetTime();
}

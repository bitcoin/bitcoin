// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"

#include "key.h"
#include "stacktraces.h"
#include "validation.h"
#include "util.h"

#include "bls/bls.h"

void CleanupBLSTests();
void CleanupBLSDkgTests();

int
main(int argc, char** argv)
{
    RegisterPrettySignalHandlers();
    RegisterPrettyTerminateHander();

    ECC_Start();
    ECCVerifyHandle verifyHandle;

    BLSInit();
    SetupEnvironment();
    fPrintToDebugLog = false; // don't want to write to debug.log file

    benchmark::BenchRunner::RunAll();

    // need to be called before global destructors kick in (PoolAllocator is needed due to many BLSSecretKeys)
    CleanupBLSDkgTests();
    CleanupBLSTests();

    ECC_Stop();
}

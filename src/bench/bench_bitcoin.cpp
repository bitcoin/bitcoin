// Copyright (c) 2015-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"

#include "core/key.h"
#include "core/random.h"
#include "core/util.h"
#include "core/validation.h"
#include "crypto/sha256.h"

int
main(int argc, char** argv)
{
    SHA256AutoDetect();
    RandomInit();
    ECC_Start();
    SetupEnvironment();
    fPrintToDebugLog = false; // don't want to write to debug.log file

    benchmark::BenchRunner::RunAll();

    ECC_Stop();
}

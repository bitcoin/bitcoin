// Copyright (c) 2015 The Syscoin Core developers
// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"

#include "key.h"
#include "validation.h"
#include "util.h"
#include "crypto/sha256.h"

int
main(int argc, char** argv)
{
	SHA256AutoDetect();
    ECC_Start();
    SetupEnvironment();
    fPrintToDebugLog = false; // don't want to write to debug.log file

    benchmark::BenchRunner::RunAll();

    ECC_Stop();
}

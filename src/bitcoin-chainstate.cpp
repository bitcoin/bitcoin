// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// The bitcoin-chainstate executable serves to surface the dependencies required
// by a program wishing to use Bitcoin Core's consensus engine as it is right
// now.
//
// DEVELOPER NOTE: Since this is a "demo-only", experimental, etc. executable,
//                 it may diverge from Bitcoin Core's coding style.
//
// It is part of the libbitcoinkernel project.

#include <bitcoin-chainstate.h>
#include <util/fs.h>
#include <validationinterface.h>

int main(int argc, char* argv[])
{
    // We do not enable logging for this app, so explicitly disable it.
    // To enable logging instead, replace with:
    //    LogInstance().m_print_to_console = true;
    //    LogInstance().StartLogging();
    LogInstance().DisableLogging();

    // SETUP: Argument parsing and handling
    if (argc != 2) {
        std::cerr
            << "Usage: " << argv[0] << " DATADIR" << std::endl
            << "Display DATADIR information, and process hex-encoded blocks on standard input." << std::endl
            << std::endl
            << "IMPORTANT: THIS EXECUTABLE IS EXPERIMENTAL, FOR TESTING ONLY, AND EXPECTED TO" << std::endl
            << "           BREAK IN FUTURE VERSIONS. DO NOT USE ON YOUR ACTUAL DATADIR." << std::endl;
        return 1;
    }

    return kernel_chainstate::process(argv[1]);
}
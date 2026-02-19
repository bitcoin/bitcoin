// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_APP_CONSTANTS_H
#define BITCOIN_COMMON_APP_CONSTANTS_H

/**
 * Common constants used across multiple command-line applications.
 */

/** Return value to indicate continuing execution (not an exit code) */
inline constexpr int CONTINUE_EXECUTION = -1;

/**
 * Common error messages used across command-line applications.
 */
namespace app_error {
    inline constexpr const char* TOO_FEW_PARAMETERS = "Error: too few parameters\n";
    inline constexpr const char* DATADIR_DOES_NOT_EXIST = "Error: Specified data directory \"%s\" does not exist.\n";
    inline constexpr const char* COMMAND_LINE_PARSE_ERROR = "Error parsing command line arguments: %s\n";
}

#endif // BITCOIN_COMMON_APP_CONSTANTS_H

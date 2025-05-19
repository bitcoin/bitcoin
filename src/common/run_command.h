// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_RUN_COMMAND_H
#define BITCOIN_COMMON_RUN_COMMAND_H

#include <string>

class UniValue;

#ifndef WIN32
/**
 * Escape a string for passing it safely to a shell command.
 */
std::string ShellEscape(const std::string& arg);
#endif

/**
 * Execute a shell command, wait for it to finish.
 *
 * @param str_command The command to execute, including any arguments
 */
void RunShell(const std::string& str_command);

/**
 * Execute a shell command in a new thread, return immediately.
 *
 * @param str_command The command to execute, including any arguments
 */
void RunShellInThread(const std::string& str_command);

/**
 * Execute a command which returns JSON, and parse the result.
 *
 * @param str_command The command to execute, including any arguments
 * @param str_std_in string to pass to stdin
 * @return parsed JSON
 */
UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in="");

#endif // BITCOIN_COMMON_RUN_COMMAND_H

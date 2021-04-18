// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INIT_H
#define BITCOIN_INIT_H

#include <memory>
#include <string>

//! Default value for -daemon option
static constexpr bool DEFAULT_DAEMON = false;
//! Default value for -daemonwait option
static constexpr bool DEFAULT_DAEMONWAIT = false;

class ArgsManager;
struct NodeContext;
namespace interfaces {
struct BlockAndHeaderTipInfo;
}
namespace boost {
class thread_group;
} // namespace boost
namespace util {
class Ref;
} // namespace util

struct ServerArgsOptions {
    constexpr ServerArgsOptions(bool gui_, bool printtoconsole_default_, bool server_default_)
        : gui{gui_}, printtoconsole_default{printtoconsole_default_}, server_default{server_default_}
    {
    }

    const bool gui;
    const bool printtoconsole_default;
    const bool server_default;
};

/** Interrupt threads */
void Interrupt(NodeContext& node);
void Shutdown(NodeContext& node);
//!Initialize the logging infrastructure
void InitLogging(const ArgsManager& args, const ServerArgsOptions& options);
//!Parameter interaction: change current parameters depending on various rules
void InitParameterInteraction(ArgsManager& args);

/** Initialize bitcoin core: Basic context setup.
 *  @note This can be done before daemonization. Do not call Shutdown() if this function fails.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInitBasicSetup(const ArgsManager& args);
/**
 * Initialization: parameter interaction.
 * @note This can be done before daemonization. Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitBasicSetup should have been called.
 */
bool AppInitParameterInteraction(const ArgsManager& args);
/**
 * Initialization sanity checks: ecc init, sanity checks, dir lock.
 * @note This can be done before daemonization. Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitParameterInteraction should have been called.
 */
bool AppInitSanityChecks();
/**
 * Lock bitcoin core data directory.
 * @note This should only be done after daemonization. Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitSanityChecks should have been called.
 */
bool AppInitLockDataDirectory();
/**
 * Initialize node and wallet interface pointers. Has no prerequisites or side effects besides allocating memory.
 */
bool AppInitInterfaces(NodeContext& node);
/**
 * Bitcoin core main initialization.
 * @note This should only be done after daemonization. Call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitLockDataDirectory should have been called.
 */
bool AppInitMain(const util::Ref& context, NodeContext& node, interfaces::BlockAndHeaderTipInfo* tip_info = nullptr);

/**
 * Register all arguments with the ArgsManager
 */
void SetupServerArgs(NodeContext& node, ServerArgsOptions options);

/** Returns licensing information (for -version) */
std::string LicenseInfo();

#endif // BITCOIN_INIT_H

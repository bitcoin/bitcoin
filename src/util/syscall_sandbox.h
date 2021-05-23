// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SYSCALL_SANDBOX_H
#define BITCOIN_UTIL_SYSCALL_SANDBOX_H

enum class SyscallSandboxPolicy {
    // 1. Initialization
    INITIALIZATION,
    INITIALIZATION_DNS_SEED,
    INITIALIZATION_LOAD_BLOCKS,
    INITIALIZATION_MAP_PORT,
    INITIALIZATION_TOR_CONTROL,

    // 2. Steady state (non-initialization, non-shutdown)
    MESSAGE_HANDLER,
    NET,
    NET_ADD_CONNECTION,
    NET_HTTP_SERVER,
    NET_HTTP_SERVER_WORKER,
    NET_OPEN_CONNECTION,
    SCHEDULER,
    TX_INDEX,
    VALIDATION_SCRIPT_CHECK,

    // 3. Shutdown
    SHUTOFF,

    // Others (non-bitcoind)
    LIBFUZZER,
    LIBFUZZER_MERGE_MODE,
};

//! Force the current thread (and threads created from the current thread) into
//! a restricted-service operating mode where only a subset of all syscalls are
//! available.
//!
//! Subsequent calls to this function can reduce the abilities further, but
//! abilities can never be regained.
//!
//! This function is a no-op unless Bitcoin Core is compiled with the configure
//! option --with-syscall-sandbox. This option is available under Linux x86_64
//! only.
void EnableSyscallSandbox(const SyscallSandboxPolicy syscall_policy);

//! Disable all further syscall sandbox restrictions that would have be made by
//! calling EnableSyscallSandbox.
//!
//! Does not revert any syscall restrictions currently in effect.
//!
//! Typically called early in main(...) to disable the syscall sandbox mechanism
//! on a per binary basis.
//!
//! This function is a no-op unless Bitcoin Core is compiled with the configure
//! option --with-syscall-sandbox. This option is available under Linux x86_64
//! only.
void DisableFurtherSyscallSandboxRestrictions();

#endif // BITCOIN_UTIL_SYSCALL_SANDBOX_H

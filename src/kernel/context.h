// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_KERNEL_CONTEXT_H
#define SYSCOIN_KERNEL_CONTEXT_H

#include <memory>

namespace kernel {
//! Context struct holding the kernel library's logically global state, and
//! passed to external libsyscoin_kernel functions which need access to this
//! state. The kernel library API is a work in progress, so state organization
//! and member list will evolve over time.
//!
//! State stored directly in this struct should be simple. More complex state
//! should be stored to std::unique_ptr members pointing to opaque types.
struct Context {
    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the kernel::Context struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    Context();
    ~Context();
};
} // namespace kernel

#endif // SYSCOIN_KERNEL_CONTEXT_H

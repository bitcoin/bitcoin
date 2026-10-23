#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
#
# Run a command with RLIMIT_NOFILE capped at $CI_LIMIT_FD_COUNT.
#
# On select()-based platforms such as NetBSD, bitcoind can use at most
# FD_SETSIZE (256) file descriptors regardless of the RLIMIT_NOFILE. Capping the
# hard limit here makes RaiseFileDescriptorLimit() return the same number on any
# platform, so an fd-budget regression (e.g. a new "Reducing -maxconnections ..."
# warning on stderr, which the functional test framework treats as a failure) is
# caught by this job instead of only on NetBSD.

export LC_ALL=C.UTF-8
set -o errexit

ulimit -S -n "${CI_LIMIT_FD_COUNT}"
ulimit -H -n "${CI_LIMIT_FD_COUNT}"
exec "$@"

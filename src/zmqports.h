// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ZMQPORTS_H
#define ZMQPORTS_H

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include <string>

// Global state
extern bool fZMQPub;

#if ENABLE_ZMQ
void ZMQShutdown();
void ZMQInitialize(const std::string &endp);
#else
static inline void ZMQInitialize(const std::string &endp) {}
static inline void ZMQShutdown() {}
#endif // ENABLE_ZMQ

#endif // ZMQPORTS_H

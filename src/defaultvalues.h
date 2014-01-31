// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DEFAULTVALUES_H
#define DEFAULTVALUES_H

#include <stdint.h>

// This file covers our default values for command-line arguments that are used by core and GUI

// -dbcache
static const int nDefaultDbCache = 25;
// max. -dbcache
static const int nMaxDbCache = sizeof(void*) > 4 ? 4096 : 1024;

// -par
static const int nDefaultPar = 0;

// -paytxfee
static const int64_t nDefaultTransactionFee = 0;

// -upnp
#ifdef USE_UPNP
static const bool fDefaultUpnp = true;
#else
static const bool fDefaultUpnp = false;
#endif

#endif // DEFAULTVALUES_H

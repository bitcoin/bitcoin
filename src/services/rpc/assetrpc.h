// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_RPC_ASSETRPC_H
#define SYSCOIN_SERVICES_RPC_ASSETRPC_H
#include <string>
#include <amount.h>
#include <script/standard.h>
CAmount getAuxFee(const std::string &public_data, const CAmount& nAmount, const uint8_t &nPrecision, CTxDestination & address);
#endif // SYSCOIN_SERVICES_RPC_ASSETRPC_H

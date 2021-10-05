// Copyright (c) 2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NEVM_NEVM_H
#define SYSCOIN_NEVM_NEVM_H

#include <vector>
#include <nevm/commondata.h>
#include <nevm/rlp.h>
#include <consensus/amount.h>
bool VerifyProof(dev::bytesConstRef path, const dev::RLP& value, const dev::RLP& parentNodes, const dev::RLP& root); 
bool parseNEVMMethodInputData(const std::vector<unsigned char>& vchInputExpectedMethodHash,  const uint8_t& nERC20Precision, const uint8_t& nLocalPrecision, const std::vector<unsigned char>& vchInputData, CAmount& outputAmount, uint64_t& nAsset, std::string& witnessAddress);
#endif // SYSCOIN_NEVM_NEVM_H

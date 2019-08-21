// Copyright (c) 2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_ETHEREUM_ETHEREUM_H
#define SYSCOIN_ETHEREUM_ETHEREUM_H

#include <vector>
#include <ethereum/commondata.h>
#include <ethereum/rlp.h>
#include <amount.h>
class CWitnessAddress;
bool VerifyProof(dev::bytesConstRef path, const dev::RLP& value, const dev::RLP& parentNodes, const dev::RLP& root); 
bool parseEthMethodInputData(const std::vector<unsigned char>& vchInputExpectedMethodHash, const std::vector<unsigned char>& vchInputData, const std::vector<unsigned char>& vchAssetContract, CAmount& outputAmount, uint32_t& nAsset, const uint8_t& nLocalPrecision, CWitnessAddress& witnessAddress);
#endif // SYSCOIN_ETHEREUM_ETHEREUM_H

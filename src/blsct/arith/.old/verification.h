// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_VERIFICATION_H
#define BLSCT_VERIFICATION_H

#ifdef _WIN32
/* Avoid redefinition warning. */
#undef ERROR
#undef WSIZE
#undef DOUBLE
#endif

#include <bls.hpp>
#include <blsct/bulletproofs.h>
#include <coins.h>
#include <consensus/validation.h>
#include <dotnav/names.h>
#include <primitives/transaction.h>
#include <random.h>
#include <schemes.hpp>
#include <utiltime.h>

bool VerifyBLSCT(const CTransaction &tx, bls::PrivateKey viewKey, std::vector<RangeproofEncodedData> &vData, const CStateViewCache& view, CValidationState& state, bool fOnlyRecover = false, CAmount nMixFee = 0);
bool VerifyBLSCTBalanceOutputs(const CTransaction &tx, bls::PrivateKey viewKey, std::vector<RangeproofEncodedData> &vData, const CStateViewCache& view, CValidationState& state, bool fOnlyRecover = false, CAmount nMixFee = 0);
bool CombineBLSCTTransactions(std::set<CTransaction> &vTx, CTransaction& outTx, const CStateViewCache& inputs, CValidationState& state, CAmount nMixFee = 0);
#endif // BLSCT_VERIFICATION_H

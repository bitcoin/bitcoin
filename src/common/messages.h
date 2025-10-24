// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file common/messages.h is a home for simple string functions returning
//! descriptive messages that are used in RPC and GUI interfaces or log
//! messages, and are called in different parts of the codebase across
//! node/wallet/gui boundaries.

#ifndef BITCOIN_COMMON_MESSAGES_H
#define BITCOIN_COMMON_MESSAGES_H

#include <string>
#include <string_view>

struct bilingual_str;

enum class FeeEstimateMode;
enum class FeeReason;
namespace node {
enum class TransactionError;
} // namespace node

namespace common {
enum class PSBTError;
bool FeeModeFromString(std::string_view mode_string, FeeEstimateMode& fee_estimate_mode);
std::string StringForFeeReason(FeeReason reason);
std::string FeeModes(const std::string& delimiter);
std::string FeeModeInfo(std::pair<std::string, FeeEstimateMode>& mode);
std::string FeeModesDetail(std::string default_info);
std::string InvalidEstimateModeErrorMessage();
bilingual_str PSBTErrorString(PSBTError error);
bilingual_str TransactionErrorString(const node::TransactionError error);
bilingual_str ResolveErrMsg(const std::string& optname, const std::string& strBind);
bilingual_str InvalidPortErrMsg(const std::string& optname, const std::string& strPort);
bilingual_str AmountHighWarn(const std::string& optname);
bilingual_str AmountErrMsg(const std::string& optname, const std::string& strValue);
} // namespace common

#endif // BITCOIN_COMMON_MESSAGES_H

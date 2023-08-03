// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_LIBSTANDARD_H
#define BITCOIN_POLICY_LIBSTANDARD_H

#ifndef EXPORT_SYMBOL
  #define EXPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LIBSTANDARD_API_VER 1

typedef enum libstandardness_result_t
{
    libstandardness_RESULT_OK = 0,
    libstandardness_RESULT_INVALID,

} libstandardness_result;

EXPORT_SYMBOL int libstandard_verify_transaction(const unsigned char *txTo, unsigned int txToLen, libstandardness_result* result);

EXPORT_SYMBOL unsigned int bitcoinconsensus_version();

#ifdef __cplusplus
} // extern "C"
#endif

#undef EXPORT_SYMBOL

#endif // BITCOIN_POLICY_LIBSTANDARD_H

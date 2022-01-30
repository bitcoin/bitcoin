#pragma once

#include <wallet/wallet.h>
#include <wallet/coincontrol.h>

// Forward Declarations
struct bilingual_str;
struct FeeCalculation;

bool CreateTransactionEx(
    CWallet& wallet,
    const std::vector<CRecipient>& vecSend,
    CTransactionRef& tx,
    CAmount& nFeeRet,
    int& nChangePosInOut,
    bilingual_str& error,
    const CCoinControl& coin_control,
    FeeCalculation& fee_calc_out,
    bool sign
);
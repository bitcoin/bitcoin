// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/sendcoinsrecipient.h>

SendCoinsRecipient::SendCoinsRecipient() :
    amount(0),
    fSubtractFeeFromAmount(false),
    nVersion(SendCoinsRecipient::CURRENT_VERSION)
{
}

SendCoinsRecipient::SendCoinsRecipient(const QString &addr, const QString &_label, const CAmount& _amount, const QString &_message):
    address(addr),
    label(_label),
    amount(_amount),
    message(_message),
    fSubtractFeeFromAmount(false),
    nVersion(SendCoinsRecipient::CURRENT_VERSION)
{
}

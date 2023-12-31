// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/context.h>

WalletContext::WalletContext(const std::unique_ptr<interfaces::CoinJoin::Loader>& coinjoin_loader) :
    m_coinjoin_loader(coinjoin_loader)
{}
WalletContext::~WalletContext() {}

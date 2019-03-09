// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCSIGNER_H
#define BITCOIN_WALLET_RPCSIGNER_H

#include <util/system.h>
#include <vector>

#ifdef HAVE_BOOST_PROCESS

class CRPCTable;
class CWallet;
class ExternalSigner;
class JSONRPCRequest;

/**
 * Figures out what external signer to use for a JSONRPCRequest. If no fingerprint
 * is specified and there is only one signer, it returns that.
 *
 * @param[in] request   JSONRPCRequest that wishes to access a signer
 * @param[in] index     request.params[index] contains the signer fingerprint
 * @param[in] pwallet   CWallet pointer to the wallet
 *
 * @return nullptr if no signer is found, or a pointer to the ExternalSigner
 */
ExternalSigner *GetSignerForJSONRPCRequest(const JSONRPCRequest& request, int index, CWallet* pwallet);

namespace interfaces {
class Chain;
class Handler;
}

void RegisterSignerRPCCommands(interfaces::Chain& chain, std::vector<std::unique_ptr<interfaces::Handler>>& handlers);

#endif // BOOST_VERSION

#endif //BITCOIN_WALLET_RPCSIGNER_H

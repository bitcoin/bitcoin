// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPC_WALLET_H
#define BITCOIN_WALLET_RPC_WALLET_H

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <span.h>

class CRPCCommand;
class RPCMethod;

namespace wallet {
std::span<const CRPCCommand> GetWalletRPCCommands();

// addresses
RPCMethod getaddressinfo();
RPCMethod getnewaddress();
RPCMethod getrawchangeaddress();
RPCMethod setlabel();
RPCMethod listaddressgroupings();
RPCMethod keypoolrefill();
RPCMethod getaddressesbylabel();
RPCMethod listlabels();
#ifdef ENABLE_EXTERNAL_SIGNER
RPCMethod walletdisplayaddress();
#endif // ENABLE_EXTERNAL_SIGNER

// backup
RPCMethod importprunedfunds();
RPCMethod importdescriptors();
RPCMethod listdescriptors();
RPCMethod backupwallet();
RPCMethod restorewallet();

// coins
RPCMethod getreceivedbyaddress();
RPCMethod getreceivedbylabel();
RPCMethod getbalance();
RPCMethod lockunspent();
RPCMethod listlockunspent();
RPCMethod getbalances();
RPCMethod listunspent();

// encryption
RPCMethod walletpassphrase();
RPCMethod walletpassphrasechange();
RPCMethod walletlock();
RPCMethod encryptwallet();

// spend
RPCMethod sendtoaddress();
RPCMethod sendmany();
RPCMethod fundrawtransaction();
RPCMethod bumpfee();
RPCMethod psbtbumpfee();
RPCMethod send();
RPCMethod sendall();
RPCMethod walletprocesspsbt();
RPCMethod walletcreatefundedpsbt();
RPCMethod signrawtransactionwithwallet();

// signmessage
RPCMethod signmessage();

// transactions
RPCMethod listreceivedbyaddress();
RPCMethod listreceivedbylabel();
RPCMethod listtransactions();
RPCMethod listrawtransactions();
RPCMethod listsinceblock();
RPCMethod gettransaction();
RPCMethod abandontransaction();
RPCMethod rescanblockchain();
RPCMethod abortrescan();
} // namespace wallet

#endif // BITCOIN_WALLET_RPC_WALLET_H

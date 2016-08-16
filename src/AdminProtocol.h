/*
 * This file is part of the bitcoin-classic project
 * Copyright (C) 2016 Tom Zander <tomz@freedommail.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ADMINPROTOCOL_H
#define ADMINPROTOCOL_H

namespace Admin {
enum ServiceIds {
    ControlService,
    LoginService,
    UtilService,
    MiningService,
    RawTransactionService,
    BlockChainService,
    NetworkService,
    WalletService
};

enum AdminTags {
    RequestId = 11
};


namespace Control {

// Control Service
enum MessageIds {
    CommandFailed,
//   getinfo
    Stop,
    StopReply
    // Maybe 'version' ? To allow a client to see if the server is a different version.
    // and then at the same time a "supports" method that returns true if a certain
    // command is supported.
};

enum Tags {
    Separator = 0,
    GenericByteData,
    FailedReason,
    FailedCommandServiceId,
    FailedCommandId,
};

}


namespace Login { // Login Service

enum MessageIds {
    LoginMessage
};

enum Tags {
    Separator = 0,
    GenericByteData,
    CookieData
};

}


namespace Util {

enum MessageIds {
    CreateAddress,
    CreateAddressReply,
//  createmultisig nrequired ["key",...]
//  estimatefee nblocks
//  estimatepriority nblocks
//  estimatesmartfee nblocks
//  estimatesmartpriority nblocks
    ValidateAddress,
    ValidateAddressReply,
//  verifymessage "bitcoinaddress" "signature" "message"
};

enum Tags {
    Separator = 0,
    GenericByteData,
    BitcoinAddress,
    ScriptPubKey,
    PrivateAddress,
    IsValid,
};
}

namespace Mining { // Mining Service

enum MessageIds {
    CreateNewBlock,
    CreateNewBlockReply

//   getblocktemplate ( "jsonrequestobject" )
//   getmininginfo
//   getnetworkhashps ( blocks height )
//   prioritisetransaction <txid> <priority delta> <fee delta>
//   setcoinbase pubkey
//   submitblock "hexdata" ( "jsonparametersobject" )
//
};

enum Tags {
    Separator = 0,
    GenericByteData,
};
}


namespace RawTransactions {

enum MessageIds {
    GetRawTransaction,
    GetRawTransactionReply,
    SendRawTransaction,
    SendRawTransactionReply,
    SignRawTransaction,
    SignRawTransactionReply
};
enum Tags {
    Separator = 0,
    GenericByteData,
    RawTransaction,
    TransactionId,   // bytearray
    Completed,       // boolean
    OutputIndex,
    ScriptPubKey,   // bytearray.   This is the output-script.
    ScriptSig,      // bytearray.   This is the input-script.
    Sequence,       // Number
    ErrorMessage,   // string
    PrivateKey,     // string
    SigHashType,    // Number
    OutputAmount    // value in satoshis
};
}


namespace BlockChain {
enum MessageIds {
    GetBlockChainInfo,
    GetBlockChainInfoReply,
    GetBestBlockHash,
    GetBestBlockHashReply,
    GetBlock,
    GetBlockReply,
//   getblockcount
//   getblockhash index
//   getblockheader "hash" ( verbose )
//   getchaintips
//   getdifficulty
//   getmempoolinfo
//   getrawmempool ( verbose )
//   gettxout "txid" n ( includemempool )
//   gettxoutproof ["txid",...] ( blockhash )
//   gettxoutsetinfo
//   verifychain ( checklevel numblocks )
//   verifytxoutproof "proof"
};

enum Tags {
    Separator = 0,
    GenericByteData,
    Verbose,    // bool
    Size,       // int
    Version,    // int
    Time,       // in seconds since epoch
    Difficulty, // double
    MedianTime, // in seconds since epoch
    ChainWork,  // a sha256

    // BlockChain-tags
    Chain = 30,
    Blocks,
    Headers,
    BestBlockHash,
    VerificationProgress,
    Pruned,
    Bip9ForkId,
    Bip9ForkStatus,

    // GetBlock-tags
    BlockHash = 40,
    Confirmations,
    Height,
    MerkleRoot,
    TxId,
    Nonce,      //int
    Bits,       // integer
    PrevBlockHash,
    NextBlockHash
};

}


namespace Network {
enum MessageIds {
//   == Network ==
//   addnode "node" "add|remove|onetry"
//   clearbanned
//   disconnectnode "node"
//   getaddednodeinfo dns ( "node" )
//   getconnectioncount
//   getnettotals
//   getnetworkinfo
//   getpeerinfo
//   listbanned
//   setban "ip(/netmask)" "add|remove" (bantime) (absolute)
};

enum Tags {
    Separator = 0,
    GenericByteData,

};
}

namespace Wallet {
enum MessageIds {
    ListUnspent,
    ListUnspentReply,
    GetNewAddress,
    GetNewAddressReply,
};

enum Tags {
    Separator = 0,
    GenericByteData,
    MinimalConfirmations,
    MaximumConfirmations,
    TransactionId,
    TXOutputIndex,
    BitcoinAddress,
    ScriptPubKey,
    Amount,
    ConfirmationCount
};
}

}
#endif

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
#include "AdminRPCBinding.h"
#include "AdminProtocol.h"

#include "streaming/MessageBuilder.h"
#include "rpcserver.h"
#include "base58.h"
#include <univalue.h>

#include <boost/algorithm/hex.hpp>

#include <streaming/MessageParser.h>

#include <list>

namespace {

// blockchain

class GetBlockChainInfo : public AdminRPCBinding::Parser
{
public:
    GetBlockChainInfo() : Parser("getblockchaininfo", Admin::BlockChain::GetBlockChainInfoReply, 500) {}
    virtual void parser(Streaming::MessageBuilder &builder, const UniValue &result) {
        const UniValue &chain = find_value(result, "chain");
        builder.add(Admin::BlockChain::Chain, chain.get_str());
        const UniValue &blocks = find_value(result, "blocks");
        builder.add(Admin::BlockChain::Blocks, blocks.get_int());
        const UniValue &headers = find_value(result, "headers");
        builder.add(Admin::BlockChain::Headers, headers.get_int());
        const UniValue &best = find_value(result, "bestblockhash");
        uint256 sha256;
        sha256.SetHex(best.get_str());
        builder.add(Admin::BlockChain::BestBlockHash, sha256);
        const UniValue &difficulty = find_value(result, "difficulty");
        builder.add(Admin::BlockChain::Difficulty, difficulty.get_real());
        const UniValue &time = find_value(result, "mediantime");
        builder.add(Admin::BlockChain::MedianTime, (uint64_t) time.get_int64());
        const UniValue &progress = find_value(result, "verificationprogress");
        builder.add(Admin::BlockChain::VerificationProgress, progress.get_real());
        const UniValue &chainwork = find_value(result, "chainwork");
        sha256.SetHex(chainwork.get_str());
        builder.add(Admin::BlockChain::ChainWork, sha256);
        const UniValue &pruned = find_value(result, "pruned");
        if (pruned.get_bool())
            builder.add(Admin::BlockChain::Pruned, true);

        const UniValue &bip9 = find_value(result, "bip9_softforks");
        bool first = true;
        for (auto fork : bip9.getValues()) {
            if (first) first = false;
            else builder.add(Admin::Wallet::Separator, true);
            const UniValue &id = find_value(fork, "id");
            builder.add(Admin::BlockChain::Bip9ForkId, id.get_str());
            const UniValue &status = find_value(fork, "status");
            // TODO change status to an enum?
            builder.add(Admin::BlockChain::Bip9ForkStatus, status.get_str());
        }
    }
};

class GetBestBlockHash : public AdminRPCBinding::Parser
{
public:
    GetBestBlockHash() : Parser("getbestblockhash", Admin::BlockChain::GetBestBlockHashReply, 50) {}
};

class GetBlock : public AdminRPCBinding::Parser
{
public:
    GetBlock() : Parser("getblock", Admin::BlockChain::GetBlockReply), m_verbose(false) {}
    virtual void createRequest(const Message &message, UniValue &output) {
        std::string txid;
        Streaming::MessageParser parser(message.body());
        while (parser.next() == Streaming::FoundTag) {
            if (parser.tag() == Admin::BlockChain::BlockHash
                    || parser.tag() == Admin::RawTransactions::GenericByteData)
                boost::algorithm::hex(parser.bytesData(), back_inserter(txid));
            else if (parser.tag() == Admin::BlockChain::Verbose)
                m_verbose = parser.boolData();
        }
        output.push_back(std::make_pair("block", UniValue(UniValue::VSTR, txid)));
        output.push_back(std::make_pair("verbose", UniValue(UniValue::VBOOL, m_verbose ? "1": "0")));
    }

    virtual int messageSizeCalc(const UniValue &result) const {
        if (m_verbose) {
            const UniValue &tx = find_value(result, "tx");
            return tx.size() * 70 + 200;
        }
        return result.get_str().size() / 2 + 20;
    }

    virtual void parser(Streaming::MessageBuilder &builder, const UniValue &result) {
        if (!m_verbose) {
            std::vector<char> answer;
            boost::algorithm::unhex(result.get_str(), back_inserter(answer));
            builder.add(1, answer);
            return;
        }

        const UniValue &hash = find_value(result, "hash");
        std::vector<char> bytearray;
        boost::algorithm::unhex(hash.get_str(), back_inserter(bytearray));
        builder.add(Admin::BlockChain::BlockHash, bytearray);
        bytearray.clear();
        const UniValue &confirmations = find_value(result, "confirmations");
        builder.add(Admin::BlockChain::Confirmations, confirmations.get_int());
        const UniValue &size = find_value(result, "size");
        builder.add(Admin::BlockChain::Size, size.get_int());
        const UniValue &height = find_value(result, "height");
        builder.add(Admin::BlockChain::Height, height.get_int());
        const UniValue &version = find_value(result, "version");
        builder.add(Admin::BlockChain::Version, version.get_int());
        const UniValue &merkleroot = find_value(result, "merkleroot");
        boost::algorithm::unhex(merkleroot.get_str(), back_inserter(bytearray));
        builder.add(Admin::BlockChain::MerkleRoot, bytearray);
        bytearray.clear();
        const UniValue &tx = find_value(result, "tx");
        bool first = true;
        for (const UniValue &transaction: tx.getValues()) {
            if (first) first = false;
            else builder.add(Admin::Wallet::Separator, true);
            boost::algorithm::unhex(transaction.get_str(), back_inserter(bytearray));
            builder.add(Admin::BlockChain::TxId, bytearray);
            bytearray.clear();
        }
        const UniValue &time = find_value(result, "time");
        builder.add(Admin::BlockChain::Time, (uint64_t) time.get_int64());
        const UniValue &mediantime = find_value(result, "mediantime");
        builder.add(Admin::BlockChain::MedianTime, (uint64_t) mediantime.get_int64());
        const UniValue &nonce = find_value(result, "nonce");
        builder.add(Admin::BlockChain::Nonce, (uint64_t) nonce.get_int64());
        const UniValue &bits = find_value(result, "bits");
        boost::algorithm::unhex(bits.get_str(), back_inserter(bytearray));
        builder.add(Admin::BlockChain::Bits, bytearray);
        bytearray.clear();
        const UniValue &difficulty = find_value(result, "difficulty");
        builder.add(Admin::BlockChain::Difficulty, difficulty.get_real());
        const UniValue &chainwork = find_value(result, "chainwork");
        boost::algorithm::unhex(chainwork.get_str(), back_inserter(bytearray));
        builder.add(Admin::BlockChain::ChainWork, bytearray);
        bytearray.clear();
        const UniValue &prevhash = find_value(result, "previousblockhash");
        boost::algorithm::unhex(prevhash.get_str(), back_inserter(bytearray));
        builder.add(Admin::BlockChain::PrevBlockHash, bytearray);
        const UniValue &nextblock = find_value(result, "nextblockhash");
        if (nextblock.isStr()) {
            boost::algorithm::unhex(nextblock.get_str(), back_inserter(bytearray));
            builder.add(Admin::BlockChain::NextBlockHash, bytearray);
        }
    }

private:
    bool m_verbose;

};

// raw transactions

class GetRawTransaction : public AdminRPCBinding::Parser
{
public:
    GetRawTransaction() : Parser("getrawtransaction", Admin::RawTransactions::GetRawTransactionReply) {}

    virtual void createRequest(const Message &message, UniValue &output) {
        std::string txid;
        Streaming::MessageParser parser(message.body());
        while (parser.next() == Streaming::FoundTag) {
            if (parser.tag() == Admin::RawTransactions::TransactionId
                    || parser.tag() == Admin::RawTransactions::GenericByteData)
                boost::algorithm::hex(parser.bytesData(), back_inserter(txid));
        }
        output.push_back(std::make_pair("parameter 1", UniValue(UniValue::VSTR, txid)));
    }

    virtual int messageSizeCalc(const UniValue &result) const {
        return result.get_str().size() / 2 + 20;
    }
};

class SendRawTransaction : public AdminRPCBinding::Parser
{
public:
    SendRawTransaction() : Parser("sendrawtransaction", Admin::RawTransactions::SendRawTransactionReply, 10) {}

    virtual void createRequest(const Message &message, UniValue &output) {
        std::string tx;
        Streaming::MessageParser parser(message.body());
        while (parser.next() == Streaming::FoundTag) {
            if (parser.tag() == Admin::RawTransactions::RawTransaction
                    || parser.tag() == Admin::RawTransactions::GenericByteData)
                boost::algorithm::hex(parser.bytesData(), back_inserter(tx));
        }
        output.push_back(std::make_pair("", UniValue(UniValue::VSTR, tx)));
    }
};

struct PrevTransaction {
    PrevTransaction() : vout(-1), amount(-1) {}
    std::string txid, scriptPubKey;
    int vout;
    int64_t amount;
    bool isValid() const {
        return vout >= 0 && !txid.empty() && !scriptPubKey.empty();
    }
};

class SignRawTransaction : public AdminRPCBinding::Parser
{
public:
    SignRawTransaction() : AdminRPCBinding::Parser("signrawtransaction", Admin::RawTransactions::SignRawTransactionReply) {}

    virtual void createRequest(const Message &message, UniValue &output) {
        output = UniValue(UniValue::VARR);
        std::list<std::string> privateKeys;
        std::list<PrevTransaction> prevTxs;
        PrevTransaction prevTx;
        int sigHashType = -1;
        std::string rawTx;

        Streaming::MessageParser parser(message.body());
        while (parser.next() == Streaming::FoundTag) {
            std::string string;
            switch (parser.tag()) {
            case Admin::RawTransactions::PrivateKey:
                privateKeys.push_back(parser.stringData());
                break;
            case Admin::RawTransactions::Separator:
                if (prevTx.isValid())
                    prevTxs.push_back(prevTx);
                prevTx = PrevTransaction();
                break;
            case Admin::RawTransactions::SigHashType:
                sigHashType = parser.intData();
                break;
            case Admin::RawTransactions::TransactionId:
                boost::algorithm::hex(parser.bytesData(), back_inserter(string));
                prevTx.txid = string;
                break;
            case Admin::RawTransactions::OutputIndex:
                prevTx.vout = parser.intData();
                break;
            case Admin::RawTransactions::ScriptPubKey:
                boost::algorithm::hex(parser.bytesData(), back_inserter(string));
                prevTx.scriptPubKey = string;
                break;
            case Admin::RawTransactions::OutputAmount:
                prevTx.amount = parser.longData();
                break;
            case Admin::RawTransactions::GenericByteData:
            case Admin::RawTransactions::RawTransaction:
                boost::algorithm::hex(parser.bytesData(), back_inserter(rawTx));
                break;
            }
        }
        if (prevTx.isValid())
            prevTxs.push_back(prevTx);

        output.push_back(UniValue(UniValue::VSTR, rawTx));

        // send previous tx
        UniValue list1(UniValue::VARR);
        for (const auto &tx : prevTxs) {
            UniValue item(UniValue::VOBJ);
            item.push_back(std::make_pair("txid", UniValue(UniValue::VSTR, tx.txid)));
            item.push_back(std::make_pair("scriptPubKey", UniValue(UniValue::VSTR, tx.scriptPubKey)));
            item.push_back(std::make_pair("vout", UniValue(tx.vout)));
            if (tx.amount != -1)
                item.push_back(std::make_pair("amount", UniValue(ValueFromAmount(tx.amount))));
            list1.push_back(item);
        }
        output.push_back(list1);

        // send private keys.
        UniValue list2(UniValue::VNULL);
        if (!privateKeys.empty()) {
            list2 = UniValue(UniValue::VARR);
            for (const std::string &str : privateKeys) {
                list2.push_back(UniValue(UniValue::VSTR, str));
            }
        }
        output.push_back(list2);

        if (sigHashType >= 0)
            output.push_back(UniValue(sigHashType));
    }

    virtual int messageSizeCalc(const UniValue &result) const {
        const UniValue &hex = find_value(result, "hex");
        const UniValue &errors = find_value(result, "errors");
        return errors.size() * 300 + hex.get_str().size() / 2 + 10;
    }

    virtual void parser(Streaming::MessageBuilder &builder, const UniValue &result) {
        const UniValue &hex = find_value(result, "hex");
        std::vector<char> bytearray;
        boost::algorithm::unhex(hex.get_str(), back_inserter(bytearray));
        builder.add(Admin::RawTransactions::RawTransaction, bytearray);
        bytearray.clear();
        const UniValue &complete = find_value(result, "complete");
        builder.add(Admin::RawTransactions::Completed, complete.getBool());
        const UniValue &errors = find_value(result, "errors");
        if (!errors.isNull()) {
            bool first = true;
            for (const UniValue &error : errors.getValues()) {
                if (first) first = false;
                else builder.add(Admin::Wallet::Separator, true);
                const UniValue &txid = find_value(error, "txid");
                boost::algorithm::unhex(txid.get_str(), back_inserter(bytearray));
                builder.add(Admin::RawTransactions::TransactionId, bytearray);
                bytearray.clear();
                const UniValue &vout = find_value(error, "vout");
                builder.add(Admin::RawTransactions::OutputIndex, vout.get_int());
                const UniValue &scriptSig = find_value(error, "scriptSig");
                boost::algorithm::unhex(scriptSig.get_str(), back_inserter(bytearray));
                builder.add(Admin::RawTransactions::ScriptSig, bytearray);
                bytearray.clear();
                const UniValue &sequence = find_value(error, "sequence");
                builder.add(Admin::RawTransactions::Sequence, (uint64_t) sequence.get_int64());
                const UniValue &errorText = find_value(error, "error");
                builder.add(Admin::RawTransactions::ErrorMessage, errorText.get_str());
            }
        }
    }
};

// wallet

class ListUnspent : public AdminRPCBinding::Parser
{
public:
    ListUnspent() : Parser("listunspent", Admin::Wallet::ListUnspentReply) {}

    virtual int messageSizeCalc(const UniValue &result) const {
        return result.size() * 300;
    }

    virtual void createRequest(const Message &message, UniValue &output) {
        std::string txid;
        int minConf = -1;
        int maxConf = -1;
        std::list<std::vector<char>> addresses;
        Streaming::MessageParser parser(message.body());
        while (parser.next() == Streaming::FoundTag) {
            if (parser.tag() == Admin::Wallet::TransactionId
                    || parser.tag() == Admin::Wallet::GenericByteData) {
                addresses.push_back(parser.bytesData());
            }
            else if (parser.tag() == Admin::Wallet::MinimalConfirmations)
                minConf = boost::get<int>(parser.data());
            else if (parser.tag() == Admin::Wallet::MaximumConfirmations)
                maxConf = boost::get<int>(parser.data());
        }

        if (!addresses.empty()) { // ensure we have useful min/max conf
            minConf = std::max(minConf, 0);
            if (maxConf == -1)
                maxConf = 1E6;
        }

        if (minConf != -1)
            output.push_back(std::make_pair("parameter 1", UniValue(minConf)));
        if (maxConf != -1)
            output.push_back(std::make_pair("parameter 2", UniValue(maxConf)));

        UniValue list(UniValue::VOBJ);
        for (const std::vector<char> &address : addresses) {
            std::string addressString;
            boost::algorithm::hex(address, back_inserter(addressString));
            list.push_back(UniValue(UniValue::VSTR, addressString));
        }
        if (list.size() > 0)
            output.push_back(std::make_pair("addresses", list));
    }

    virtual void parser(Streaming::MessageBuilder &builder, const UniValue &result) {
        bool first = true;
        for (const UniValue &item : result.getValues()) {
            if (first) first = false;
            else builder.add(Admin::Wallet::Separator, true);
            const UniValue &txid = find_value(item, "txid");
            std::vector<char> bytearray;
            boost::algorithm::unhex(txid.get_str(), back_inserter(bytearray));
            builder.add(Admin::Wallet::TransactionId, bytearray);
            bytearray.clear();
            const UniValue &vout = find_value(item, "vout");
            builder.add(Admin::Wallet::TXOutputIndex, vout.get_int());
            const UniValue &address = find_value(item, "address");
            builder.add(Admin::Wallet::BitcoinAddress, address.get_str());
            const UniValue &scriptPubKey = find_value(item, "scriptPubKey");
            boost::algorithm::unhex(scriptPubKey.get_str(), back_inserter(bytearray));
            builder.add(Admin::Wallet::ScriptPubKey, bytearray);
            bytearray.clear();
            const UniValue &amount = find_value(item, "amount");
            builder.add(Admin::Wallet::Amount, (uint64_t) AmountFromValue(amount));
            const UniValue &confirmations = find_value(item, "confirmations");
            builder.add(Admin::Wallet::ConfirmationCount, confirmations.get_int());
        }
    }
};

class GetNewAddress : public AdminRPCBinding::Parser
{
public:
    GetNewAddress() : Parser("getnewaddress", Admin::Wallet::GetNewAddressReply, 50) {}
    virtual void parser(Streaming::MessageBuilder &builder, const UniValue &result) {
        builder.add(Admin::Wallet::BitcoinAddress, result.get_str());
    }
};

// Util

class CreateAddress : public AdminRPCBinding::Parser
{
public:
    CreateAddress() : Parser("createaddress", Admin::Util::CreateAddressReply, 150) {}

    virtual void parser(Streaming::MessageBuilder &builder, const UniValue &result) {
        const UniValue &address = find_value(result, "address");
        builder.add(Admin::Util::BitcoinAddress, address.get_str());
        const UniValue &scriptPubKey = find_value(result, "scriptPubKey");
        std::vector<char> bytearray;
        boost::algorithm::unhex(scriptPubKey.get_str(), back_inserter(bytearray));
        builder.add(Admin::Util::ScriptPubKey, bytearray);
        bytearray.clear();
        const UniValue &priv = find_value(result, "private");
        builder.add(Admin::Util::PrivateAddress, priv.get_str());
    }
};

class ValidateAddress : public AdminRPCBinding::Parser {
public:
    ValidateAddress() : Parser("validateaddress", Admin::Util::ValidateAddressReply, 300) {}

    virtual void parser(Streaming::MessageBuilder &builder, const UniValue &result) {
        const UniValue &isValid = find_value(result, "isvalid");
        builder.add(Admin::Util::IsValid, isValid.getBool());
        const UniValue &address = find_value(result, "address");
        builder.add(Admin::Util::BitcoinAddress, address.get_str());
        const UniValue &scriptPubKey = find_value(result, "scriptPubKey");
        std::vector<char> bytearray;
        boost::algorithm::unhex(scriptPubKey.get_str(), back_inserter(bytearray));
        builder.add(Admin::Util::ScriptPubKey, bytearray);
        bytearray.clear();
    }
    virtual void createRequest(const Message &message, UniValue &output) {
        Streaming::MessageParser parser(message.body());
        while (parser.next() == Streaming::FoundTag) {
            if (parser.tag() == Admin::Util::BitcoinAddress) {
                output.push_back(std::make_pair("param 1", UniValue(UniValue::VSTR, parser.stringData())));
                return;
            }
        }
    }
};
}


AdminRPCBinding::Parser *AdminRPCBinding::createParser(const Message &message)
{
    switch (message.serviceId()) {
    case Admin::BlockChainService:
        switch (message.messageId()) {
        case Admin::BlockChain::GetBlockChainInfo:
            return new GetBlockChainInfo();
        case Admin::BlockChain::GetBestBlockHash:
            return new GetBestBlockHash();
        case Admin::BlockChain::GetBlock:
            return new GetBlock();
        }
        break;
    case Admin::ControlService:
        switch (message.messageId()) {
        case Admin::Control::Stop:
            return new Parser("stop", Admin::Control::StopReply);
            break;
        }
        break;
    case Admin::RawTransactionService:
        switch (message.messageId()) {
        case Admin::RawTransactions::GetRawTransaction:
            return new GetRawTransaction();
        case Admin::RawTransactions::SendRawTransaction:
            return new SendRawTransaction();
        case Admin::RawTransactions::SignRawTransaction:
            return new SignRawTransaction();
        }
        break;
    case Admin::WalletService:
        switch (message.messageId()) {
        case Admin::Wallet::ListUnspent:
            return new ListUnspent();
        case Admin::Wallet::GetNewAddress:
            return new GetNewAddress();
        }
        break;
    case Admin::UtilService:
        switch (message.messageId()) {
        case Admin::Util::CreateAddress:
            return new CreateAddress();
        case Admin::Util::ValidateAddress:
            return new ValidateAddress();
        }
        break;
    }
    throw std::runtime_error("Unsupported command");
}


AdminRPCBinding::Parser::Parser(const std::string &method, int answerMessageId, int reserve)
    : m_reserve(reserve),
      m_answerMessageId(answerMessageId),
      m_method(method)
{
}

void AdminRPCBinding::Parser::parser(Streaming::MessageBuilder &builder, const UniValue &result)
{
    std::vector<char> answer;
    boost::algorithm::unhex(result.get_str(), back_inserter(answer));
    builder.add(1, answer);
}

void AdminRPCBinding::Parser::createRequest(const Message &, UniValue &)
{
}

int AdminRPCBinding::Parser::messageSizeCalc(const UniValue &result) const
{
    return result.get_str().size() + 20;
}

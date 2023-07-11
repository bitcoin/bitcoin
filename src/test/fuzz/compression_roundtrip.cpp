// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockstorage.h>
#include <node/transaction.h>
#include <node/chainstate.h>
#include <node/miner.h>
#include <node/coin.h>
#include <primitives/transaction.h>
#include <primitives/compression.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <test/util/setup_common.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/strencodings.h>
#include <cassert>
#include <chainparams.h>
#include <cmath>
#include <coins.h>
#include <core_io.h>
#include <key_io.h>
#include <pow.h>
#include <random.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <streams.h>
#include <uint256.h>
#include <univalue.h>
#include <validation.h>

using node::FindCoins;

namespace {
    class SecpContext {
            secp256k1_context* ctx;

        public:
            SecpContext() {
                ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
            }
            ~SecpContext() {
                secp256k1_context_destroy(ctx);
            }
            secp256k1_context* GetContext() {
                return ctx;
            }
    };
    struct Location {
        secp256k1_keypair keypair;
        CScript scriptPubKey;
        CScript redeemScript;
    };
    struct UTXO {
        uint256 txid;
        uint32_t vout;
        Location location;
        CAmount nValue;
        bool signable;
    };
    struct CompressionRoundtripFuzzTestingSetup : public TestChain100Setup {
        CompressionRoundtripFuzzTestingSetup(const ChainType& chain_type, const std::vector<const char*>& extra_args) : TestChain100Setup{chain_type, extra_args}
        {}

        std::vector<unsigned char> GetSerializedPubKey(secp256k1_pubkey pubkey, bool compressed) {
            size_t p_size = 33;
            auto p_type = SECP256K1_EC_COMPRESSED;
            if (!compressed) {
                p_size = 65;
                p_type = SECP256K1_EC_UNCOMPRESSED;
            }
            std::vector<unsigned char> p_vec(p_size);
            secp256k1_ec_pubkey_serialize(secp256k1_context_static, p_vec.data(), &p_size, &pubkey, p_type);
            return p_vec;
        };
        std::tuple<uint32_t, std::vector<CCompressedInput>> CompressOutPoints(CTransaction tx, bool compress) {
            LOCK(cs_main);
            std::vector<std::string> warnings;
            return m_node.chainman->ActiveChainstate().CompressOutPoints(tx, compress, warnings);
        }

        std::tuple<std::vector<COutPoint>, std::vector<CTxOut>> DecompressOutPoints(CCompressedTransaction tx) {
            LOCK(cs_main);
            std::vector<COutPoint> prevouts;
            CHECK_NONFATAL(m_node.chainman->ActiveChainstate().DecompressOutPoints(tx.minimumHeight(), tx.vin(), prevouts));
            std::vector<Coin> coins = FindCoins(m_node, prevouts);
            std::vector<CTxOut> outs;
            outs.reserve(coins.size());
            for (auto& coin : coins) {
                outs.push_back(coin.out);
            }
            return std::make_tuple(prevouts, outs);
        }

        bool IsTopBlock(CBlock block) {
            LOCK(cs_main);
            return m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash();
        }

       Location DeriveScript(int scriptType, secp256k1_pubkey pubkey) {
            switch (scriptType) {
                case 0: //Uncompressed PUBKEY
                case 1: { //Compressed PUBKEY
                    bool compressed = static_cast<bool>(scriptType);
                    CScript scriptPubKey;
                    scriptPubKey << GetSerializedPubKey(pubkey, compressed);
                    scriptPubKey << opcodetype::OP_CHECKSIG;
                    return Location{secp256k1_keypair(), scriptPubKey, CScript()};
                }
                case 2: //Uncompressed PUBKEYHASH
                case 3: { //Compressed PUBKEYHASH
                    bool compressed = static_cast<bool>(scriptType-2);
                    return Location{secp256k1_keypair(), GetScriptForDestination(PKHash(CPubKey(GetSerializedPubKey(pubkey, compressed)))), CScript()};
                }
                case 4: { //WITNESS_V0_KEYHASH
                    return Location{secp256k1_keypair(), GetScriptForDestination(WitnessV0KeyHash(CPubKey(GetSerializedPubKey(pubkey, true)))), CScript()};
                }
                case 5: { //WITNESS_V1_TAPROOT
                    secp256k1_xonly_pubkey xonlyPubkey;
                    assert(secp256k1_xonly_pubkey_from_pubkey(secp256k1_context_static, &xonlyPubkey, nullptr, &pubkey));
                    std::vector<unsigned char> xonlyPubkeyBytes(32);
                    secp256k1_xonly_pubkey_serialize(secp256k1_context_static, &xonlyPubkeyBytes[0], &xonlyPubkey);
                    return Location{secp256k1_keypair(), GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey(xonlyPubkeyBytes))), CScript()};
                }
                case 6: //SCRIPTHASH(Uncompressed PUBKEYHASH)
                case 7: //SCRIPTHASH(Compressed PUBKEYHASH)
                case 8: { //SCRIPTHASH(WITNESS_V0_KEYHASH)
                    CScript redeemScript = DeriveScript((scriptType%3)+2, pubkey).scriptPubKey;
                    return Location{secp256k1_keypair(), GetScriptForDestination(ScriptHash(redeemScript)), redeemScript};
                }
                case 9: {//WITNESS_V0_SCRIPTHASH(Compressed PUBKEYHASH)
                    CScript redeemScript = DeriveScript(3, pubkey).scriptPubKey;
                    return Location{secp256k1_keypair(), GetScriptForDestination(WitnessV0ScriptHash(redeemScript)), redeemScript};
                }
                default:
                    assert(false);
            }
        }

        bool GenerateLocation(secp256k1_context *ctx, std::function<std::optional<std::vector<unsigned char>>(int)> getrandbytes, Location& location, bool sign = true) {
            secp256k1_keypair keypair;
            auto optionRandBytes = getrandbytes(32);
            if (!optionRandBytes.has_value()) return false;
            std::vector<unsigned char> randBytes = optionRandBytes.value();

            if (!sign) {
                location.scriptPubKey = CScript(randBytes.begin(), randBytes.end());
                return true;
            }

            if (!secp256k1_keypair_create(ctx, &keypair, randBytes.data())) return false;
            secp256k1_pubkey pubkey;
            assert(secp256k1_keypair_pub(secp256k1_context_static, &pubkey, &keypair));

            auto optionRandByte = getrandbytes(1);
            if (!optionRandBytes.has_value()) return false;
            int scriptType = optionRandBytes.value()[0]%10;
            location = DeriveScript(scriptType, pubkey);
            location.keypair = keypair;
            return true;
        }

        bool SignTransaction(CMutableTransaction& mtx, const std::vector<secp256k1_keypair>& keypairs, const std::vector<CScript>& redeemScripts) {
            FillableSigningProvider keystore;
            for (const secp256k1_keypair& keypair : keypairs) {
                std::vector<unsigned char> secret_key(32);
                if (!secp256k1_keypair_sec(secp256k1_context_static, &secret_key[0], &keypair)) return false;
                CKey key;
                key.Set(secret_key.begin(), secret_key.end(), true);
                keystore.AddKey(key);
                if (!key.IsValid()) return false;
                CKey key2;
                key2.Set(secret_key.begin(), secret_key.end(), false);
                keystore.AddKey(key2);
                if (!key2.IsValid()) return false;
            }

            for (const CScript& redeemScript: redeemScripts) {
                keystore.AddCScript(redeemScript);
                keystore.AddCScript(GetScriptForDestination(WitnessV0ScriptHash(redeemScript)));
            }
            std::map<COutPoint, Coin> coins;
            for (const CTxIn& txin : mtx.vin) {
                coins[txin.prevout];
            }
            FindCoins(m_node, coins);

            std::map<int, bilingual_str> input_errors;
            ::SignTransaction(mtx, &keystore, coins, SIGHASH_ALL, input_errors);
            return input_errors.size() == 0;
        }
    };
    secp256k1_context* ctx = nullptr;
    SecpContext secp_context = SecpContext();
    CompressionRoundtripFuzzTestingSetup* fuzz_ctx = nullptr;
    std::vector<std::tuple<uint64_t, uint64_t>> savings;

    std::vector<UTXO> unspent_transactions;
    std::vector<UTXO> coinbase_transactions;
    CompressionRoundtripFuzzTestingSetup* InitializeCompressionRoundtripFuzzTestingSetup()
    {
        static const auto setup = MakeNoLogFileContext<CompressionRoundtripFuzzTestingSetup>();
        return setup.get();
    }
};

void compression_roundtrip_initialize()
{
    SelectParams(ChainType::REGTEST);
    fuzz_ctx = InitializeCompressionRoundtripFuzzTestingSetup();
    ctx = secp_context.GetContext();
    FastRandomContext frandom_ctx{uint256{125}};
    auto getrandbytes = [&frandom_ctx](int len) -> std::optional<std::vector<unsigned char>> { return frandom_ctx.randbytes(len);};

    //Create 300 coinbase transactions, Ignore the first hundred, Push the second hundred UTXOs to coinbase_transactions, and add the final hundred to the unspent_transaction to be used in the fuzz test
    for (int i = 0; i < 300; i++) {
        Location location;
        assert(fuzz_ctx->GenerateLocation(ctx, getrandbytes, location));

        CBlock coinbase_block = fuzz_ctx->CreateAndProcessBlock({}, location.scriptPubKey);
        assert(fuzz_ctx->IsTopBlock(coinbase_block));
        UTXO transaction = UTXO{coinbase_block.vtx.at(0)->GetHash(), 0, location, coinbase_block.vtx.at(0)->vout.at(0).nValue, true};
        if (i > 100) {
            if (i > 200) {
                unspent_transactions.push_back(transaction);
            } else {
                coinbase_transactions.push_back(transaction);
            }
        }
    }

    //Loop through the coinbase_transactions to assemble a bunch of random valued UTXOs for the fuzz test
    for (const auto& cbtx : coinbase_transactions) {

        CMutableTransaction mtx;
        mtx.nVersion = 0;
        mtx.nLockTime = 0;

        CTxIn in;
        in.prevout = COutPoint{Txid::FromUint256(cbtx.txid), cbtx.vout};
        in.nSequence = 0;
        mtx.vin.push_back(in);

        uint32_t index = 0;
        std::vector<UTXO> partial_transactions;
        uint32_t remaining_amount = cbtx.nValue;
        //Loop through the value of the coinbase transaction to create random valued outputs
        LIMITED_WHILE(remaining_amount > 2000, 10000) {
            uint32_t amount = frandom_ctx.randrange(remaining_amount-1000)+1;
            remaining_amount -= amount;

            bool sign = frandom_ctx.randbool();
            Location location;
            assert(fuzz_ctx->GenerateLocation(ctx, getrandbytes, location, sign));

            CTxOut out;
            out.nValue = amount;
            out.scriptPubKey = location.scriptPubKey;
            mtx.vout.push_back(out);

            partial_transactions.push_back(UTXO{uint256{0}, index, location, amount, sign});
            index++;
        }
        assert(mtx.vout.size() != 0);
        assert(fuzz_ctx->SignTransaction(mtx, {cbtx.location.keypair}, {cbtx.location.redeemScript}));
        //Add random valued UTXOs to the unspent_transactions after adding its txid
        uint256 txid = mtx.GetHash();
        for (auto &ptx : partial_transactions) {
            ptx.txid = txid;
            unspent_transactions.push_back(ptx);
        }

        Location location;
        assert(fuzz_ctx->GenerateLocation(ctx, getrandbytes, location));
        CScript main_scriptPubKey = location.scriptPubKey;
        CBlock main_block = fuzz_ctx->CreateAndProcessBlock({mtx}, main_scriptPubKey);
        assert(fuzz_ctx->IsTopBlock(main_block));
        //Add the resulting coinbase transaction to the unspent_transactions for the fuzz test
        unspent_transactions.push_back(UTXO{main_block.vtx[0]->GetHash(), 0, location, main_block.vtx[0]->vout[0].nValue, true});
    }
}

FUZZ_TARGET(compression_roundtrip, .init=compression_roundtrip_initialize)
{
    std::cout << "STARTFUZZ" << std::endl;
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    auto getrandbytes = [&fdp](int len) -> std::optional<std::vector<unsigned char>> {
        std::vector<unsigned char> data = fdp.ConsumeBytes<uint8_t>(len);
        if (data.size() != (size_t)len) return {};
        return data;
    };

    //Create the transaction to be compressed
    CMutableTransaction mtx;
    mtx.nVersion = fdp.ConsumeIntegral<int32_t>();
    mtx.nLockTime = fdp.ConsumeIntegral<uint32_t>();

    //Generate and add inputs to the Transaction
    int64_t total = 0;
    std::vector<secp256k1_keypair> keypairs;
    std::vector<CScript> redeemScripts;
    std::vector<int> used_indexs;
    bool sign_all = true;

    LIMITED_WHILE(total == 0 || fdp.ConsumeBool(), static_cast<unsigned char>(unspent_transactions.size()-1)) {
        int index = fdp.ConsumeIntegralInRange<int>(0, unspent_transactions.size()-1);
        if (std::find(used_indexs.begin(), used_indexs.end(), index) != used_indexs.end())
            break;
        used_indexs.push_back(index);
        UTXO tx = unspent_transactions[index];

        keypairs.push_back(tx.location.keypair);
        redeemScripts.push_back(tx.location.redeemScript);
        total += static_cast<int64_t>(tx.nValue);
        if (!tx.signable) sign_all = false;

        CTxIn in;
        in.prevout = COutPoint{Txid::FromUint256(tx.txid), tx.vout};
        in.nSequence = fdp.ConsumeIntegral<uint32_t>();
        mtx.vin.push_back(in);
    }


    //Generate and add outputs to the transaction based on the inputs with random output values
    int64_t remaining_amount = total;
    LIMITED_WHILE(remaining_amount > 2000 && (fdp.ConsumeBool() || mtx.vout.size() == 0), 10000) {
        CTxOut out;
        if (sign_all) {
            int64_t limit = pow(2, 16);
            int range_amount;
            if (remaining_amount > limit) {
                range_amount = limit-1000;
            } else {
                range_amount = remaining_amount-1000;
            }
            int64_t amount = fdp.ConsumeIntegralInRange<int64_t>(1, range_amount);
            remaining_amount -= amount;
            out.nValue = amount;
        } else {
            out.nValue = fdp.ConsumeIntegral<int64_t>();
        }
        Location location;
        if (!fuzz_ctx->GenerateLocation(ctx, getrandbytes, location)) return;
        out.scriptPubKey = location.scriptPubKey;
        mtx.vout.push_back(out);
    }
    if (mtx.vout.size() == 0) return;

    //If all randomly chosen inputs were signable sign the transaction
    if (sign_all) {
        assert(fuzz_ctx->SignTransaction(mtx, keypairs, redeemScripts));
    } else {
        fuzz_ctx->SignTransaction(mtx, keypairs, redeemScripts);
    }


    const CTransaction tx = CTransaction(mtx);

    std::cout << tx.ToString() << std::endl;

    bool compress_outpoints = fdp.ConsumeBool();
    //Compressed OutPoints for Inputs
    std::tuple<uint32_t, std::vector<CCompressedInput>> cinputstup = fuzz_ctx->CompressOutPoints(tx, compress_outpoints);

    //Compress Transaction
    CCompressedTransaction compressed_transaction = CCompressedTransaction(tx, std::get<0>(cinputstup), std::get<1>(cinputstup));
    std::cout << compressed_transaction.ToString() << std::endl;
    for (size_t i = 0; i < compressed_transaction.vin().size(); i++) {
        if (!std::get<1>(cinputstup)[i].scriptPubKey().IsPayToPublicKey()){
            assert(compressed_transaction.vin()[i].warning().empty());
        }
    }

    //Serialize Transaction
    DataStream stream;
    compressed_transaction.Serialize(stream);

    //Deserialize Transaction
    CCompressedTransaction uct = CCompressedTransaction(deserialize, stream);
    assert(compressed_transaction == uct);

    //Decompress OutPoints for Inputs
    std::tuple<std::vector<COutPoint>, std::vector<CTxOut>> result2 = fuzz_ctx->DecompressOutPoints(uct);
    //Decompress Transaction
    CTransaction new_tx = CTransaction(CMutableTransaction(uct, std::get<0>(result2), std::get<1>(result2)));
    std::cout << new_tx.ToString() << std::endl;
    //Verify Decompressed Transaction matches original
    assert(tx == new_tx);
}

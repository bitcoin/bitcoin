// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsbase.h>
#include <key.h>
#include <key_io.h>
#include <outputtype.h>
#include <policy/policy.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <script/keyorigin.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <util/memory.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

void initialize()
{
    static const ECCVerifyHandle ecc_verify_handle;
    ECC_Start();
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const CKey key = [&] {
        CKey k;
        k.Set(buffer.begin(), buffer.end(), true);
        return k;
    }();
    if (!key.IsValid()) {
        return;
    }

    {
        assert(key.begin() + key.size() == key.end());
        assert(key.IsCompressed());
        assert(key.size() == 32);
        assert(DecodeSecret(EncodeSecret(key)) == key);
    }

    {
        CKey invalid_key;
        assert(!(invalid_key == key));
        assert(!invalid_key.IsCompressed());
        assert(!invalid_key.IsValid());
        assert(invalid_key.size() == 0);
    }

    {
        CKey uncompressed_key;
        uncompressed_key.Set(buffer.begin(), buffer.end(), false);
        assert(!(uncompressed_key == key));
        assert(!uncompressed_key.IsCompressed());
        assert(key.size() == 32);
        assert(uncompressed_key.begin() + uncompressed_key.size() == uncompressed_key.end());
        assert(uncompressed_key.IsValid());
    }

    {
        CKey copied_key;
        copied_key.Set(key.begin(), key.end(), key.IsCompressed());
        assert(copied_key == key);
    }

    {
        CKey negated_key = key;
        negated_key.Negate();
        assert(negated_key.IsValid());
        assert(!(negated_key == key));

        negated_key.Negate();
        assert(negated_key == key);
    }

    const uint256 random_uint256 = Hash(buffer);

    {
        CKey child_key;
        ChainCode child_chaincode;
        const bool ok = key.Derive(child_key, child_chaincode, 0, random_uint256);
        assert(ok);
        assert(child_key.IsValid());
        assert(!(child_key == key));
        assert(child_chaincode != random_uint256);
    }

    const CPubKey pubkey = key.GetPubKey();

    {
        assert(pubkey.size() == 33);
        assert(key.VerifyPubKey(pubkey));
        assert(pubkey.GetHash() != random_uint256);
        assert(pubkey.begin() + pubkey.size() == pubkey.end());
        assert(pubkey.data() == pubkey.begin());
        assert(pubkey.IsCompressed());
        assert(pubkey.IsValid());
        assert(pubkey.IsFullyValid());
        assert(HexToPubKey(HexStr(pubkey)) == pubkey);
        assert(GetAllDestinationsForKey(pubkey).size() == 3);
    }

    {
        CDataStream data_stream{SER_NETWORK, INIT_PROTO_VERSION};
        pubkey.Serialize(data_stream);

        CPubKey pubkey_deserialized;
        pubkey_deserialized.Unserialize(data_stream);
        assert(pubkey_deserialized == pubkey);
    }

    {
        const CScript tx_pubkey_script = GetScriptForRawPubKey(pubkey);
        assert(!tx_pubkey_script.IsPayToScriptHash());
        assert(!tx_pubkey_script.IsPayToWitnessScriptHash());
        assert(!tx_pubkey_script.IsPushOnly());
        assert(!tx_pubkey_script.IsUnspendable());
        assert(tx_pubkey_script.HasValidOps());
        assert(tx_pubkey_script.size() == 35);

        const CScript tx_multisig_script = GetScriptForMultisig(1, {pubkey});
        assert(!tx_multisig_script.IsPayToScriptHash());
        assert(!tx_multisig_script.IsPayToWitnessScriptHash());
        assert(!tx_multisig_script.IsPushOnly());
        assert(!tx_multisig_script.IsUnspendable());
        assert(tx_multisig_script.HasValidOps());
        assert(tx_multisig_script.size() == 37);

        FillableSigningProvider fillable_signing_provider;
        assert(IsSolvable(fillable_signing_provider, tx_pubkey_script));
        assert(IsSolvable(fillable_signing_provider, tx_multisig_script));
        assert(!IsSegWitOutput(fillable_signing_provider, tx_pubkey_script));
        assert(!IsSegWitOutput(fillable_signing_provider, tx_multisig_script));
        assert(fillable_signing_provider.GetKeys().size() == 0);
        assert(!fillable_signing_provider.HaveKey(pubkey.GetID()));

        const bool ok_add_key = fillable_signing_provider.AddKey(key);
        assert(ok_add_key);
        assert(fillable_signing_provider.HaveKey(pubkey.GetID()));

        FillableSigningProvider fillable_signing_provider_pub;
        assert(!fillable_signing_provider_pub.HaveKey(pubkey.GetID()));

        const bool ok_add_key_pubkey = fillable_signing_provider_pub.AddKeyPubKey(key, pubkey);
        assert(ok_add_key_pubkey);
        assert(fillable_signing_provider_pub.HaveKey(pubkey.GetID()));

        TxoutType which_type_tx_pubkey;
        const bool is_standard_tx_pubkey = IsStandard(tx_pubkey_script, which_type_tx_pubkey);
        assert(is_standard_tx_pubkey);
        assert(which_type_tx_pubkey == TxoutType::PUBKEY);

        TxoutType which_type_tx_multisig;
        const bool is_standard_tx_multisig = IsStandard(tx_multisig_script, which_type_tx_multisig);
        assert(is_standard_tx_multisig);
        assert(which_type_tx_multisig == TxoutType::MULTISIG);

        std::vector<std::vector<unsigned char>> v_solutions_ret_tx_pubkey;
        const TxoutType outtype_tx_pubkey = Solver(tx_pubkey_script, v_solutions_ret_tx_pubkey);
        assert(outtype_tx_pubkey == TxoutType::PUBKEY);
        assert(v_solutions_ret_tx_pubkey.size() == 1);
        assert(v_solutions_ret_tx_pubkey[0].size() == 33);

        std::vector<std::vector<unsigned char>> v_solutions_ret_tx_multisig;
        const TxoutType outtype_tx_multisig = Solver(tx_multisig_script, v_solutions_ret_tx_multisig);
        assert(outtype_tx_multisig == TxoutType::MULTISIG);
        assert(v_solutions_ret_tx_multisig.size() == 3);
        assert(v_solutions_ret_tx_multisig[0].size() == 1);
        assert(v_solutions_ret_tx_multisig[1].size() == 33);
        assert(v_solutions_ret_tx_multisig[2].size() == 1);

        OutputType output_type{};
        const CTxDestination tx_destination = GetDestinationForKey(pubkey, output_type);
        assert(output_type == OutputType::LEGACY);
        assert(IsValidDestination(tx_destination));
        assert(CTxDestination{PKHash{pubkey}} == tx_destination);

        const CScript script_for_destination = GetScriptForDestination(tx_destination);
        assert(script_for_destination.size() == 25);

        const std::string destination_address = EncodeDestination(tx_destination);
        assert(DecodeDestination(destination_address) == tx_destination);

        const CPubKey pubkey_from_address_string = AddrToPubKey(fillable_signing_provider, destination_address);
        assert(pubkey_from_address_string == pubkey);

        CKeyID key_id = pubkey.GetID();
        assert(!key_id.IsNull());
        assert(key_id == CKeyID{key_id});
        assert(key_id == GetKeyForDestination(fillable_signing_provider, tx_destination));

        CPubKey pubkey_out;
        const bool ok_get_pubkey = fillable_signing_provider.GetPubKey(key_id, pubkey_out);
        assert(ok_get_pubkey);

        CKey key_out;
        const bool ok_get_key = fillable_signing_provider.GetKey(key_id, key_out);
        assert(ok_get_key);
        assert(fillable_signing_provider.GetKeys().size() == 1);
        assert(fillable_signing_provider.HaveKey(key_id));

        KeyOriginInfo key_origin_info;
        const bool ok_get_key_origin = fillable_signing_provider.GetKeyOrigin(key_id, key_origin_info);
        assert(!ok_get_key_origin);
    }

    {
        const std::vector<unsigned char> vch_pubkey{pubkey.begin(), pubkey.end()};
        assert(CPubKey::ValidSize(vch_pubkey));
        assert(!CPubKey::ValidSize({pubkey.begin(), pubkey.begin() + pubkey.size() - 1}));

        const CPubKey pubkey_ctor_1{vch_pubkey};
        assert(pubkey == pubkey_ctor_1);

        const CPubKey pubkey_ctor_2{vch_pubkey.begin(), vch_pubkey.end()};
        assert(pubkey == pubkey_ctor_2);

        CPubKey pubkey_set;
        pubkey_set.Set(vch_pubkey.begin(), vch_pubkey.end());
        assert(pubkey == pubkey_set);
    }

    {
        const CPubKey invalid_pubkey{};
        assert(!invalid_pubkey.IsValid());
        assert(!invalid_pubkey.IsFullyValid());
        assert(!(pubkey == invalid_pubkey));
        assert(pubkey != invalid_pubkey);
        assert(pubkey < invalid_pubkey);
    }

    {
        // Cover CPubKey's operator[](unsigned int pos)
        unsigned int sum = 0;
        for (size_t i = 0; i < pubkey.size(); ++i) {
            sum += pubkey[i];
        }
        assert(std::accumulate(pubkey.begin(), pubkey.end(), 0U) == sum);
    }

    {
        CPubKey decompressed_pubkey = pubkey;
        assert(decompressed_pubkey.IsCompressed());

        const bool ok = decompressed_pubkey.Decompress();
        assert(ok);
        assert(!decompressed_pubkey.IsCompressed());
        assert(decompressed_pubkey.size() == 65);
    }

    {
        std::vector<unsigned char> vch_sig;
        const bool ok = key.Sign(random_uint256, vch_sig, false);
        assert(ok);
        assert(pubkey.Verify(random_uint256, vch_sig));
        assert(CPubKey::CheckLowS(vch_sig));

        const std::vector<unsigned char> vch_invalid_sig{vch_sig.begin(), vch_sig.begin() + vch_sig.size() - 1};
        assert(!pubkey.Verify(random_uint256, vch_invalid_sig));
        assert(!CPubKey::CheckLowS(vch_invalid_sig));
    }

    {
        std::vector<unsigned char> vch_compact_sig;
        const bool ok_sign_compact = key.SignCompact(random_uint256, vch_compact_sig);
        assert(ok_sign_compact);

        CPubKey recover_pubkey;
        const bool ok_recover_compact = recover_pubkey.RecoverCompact(random_uint256, vch_compact_sig);
        assert(ok_recover_compact);
        assert(recover_pubkey == pubkey);
    }

    {
        CPubKey child_pubkey;
        ChainCode child_chaincode;
        const bool ok = pubkey.Derive(child_pubkey, child_chaincode, 0, random_uint256);
        assert(ok);
        assert(child_pubkey != pubkey);
        assert(child_pubkey.IsCompressed());
        assert(child_pubkey.IsFullyValid());
        assert(child_pubkey.IsValid());
        assert(child_pubkey.size() == 33);
        assert(child_chaincode != random_uint256);
    }

    const CPrivKey priv_key = key.GetPrivKey();

    {
        for (const bool skip_check : {true, false}) {
            CKey loaded_key;
            const bool ok = loaded_key.Load(priv_key, pubkey, skip_check);
            assert(ok);
            assert(key == loaded_key);
        }
    }
}

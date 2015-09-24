#include "omnicore/rpcrawtx.h"

#include "omnicore/omnicore.h"
#include "omnicore/rpc.h"
#include "omnicore/rpctxobject.h"
#include "omnicore/utilsbitcoin.h"

#include "coins.h"
#include "core_io.h"
#include "primitives/transaction.h"
#include "rpcserver.h"
#include "sync.h"
#include "uint256.h"

#include "json/json_spirit_value.h"

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include <stdint.h>
#include <algorithm>
#include <stdexcept>
#include <string>

using mastercore::GetHeight;
using mastercore::cs_tx_cache;
using mastercore::view;

using namespace json_spirit;

Value omni_decodetransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "omni_decodetransaction \"txhex\" ( \"prevtxs\" )\n"

            "\nDecodes an Omni transaction.\n"

            "\nIf the inputs of the transaction are not in the chain, then they must be provided, because "
            "the transaction inputs are used to identify the sender of a transaction.\n"

            "\nArguments:\n"
            "1. txhex                (string, required) the raw transaction to decode\n"
            "2. prevtxs              (string, optional) a JSON array of transaction inputs (default: none)\n"
            "     [\n"
            "       {\n"
            "         \"txid\":\"hash\",          (string, required) the transaction hash\n"
            "         \"vout\":n,               (number, required) the output number\n"
            "         \"scriptPubKey\":\"hex\",   (string, required) the output script\n"
            "         \"value\":n.nnnnnnnn      (number, required) the output value\n"
            "       }\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
            "  \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in bitcoins\n"
            "  \"sendingaddress\" : \"address\",     (string) the Bitcoin address of the sender\n"
            "  \"referenceaddress\" : \"address\",   (string) a Bitcoin address used as reference (if any)\n"
            "  \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
            "  \"version\" : n,                    (number) the transaction version\n"
            "  \"type_int\" : n,                   (number) the transaction type as number\n"
            "  \"type\" : \"type\",                  (string) the transaction type as string\n"
            "  [...]                             (mixed) other transaction type specific properties\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_decodetransaction", "\"010000000163af14ce6d477e1c793507e32a5b7696288fa89705c0d02a3f66beb3c5b8afee0100000000ffffffff02ac020000000000004751210261ea979f6a06f9dafe00fb1263ea0aca959875a7073556a088cdfadcd494b3752102a3fd0a8a067e06941e066f78d930bfc47746f097fcd3f7ab27db8ddf37168b6b52ae22020000000000001976a914946cb2e08075bcbaf157e47bcb67eb2b2339d24288ac00000000\" \"[{\\\"txid\\\":\\\"eeafb8c5b3be663f2ad0c00597a88f2896765b2ae30735791c7e476dce14af63\\\",\\\"vout\\\":1,\\\"scriptPubKey\\\":\\\"76a9149084c0bd89289bc025d0264f7f23148fb683d56c88ac\\\",\\\"value\\\":0.0001123}]\"")
            + HelpExampleRpc("omni_decodetransaction", "\"010000000163af14ce6d477e1c793507e32a5b7696288fa89705c0d02a3f66beb3c5b8afee0100000000ffffffff02ac020000000000004751210261ea979f6a06f9dafe00fb1263ea0aca959875a7073556a088cdfadcd494b3752102a3fd0a8a067e06941e066f78d930bfc47746f097fcd3f7ab27db8ddf37168b6b52ae22020000000000001976a914946cb2e08075bcbaf157e47bcb67eb2b2339d24288ac00000000\", \"[{\\\"txid\\\":\\\"eeafb8c5b3be663f2ad0c00597a88f2896765b2ae30735791c7e476dce14af63\\\",\\\"vout\\\":1,\\\"scriptPubKey\\\":\\\"76a9149084c0bd89289bc025d0264f7f23148fb683d56c88ac\\\",\\\"value\\\":0.0001123}]\"")
        );

    CTransaction tx;
    if (!DecodeHexTx(tx, params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Transaction deserialization failed");
    }

    // use a dummy coins view to store the user provided transaction inputs
    CCoinsView viewDummyTemp;
    CCoinsViewCache viewTemp(&viewDummyTemp);

    if (params.size() > 1 && params[1].type() != json_spirit::null_type) {
        Array prevTxs = params[1].get_array();
        BOOST_FOREACH(Value& p, prevTxs) {
            if (p.type() != json_spirit::obj_type) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\",\"value\":n.nnnnnnnn}");
            }
            Object prevOut = p.get_obj();
            RPCTypeCheck(prevOut, boost::assign::map_list_of("txid", json_spirit::str_type)("vout", json_spirit::int_type)("scriptPubKey", json_spirit::str_type)("value", json_spirit::real_type));

            uint256 txid = ParseHashO(prevOut, "txid");
            int nOut = json_spirit::find_value(prevOut, "vout").get_int();
            if (nOut < 0) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");
            }
            std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            Value outputValue = json_spirit::find_value(prevOut, "value");
            {
                CCoinsModifier coins = viewTemp.ModifyCoins(txid);
                if ((size_t) nOut >= coins->vout.size()) {
                    coins->vout.resize(nOut+1);
                }
                coins->vout[nOut].scriptPubKey = CScript(pkData.begin(), pkData.end());
                coins->vout[nOut].nValue = AmountFromValue(outputValue);
            }
        }
    }

    Object txObj;
    int populateResult = -3331;
    {
        LOCK(cs_tx_cache);
        // temporarily switch global coins view cache for transaction inputs
        std::swap(view, viewTemp);
        // then get the results
        populateResult = populateRPCTransactionObject(tx, 0, txObj);
        // and restore the original, unpolluted coins view cache
        std::swap(viewTemp, view);
    }

    if (populateResult != 0) PopulateFailure(populateResult);

    return txObj;
}


#include <iostream>
#include <fstream>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/test/unit_test.hpp>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#include "main.h"
#include "wallet.h"

using namespace std;
using namespace json_spirit;
using namespace boost::algorithm;

extern uint256 SignatureHash(CScript scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);
extern bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn,
                         bool fValidatePayToScriptHash, int nHashType);

CScript
ParseScript(string s)
{
    CScript result;

    static map<string, opcodetype> mapOpNames;

    if (mapOpNames.size() == 0)
    {
        for (int op = OP_NOP; op <= OP_NOP10; op++)
        {
            const char* name = GetOpName((opcodetype)op);
            if (strcmp(name, "OP_UNKNOWN") == 0)
                continue;
            string strName(name);
            mapOpNames[strName] = (opcodetype)op;
            // Convenience: OP_ADD and just ADD are both recognized:
            replace_first(strName, "OP_", "");
            mapOpNames[strName] = (opcodetype)op;
        }
    }

    vector<string> words;
    split(words, s, is_any_of(" \t\n"), token_compress_on);

    BOOST_FOREACH(string w, words)
    {
        if (all(w, is_digit()) ||
            (starts_with(w, "-") && all(string(w.begin()+1, w.end()), is_digit())))
        {
            // Number
            int64 n = atoi64(w);
            result << n;
        }
        else if (starts_with(w, "0x") && IsHex(string(w.begin()+2, w.end())))
        {
            // Hex data:
            result << ParseHex(string(w.begin()+2, w.end()));
        }
        else if (s.size() >= 2 && starts_with(w, "'") && ends_with(w, "'"))
        {
            // Single-quoted string, pushed as data:
            std::vector<unsigned char> value(s.begin()+1, s.end()-1);
            result << value;
        }
        else if (mapOpNames.count(w))
        {
            // opcode, e.g. OP_ADD or OP_1:
            result << mapOpNames[w];
        }
        else
        {
            BOOST_ERROR("Parse error: " << s);
            return CScript();
        }                        
    }

    return result;
}

Array
read_json(const std::string& filename)
{
    namespace fs = boost::filesystem;
    fs::path testFile = fs::current_path() / "test" / "data" / filename;

#ifdef TEST_DATA_DIR
    if (!fs::exists(testFile))
    {
        testFile = fs::path(BOOST_PP_STRINGIZE(TEST_DATA_DIR)) / filename;
    }
#endif

    ifstream ifs(testFile.string().c_str(), ifstream::in);
    Value v;
    if (!read_stream(ifs, v))
    {
        BOOST_ERROR("Cound not find/open " << filename);
        return Array();
    }
    if (v.type() != array_type)
    {
        BOOST_ERROR(filename << " does not contain a json array");
        return Array();
    }

    return v.get_array();
}

BOOST_AUTO_TEST_SUITE(script_tests)

BOOST_AUTO_TEST_CASE(script_valid)
{
    // Read tests from test/data/script_valid.json
    // Format is an array of arrays
    // Inner arrays are [ "scriptSig", "scriptPubKey" ]
    // ... where scriptSig and scriptPubKey are stringified
    // scripts.
    Array tests = read_json("script_valid.json");

    BOOST_FOREACH(Value& tv, tests)
    {
        Array test = tv.get_array();
        string strTest = write_string(tv, false);
        if (test.size() < 2) // Allow size > 2; extra stuff ignored (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        string scriptSigString = test[0].get_str();
        CScript scriptSig = ParseScript(scriptSigString);
        string scriptPubKeyString = test[1].get_str();
        CScript scriptPubKey = ParseScript(scriptPubKeyString);

        CTransaction tx;
        BOOST_CHECK_MESSAGE(VerifyScript(scriptSig, scriptPubKey, tx, 0, true, SIGHASH_NONE), strTest);
    }
}

BOOST_AUTO_TEST_CASE(script_invalid)
{
    // Scripts that should evaluate as invalid
    Array tests = read_json("script_invalid.json");

    BOOST_FOREACH(Value& tv, tests)
    {
        Array test = tv.get_array();
        string strTest = write_string(tv, false);
        if (test.size() < 2) // Allow size > 2; extra stuff ignored (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        string scriptSigString = test[0].get_str();
        CScript scriptSig = ParseScript(scriptSigString);
        string scriptPubKeyString = test[1].get_str();
        CScript scriptPubKey = ParseScript(scriptPubKeyString);

        CTransaction tx;
        BOOST_CHECK_MESSAGE(!VerifyScript(scriptSig, scriptPubKey, tx, 0, true, SIGHASH_NONE), strTest);
    }
}

BOOST_AUTO_TEST_CASE(script_PushData)
{
    // Check that PUSHDATA1, PUSHDATA2, and PUSHDATA4 create the same value on
    // the stack as the 1-75 opcodes do.
    static const unsigned char direct[] = { 1, 0x5a };
    static const unsigned char pushdata1[] = { OP_PUSHDATA1, 1, 0x5a };
    static const unsigned char pushdata2[] = { OP_PUSHDATA2, 1, 0, 0x5a };
    static const unsigned char pushdata4[] = { OP_PUSHDATA4, 1, 0, 0, 0, 0x5a };

    vector<vector<unsigned char> > directStack;
    BOOST_CHECK(EvalScript(directStack, CScript(&direct[0], &direct[sizeof(direct)]), CTransaction(), 0, 0));

    vector<vector<unsigned char> > pushdata1Stack;
    BOOST_CHECK(EvalScript(pushdata1Stack, CScript(&pushdata1[0], &pushdata1[sizeof(pushdata1)]), CTransaction(), 0, 0));
    BOOST_CHECK(pushdata1Stack == directStack);

    vector<vector<unsigned char> > pushdata2Stack;
    BOOST_CHECK(EvalScript(pushdata2Stack, CScript(&pushdata2[0], &pushdata2[sizeof(pushdata2)]), CTransaction(), 0, 0));
    BOOST_CHECK(pushdata2Stack == directStack);

    vector<vector<unsigned char> > pushdata4Stack;
    BOOST_CHECK(EvalScript(pushdata4Stack, CScript(&pushdata4[0], &pushdata4[sizeof(pushdata4)]), CTransaction(), 0, 0));
    BOOST_CHECK(pushdata4Stack == directStack);
}

CScript
sign_multisig(CScript scriptPubKey, std::vector<CKey> keys, CTransaction transaction)
{
    uint256 hash = SignatureHash(scriptPubKey, transaction, 0, SIGHASH_ALL);

    CScript result;
    //
    // NOTE: CHECKMULTISIG has an unfortunate bug; it requires
    // one extra item on the stack, before the signatures.
    // Putting OP_0 on the stack is the workaround;
    // fixing the bug would mean splitting the blockchain (old
    // clients would not accept new CHECKMULTISIG transactions,
    // and vice-versa)
    //
    result << OP_0;
    BOOST_FOREACH(CKey key, keys)
    {
        vector<unsigned char> vchSig;
        BOOST_CHECK(key.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        result << vchSig;
    }
    return result;
}
CScript
sign_multisig(CScript scriptPubKey, CKey key, CTransaction transaction)
{
    std::vector<CKey> keys;
    keys.push_back(key);
    return sign_multisig(scriptPubKey, keys, transaction);
}

BOOST_AUTO_TEST_CASE(script_CHECKMULTISIG12)
{
    CKey key1, key2, key3;
    key1.MakeNewKey(true);
    key2.MakeNewKey(false);
    key3.MakeNewKey(true);

    CScript scriptPubKey12;
    scriptPubKey12 << OP_1 << key1.GetPubKey() << key2.GetPubKey() << OP_2 << OP_CHECKMULTISIG;

    CTransaction txFrom12;
    txFrom12.vout.resize(1);
    txFrom12.vout[0].scriptPubKey = scriptPubKey12;

    CTransaction txTo12;
    txTo12.vin.resize(1);
    txTo12.vout.resize(1);
    txTo12.vin[0].prevout.n = 0;
    txTo12.vin[0].prevout.hash = txFrom12.GetHash();
    txTo12.vout[0].nValue = 1;

    CScript goodsig1 = sign_multisig(scriptPubKey12, key1, txTo12);
    BOOST_CHECK(VerifyScript(goodsig1, scriptPubKey12, txTo12, 0, true, 0));
    txTo12.vout[0].nValue = 2;
    BOOST_CHECK(!VerifyScript(goodsig1, scriptPubKey12, txTo12, 0, true, 0));

    CScript goodsig2 = sign_multisig(scriptPubKey12, key2, txTo12);
    BOOST_CHECK(VerifyScript(goodsig2, scriptPubKey12, txTo12, 0, true, 0));

    CScript badsig1 = sign_multisig(scriptPubKey12, key3, txTo12);
    BOOST_CHECK(!VerifyScript(badsig1, scriptPubKey12, txTo12, 0, true, 0));
}

BOOST_AUTO_TEST_CASE(script_CHECKMULTISIG23)
{
    CKey key1, key2, key3, key4;
    key1.MakeNewKey(true);
    key2.MakeNewKey(false);
    key3.MakeNewKey(true);
    key4.MakeNewKey(false);

    CScript scriptPubKey23;
    scriptPubKey23 << OP_2 << key1.GetPubKey() << key2.GetPubKey() << key3.GetPubKey() << OP_3 << OP_CHECKMULTISIG;

    CTransaction txFrom23;
    txFrom23.vout.resize(1);
    txFrom23.vout[0].scriptPubKey = scriptPubKey23;

    CTransaction txTo23;
    txTo23.vin.resize(1);
    txTo23.vout.resize(1);
    txTo23.vin[0].prevout.n = 0;
    txTo23.vin[0].prevout.hash = txFrom23.GetHash();
    txTo23.vout[0].nValue = 1;

    std::vector<CKey> keys;
    keys.push_back(key1); keys.push_back(key2);
    CScript goodsig1 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(VerifyScript(goodsig1, scriptPubKey23, txTo23, 0, true, 0));

    keys.clear();
    keys.push_back(key1); keys.push_back(key3);
    CScript goodsig2 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(VerifyScript(goodsig2, scriptPubKey23, txTo23, 0, true, 0));

    keys.clear();
    keys.push_back(key2); keys.push_back(key3);
    CScript goodsig3 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(VerifyScript(goodsig3, scriptPubKey23, txTo23, 0, true, 0));

    keys.clear();
    keys.push_back(key2); keys.push_back(key2); // Can't re-use sig
    CScript badsig1 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(!VerifyScript(badsig1, scriptPubKey23, txTo23, 0, true, 0));

    keys.clear();
    keys.push_back(key2); keys.push_back(key1); // sigs must be in correct order
    CScript badsig2 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(!VerifyScript(badsig2, scriptPubKey23, txTo23, 0, true, 0));

    keys.clear();
    keys.push_back(key3); keys.push_back(key2); // sigs must be in correct order
    CScript badsig3 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(!VerifyScript(badsig3, scriptPubKey23, txTo23, 0, true, 0));

    keys.clear();
    keys.push_back(key4); keys.push_back(key2); // sigs must match pubkeys
    CScript badsig4 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(!VerifyScript(badsig4, scriptPubKey23, txTo23, 0, true, 0));

    keys.clear();
    keys.push_back(key1); keys.push_back(key4); // sigs must match pubkeys
    CScript badsig5 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(!VerifyScript(badsig5, scriptPubKey23, txTo23, 0, true, 0));

    keys.clear(); // Must have signatures
    CScript badsig6 = sign_multisig(scriptPubKey23, keys, txTo23);
    BOOST_CHECK(!VerifyScript(badsig6, scriptPubKey23, txTo23, 0, true, 0));
}    


BOOST_AUTO_TEST_SUITE_END()

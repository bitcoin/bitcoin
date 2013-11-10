//
// Unit tests for canonical signatures
//



#include "script.h"
#include "util.h"
#include "data/sig_noncanonical.json.h"
#include "data/sig_canonical.json.h"

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include "json/json_spirit_writer_template.h"
#include <openssl/ecdsa.h>

using namespace std;
using namespace json_spirit;


// In script_tests.cpp
extern Array read_json(const std::string& jsondata);

BOOST_AUTO_TEST_SUITE(canonical_tests)

// OpenSSL-based test for canonical signature (without test for hashtype byte)
bool static IsCanonicalSignature_OpenSSL_inner(const std::vector<unsigned char>& vchSig)
{
    if (vchSig.size() == 0)
        return false;
    const unsigned char *input = &vchSig[0];
    ECDSA_SIG *psig = NULL;
    d2i_ECDSA_SIG(&psig, &input, vchSig.size());
    if (psig == NULL)
        return false;
    unsigned char buf[256];
    unsigned char *pbuf = buf;
    unsigned int nLen = i2d_ECDSA_SIG(psig, NULL);
    if (nLen != vchSig.size()) {
        ECDSA_SIG_free(psig);
        return false;
    }
    nLen = i2d_ECDSA_SIG(psig, &pbuf);
    ECDSA_SIG_free(psig);
    return (memcmp(&vchSig[0], &buf[0], nLen) == 0);
}

// OpenSSL-based test for canonical signature
bool static IsCanonicalSignature_OpenSSL(const std::vector<unsigned char> &vchSignature) {
    if (vchSignature.size() < 1)
        return false;
    if (vchSignature.size() > 127)
        return false;
    if (vchSignature[vchSignature.size() - 1] & 0x7C)
        return false;

    std::vector<unsigned char> vchSig(vchSignature);
    vchSig.pop_back();
    if (!IsCanonicalSignature_OpenSSL_inner(vchSig))
        return false;
    return true;
}

BOOST_AUTO_TEST_CASE(script_canon)
{
    Array tests = read_json(std::string(json_tests::sig_canonical, json_tests::sig_canonical + sizeof(json_tests::sig_canonical)));

    BOOST_FOREACH(Value &tv, tests) {
        string test = tv.get_str();
        if (IsHex(test)) {
            std::vector<unsigned char> sig = ParseHex(test);
            BOOST_CHECK_MESSAGE(IsCanonicalSignature(sig, SCRIPT_VERIFY_STRICTENC), test);
            BOOST_CHECK_MESSAGE(IsCanonicalSignature_OpenSSL(sig), test);
        }
    }
}

BOOST_AUTO_TEST_CASE(script_noncanon)
{
    Array tests = read_json(std::string(json_tests::sig_noncanonical, json_tests::sig_noncanonical + sizeof(json_tests::sig_noncanonical)));

    BOOST_FOREACH(Value &tv, tests) {
        string test = tv.get_str();
        if (IsHex(test)) {
            std::vector<unsigned char> sig = ParseHex(test);
            BOOST_CHECK_MESSAGE(!IsCanonicalSignature(sig, SCRIPT_VERIFY_STRICTENC), test);
            BOOST_CHECK_MESSAGE(!IsCanonicalSignature_OpenSSL(sig), test);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

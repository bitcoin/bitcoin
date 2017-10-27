// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "smsg/smessage.h"

#include "test/test_particl.h"
#include "net.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <boost/test/unit_test.hpp>

struct SmsgTestingSetup : public TestingSetup {
    SmsgTestingSetup() : TestingSetup(CBaseChainParams::MAIN, true) {}
};

BOOST_FIXTURE_TEST_SUITE(smsg_tests, SmsgTestingSetup)



BOOST_AUTO_TEST_CASE(smsg_test_ckeyId_inits_null)
{
    CKeyID k;
    BOOST_CHECK(k.IsNull());
}

BOOST_AUTO_TEST_CASE(smsg_test)
{
#ifdef ENABLE_WALLET
    SeedInsecureRand();
    fParticlMode = true;
    g_connman = std::unique_ptr<CConnman>(new CConnman(GetRand(std::numeric_limits<uint64_t>::max()), GetRand(std::numeric_limits<uint64_t>::max())));

    const std::string sTestMessage =
        "A short test message 0123456789 !@#$%^&*()_+-=";

    fSecMsgEnabled = true;
    int rv = 0;
    const int nKeys = 12;
    CWallet keystore;
    std::vector<CKey> keyOwn(nKeys);
    for (int i = 0; i < nKeys; i++)
    {
        InsecureNewKey(keyOwn[i], true);
        LOCK(keystore.cs_wallet);
        keystore.AddKey(keyOwn[i]);
    };

    std::vector<CKey> keyRemote(nKeys);
    for (int i = 0; i < nKeys; i++)
    {
        InsecureNewKey(keyRemote[i], true);
        LOCK(keystore.cs_wallet);
        keystore.AddKey(keyRemote[i]); // need pubkey
    };

    BOOST_CHECK(true == SecureMsgStart(&keystore, false, false));

    CKeyID idNull;
    BOOST_CHECK(idNull.IsNull());

    for (int i = 0; i < nKeys; i++)
    {
        SecureMessage smsg;
        MessageData msg;
        CKeyID kFrom = keyOwn[i].GetPubKey().GetID();
        CKeyID kTo = keyRemote[i].GetPubKey().GetID();
        CKeyID kFail = keyRemote[(i+1) % nKeys].GetPubKey().GetID();
        CBitcoinAddress addrFrom(kFrom);
        CBitcoinAddress addrTo(kTo);
        CBitcoinAddress addrFail(kFail);
        std::string sAddrFrom = addrFrom.ToString();
        std::string sAddrTo = addrTo.ToString();
        std::string sAddrFail = addrFail.ToString();

        bool fSendAnonymous = rand() % 3 == 0;

        BOOST_MESSAGE("sAddrFrom " << (fSendAnonymous ? std::string("Anon") : sAddrFrom));
        BOOST_MESSAGE("sAddrTo " << sAddrTo);

        BOOST_CHECK_MESSAGE(0 == (rv = SecureMsgEncrypt(smsg, fSendAnonymous ? idNull : kFrom, kTo, sTestMessage)), "SecureMsgEncrypt " << rv);

        BOOST_CHECK_MESSAGE(0 == (rv = SecureMsgSetHash((uint8_t*)&smsg, ((uint8_t*)&smsg) + SMSG_HDR_LEN, smsg.nPayload)), "SecureMsgSetHash " << rv);

        BOOST_CHECK_MESSAGE(0 == (rv = SecureMsgValidate((uint8_t*)&smsg, ((uint8_t*)&smsg) + SMSG_HDR_LEN, smsg.nPayload)), "SecureMsgValidate " << rv);

        BOOST_CHECK_MESSAGE(0 == (rv = SecureMsgDecrypt(false, kTo, smsg, msg)), "SecureMsgDecrypt " << rv);

        BOOST_CHECK(msg.vchMessage.size()-1 == sTestMessage.size()
            && 0 == memcmp(&msg.vchMessage[0], sTestMessage.data(), msg.vchMessage.size()-1));

        BOOST_CHECK_MESSAGE(1 == (rv = SecureMsgDecrypt(false, kFail, smsg, msg)), "SecureMsgDecrypt " << rv);
    };

    SecureMsgShutdown();

    CConnman *p = g_connman.release();
    delete p;
#endif
}

BOOST_AUTO_TEST_SUITE_END()

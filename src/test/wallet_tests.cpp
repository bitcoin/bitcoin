#include "../headers.h"

using namespace std;
extern string MakeMultisignScript(string strAddresses, CScript& scriptPubKey);
extern bool IsMultisignScript(const CScript& scriptPubKey);
extern bool ExtractMultisignAddress(const CScript& scriptPubKey, string& address);

static void ShutdownAtExit() {
    static bool fDidShutdown = false;
    if (!fDidShutdown)
        DBFlush(true);
    fDidShutdown = true;
}

struct WalletFixture
{
    CWallet* wallet;
    CWallet* wallet2;
    CWallet* wallet3;
    string strAddress1;
    string strAddress2;
    string strAddress3;
    string strAddressOut;
    string strMultisignAddress;
    string strMultisignAddress2;
    string strMultisignAddress2of3;

    WalletFixture()
    {
        fTestNet = true;
        bool fFirst = true;

        wallet = new CWallet("test_wallet1.dat");
        wallet->LoadWallet(fFirst);
        wallet2 = new CWallet("test_wallet2.dat");
        wallet2->LoadWallet(fFirst);
        wallet3 = new CWallet("test_wallet3.dat");
        wallet3->LoadWallet(fFirst);
        strAddress1 = PubKeyToAddress(wallet->GetKeyFromKeyPool());
        //cout << "a1:" << strAddress1 << "\n";
        strAddressOut = PubKeyToAddress(wallet->GetKeyFromKeyPool());
        strAddress2 = PubKeyToAddress(wallet2->GetKeyFromKeyPool());
        strAddress3 = PubKeyToAddress(wallet3->GetKeyFromKeyPool());
        //cout << "a2:" << strAddress2 << "\n";
        strMultisignAddress = string("1,") + strAddress1 + "," + strAddress2;
        strMultisignAddress2 = string("2,") + strAddress1 + "," + strAddress2;
        strMultisignAddress2of3 = string("2,") + strAddress1 + "," + strAddress2 + "," + strAddress3;
    }

    ~WalletFixture()
    {
        delete wallet;
        delete wallet2;
        delete wallet3;
        atexit(ShutdownAtExit);
    }
};

BOOST_FIXTURE_TEST_SUITE(wallet_tests, WalletFixture);

BOOST_AUTO_TEST_CASE(keys)
{
    vector<unsigned char> vchPubKey = wallet->GetKeyFromKeyPool();
    BOOST_CHECK(wallet->mapKeys.count(vchPubKey));
}

BOOST_AUTO_TEST_CASE(multisign_address_format)
{
    CScript script;
    fTestNet = true;
    BOOST_CHECK(MakeMultisignScript("1", script) == _("Invalid multisign address format"));
    BOOST_CHECK(MakeMultisignScript(string("2,") + strAddress1, script) == _("Invalid multisign address format"));
    BOOST_CHECK(MakeMultisignScript(string("0,") + strAddress1, script) == _("Invalid multisign address format"));
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress, script) == "");
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress2, script) == "");
    BOOST_CHECK(MakeMultisignScript(string("2,") + strAddress1 + ",v" + strAddress2, script) == "Invalid bitcoin address");
}

BOOST_AUTO_TEST_CASE(multisign_script)
{
    CScript script;
    string address;
    BOOST_CHECK(!IsMultisignScript(script));
    BOOST_CHECK(!ExtractMultisignAddress(script, address));

    BOOST_CHECK(MakeMultisignScript(strMultisignAddress, script) == "");
    BOOST_CHECK(IsMultisignScript(script));

    BOOST_CHECK(ExtractMultisignAddress(script, address));
    BOOST_CHECK(address == string("multisign(") + strMultisignAddress + ")");
}

BOOST_AUTO_TEST_CASE(multisign_redeem)
{
    CScript script;
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress, script) == "");
    CWalletTx wtxSend;
    wtxSend.vout.push_back(CTxOut(int64(COIN), script));
    uint256 hashSend = wtxSend.GetHash();

    CWalletTx wtxRedeem;
    pair<string,string> res;
    res = wallet->SendMoneyFromMultisignTx("aaaaaaa", wtxSend, "", wtxRedeem, false);

    BOOST_CHECK(res == make_pair(string("error"), string(_("Invalid bitcoin address"))));

    res = wallet->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "verified");

    res = wallet2->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "verified");

    res = wallet3->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "partial");
}

BOOST_AUTO_TEST_CASE(multisign_redeem2)
{
    CScript script;
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress2, script) == "");
    CWalletTx wtxSend;
    wtxSend.vout.push_back(CTxOut(int64(COIN), script));
    uint256 hashSend = wtxSend.GetHash();

    CWalletTx wtxRedeem;
    pair<string,string> res;

    res = wallet->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "partial");

    res = wallet2->SendMoneyFromMultisignTx(strAddressOut, wtxSend, res.second, wtxRedeem, false);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();

    res = wallet2->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "partial");

    res = wallet->SendMoneyFromMultisignTx(strAddressOut, wtxSend, res.second, wtxRedeem, false);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();
    res = wallet3->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "partial");
}

BOOST_AUTO_TEST_CASE(multisign_redeem2of3)
{
    CScript script;
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress2of3, script) == "");
    CWalletTx wtxSend;
    wtxSend.vout.push_back(CTxOut(int64(COIN), script));
    uint256 hashSend = wtxSend.GetHash();

    CWalletTx wtxRedeem;
    pair<string,string> res;

    res = wallet->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "partial");

    res = wallet2->SendMoneyFromMultisignTx(strAddressOut, wtxSend, res.second, wtxRedeem, false);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();

    res = wallet2->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "partial");

    res = wallet->SendMoneyFromMultisignTx(strAddressOut, wtxSend, res.second, wtxRedeem, false);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();
    res = wallet3->SendMoneyFromMultisignTx(strAddressOut, wtxSend, "", wtxRedeem, false);
    BOOST_CHECK(res.first == "partial");

    res = wallet->SendMoneyFromMultisignTx(strAddressOut, wtxSend, res.second, wtxRedeem, false);
    BOOST_CHECK(res.first == "verified");
}

BOOST_AUTO_TEST_SUITE_END()

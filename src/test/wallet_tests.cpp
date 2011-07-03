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
        nTransactionFee = 0;
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

#define SENDMS(wallet, value, partial) res = wallet->SendMoneyFromMultisignTx(strAddressOut, wtxSend, value, partial, wtxRedeem, false)

BOOST_AUTO_TEST_CASE(multisign_redeem)
{
    CScript script;
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress, script) == "");
    CWalletTx wtxSend;
    wtxSend.vout.push_back(CTxOut(COIN, script));
    uint256 hashSend = wtxSend.GetHash();

    CWalletTx wtxRedeem;
    pair<string,string> res;
    res = wallet->SendMoneyFromMultisignTx("aaaaaaa", wtxSend, COIN, "", wtxRedeem, false);

    BOOST_CHECK(res == make_pair(string("error"), string(_("Invalid bitcoin address"))));

    SENDMS(wallet, COIN, "");
    BOOST_CHECK(res.first == "verified");

    SENDMS(wallet2, COIN, "");
    BOOST_CHECK(res.first == "verified");

// Deserialize and check the outputs
    CScript scriptOut;
    scriptOut.SetBitcoinAddress(strAddressOut);
    CDataStream ss(ParseHex(res.second));
    ss >> wtxRedeem;
    BOOST_CHECK(wtxRedeem.vout.size() == 1);
    BOOST_CHECK(wtxRedeem.vout[0].nValue == COIN);
    BOOST_CHECK(wtxRedeem.vout[0].scriptPubKey == scriptOut);

    SENDMS(wallet3, COIN, "");
    BOOST_CHECK(res.first == "partial");
}

BOOST_AUTO_TEST_CASE(multisign_redeem2)
{
    CScript script;
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress2, script) == "");
    CWalletTx wtxSend;
    wtxSend.vout.push_back(CTxOut(COIN, script));
    uint256 hashSend = wtxSend.GetHash();

    CWalletTx wtxRedeem;
    pair<string,string> res;

    SENDMS(wallet, COIN, "");
    BOOST_CHECK(res.first == "partial");

    SENDMS(wallet2, COIN, res.second);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();

    SENDMS(wallet2, COIN, "");
    BOOST_CHECK(res.first == "partial");

    SENDMS(wallet, COIN, res.second);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();
    SENDMS(wallet, COIN, "");
    SENDMS(wallet3, COIN, res.second);
    BOOST_CHECK(res.first == "partial");
}

BOOST_AUTO_TEST_CASE(multisign_redeem2of3)
{
    CScript script;
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress2of3, script) == "");
    CWalletTx wtxSend;
    wtxSend.vout.push_back(CTxOut(COIN, script));
    uint256 hashSend = wtxSend.GetHash();

    CWalletTx wtxRedeem;
    pair<string,string> res;

    SENDMS(wallet, COIN, "");
    BOOST_CHECK(res.first == "partial");

    SENDMS(wallet2, COIN, res.second);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();

    SENDMS(wallet2, COIN, "");
    BOOST_CHECK(res.first == "partial");

    SENDMS(wallet, COIN, res.second);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();

    SENDMS(wallet3, COIN, "");
    BOOST_CHECK(res.first == "partial");

    SENDMS(wallet, COIN, res.second);
    BOOST_CHECK(res.first == "verified");
}

BOOST_AUTO_TEST_CASE(multisign_redeem_change)
{
    // Test with change and transaction fee
    nTransactionFee = 100;

    CScript script;
    BOOST_CHECK(MakeMultisignScript(strMultisignAddress2, script) == "");
    CWalletTx wtxSend;
    wtxSend.vout.push_back(CTxOut(COIN + 100, script));
    uint256 hashSend = wtxSend.GetHash();

    CWalletTx wtxRedeem;
    pair<string,string> res;

    // Insufficient funds
    SENDMS(wallet, COIN + 1, "");
    BOOST_CHECK(res.first == "error");

    // Change mismatches
    SENDMS(wallet, COIN, "");
    BOOST_CHECK(res.first == "partial");

    SENDMS(wallet2, COIN - 1, res.second);
    BOOST_CHECK(res.first == "error");

    wtxRedeem = CWalletTx();
    SENDMS(wallet, COIN - 1, "");
    BOOST_CHECK(res.first == "partial");

    SENDMS(wallet2, COIN, res.second);
    BOOST_CHECK(res.first == "error");

    // Correct
    wtxRedeem = CWalletTx();
    SENDMS(wallet, COIN - 1, "");
    SENDMS(wallet2, COIN - 1, res.second);
    BOOST_CHECK(res.first == "verified");

    wtxRedeem = CWalletTx();

// Deserialize and check the outputs
    CScript scriptOut;
    scriptOut.SetBitcoinAddress(strAddressOut);
    CDataStream ss(ParseHex(res.second));
    ss >> wtxRedeem;
    BOOST_CHECK(wtxRedeem.vout.size() == 2);
    cout << wtxRedeem.vout[0].nValue << "\n";
    BOOST_CHECK(wtxRedeem.vout[0].nValue == COIN - 1);
    BOOST_CHECK(wtxRedeem.vout[0].scriptPubKey == scriptOut);
    BOOST_CHECK(wtxRedeem.vout[1].nValue == 1);
    BOOST_CHECK(wtxRedeem.vout[1].scriptPubKey == script);
}

BOOST_AUTO_TEST_SUITE_END()

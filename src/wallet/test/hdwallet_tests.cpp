// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/hdwallet.h>

#include <wallet/test/hdwallet_test_fixture.h>
#include <base58.h>
#include <chainparams.h>
#include <smsg/smessage.h>
#include <smsg/crypter.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(hdwallet_tests, HDWalletTestingSetup)


class Test1
{
public:
    Test1(std::string s1, std::string s2, int nH, std::string s3) : sPassphrase(s1), sSeed(s2), nHash(nH), sOutput(s3) {};
    std::string sPassphrase;
    std::string sSeed;
    int nHash;
    std::string sOutput;
};


std::vector<Test1> vTestVector1 = {
    Test1("crazy horse battery staple", "Bitcoin seed", 50000, "xprv9s21ZrQH143K2JF8RafpqtKiTbsbaxEeUaMnNHsm5o6wCW3z8ySyH4UxFVSfZ8n7ESu7fgir8imbZKLYVBxFPND1pniTZ81vKfd45EHKX73"),
    Test1("doesn'tneedtobewords",       "Bitcoin seed", 50000, "xprv9s21ZrQH143K24uheQvx9etuBgxXfGZrBuHdmmZXQ1Gv9n1sXE2BE85PHRmSFixb1i8ngZFV6n4mufebUEg55n1epDFsFXKhQ4abU7gvThb")
};

BOOST_AUTO_TEST_CASE(new_ext_key)
{
    // Match keys from http://bip32.org/

    CHDWallet *pwallet = pwalletMain.get();

    for (auto it = vTestVector1.begin(); it != vTestVector1.end(); ++it)
    {
        CExtKey ekTest;

        BOOST_CHECK(0 == pwallet->ExtKeyNew32(ekTest, it->sPassphrase.c_str(), it->nHash, it->sSeed.c_str()));

        CExtKeyPair ekp(ekTest);
        CExtKey58 ek58;
        ek58.SetKey(ekp, CChainParams::EXT_SECRET_KEY_BTC);
        BOOST_CHECK(ek58.ToString() == it->sOutput);
    };
}

static const std::string strSecret1C("GzFRfngjf5aHMuAzWDZWzJ8eYqMzp29MmkCp6NgzkXFibrh45tTc");
static const std::string strSecret2C("H5hDgLvFjLcZG9jyxkUTJ28P6N5T7iMBQ79boMuaPafxXuy8hb9n");

BOOST_AUTO_TEST_CASE(stealth)
{
    CHDWallet *pwallet = pwalletMain.get();

    ECC_Start_Stealth();
    CStealthAddress sx;
    BOOST_CHECK(true == sx.SetEncoded("SPGyji8uZFip6H15GUfj6bsutRVLsCyBFL3P7k7T7MUDRaYU8GfwUHpfxonLFAvAwr2RkigyGfTgWMfzLAAP8KMRHq7RE8cwpEEekH"));

    CAmount nValue = 1;

    std::string strError;
    std::string sNarr;


    // No bitfield, no narration
    std::vector<CTempRecipient> vecSend;
    CTempRecipient r;
    r.nType = OUTPUT_STANDARD;
    r.SetAmount(nValue);
    r.fSubtractFeeFromAmount = false;
    r.address = sx;
    r.sNarration = sNarr;
    vecSend.push_back(r);

    BOOST_CHECK(0 == pwallet->ExpandTempRecipients(vecSend, NULL, strError));
    BOOST_CHECK(2 == vecSend.size());
    BOOST_CHECK(34 == vecSend[1].vData.size());


    // No bitfield, with narration
    vecSend.clear();
    sNarr = "test narration";
    r.sNarration = sNarr;
    vecSend.push_back(r);
    BOOST_CHECK(0 == pwallet->ExpandTempRecipients(vecSend, NULL, strError));
    BOOST_CHECK(2 == vecSend.size());
    BOOST_REQUIRE(51 == vecSend[1].vData.size());
    BOOST_REQUIRE(vecSend[1].vData[34] == DO_NARR_CRYPT);

    CBitcoinSecret bsecret1;
    BOOST_CHECK(bsecret1.SetString(strSecret1C));
    //BOOST_CHECK(bsecret2.SetString(strSecret2C));

    CKey sScan = bsecret1.GetKey();

    CKey sShared;
    ec_point pkExtracted;
    ec_point vchEphemPK(vecSend[1].vData.begin() + 1, vecSend[1].vData.begin() + 34);
    std::vector<uint8_t> vchENarr(vecSend[1].vData.begin() + 35, vecSend[1].vData.end());


    BOOST_REQUIRE(StealthSecret(sScan, vchEphemPK, sx.spend_pubkey, sShared, pkExtracted) == 0);

    SecMsgCrypter crypter;
    crypter.SetKey(sShared.begin(), &vchEphemPK[0]);
    std::vector<uint8_t> vchNarr;
    BOOST_REQUIRE(crypter.Decrypt(&vchENarr[0], vchENarr.size(), vchNarr));
    std::string sNarrRecovered = std::string(vchNarr.begin(), vchNarr.end());
    BOOST_CHECK(sNarr == sNarrRecovered);


    // With bitfield, no narration
    vecSend.clear();
    sNarr = "";
    sx.prefix.number_bits = 5;
    sx.prefix.bitfield = 0xaaaaaaaa;
    r.address = sx;
    r.sNarration = sNarr;
    vecSend.push_back(r);
    BOOST_CHECK(0 == pwallet->ExpandTempRecipients(vecSend, NULL, strError));
    BOOST_CHECK(2 == vecSend.size());
    BOOST_REQUIRE(39 == vecSend[1].vData.size());
    BOOST_CHECK(vecSend[1].vData[34] == DO_STEALTH_PREFIX);
    uint32_t prefix, mask = SetStealthMask(sx.prefix.number_bits);
    memcpy(&prefix, &vecSend[1].vData[35], 4);

    BOOST_CHECK((prefix & mask) == (sx.prefix.bitfield & mask));


    // With bitfield, with narration
    vecSend.clear();
    sNarr = "another test narration";
    sx.prefix.number_bits = 18;
    sx.prefix.bitfield = 0xaaaaaaaa;
    r.address = sx;
    r.sNarration = sNarr;
    vecSend.push_back(r);
    BOOST_CHECK(0 == pwallet->ExpandTempRecipients(vecSend, NULL, strError));
    BOOST_CHECK(2 == vecSend.size());
    BOOST_REQUIRE(72 == vecSend[1].vData.size());

    BOOST_CHECK(vecSend[1].vData[34] == DO_STEALTH_PREFIX);
    mask = SetStealthMask(sx.prefix.number_bits);
    memcpy(&prefix, &vecSend[1].vData[35], 4);

    BOOST_CHECK((prefix & mask) == (sx.prefix.bitfield & mask));

    BOOST_CHECK(vecSend[1].vData[39] == DO_NARR_CRYPT);
    vchEphemPK.resize(33);
    memcpy(&vchEphemPK[0], &vecSend[1].vData[1], 33);

    vchENarr = std::vector<uint8_t>(vecSend[1].vData.begin() + 40, vecSend[1].vData.end());


    BOOST_REQUIRE(StealthSecret(sScan, vchEphemPK, sx.spend_pubkey, sShared, pkExtracted) == 0);

    crypter.SetKey(sShared.begin(), &vchEphemPK[0]);
    BOOST_REQUIRE(crypter.Decrypt(&vchENarr[0], vchENarr.size(), vchNarr));
    sNarrRecovered = std::string(vchNarr.begin(), vchNarr.end());
    BOOST_CHECK(sNarr == sNarrRecovered);


    ECC_Stop_Stealth();
}

BOOST_AUTO_TEST_CASE(stealth_key_index)
{
    CHDWallet *pwallet = pwalletMain.get();

    //ECC_Start_Stealth();
    CStealthAddress sx;
    BOOST_CHECK(sx.SetEncoded("SPGyji8uZFip6H15GUfj6bsutRVLsCyBFL3P7k7T7MUDRaYU8GfwUHpfxonLFAvAwr2RkigyGfTgWMfzLAAP8KMRHq7RE8cwpEEekH"));

    CStealthAddressIndexed sxi;
    uint32_t sxId;
    sx.ToRaw(sxi.addrRaw);
    BOOST_CHECK(pwallet->GetStealthKeyIndex(sxi, sxId));
    BOOST_CHECK(sxId == 1);


    BOOST_CHECK(sx.SetEncoded("SPGx7SrLpMcMUjJhQkMp7D8eRAxzVj34StgQdYHr9887nCNBAiUTr4eiJKunzDaBxUqTWGX1sCCJxvUH9WG1JkJw9o15Xn2JSjnpD9"));
    sx.ToRaw(sxi.addrRaw);
    BOOST_CHECK(pwallet->GetStealthKeyIndex(sxi, sxId));
    BOOST_CHECK(sxId == 2);

    BOOST_CHECK(sx.SetEncoded("SPGwdFXLfjt3yQLzVhwbQLriSBbSF3gbmBsTDtA4Sjkz5aCDvmPgw3EqT51YqbxanMzFmAUSWtvCheFvUeWc56QH7sYD4nUKVX8kz2"));
    sx.ToRaw(sxi.addrRaw);
    BOOST_CHECK(pwallet->GetStealthKeyIndex(sxi, sxId));
    BOOST_CHECK(sxId == 3);

    CStealthAddress sxOut;
    BOOST_CHECK(pwallet->GetStealthByIndex(2, sxOut));
    BOOST_CHECK(sxOut.ToString() == "SPGx7SrLpMcMUjJhQkMp7D8eRAxzVj34StgQdYHr9887nCNBAiUTr4eiJKunzDaBxUqTWGX1sCCJxvUH9WG1JkJw9o15Xn2JSjnpD9");


    BOOST_CHECK(sx.SetEncoded("SPGyji8uZFip6H15GUfj6bsutRVLsCyBFL3P7k7T7MUDRaYU8GfwUHpfxonLFAvAwr2RkigyGfTgWMfzLAAP8KMRHq7RE8cwpEEekH"));
    sx.ToRaw(sxi.addrRaw);
    BOOST_CHECK(pwallet->GetStealthKeyIndex(sxi, sxId));
    BOOST_CHECK(sxId == 1);


    CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
    uint160 hash;
    uint32_t nIndex;
    for (size_t k = 0; k < 512; ++k)
    {
        LOCK(pwallet->cs_wallet);
        pwallet->IndexStealthKey(&wdb, hash, sxi, nIndex);
    };
    BOOST_CHECK(nIndex == 515);

    //ECC_Stop_Stealth();
}

BOOST_AUTO_TEST_CASE(ext_key_index)
{
    CHDWallet *pwallet = pwalletMain.get();

    CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
    CKeyID dummy;
    uint32_t nIndex;
    for (size_t k = 0; k < 512; ++k)
    {
        LOCK(pwallet->cs_wallet);
        pwallet->ExtKeyNewIndex(&wdb, dummy, nIndex);
    };
    BOOST_CHECK(nIndex == 512);
}



BOOST_AUTO_TEST_SUITE_END()

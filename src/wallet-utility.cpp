#include <iostream>
#include <string>

// Include local headers
#include "wallet/walletdb.h"
#include "util.h"
#include "base58.h"
#include "wallet/crypter.h"
#include <boost/foreach.hpp>


void show_help()
{
    std::cout <<
        "This program outputs Bitcoin addresses and private keys from a wallet.dat file" << std::endl
        << std::endl
        << "Usage and options: "
        << std::endl
        << " -datadir=<directory> to tell the program where your wallet is"
        << std::endl
        << " -wallet=<name> (Optional) if your wallet is not named wallet.dat"
        << std::endl
        << " -regtest or -testnet (Optional) dumps addresses from regtest/testnet"
        << std::endl
        << " -dumppass (Optional)if you want to extract private keys associated with addresses"
        << std::endl
        << "    -pass=<walletpassphrase> if you have encrypted private keys stored in your wallet"
        << std::endl;
}


class WalletUtilityDB : public CDB
{
    private:
        typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
        MasterKeyMap mapMasterKeys;
        unsigned int nMasterKeyMaxID;
        SecureString mPass;
        std::vector<CKeyingMaterial> vMKeys;

    public:
        WalletUtilityDB(const std::string& strFilename, const char* pszMode = "r+", bool fFlushOnClose = true) : CDB(strFilename, pszMode, fFlushOnClose)
    {
        nMasterKeyMaxID = 0;
        mPass.reserve(100);
    }

        std::string getAddress(CDataStream ssKey);
        std::string getKey(CDataStream ssKey, CDataStream ssValue);
        std::string getCryptedKey(CDataStream ssKey, CDataStream ssValue, std::string masterPass);
        bool updateMasterKeys(CDataStream ssKey, CDataStream ssValue);
        bool parseKeys(bool dumppriv, std::string masterPass);

        bool DecryptSecret(const std::vector<unsigned char>& vchCiphertext, const uint256& nIV, CKeyingMaterial& vchPlaintext);
        bool Unlock();
        bool DecryptKey(const std::vector<unsigned char>& vchCryptedSecret, const CPubKey& vchPubKey, CKey& key);
};


/*
 * Address from a public key in base58
 */
std::string WalletUtilityDB::getAddress(CDataStream ssKey)
{
    CPubKey vchPubKey;
    ssKey >> vchPubKey;
    CKeyID id = vchPubKey.GetID();
    std::string strAddr = CBitcoinAddress(id).ToString();

    return strAddr;
}


/*
 * Non encrypted private key in WIF
 */
std::string WalletUtilityDB::getKey(CDataStream ssKey, CDataStream ssValue)
{
    std::string strKey;
    CPubKey vchPubKey;
    ssKey >> vchPubKey;
    CPrivKey pkey;
    CKey key;

    ssValue >> pkey;
    if (key.Load(pkey, vchPubKey, true))
        strKey = CBitcoinSecret(key).ToString();

    return strKey;
}


bool WalletUtilityDB::DecryptSecret(const std::vector<unsigned char>& vchCiphertext, const uint256& nIV, CKeyingMaterial& vchPlaintext)
{
    CCrypter cKeyCrypter;
    std::vector<unsigned char> chIV(WALLET_CRYPTO_KEY_SIZE);
    memcpy(&chIV[0], &nIV, WALLET_CRYPTO_KEY_SIZE);

    BOOST_FOREACH(const CKeyingMaterial vMKey, vMKeys)
    {
        if(!cKeyCrypter.SetKey(vMKey, chIV))
            continue;
        if (cKeyCrypter.Decrypt(vchCiphertext, *((CKeyingMaterial*)&vchPlaintext)))
            return true;
    }
    return false;
}


bool WalletUtilityDB::Unlock()
{
    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    BOOST_FOREACH(const MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
    {
        if(!crypter.SetKeyFromPassphrase(mPass, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
            return false;
        if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
            continue; // try another master key
        vMKeys.push_back(vMasterKey);
    }
    return true;
}


bool WalletUtilityDB::DecryptKey(const std::vector<unsigned char>& vchCryptedSecret, const CPubKey& vchPubKey, CKey& key)
{
    CKeyingMaterial vchSecret;
    if(!DecryptSecret(vchCryptedSecret, vchPubKey.GetHash(), vchSecret))
        return false;

    if (vchSecret.size() != 32)
        return false;

    key.Set(vchSecret.begin(), vchSecret.end(), vchPubKey.IsCompressed());
    return true;
}


/*
 * Encrypted private key in WIF format
 */
std::string WalletUtilityDB::getCryptedKey(CDataStream ssKey, CDataStream ssValue, std::string masterPass)
{
    mPass = masterPass.c_str();
    CPubKey vchPubKey;
    ssKey >> vchPubKey;
    CKey key;

    std::vector<unsigned char> vKey;
    ssValue >> vKey;

    if (!Unlock())
        return "";

    if(!DecryptKey(vKey, vchPubKey, key))
        return "";

    std::string strKey = CBitcoinSecret(key).ToString();
    return strKey;
}


/*
 * Master key derivation
 */
bool WalletUtilityDB::updateMasterKeys(CDataStream ssKey, CDataStream ssValue)
{
    unsigned int nID;
    ssKey >> nID;
    CMasterKey kMasterKey;
    ssValue >> kMasterKey;
    if (mapMasterKeys.count(nID) != 0)
    {
        std::cout << "Error reading wallet database: duplicate CMasterKey id " << nID << std::endl;
        return false;
    }
    mapMasterKeys[nID] = kMasterKey;

    if (nMasterKeyMaxID < nID)
        nMasterKeyMaxID = nID;

    return true;
}


/*
 * Look at all the records and parse keys for addresses and private keys
 */
bool WalletUtilityDB::parseKeys(bool dumppriv, std::string masterPass)
{
    DBErrors result = DB_LOAD_OK;
    std::string strType;
    bool first = true;

    try {
        Dbc* pcursor = GetCursor();
        if (!pcursor)
        {
            LogPrintf("Error getting wallet database cursor\n");
            result = DB_CORRUPT;
        }

        if (dumppriv)
        {
            while (result == DB_LOAD_OK && true)
            {
                CDataStream ssKey(SER_DISK, CLIENT_VERSION);
                CDataStream ssValue(SER_DISK, CLIENT_VERSION);
                int result = ReadAtCursor(pcursor, ssKey, ssValue);

                if (result == DB_NOTFOUND) {
                    break;
                }
                else if (result != 0)
                {
                    LogPrintf("Error reading next record from wallet database\n");
                    result = DB_CORRUPT;
                    break;
                }

                ssKey >> strType;
                if (strType == "mkey")
                {
                    updateMasterKeys(ssKey, ssValue);
                }
            }
            pcursor->close();
            pcursor = GetCursor();
        }

        while (result == DB_LOAD_OK && true)
        {
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = ReadAtCursor(pcursor, ssKey, ssValue);

            if (ret == DB_NOTFOUND)
            {
                std::cout << " ]" << std::endl;
                first = true;
                break;
            }
            else if (ret != DB_LOAD_OK)
            {
                LogPrintf("Error reading next record from wallet database\n");
                result = DB_CORRUPT;
                break;
            }

            ssKey >> strType;

            if (strType == "key" || strType == "ckey")
            {
                std::string strAddr = getAddress(ssKey);
                std::string strKey = "";


                if (dumppriv && strType == "key")
                    strKey = getKey(ssKey, ssValue);
                if (dumppriv && strType == "ckey")
                {
                    if (masterPass == "")
                    {
                        std::cout << "Encrypted wallet, please provide a password. See help below" << std::endl;
                        show_help();
                        result = DB_LOAD_FAIL;
                        break;
                    }
                    strKey = getCryptedKey(ssKey, ssValue, masterPass);
                }

                if (strAddr != "")
                {
                    if (first)
                        std::cout << "[ ";
                    else
                        std::cout << ", ";
                }

                if (dumppriv)
                {
                    std::cout << "{\"addr\" : \"" + strAddr + "\", "
                        << "\"pkey\" : \"" + strKey + "\"}"
                        << std::flush;
                }
                else
                {
                    std::cout << "\"" + strAddr + "\"";
                }

                first = false;
            }
        }

        pcursor->close();
    } catch (DbException &e) {
        std::cout << "DBException caught " << e.get_errno() << std::endl;
    } catch (std::exception &e) {
        std::cout << "Exception caught " << std::endl;
    }

    if (result == DB_LOAD_OK)
        return true;
    else
        return false;
}


int main(int argc, char* argv[])
{
    ParseParameters(argc, argv);
    std::string walletFile = GetArg("-wallet", "wallet.dat");
    std::string masterPass = GetArg("-pass", "");
    bool fDumpPass = GetBoolArg("-dumppass", false);
    bool help = GetBoolArg("-h", false);
    bool result = false;

    if (help)
    {
        show_help();
        return 0;
    }

    try {
        SelectParams(ChainNameFromCommandLine());
        result = WalletUtilityDB(walletFile, "r").parseKeys(fDumpPass, masterPass);
    }
    catch (const std::exception& e) {
        std::cout << "Error opening wallet file " << walletFile << std::endl;
        std::cout << e.what() << std::endl;
    }

    if (result)
        return 0;
    else
        return -1;
}

// Copyright (c) 2012-2013 Eric Lombrozo, The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletmanager.h"
#include "walletdb.h"

using namespace std;

////////////////////////////////////////////////////////////
//
// TODO: Move GetFilesAtPath to utils.h/utils.cpp
//
namespace file_option_flags
{
    const unsigned int REGULAR_FILES = 0x01;
    const unsigned int DIRECTORIES = 0x02;
};

vector<string> GetFilesAtPath(const boost::filesystem::path& _path, unsigned int flags)
{
    vector<string> vstrFiles;
    if (!boost::filesystem::exists(_path))
        throw runtime_error("Path does not exist.");
    
    if ((flags & file_option_flags::REGULAR_FILES) && boost::filesystem::is_regular_file(_path))
    {
#if defined (BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION == 3
        vstrFiles.push_back(_path.filename().string());
#else
        vstrFiles.push_back(_path.filename());
#endif
        return vstrFiles;
    }
    if (boost::filesystem::is_directory(_path))
    {
        vector<boost::filesystem::path> vPaths;
        copy(boost::filesystem::directory_iterator(_path), boost::filesystem::directory_iterator(), back_inserter(vPaths));
        BOOST_FOREACH(const boost::filesystem::path& pFile, vPaths)
        {
            if (((flags & file_option_flags::REGULAR_FILES) && boost::filesystem::is_regular_file(pFile)) ||
                ((flags & file_option_flags::DIRECTORIES) && boost::filesystem::is_directory(pFile)))
#if defined (BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION == 3
                vstrFiles.push_back(pFile.filename().string());
#else
            vstrFiles.push_back(pFile.filename());
#endif
        }
        return vstrFiles;
    }
    throw runtime_error("Path exists but is neither a regular file nor a directory.");
}
//
////////////////////////////////////////////////////////////


// TODO: Remove these functions
bool static InitError(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_ERROR);
    return false;
}

bool static InitWarning(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING);
    return true;
}

// TODO: Remove dependencies for I/O on printf to debug.log, InitError, and InitWarning
// TODO: Improve error messages and error handling.
void CWalletManager::LoadWallet(const string& strName)
{
    ostringstream err;
    string strFile = "wallet";
    CWallet* pWallet = NULL;
    int64 nStart = GetTimeMillis();

    // Check that the wallet name is valid. A wallet name can only contain alphanumerics and underscores.
    if (!CWalletManager::IsValidName(strName))
    {
        err << _("Invalid wallet name \"") << strName << _("\"."); 
        throw runtime_error(err.str());
    }

    {
        LOCK(cs_WalletManager);    
        
        // Check that wallet is not already loaded
        if (wallets.count(strName) > 0)
        {
            err << _("A wallet with the name ") << strName << _(" is already loaded.");
            throw runtime_error(err.str());
        }
        
        // Wallet file name for wallet foo will be wallet-foo.dat
        // The empty string is reserved for the default wallet whose file is wallet.dat
        if (strName.size() > 0)
        {
            strFile += "-" + strName;
        }
        strFile += ".dat";
        
        printf("Loading wallet %s from %s...\n", strName.c_str(), strFile.c_str());
        bool fFirstRun = true;
        DBErrors nLoadWalletRet;
        
        try
        {
            pWallet = new CWallet(strFile);
            nLoadWalletRet = pWallet->LoadWallet(fFirstRun);
        }
        catch (const exception& e)
        {
            err << _("Critical error loading wallet ") << strName << _(" from ") << strFile << ": " << e.what();
            throw runtime_error(err.str());
        }
        catch (...)
        {
            err << _("Critical error loading wallet ") << strName <<_(" from ") << strFile;
            throw runtime_error(err.str());
        }
        
        if (nLoadWalletRet != DB_LOAD_OK)
        {
            if (nLoadWalletRet == DB_CORRUPT)
            {
                delete pWallet;
                err << _("Error loading ") << strFile << _(": Wallet corrupted.");
                throw runtime_error(err.str());
            }
            else if (nLoadWalletRet == DB_NONCRITICAL_ERROR)
            {
                err << _("Warning: error reading ") << strFile
                    << _(": All keys read correctly, but transaction data or address book entries might be missing or incorrect.");
                printf("%s\n", err.str().c_str());
                InitWarning(err.str());
                err.str("");
            }
            else if (nLoadWalletRet == DB_TOO_NEW)
            {
                err << _("Error loading ") << strFile << _(": Wallet requires newer version of Bitcoin.");
                throw runtime_error(err.str());
            }
            else if (nLoadWalletRet == DB_NEED_REWRITE)
            {
                err << _("Wallet needed to be rewritten: restart Bitcoin to complete.");
                InitError(err.str());
                throw runtime_error(err.str());
            }
            else
            {
                err << _("Error loading ") << strFile << _(": Unknown database error.");
                throw runtime_error(err.str());
            }
        }
        
        if (GetBoolArg("-upgradewallet", fFirstRun))
        {
            int nMaxVersion = GetArg("-upgradewallet", 0);
            if (nMaxVersion == 0) // the -upgradewallet without argument case
            {
                printf("Performing wallet upgrade to %i for wallet %s.\n", FEATURE_LATEST, strName.c_str());
                nMaxVersion = CLIENT_VERSION;
                pWallet->SetMinVersion(FEATURE_LATEST); // permanently upgrade the wallet immediately
            }
            else
            {
                printf("Allowing wallet upgrade up to %i for wallet %s.\n", nMaxVersion, strName.c_str());
            }
            if (nMaxVersion < pWallet->GetVersion())
            {
                err << _("Cannot downgrade wallet ") << strName << _(".");
                printf("%s\n", err.str().c_str());
                err.str("");
            }
            pWallet->SetMaxVersion(nMaxVersion);
        }
        
        if (fFirstRun)
        {
            // Create new keyUser and set as default key
            RandAddSeedPerfmon();
            
            CPubKey newDefaultKey;
            if (!pWallet->GetKeyFromPool(newDefaultKey, false))
            {
                err << _("Cannot initialize keypool for wallet ") << strName << _(".");
                printf("%s\n", err.str().c_str());
                err.str("");
            }
            pWallet->SetDefaultKey(newDefaultKey);
            if (!pWallet->SetAddressBookName(pWallet->vchDefaultKey.GetID(), ""))
            {
                err << _("Cannot write default address.");
                printf("%s\n", err.str().c_str());
                err.str("");
            }
        }
        
        printf("    wallet      %15"PRI64d"ms\n", GetTimeMillis() - nStart);
        
        boost::shared_ptr<CWallet> spWallet(pWallet);
        this->wallets[strName] = spWallet;
        RegisterWallet(pWallet);
    }
    
    CBlockIndex *pindexRescan = pindexBest;
    if (GetBoolArg("-rescan", false))
    {
        pindexRescan = pindexGenesisBlock;
    }
    else
    {
        CWalletDB walletdb(strFile);
        CBlockLocator locator;
        if (walletdb.ReadBestBlock(locator))
        {
            pindexRescan = locator.GetBlockIndex();
        }
        else
        {
            pindexRescan = pindexGenesisBlock;
        }
    }
    if (pindexBest && pindexBest != pindexRescan)
    {
        uiInterface.InitMessage(_("Rescanning..."));
        printf("Rescanning last %i blocks (from block %i)...\n", pindexBest->nHeight - pindexRescan->nHeight, pindexRescan->nHeight);
        nStart = GetTimeMillis();
        pWallet->ScanForWalletTransactions(pindexRescan, true);
        printf(" rescan      %15"PRI64d"ms\n", GetTimeMillis() - nStart);
        pWallet->SetBestChain(CBlockLocator(pindexBest));
        nWalletDBUpdated++;
    }
}

bool CWalletManager::UnloadWallet(const std::string& strName)
{
    {
        LOCK(cs_WalletManager);
        if (!wallets.count(strName)) return false;
        boost::shared_ptr<CWallet> spWallet(wallets[strName]);
        printf("Unloading wallet %s\n", strName.c_str());
        {
            LOCK(spWallet->cs_wallet);
            UnregisterWallet(spWallet.get());
            wallets.erase(strName);
        }
    }
    return true;
}

void CWalletManager::UnloadAllWallets()
{
    {
        LOCK(cs_WalletManager);
        vector<string> vstrNames;
        vector<boost::shared_ptr<CWallet> > vpWallets;
        BOOST_FOREACH(const wallet_map::value_type& item, wallets)
        {
            vstrNames.push_back(item.first);
            vpWallets.push_back(item.second);
        }
        
        for (unsigned int i = 0; i < vstrNames.size(); i++)
        {
            printf("Unloading wallet %s\n", vstrNames[i].c_str());
            {
                LOCK(vpWallets[i]->cs_wallet);
                UnregisterWallet(vpWallets[i].get());
                wallets.erase(vstrNames[i]);
            }
        }
    }
}

boost::shared_ptr<CWallet> CWalletManager::GetWallet(const string& strName)
{
    {
        LOCK(cs_WalletManager);
        if (!wallets.count(strName)) {
            throw runtime_error("CWalletManager::GetWallet() - Wallet not loaded.");
        }
        return wallets[strName];
    }
}

const boost::regex CWalletManager::WALLET_NAME_REGEX("[a-zA-Z0-9_]*");
const boost::regex CWalletManager::WALLET_FILE_REGEX("wallet-([a-zA-Z0-9_]+)\\.dat");

bool CWalletManager::IsValidName(const string& strName)
{
    return boost::regex_match(strName, CWalletManager::WALLET_NAME_REGEX);
}

vector<string> CWalletManager::GetWalletsAtPath(const boost::filesystem::path& pathWallets)
{
    vector<string> vstrFiles = GetFilesAtPath(pathWallets, file_option_flags::REGULAR_FILES);
    vector<string> vstrNames;
    boost::cmatch match;
    BOOST_FOREACH(const string& strFile, vstrFiles)
    {
        if (boost::regex_match(strFile.c_str(), match, CWalletManager::WALLET_FILE_REGEX))
            vstrNames.push_back(string(match[1].first, match[1].second));
    }
    return vstrNames;
}

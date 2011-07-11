// Copyright (c) 2011 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "init.h" // for pwalletMain
#include "bitcoinrpc.h"

// #include <boost/asio.hpp>
// #include <boost/iostreams/concepts.hpp>
// #include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
// #ifdef USE_SSL
// #include <boost/asio/ssl.hpp> 
// typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLStream;
// #endif
// #include <boost/xpressive/xpressive_dynamic.hpp>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#define printf OutputDebugStringF

// using namespace boost::asio;
using namespace json_spirit;
using namespace std;

extern Object JSONRPCError(int code, const string& message);

class CTxDump
{
public:
    CBlockIndex *pindex;
    int64 nValue;
    bool fSpent;
    CWalletTx* ptx;
    int nOut;
    CTxDump(CWalletTx* ptx = NULL, int nOut = -1)
    {
        pindex = NULL;
        nValue = 0;
        fSpent = false;
        this->ptx = ptx;
        this->nOut = nOut;
    }
};

class CKeyDump
{
private:
    void Init()
    {
        strLabel = "";
        pindexUsed = NULL;
        pindexAvail = NULL;
        nValue = 0;
        nAvail = 0;
        fReserve = false;
        fUsed = false;
        fLabel = false;
    }
public:
    CBitcoinSecret secret;
    string strLabel;
    bool fLabel;
    CBlockIndex *pindexUsed;
    CBlockIndex *pindexAvail;
    int64 nValue;
    int64 nAvail;
    vector<CTxDump> vtxdmp;
    bool fReserve;
    bool fUsed;
    CKeyDump(const CSecret &vchSecretIn) : secret(vchSecretIn)
    {
        Init();
    }
    CKeyDump()
    {
        Init();
    }
};

void static GetWalletDump(map<CBitcoinAddress,CKeyDump> &mapDump)
{
    CRITICAL_BLOCK(cs_main)
    CRITICAL_BLOCK(pwalletMain->cs_wallet)
    {
        set<CBitcoinAddress> setAddress;
        pwalletMain->GetKeys(setAddress);
        for (set<CBitcoinAddress>::iterator it = setAddress.begin(); it != setAddress.end(); ++it)
        {
            CKey key;
            if (pwalletMain->GetKey(*it, key))
                mapDump[*it] = CKeyDump(key.GetSecret());
        }
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            CWalletTx* coin=&(*it).second;
            CBlockIndex *pindex;
            if (coin->GetDepthInMainChain(pindex)>0)
            {
                for (int i=0; i < coin->vout.size(); i++) 
                {
                    CTxOut &out = coin->vout[i];
                    int64 nCredit = pwalletMain->GetCredit(out);
                    if (nCredit > 0)
                    {
                        CTxDump txdmp(coin,i);
                        txdmp.pindex = pindex;
                        txdmp.nValue = nCredit;
                        txdmp.fSpent = coin->IsSpent(i);
                        CBitcoinAddress vchAddress;
                        ExtractAddress(out.scriptPubKey, pwalletMain, vchAddress);
                        if (mapDump.count(vchAddress) == 0)
                        {
                            CSecret vchSecretDummy;
                            mapDump[vchAddress] = CKeyDump(vchSecretDummy);
                        }
                        CKeyDump &keydump = mapDump[vchAddress];
                        if (keydump.pindexUsed==NULL || keydump.pindexUsed->nHeight > txdmp.pindex->nHeight)
                            keydump.pindexUsed = txdmp.pindex;
                        if (!txdmp.fSpent && (keydump.pindexAvail==NULL || keydump.pindexAvail->nHeight > txdmp.pindex->nHeight))
                            keydump.pindexAvail = txdmp.pindex;
                        keydump.nValue += txdmp.nValue;
                        if (!txdmp.fSpent)
                            keydump.nAvail += txdmp.nValue;
                        keydump.vtxdmp.push_back(txdmp);
                        keydump.fUsed = true;
                    }
                }
            }
        }
        map<CBitcoinAddress, int64> mapReserveAddresses;
        pwalletMain->GetAllReserveAddresses(mapReserveAddresses);
        for (map<CBitcoinAddress, int64>::const_iterator it = mapReserveAddresses.begin(); it != mapReserveAddresses.end(); ++it)
        {
             const CBitcoinAddress &address = (*it).first;
             if (mapDump.count(address) && !mapDump[address].fUsed)
                 mapDump[address].fReserve = true;
        }
        for (map<CBitcoinAddress, CKeyDump>::iterator it = mapDump.begin(); it != mapDump.end(); ++it)
        {
            const CBitcoinAddress& vchAddress = (*it).first;
            if (pwalletMain->mapAddressBook.count(vchAddress))
            {
                (*it).second.fLabel = true;
                (*it).second.fReserve = false;
                (*it).second.strLabel = pwalletMain->mapAddressBook[vchAddress];
            }
        }
    }
}

Value importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "importprivkey <bitcoinprivkey> [label]\n"
            "Adds a private key (as returned by dumpprivkey) to your wallet.");

    string strSecret = params[0].get_str();
    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();
    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(-5,"Invalid private key");

    CKey key;
    key.SetSecret(vchSecret.GetSecret());
    CBitcoinAddress vchAddress = CBitcoinAddress(key.GetPubKey());

    CRITICAL_BLOCK(cs_main)
    CRITICAL_BLOCK(pwalletMain->cs_wallet)
    {
        pwalletMain->MarkDirty();
        pwalletMain->SetAddressBookName(vchAddress, strLabel);

        if (!pwalletMain->AddKey(key))
            throw JSONRPCError(-4,"Error adding key to wallet");

        pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
        pwalletMain->ReacceptWalletTransactions();
    }

    MainFrameRepaint();

    return Value::null;
}

Value removeprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "removeprivkey <bitcoinprivkey>\n"
            "Removes a private key (as returned by dumpprivkey) from your wallet.\n"
            "Warning: this will remove transactions from your wallet, and may change the balance of unrelated accounts.\n");

    string strSecret = params[0].get_str();
    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(-5,"Invalid private key");

    CKey key;
    key.SetSecret(vchSecret.GetSecret());
    CBitcoinAddress address(key.GetPubKey());

    CRITICAL_BLOCK(pwalletMain->cs_wallet)
    {
        pwalletMain->MarkDirty();
        if (!pwalletMain->RemoveKey(address))
            throw JSONRPCError(-4,"Error removing key from wallet");

        pwalletMain->DelAddressBookName(address);
    }

    MainFrameRepaint();

    return Value::null;
}



Value dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey <bitcoinaddress>\n"
            "Reveals the private key corresponding to <bitcoinaddress>.");

    string strAddress = params[0].get_str();
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(-5, "Invalid bitcoin address");
    CSecret vchSecret;
    if (!pwalletMain->GetSecret(address, vchSecret))
        throw JSONRPCError(-4,"Private key for address " + strAddress + " is not known");
    return CBitcoinSecret(vchSecret).ToString();
}

Value importwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1)
        throw runtime_error(
            "importwallet <file> [options]...\n"
            "Import dumped wallet.\nOptions:\n"
            "    rescan:   rescan for transactions after dump was made\n"
        );

    bool fRescan=false;
    for (int i = 1; i<params.size(); i++)
    {
        string strArg = params[i].get_str();
        if (strArg == "rescan")
            fRescan = true;
    }

    CRITICAL_BLOCK(cs_main)
    CRITICAL_BLOCK(pwalletMain->cs_wallet)
    {
        pwalletMain->MarkDirty();

        int nKnownHeight = 0;
        int nRescanHeight = pindexBest->nHeight;
        set<uint256> setRescanTx;
        set<CBitcoinAddress> setReserveKey;

        Object objDump = params[0].get_obj();

        Value valLoc = find_value(objDump, "loc");
        if (valLoc.type() != null_type)
        {
            vector<uint256> vblockKnown;
            Object objLoc = valLoc.get_obj();
            map<string,Value> mapLoc;
            obj_to_map(objLoc, mapLoc);
            BOOST_FOREACH(PAIRTYPE(const string, Value)& item, mapLoc)
                vblockKnown.push_back(uint256(item.first));
            CBlockLocator locatorDump = vblockKnown;
            nKnownHeight = locatorDump.GetHeight();
            printf("import: known height: %i\n", nKnownHeight);
        }

        if (fRescan)
            nRescanHeight = nKnownHeight;

        Value valKeys = find_value(objDump, "keys");
        if (valKeys.type() != null_type)
        {
            Array arrKeys = valKeys.get_array();
            BOOST_FOREACH(const Value& valKey, arrKeys)
            {
                // iterate over keys
                Object objKey = valKey.get_obj();
                string strSecret = find_value(objKey, "sec").get_str();
                CBitcoinSecret secret;
                if (!secret.SetString(strSecret))
                    continue;
                CKey key;
                if (!key.SetSecret(secret.GetSecret()))
                    continue;
                vector<unsigned char> vchPubKey = key.GetPubKey();
                CBitcoinAddress address(vchPubKey);
                string strAddress = address.ToString();
                printf("import: added address %s\n",strAddress.c_str());
                pwalletMain->AddKey(key);
                Value valLabel = find_value(objKey, "label");
                if (valLabel.type() != null_type)
                {
                    string strLabel = valLabel.get_str();
                    pwalletMain->SetAddressBookName(address, strLabel);
                    printf("import: set label of address %s to '%s'\n",strAddress.c_str(),strLabel.c_str());
                }
                if (find_value(objKey, "reserve").type() != null_type)
                {
                    setReserveKey.insert(address);
                    printf("import: %s preliminarily marked reserve\n",strAddress.c_str());
                }
                Value valTxs = find_value(objKey, "tx");
                if (valTxs.type() != null_type) // transactions are listed, check them
                {
                    Object objTxs = valTxs.get_obj();
                    map<string, Value> mapTxs;
                    obj_to_map(objTxs, mapTxs);
                    BOOST_FOREACH(PAIRTYPE(const string, Value)& item, mapTxs) // loop over transactions
                    {
                        Object objTx = item.second.get_obj();
                        Value valTxHeight = find_value(objTx, "height");
                        int nTxHeight = -1;
                        string strTxOut = item.first;
                        size_t nPos = strTxOut.find_first_of(':');
                        if (nPos != string::npos) // strip off nOut from tx string
                            strTxOut = strTxOut.substr(0, nPos);
                        if (valTxHeight.type() != null_type)
                            nTxHeight = valTxHeight.get_int();
                        if (nTxHeight <= nRescanHeight) // tx height is unknown, or below height from which we will rescan anyway
                        {
                            uint256 hashTx = uint256(strTxOut);
                            setRescanTx.insert(hashTx);
                            printf("import: %s will cause rescan of tx %s\n",hashTx.GetHex().c_str());
                        }
                    }
                }
                else
                {
                    Value valHeight = find_value(objKey, "height");
                    if (valHeight.type() != null_type)
                    {
                        int nHeight = valHeight.get_int();
                        if (nHeight <= nRescanHeight) // height is below height from which we will rescan anyway
                        {
                            nRescanHeight = nHeight-1;
                            printf("import: %s will need rescan from block %i\n",strAddress.c_str(),nHeight);
                        }
                    }
                }
            }
        }

        // rescan separate tx's
        BOOST_FOREACH(const uint256& hashTx, setRescanTx)
        {
            printf("import: rescanning tx %s\n",hashTx.GetHex().c_str());
            pwalletMain->ScanForWalletTransaction(hashTx);
        }
        CBlockIndex* pindex = pindexGenesisBlock;
        while (pindex && pindex->nHeight <= nRescanHeight)
            pindex = pindex->pnext;
        // rescan the end completely
        printf("import: rescanning from height %i\n",nRescanHeight+1);
        pwalletMain->ScanForWalletTransactions(pindex, true);

        if (nRescanHeight != pindexBest->nHeight && !setReserveKey.empty())
        {
            printf("import: scanning preliminary reserve keys\n");
            // scan wallet for transactions crediting imported reserve keys
            BOOST_FOREACH(const PAIRTYPE(uint256,CWalletTx)& item, pwalletMain->mapWallet)
            {
                BOOST_FOREACH(const CTxOut& txout, item.second.vout)
                {
                    CBitcoinAddress address;
                    if (ExtractAddress(txout.scriptPubKey, pwalletMain, address))
                    {
                        if (setReserveKey.count(address))
                        {
                            printf("import: wallettx %s credits %s: not reserve\n",item.first.GetHex().c_str(),address.ToString().c_str());
                            setReserveKey.erase(address);
                        }
                    }
                }
            }
        }

        // add remaining reserve keys to pool
        BOOST_FOREACH(const CBitcoinAddress& address, setReserveKey)
        {
            vector<unsigned char> vchPubKey;
            if (!pwalletMain->GetPubKey(address, vchPubKey))
                continue;
            printf("import: adding %s to key pool\n",address.ToString().c_str());
            pwalletMain->AddReserveKey(CKeyPool(vchPubKey));
        }

        pwalletMain->ReacceptWalletTransactions();
    }

    MainFrameRepaint();

    printf("import: done\n");
    return "";
}

Value dumpwallet(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "dumpwallet [option]...\n"
            "Dump wallet.\nOptions:\n"
            "    block:      include block information\n"
            "    tx:         include transaction-specific information\n"
            "    noreserve:  do not dump reserve keys\n"
            "    nolabel:    do not include address labels\n"
            "    nospent:    do not include spent keys/transactions\n"
            "    terse:      do not include addresses and amounts\n"
            "    compact:    no indentation or newlines\n"
            "Not included in dump: configuration settings, unconfirmed\n"
            "transactions, account information"
        );

    bool fBlock=false, fTx=false, fReserve=true, fLabel=true, fSpent=true,
         fAddr=true, fAmount=true, fCompact=false;
    for (int i = 0; i<params.size(); i++)
    {
        string strArg = params[i].get_str();
        if (strArg == "block")
            fBlock = true;
        if (strArg == "tx")
            fTx = true;
        if (strArg == "noreserve")
            fReserve = false;
        if (strArg == "nolabel")
            fLabel = false;
        if (strArg == "nospent")
            fSpent = false;
        if (strArg == "terse")
        {
            fAddr = false;
            fAmount = false;
        }
        if (strArg == "compact")
            fCompact = true;
        if (strArg == "backup")
        {
            fReserve = true;
            fLabel = true;
            fSpent = true;
        }
        if (strArg == "scratchoff")
        {
            fBlock = false;
            fTx = true;
            fReserve = false;
            fLabel = false;
            fSpent = false;
            fAddr = false;
            fAmount = false;
            fCompact = true;
        }
    }

    map<CBitcoinAddress,CKeyDump> mapDump;
    GetWalletDump(mapDump);
    Array keys;
    int nHeight = pindexBest->nHeight;
    for (map<CBitcoinAddress, CKeyDump>::iterator it = mapDump.begin(); it != mapDump.end(); ++it)
    {
        CKeyDump &keydump = (*it).second;
        if (!fReserve && keydump.fReserve)
            continue;
        if (!fSpent && keydump.nAvail==0)
            continue;
        Object jsonKey;
        if (fAddr)
            jsonKey.push_back(Pair("addr",(*it).first.ToString()));
        jsonKey.push_back(Pair("sec",keydump.secret.ToString()));
        if (fLabel && keydump.fLabel)
            jsonKey.push_back(Pair("label",keydump.strLabel));
        if (fBlock && !fTx && fSpent)
        {
            if (keydump.pindexUsed)
                jsonKey.push_back(Pair("height",(boost::int64_t)keydump.pindexUsed->nHeight));
            else
                jsonKey.push_back(Pair("height",(boost::int64_t)nHeight));
        }
        if (fBlock && !fTx)
        {
            if (keydump.pindexAvail)
                jsonKey.push_back(Pair("heightAvail",(boost::int64_t)keydump.pindexAvail->nHeight));
            else
                jsonKey.push_back(Pair("heightAvail",(boost::int64_t)nHeight));
        }
        if (fAmount && !fTx && keydump.fUsed && fSpent)
            jsonKey.push_back(Pair("value",FormatMoney(keydump.nValue)));
        if (fAmount && !fTx && keydump.nAvail>0)
            jsonKey.push_back(Pair("valueAvail",FormatMoney(keydump.nAvail)));
        if (keydump.fReserve)
            jsonKey.push_back(Pair("reserve",(boost::int64_t)1));
        if (fTx)
        {
            Object jsonTxs;
            BOOST_FOREACH(CTxDump& txdump, keydump.vtxdmp)
            {
                Object jsonTx;
                if (!fSpent && txdump.fSpent)
                    continue;
                if (txdump.pindex)
                {
                    if (fBlock) 
                        jsonTx.push_back(Pair("height",(boost::int64_t)txdump.pindex->nHeight));
                }
                if (fAmount)
                    jsonTx.push_back(Pair("value",FormatMoney(txdump.nValue)));
                if (txdump.fSpent)
                    jsonTx.push_back(Pair("spent",(boost::int64_t)1));
                jsonTxs.push_back(Pair(txdump.ptx->GetHash().GetHex() + ":" + boost::lexical_cast<std::string>(txdump.nOut),jsonTx));
            }
            jsonKey.push_back(Pair("tx",jsonTxs));
        }
        keys.push_back(jsonKey);
    }
    Object ret;
    ret.push_back(Pair("keys",keys));
    if (fBlock)
    {
        Object loc;
        CBlockIndex* pindexLoop = pindexBest;
        int nStep = 1;
        int nElem = 0;
        while (pindexLoop)
        {
            loc.push_back(Pair(pindexLoop->GetBlockHash().GetHex(),pindexLoop->nHeight));
            nElem++;
            for (int i = 0; pindexLoop && i < nStep; i++)
                pindexLoop = pindexLoop->pprev;
            if (nElem > 5)
                nStep *= 2;
        }
        ret.push_back(Pair("loc",loc));
    }
    if (fCompact)
        return write_string(Value(ret), false);
    return ret;
}

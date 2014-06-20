#include "redlist.h"
#include "main.h"
#include "base58.h"
#include <curl/curl.h>

using namespace std;
using namespace boost;

RedList* globalRedlist;

uint256 GetWorkSwitchThreshold()
{
    return REDLIST_SWITCH_THRESHOLD;
}

bool IsRedlistedBlock(const CBlock& block, CCoinsViewCache* view)
{
    BOOST_FOREACH(const CTransaction& tx, block.vtx)
    {
        if(IsRedlistedTransaction(tx, view))
            return true;
    }

    return false;
}

class CScriptKeysVisitor : public boost::static_visitor<void> {
private:
    std::vector<CKeyID> &vKeys;

public:
    CScriptKeysVisitor(std::vector<CKeyID> &vKeysIn) : vKeys(vKeysIn) {}

    void Process(const CScript &script) {
        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;
        if (ExtractDestinations(script, type, vDest, nRequired)) {
            BOOST_FOREACH(const CTxDestination &dest, vDest)
                boost::apply_visitor(*this, dest);
        }
    }

    void operator()(const CKeyID &keyId) {
        vKeys.push_back(keyId);
    }

    void operator()(const CScriptID &scriptId) {}

    void operator()(const CNoDestination &none) {}
};

void ExtractKeys(const CScript& scriptPubKey, std::vector<CKeyID> &vKeys) {
    CScriptKeysVisitor(vKeys).Process(scriptPubKey);
}

bool IsRedlistedTransaction(const CTransaction& tx, CCoinsViewCache* view)
{
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        if (IsRedlistedScriptPubKey(getRedList(), txout.scriptPubKey))
            return true;
    }

    // if it's not available, then the output is spent
    if(view->HaveInputs(tx) && !tx.IsCoinBase()) {
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
        {
            if (IsRedlistedScriptSig(getRedList(), txin.scriptSig))
                return true;
        }
    }

    return false;
}

bool IsRedlistedPubKey(RedList* rlist, basic_string<char> hashPubKey)
{
    return !(rlist->find(hashPubKey) == rlist->end());
}

bool IsRedlistedScriptPubKey(RedList* rlist, CScript scriptPubKey)
{
    LogPrintf("Checking scriptPubKey: \n%d\n", scriptPubKey.ToString());
    std::vector<CKeyID> vKeys;
    ExtractKeys(scriptPubKey, vKeys);
    BOOST_FOREACH(const CKeyID &key, vKeys)
    {
        CBitcoinAddress addr;
        addr.Set(key);
        LogPrintf("Transaction contained key: %s\n", addr.ToString());
        if(IsRedlistedPubKey(getRedList(), addr.ToString()))
        {    
            LogPrintf("CScript contains redlisted key: %s\n", addr.ToString());
            return true;
        }
    }
    return false;
}

bool IsRedlistedScriptSig(RedList* rlist, CScript scriptSig)
{
    
    LogPrintf("Checking scriptSig: \n%s\n", scriptSig.ToString()); 
    CScript::const_iterator pbeg = scriptSig.begin();
    CScript::const_iterator pend = scriptSig.end();
    opcodetype opcode;
    vector <unsigned char> vch;
   
    while(pbeg < pend)
    {
        scriptSig.GetOp(pbeg, opcode, vch);
        CBitcoinAddress addr;
        addr.Set(CKeyID(Hash160(vch)));
        if(IsRedlistedPubKey(getRedList(), addr.ToString()))
        {
            LogPrintf("ScriptSig contains redlisted key: %s\n", addr.ToString());
            return true;
        }
    }
    return false;
}

class curlbuffer {
    public:
        char* ptr;
        size_t len;

        curlbuffer()
        {
            len = 0;
            ptr = (char*) malloc(sizeof(char)*1);
            ptr[0] = '\0';
        }
};

size_t writefunc(char* ptr, size_t size, size_t nmemb, curlbuffer* b)
{
    size_t new_len = b->len + size*nmemb;
    b->ptr = (char*) realloc(b->ptr, new_len+1);
    if (b->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        return 0;
    }

    memcpy(b->ptr+b->len, ptr, size*nmemb);
    b->ptr[new_len] = '\0';
    b->len = new_len;

    return size*nmemb;
}

/**
 * Create a new redlist and return the pointer
 */
RedList* buildRedList()
{
    RedList* rList = new RedList;
    CURL* curl;
    CURLcode res;
    curlbuffer* buffer = new curlbuffer();
    basic_string<char> body;

    // TODO init can only be done once...
    curl = curl_easy_init();

    // TODO error handling
    if(!curl) {
        LogPrintf("Could not init curl.");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, REDLIST_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

    // perform the request, res will get the return code
    res = curl_easy_perform(curl);

    // check for errors
    // TODO exit?
    if(res != CURLE_OK) {
        LogPrintf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return NULL;
    }

    // always cleanup
    curl_easy_cleanup(curl);

    // get the list from the "HTTPS" source ;-)
    body = basic_string<char>(buffer->ptr);

    // tokenize it
    tokenizer<> tok(body);
    for(tokenizer<>::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
    {
        rList->insert(*beg);
    }

    return rList;
}

void redlistAddress(basic_string<char> address)
{
    if(globalRedlist == NULL)
    {
        globalRedlist = new RedList;
    }
    globalRedlist->insert(address);
}

RedList* getRedList() {
    // TODO rebuild if redlist is outdated
    if(globalRedlist == NULL) {
        updateRedList();
    }

    return globalRedlist;
}

void resetRedlist()
{
    globalRedlist = NULL;
}

bool updateRedList() {
    RedList* list = buildRedList();

    if(list == NULL ) {
        return false;
    }

    globalRedlist = list;
    return true;
}

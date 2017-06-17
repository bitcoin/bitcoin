#include <boost/lexical_cast.hpp>

#include "main.h"
#include "init.h"
#include "bitcoinrpc.h"
#include "kernelrecord.h"

using namespace json_spirit;
using namespace std;

string AccountFromValue(const Value& value);

Value listminting(const Array& params, bool fHelp)
{
    if(fHelp || params.size() > 2)
        throw runtime_error(
                "listminting [count=-1] [from=0]\n"
                "Return all mintable outputs and provide details for each of them.");

    int64 count = -1;
    if(params.size() > 0)
        count = params[0].get_int();

    int64 from = 0;
    if(params.size() > 1)
        from = params[1].get_int();

    Array ret;

    const CBlockIndex *p = GetLastBlockIndex(pindexBest, true);
    double difficulty = p->GetBlockDifficulty();

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {

        std::vector<KernelRecord> txList = KernelRecord::decomposeOutput(pwalletMain, it->second);
        int minAge = nStakeMinAge / 60 / 60 / 24;
        BOOST_FOREACH(KernelRecord& kr, txList) {
            if(!kr.spent) {

                if(count > 0 && ret.size() >= count) {
                    break;
                }

                string strTime = boost::lexical_cast<std::string>(kr.nTime);
                string strAmount = boost::lexical_cast<std::string>(kr.nValue);
                string strAge = boost::lexical_cast<std::string>(kr.getAge());
                string strCoinAge = boost::lexical_cast<std::string>(kr.coinAge);

                Array params;
                params.push_back(kr.address);
                string account = AccountFromValue(getaccount(params, false));

                string status = "immature";
                int searchInterval = 0;
                int attemps = 0;
                if(kr.getAge() >=  minAge)
                {
                    status = "mature";
                    searchInterval = (int)nLastCoinStakeSearchInterval;
                    attemps = GetAdjustedTime() - kr.nTime - nStakeMinAge;
                }

                Object obj;
                obj.push_back(Pair("account",                   account));
                obj.push_back(Pair("address",                   kr.address));
                obj.push_back(Pair("input-txid",                kr.hash.ToString()));
                obj.push_back(Pair("time",                      strTime));
                obj.push_back(Pair("amount",                    strAmount));
                obj.push_back(Pair("status",                    status));
                obj.push_back(Pair("age-in-day",                strAge));
                obj.push_back(Pair("coin-day-weight",           strCoinAge));
                obj.push_back(Pair("proof-of-stake-difficulty", difficulty));
                obj.push_back(Pair("minting-probability-10min", kr.getProbToMintWithinNMinutes(difficulty, 10)));
                obj.push_back(Pair("minting-probability-24h",   kr.getProbToMintWithinNMinutes(difficulty, 60*24)));
                obj.push_back(Pair("minting-probability-30d",   kr.getProbToMintWithinNMinutes(difficulty, 60*24*30)));
                obj.push_back(Pair("minting-probability-90d",   kr.getProbToMintWithinNMinutes(difficulty, 60*24*90)));
                obj.push_back(Pair("search-interval-in-sec",    searchInterval));
                obj.push_back(Pair("attempts",                  attemps));
                ret.push_back(obj);
            }
        }
    }

    return ret;
}

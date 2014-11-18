#include "kernelrecord.h"

#include "wallet.h"
#include "base58.h"
#include "main.h"

using namespace std;

bool KernelRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {
        if (wtx.GetDepthInMainChain() < 2)
        {
            return false;
        }
    }

    if(!wtx.IsTrusted())
    {
        return false;
    }

    return true;
}

/*
 * Decompose CWallet transaction to model kernel records.
 */
vector<KernelRecord> KernelRecord::decomposeOutput(const CWallet *wallet, const CWalletTx &wtx)
{
    vector<KernelRecord> parts;
    int64 nTime = wtx.GetTxTime();
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;
    int64 nDayWeight = (min((GetAdjustedTime() - nTime), (int64)(nStakeMaxAge+nStakeMinAge)) - nStakeMinAge); // DayWeight * 86400, чтобы был
                                                                                                              // правильный расчёт CoinAge                                                         
    if (showTransaction(wtx))
    {
        for (int nOut = 0; nOut < wtx.vout.size(); nOut++)
        {
            CTxOut txOut = wtx.vout[nOut];
            if( wallet->IsMine(txOut) ) {
                CTxDestination address;
                std::string addrStr;

                uint64 coinAge = max( (txOut.nValue * nDayWeight) / (COIN * 86400), (int64)0);

                if (ExtractDestination(txOut.scriptPubKey, address))
                {
                    // Sent to Bitcoin Address
                    addrStr = CBitcoinAddress(address).ToString();
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    addrStr = mapValue["to"];
                }

                parts.push_back(KernelRecord(hash, nTime, addrStr, txOut.nValue, wtx.IsSpent(nOut), coinAge));
            }
        }
    }

    return parts;
}

std::string KernelRecord::getTxID()
{
    return hash.ToString() + strprintf("-%03d", idx);
}

int64 KernelRecord::getAge() const
{
    return (GetAdjustedTime() - nTime) / 86400;
}

double KernelRecord::getProbToMintStake(double difficulty, int timeOffset) const
{
    //double maxTarget = pow(static_cast<double>(2), 224);
    //double target = maxTarget / difficulty;
    //int dayWeight = (min((GetAdjustedTime() - nTime) + timeOffset, (int64)(nStakeMinAge+nStakeMaxAge)) - nStakeMinAge) / 86400;
    //uint64 coinAge = max(nValue * dayWeight / COIN, (int64)0);
    //return target * coinAge / pow(static_cast<double>(2), 256);
    int Weight = (min((GetAdjustedTime() - nTime) + timeOffset, (int64)(nStakeMinAge+nStakeMaxAge)) - nStakeMinAge);
    uint64 coinAge = max(nValue * Weight / (COIN * 86400), (int64)0);
    return coinAge / (pow(static_cast<double>(2),32) * difficulty);
}

double KernelRecord::getProbToMintWithinNMinutes(double difficulty, int minutes)
{
    if(difficulty != prevDifficulty || minutes != prevMinutes)
    {
        double prob = 1;
        double p;
        int d = minutes / (60 * 24); // Number of full days
        int m = minutes % (60 * 24); // Number of minutes in the last day
        int i, timeOffset;

        // Probabilities for the first d days
        for(i = 0; i < d; i++)
        {
            timeOffset = i * 86400;
            p = pow(1 - getProbToMintStake(difficulty, timeOffset), 86400);
            prob *= p;
        }

        // Probability for the m minutes of the last day
        timeOffset = d * 86400;
        p = pow(1 - getProbToMintStake(difficulty, timeOffset), 60 * m);
        prob *= p;

        prob = 1 - prob;
        prevProbability = prob;
        prevDifficulty = difficulty;
        prevMinutes = minutes;
    }
    return prevProbability;
}

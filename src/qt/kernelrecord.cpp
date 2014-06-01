#include "kernelrecord.h"

#include "wallet.h"
#include "base58.h"


bool KernelRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {
        if (wtx.GetDepthInMainChain() < 2)
        {
            return false;
        }
    }
    return true;
}

/*
 * Decompose CWallet transaction to model kernel records.
 */
QList<KernelRecord> KernelRecord::decomposeOutput(const CWallet *wallet, const CWalletTx &wtx)
{
    QList<KernelRecord> parts;
    int64 nTime = wtx.GetTxTime();
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;
    CTxDB txdb("r");

    if (showTransaction(wtx))
    {
        for (int nOut = 0; nOut < wtx.vout.size(); nOut++)
        {
            CTxOut txOut = wtx.vout[nOut];
            if( wallet->IsMine(txOut) ) {
                CTxDestination address;
                std::string addrStr;
                uint64 coinAge;
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

                wtx.GetCoinAge(txdb, coinAge);
                parts.append(KernelRecord(hash, nTime, addrStr, txOut.nValue, wtx.IsSpent(nOut), coinAge));
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
    return (time(NULL) - nTime) / 86400;
}

double KernelRecord::getProbToMintStake(double difficulty) const
{
    double maxTarget = pow(2, 224);
    double target = maxTarget / difficulty;
    return target * coinAge / pow(2, 256);
}

double KernelRecord::getProbToMintWithinNMinutes(double difficulty, int minutes) const
{
    double prob = 1;
    double p = 0;
    int n = minutes / 10;
    for(int i=0; i<n; i++)
    {
         p = getProbToMintStake(difficulty);
         p = 1 - pow(1 - p, 60 * 10);
         prob = prob * (1-p);
    }
    prob = 1 - prob;
    return prob;
}

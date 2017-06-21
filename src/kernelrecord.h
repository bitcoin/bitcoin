#ifndef KERNELRECORD_H
#define KERNELRECORD_H

#include "uint256.h"

class CWallet;
class CWalletTx;

class KernelRecord
{
public:
    KernelRecord():
        hash(), nTime(0), address(""), nValue(0), idx(0), spent(false), coinAge(0), prevMinutes(0), prevDifficulty(0), prevProbability(0)
    {
    }

    KernelRecord(uint256 hash, int64 nTime):
            hash(hash), nTime(nTime), address(""), nValue(0), idx(0), spent(false), coinAge(0), prevMinutes(0), prevDifficulty(0), prevProbability(0)
    {
    }

    KernelRecord(uint256 hash, int64 nTime,
                 const std::string &address,
                 int64 nValue, int idx, bool spent, int64 coinAge):
        hash(hash), nTime(nTime), address(address), nValue(nValue),
        idx(idx), spent(spent), coinAge(coinAge), prevMinutes(0), prevDifficulty(0), prevProbability(0)
    {
    }

    static bool showTransaction(const CWalletTx &wtx);
    static std::vector<KernelRecord> decomposeOutput(const CWallet *wallet, const CWalletTx &wtx);


    uint256 hash;
    int64 nTime;
    std::string address;
    int64 nValue;
    int idx;
    bool spent;
    int64 coinAge;

    std::string getTxID();
    int64 getAge() const;
    double getProbToMintStake(double difficulty, int timeOffset = 0) const;
    double getProbToMintWithinNMinutes(double difficulty, int minutes);
protected:
    int prevMinutes;
    double prevDifficulty;
    double prevProbability;
};

#endif // KERNELRECORD_H

#ifndef KERNELRECORD_H
#define KERNELRECORD_H

#include "uint256.h"

#include <QList>

class CWallet;
class CWalletTx;

class KernelRecord
{
public:
    KernelRecord():
        hash(), time(0), address(""), nValue(0), idx(0), spent(false)
    {
    }

    KernelRecord(uint256 hash, int64 time):
            hash(hash), time(time), address(""), nValue(0), idx(0), spent(false)
    {
    }

    KernelRecord(uint256 hash, int64 time,
                 const std::string &address,
                 int64 nValue, bool spent):
        hash(hash), time(time), address(address), nValue(nValue),
        idx(0), spent(spent)
    {
    }

    static bool showTransaction(const CWalletTx &wtx);
    static QList<KernelRecord> decomposeOutput(const CWallet *wallet, const CWalletTx &wtx);


    uint256 hash;
    int64 time;
    std::string address;
    int64 nValue;
    bool spent;
    int idx;

    std::string getTxID();

};

#endif // KERNELRECORD_H

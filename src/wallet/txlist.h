#pragma once

#include <script/address.h>
#include <wallet/txrecord.h>
#include <wallet/ismine.h>
#include <boost/optional.hpp>

// Forward Declarations
class CTxOutput;
class CWallet;
class CWalletTx;

class TxList
{
    const CWallet& m_wallet;

public:
    TxList(const CWallet& wallet)
        : m_wallet(wallet) {}

    std::vector<WalletTxRecord> ListAll(const isminefilter& filter_ismine = ISMINE_ALL);
    std::vector<WalletTxRecord> List(
        const CWalletTx& wtx,
        const isminefilter& filter_ismine,
        const boost::optional<int>& nMinDepth,
        const boost::optional<std::string>& filter_label
    );

private:
    void List(std::vector<WalletTxRecord>& tx_records, const CWalletTx& wtx, const isminefilter& filter_ismine);

    void List_Credit(std::vector<WalletTxRecord>& tx_records, const CWalletTx& wtx, const isminefilter& filter_ismine);
    void List_Debit(std::vector<WalletTxRecord>& tx_records, const CWalletTx& wtx, const isminefilter& filter_ismine);
    void List_SelfSend(std::vector<WalletTxRecord>& tx_records, const CWalletTx& wtx, const isminefilter& filter_ismine);

    isminetype IsAddressMine(const CTxOutput& txout);
    DestinationAddr GetAddress(const CTxOutput& output);
    bool IsAllFromMe(const CWalletTx& wtx);
    bool IsAllToMe(const CWalletTx& wtx);
    bool IsMine(const CWalletTx& wtx);
};
// Copyright (c) 2012-2020 The Peercoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PEERCOIN_QT_MINTINGTABLEMODEL_H
#define PEERCOIN_QT_MINTINGTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class CWallet;
class MintingTablePriv;
class MintingFilterProxy;
class KernelRecord;
class WalletModel;

/** UI model for the minting table of a wallet.
 */
class MintingTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MintingTableModel(CWallet* wallet, WalletModel *parent = 0);
    ~MintingTableModel();

    enum ColumnIndex {
        TxHash = 0,
        Address = 1,
        Age = 2,
        Balance = 3,
        CoinDay = 4,
        MintProbability = 5
    };


    void setMintingProxyModel(MintingFilterProxy *mintingProxy);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

    void setMintingInterval(int interval);

private:
    CWallet* wallet;
    WalletModel *walletModel;
    QStringList columns;
    int mintingInterval;
    MintingTablePriv *priv;
    MintingFilterProxy *mintingProxyModel;
    int cachedNumBlocks;

    QString lookupAddress(const std::string &address, bool tooltip) const;

    double getDayToMint(KernelRecord *wtx) const;
    QString formatDayToMint(KernelRecord *wtx) const;
    QString formatTxAddress(const KernelRecord *wtx, bool tooltip) const;
    QString formatTxHash(const KernelRecord *wtx) const;
    QString formatTxAge(const KernelRecord *wtx) const;
    QString formatTxBalance(const KernelRecord *wtx) const;
    QString formatTxCoinDay(const KernelRecord *wtx) const;

public Q_SLOTS:
    void updateTransaction(const QString &hash, int status);
    void updateAge();
    void updateDisplayUnit();

    friend class MintingTablePriv;
};

#endif // PEERCOIN_QT_MINTINGTABLEMODEL_H

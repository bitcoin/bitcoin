#ifndef MINTINGTABLEMODEL_H
#define MINTINGTABLEMODEL_H


#include <QAbstractTableModel>
#include <QStringList>

class CWallet;
class MintingTablePriv;
class KernelRecord;
class WalletModel;

class MintingTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit MintingTableModel(CWallet * wallet, WalletModel *parent = 0);
    ~MintingTableModel();

    enum ColumnIndex {
        Address = 0,
        Age = 1,
        Balance = 2,
        CoinDay = 3,
        MintProbabilityPerDay = 4
    };


    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

private:
    CWallet* wallet;
    WalletModel *walletModel;
    QStringList columns;
    MintingTablePriv *priv;

    QString lookupAddress(const std::string &address, bool tooltip) const;

    QString formatTxAddress(const KernelRecord *wtx, bool tooltip) const;
    QString formatTxAge(const KernelRecord *wtx) const;
    QString formatTxBalance(const KernelRecord *wtx) const;
    QString formatTxCoinDay(const KernelRecord *wtx) const;
private slots:
    void update();

    friend class MintingTablePriv;
};

#endif // MINTINGTABLEMODEL_H

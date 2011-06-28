#ifndef TRANSACTIONTABLEMODEL_H
#define TRANSACTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class CWallet;
class TransactionTablePriv;
class TransactionRecord;

class TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TransactionTableModel(CWallet* wallet, QObject *parent = 0);
    ~TransactionTableModel();

    enum {
        Status = 0,
        Date = 1,
        Type = 2,
        ToAddress = 3,
        Amount = 4
    } ColumnIndex;

    // Roles to get specific information from a transaction row
    enum {
        // Type of transaction
        TypeRole = Qt::UserRole,
        // Date and time this transaction was created
        DateRole,
        // Long description (HTML format)
        LongDescriptionRole,
        // Address of transaction
        AddressRole,
        // Label of address related to transaction
        LabelRole,
        // Absolute net amount of transaction
        AbsoluteAmountRole
    } RoleIndex;

    /* TypeRole values */
    static const QString Sent;
    static const QString Received;
    static const QString Other;

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
private:
    CWallet* wallet;
    QStringList columns;
    TransactionTablePriv *priv;

    QString labelForAddress(const std::string &address) const;
    QString lookupAddress(const std::string &address) const;
    QVariant formatTxStatus(const TransactionRecord *wtx) const;
    QVariant formatTxDate(const TransactionRecord *wtx) const;
    QVariant formatTxType(const TransactionRecord *wtx) const;
    QVariant formatTxToAddress(const TransactionRecord *wtx) const;
    QVariant formatTxAmount(const TransactionRecord *wtx) const;
    QVariant formatTxDecoration(const TransactionRecord *wtx) const;

private slots:
    void update();

    friend class TransactionTablePriv;
};

#endif


#ifndef TRANSACTIONTABLEMODEL_H
#define TRANSACTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class TransactionTablePriv;
class TransactionRecord;

class TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TransactionTableModel(QObject *parent = 0);
    ~TransactionTableModel();

    enum {
        Status = 0,
        Date = 1,
        Description = 2,
        Debit = 3,
        Credit = 4
    } ColumnIndex;

    enum {
        TypeRole = Qt::UserRole,
        LongDescriptionRole = Qt::UserRole+1
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
    QStringList columns;
    TransactionTablePriv *priv;

    QVariant formatTxStatus(const TransactionRecord *wtx) const;
    QVariant formatTxDate(const TransactionRecord *wtx) const;
    QVariant formatTxDescription(const TransactionRecord *wtx) const;
    QVariant formatTxDebit(const TransactionRecord *wtx) const;
    QVariant formatTxCredit(const TransactionRecord *wtx) const;
    QVariant formatTxDecoration(const TransactionRecord *wtx) const;

private slots:
    void update();

    friend class TransactionTablePriv;
};

#endif


#ifndef TRANSACTIONTABLEMODEL_H
#define TRANSACTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TransactionTableModel(QObject *parent = 0);

    enum {
        Status = 0,
        Date = 1,
        Description = 2,
        Debit = 3,
        Credit = 4
    } ColumnIndex;

    enum {
        TypeRole = Qt::UserRole
    } RoleIndex;

    /* Transaction type */
    static const QString Sent;
    static const QString Received;
    static const QString Generated;

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
private:
    QStringList columns;
};

#endif


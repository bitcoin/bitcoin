#ifndef H_TRANSACTIONTABLEMODEL
#define H_TRANSACTIONTABLEMODEL

#include <QAbstractTableModel>
#include <QStringList>

class TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    TransactionTableModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
private:
    QStringList columns;
};

#endif


#include "TransactionTableModel.h"

TransactionTableModel::TransactionTableModel(QObject *parent):
        QAbstractTableModel(parent)
{
    columns << "Status" << "Date" << "Description" << "Debit" << "Credit";
}

int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 5;
}

int TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        /* index.row(), index.column() */
        /* Return QString */
        return QString("test");
    }
    return QVariant();
}

QVariant TransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if(orientation == Qt::Horizontal)
    {
        return columns[section];
    }
    return QVariant();
}

Qt::ItemFlags TransactionTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index);
}

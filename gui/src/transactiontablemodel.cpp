#include "transactiontablemodel.h"
#include "main.h"

#include <QLocale>

const QString TransactionTableModel::Sent = "s";
const QString TransactionTableModel::Received = "r";
const QString TransactionTableModel::Generated = "g";

/* Credit and Debit columns are right-aligned as they contain numbers */
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter
    };

TransactionTableModel::TransactionTableModel(QObject *parent):
        QAbstractTableModel(parent)
{
    columns << tr("Status") << tr("Date") << tr("Description") << tr("Debit") << tr("Credit");
}

int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
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
        return QLocale::system().toString(index.row());
    } else if (role == Qt::TextAlignmentRole)
    {
        return column_alignments[index.column()];
    } else if (role == TypeRole)
    {
        /* user role: transaction type for filtering
           "s" (sent)
           "r" (received)
           "g" (generated)
        */
        switch(index.row() % 3)
        {
        case 0: return QString("s");
        case 1: return QString("r");
        case 2: return QString("o");
        }
    }
    return QVariant();
}

QVariant TransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            return columns[section];
        }
    } else if (role == Qt::TextAlignmentRole)
    {
        return column_alignments[section];
    }
    return QVariant();
}

Qt::ItemFlags TransactionTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index);
}

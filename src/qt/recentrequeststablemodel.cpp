// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "recentrequeststablemodel.h"

#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"

#include "clientversion.h"
#include "streams.h"


RecentRequestsTableModel::RecentRequestsTableModel(CWallet *wallet, WalletModel *parent) :
    QAbstractTableModel(parent), walletModel(parent)
{
    Q_UNUSED(wallet);
    nReceiveRequestsMaxId = 0;

    // Load entries from wallet
    std::vector<std::string> vReceiveRequests;
    parent->loadReceiveRequests(vReceiveRequests);
    for (const std::string& request : vReceiveRequests)
        addNewRequest(request);

    /* These columns must match the indices in the ColumnIndex enumeration */
    columns << tr("Date") << tr("Label") << tr("Message") << getAmountTitle();

    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
}

RecentRequestsTableModel::~RecentRequestsTableModel()
{
    /* Intentionally left empty */
}

int RecentRequestsTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return list.length();
}

int RecentRequestsTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return columns.length();
}

QVariant RecentRequestsTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() >= list.length())
        return QVariant();

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        const RecentRequestEntry *rec = &list[index.row()];
        switch(index.column())
        {
        case Date:
            return GUIUtil::dateTimeStr(rec->date);
        case Label:
            if(rec->recipient.label.isEmpty() && role == Qt::DisplayRole)
            {
                return tr("(no label)");
            }
            else
            {
                return rec->recipient.label;
            }
        case Message:
            if(rec->recipient.message.isEmpty() && role == Qt::DisplayRole)
            {
                return tr("(no message)");
            }
            else
            {
                return rec->recipient.message;
            }
        case Amount:
            if (rec->recipient.amount == 0 && role == Qt::DisplayRole)
                return tr("(no amount requested)");
            else if (role == Qt::EditRole)
                return BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rec->recipient.amount, false, BitcoinUnits::separatorNever);
            else
                return BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rec->recipient.amount);
        }
    }
    else if (role == Qt::TextAlignmentRole)
    {
        if (index.column() == Amount)
            return (int)(Qt::AlignRight|Qt::AlignVCenter);
    }
    return QVariant();
}

bool RecentRequestsTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return true;
}

QVariant RecentRequestsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}

/** Updates the column title to "Amount (DisplayUnit)" and emits headerDataChanged() signal for table headers to react. */
void RecentRequestsTableModel::updateAmountColumnTitle()
{
    columns[Amount] = getAmountTitle();
    Q_EMIT headerDataChanged(Qt::Horizontal,Amount,Amount);
}

/** Gets title for amount column including current display unit if optionsModel reference available. */
QString RecentRequestsTableModel::getAmountTitle()
{
    return (this->walletModel->getOptionsModel() != nullptr) ? tr("Requested") + " ("+BitcoinUnits::name(this->walletModel->getOptionsModel()->getDisplayUnit()) + ")" : "";
}

QModelIndex RecentRequestsTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return createIndex(row, column);
}

bool RecentRequestsTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    if(count > 0 && row >= 0 && (row+count) <= list.size())
    {
        const RecentRequestEntry *rec;
        for (int i = 0; i < count; ++i)
        {
            rec = &list[row+i];
            if (!walletModel->saveReceiveRequest(rec->recipient.address.toStdString(), rec->id, ""))
                return false;
        }

        beginRemoveRows(parent, row, row + count - 1);
        list.erase(list.begin() + row, list.begin() + row + count);
        endRemoveRows();
        return true;
    } else {
        return false;
    }
}

Qt::ItemFlags RecentRequestsTableModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

// called when adding a request from the GUI
void RecentRequestsTableModel::addNewRequest(const SendCoinsRecipient &recipient)
{
    RecentRequestEntry newEntry;
    newEntry.id = ++nReceiveRequestsMaxId;
    newEntry.date = QDateTime::currentDateTime();
    newEntry.recipient = recipient;

    CDataStream ss(SER_DISK, CLIENT_VERSION);
    ss << newEntry;

    if (!walletModel->saveReceiveRequest(recipient.address.toStdString(), newEntry.id, ss.str()))
        return;

    addNewRequest(newEntry);
}

// called from ctor when loading from wallet
void RecentRequestsTableModel::addNewRequest(const std::string &recipient)
{
    std::vector<char> data(recipient.begin(), recipient.end());
    CDataStream ss(data, SER_DISK, CLIENT_VERSION);

    RecentRequestEntry entry;
    ss >> entry;

    if (entry.id == 0) // should not happen
        return;

    if (entry.id > nReceiveRequestsMaxId)
        nReceiveRequestsMaxId = entry.id;

    addNewRequest(entry);
}

// actually add to table in GUI
void RecentRequestsTableModel::addNewRequest(RecentRequestEntry &recipient)
{
    beginInsertRows(QModelIndex(), 0, 0);
    list.prepend(recipient);
    endInsertRows();
}

void RecentRequestsTableModel::sort(int column, Qt::SortOrder order)
{
    qSort(list.begin(), list.end(), RecentRequestEntryLessThan(column, order));
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(list.size() - 1, NUMBER_OF_COLUMNS - 1, QModelIndex()));
}

void RecentRequestsTableModel::updateDisplayUnit()
{
    updateAmountColumnTitle();
}

bool RecentRequestEntryLessThan::operator()(RecentRequestEntry &left, RecentRequestEntry &right) const
{
    RecentRequestEntry *pLeft = &left;
    RecentRequestEntry *pRight = &right;
    if (order == Qt::DescendingOrder)
        std::swap(pLeft, pRight);

    switch(column)
    {
    case RecentRequestsTableModel::Date:
        return pLeft->date.toTime_t() < pRight->date.toTime_t();
    case RecentRequestsTableModel::Label:
        return pLeft->recipient.label < pRight->recipient.label;
    case RecentRequestsTableModel::Message:
        return pLeft->recipient.message < pRight->recipient.message;
    case RecentRequestsTableModel::Amount:
        return pLeft->recipient.amount < pRight->recipient.amount;
    default:
        return pLeft->id < pRight->id;
    }
}

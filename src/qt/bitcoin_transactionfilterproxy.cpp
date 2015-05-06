// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_transactionfilterproxy.h"

#include "bitcoin_transactiontablemodel.h"
#include "bitcoin_transactionrecord.h"

#include <cstdlib>

#include <QDateTime>

// Earliest date that can be represented (far in the past)
const QDateTime Bitcoin_TransactionFilterProxy::MIN_DATE = QDateTime::fromTime_t(0);
// Last date that can be represented (far in the future)
const QDateTime Bitcoin_TransactionFilterProxy::MAX_DATE = QDateTime::fromTime_t(0xFFFFFFFF);

Bitcoin_TransactionFilterProxy::Bitcoin_TransactionFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    dateFrom(MIN_DATE),
    dateTo(MAX_DATE),
    addrPrefix(),
    typeFilter(ALL_TYPES),
    minAmount(0),
    limitRows(-1),
    showInactive(true)
{
}

bool Bitcoin_TransactionFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int type = index.data(Bitcoin_TransactionTableModel::TypeRole).toInt();
    QDateTime datetime = index.data(Bitcoin_TransactionTableModel::DateRole).toDateTime();
    QString address = index.data(Bitcoin_TransactionTableModel::AddressRole).toString();
    QString label = index.data(Bitcoin_TransactionTableModel::LabelRole).toString();
    qint64 amount = llabs(index.data(Bitcoin_TransactionTableModel::AmountRole).toLongLong());
    int status = index.data(Bitcoin_TransactionTableModel::StatusRole).toInt();

    if(!showInactive && status == TransactionStatus::Conflicted)
        return false;
    if(!(TYPE(type) & typeFilter))
        return false;
    if(datetime < dateFrom || datetime > dateTo)
        return false;
    if (!address.contains(addrPrefix, Qt::CaseInsensitive) && !label.contains(addrPrefix, Qt::CaseInsensitive))
        return false;
    if(amount < minAmount)
        return false;

    return true;
}

void Bitcoin_TransactionFilterProxy::setDateRange(const QDateTime &from, const QDateTime &to)
{
    this->dateFrom = from;
    this->dateTo = to;
    invalidateFilter();
}

void Bitcoin_TransactionFilterProxy::setAddressPrefix(const QString &addrPrefix)
{
    this->addrPrefix = addrPrefix;
    invalidateFilter();
}

void Bitcoin_TransactionFilterProxy::setTypeFilter(quint32 modes)
{
    this->typeFilter = modes;
    invalidateFilter();
}

void Bitcoin_TransactionFilterProxy::setMinAmount(qint64 minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
}

void Bitcoin_TransactionFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}

void Bitcoin_TransactionFilterProxy::setShowInactive(bool showInactive)
{
    this->showInactive = showInactive;
    invalidateFilter();
}

int Bitcoin_TransactionFilterProxy::rowCount(const QModelIndex &parent) const
{
    if(limitRows != -1)
    {
        return std::min(QSortFilterProxyModel::rowCount(parent), limitRows);
    }
    else
    {
        return QSortFilterProxyModel::rowCount(parent);
    }
}

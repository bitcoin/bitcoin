#include "mintingfilterproxy.h"

const quint32 MintingFilterProxy::MIN_AGE = 0;
const quint32 MintingFilterProxy::MAX_AGE = 90;


MintingFilterProxy::MintingFilterProxy(QObject * parent) :
    QSortFilterProxyModel(parent),
    typeFilter(ALL_TYPES),
    minAmount(0),
    limitRows(-1)
{
}


bool MintingFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
   /* QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int type = index.data(TransactionTableModel::TypeRole).toInt();
    QDateTime datetime = index.data(TransactionTableModel::DateRole).toDateTime();
    QString address = index.data(TransactionTableModel::AddressRole).toString();
    QString label = index.data(TransactionTableModel::LabelRole).toString();
    qint64 amount = llabs(index.data(TransactionTableModel::AmountRole).toLongLong());

    if(!(TYPE(type) & typeFilter))
        return false;
    if(datetime < dateFrom || datetime > dateTo)
        return false;
    if (!address.contains(addrPrefix, Qt::CaseInsensitive) && !label.contains(addrPrefix, Qt::CaseInsensitive))
        return false;
    if(amount < minAmount)
        return false;*/

    return true;
}


void MintingFilterProxy::setTypeFilter(quint32 modes)
{
    this->typeFilter = modes;
    invalidateFilter();
}

void MintingFilterProxy::setMinAmount(qint64 minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
}

void MintingFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}

int MintingFilterProxy::rowCount(const QModelIndex &parent) const
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

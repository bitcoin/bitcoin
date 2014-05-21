#ifndef MINTINGFILTERPROXY_H
#define MINTINGFILTERPROXY_H

#include <QSortFilterProxyModel>
#include <QDateTime>

class MintingFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit MintingFilterProxy(QObject *parent = 0);

    static const quint32 MIN_AGE;
    static const quint32 MAX_AGE;

    static const quint32 ALL_TYPES = 0xFFFFFFFF;
    void setTypeFilter(quint32 modes);
    void setMinAmount(qint64 minimum);
    void setLimit(int limit);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;

private:
    quint32 typeFilter;
    qint64 minAmount;
    int limitRows;

};

#endif // MINTINGFILTERPROXY_H

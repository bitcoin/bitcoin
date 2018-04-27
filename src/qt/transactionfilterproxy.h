#ifndef TRANSACTIONFILTERPROXY_H
#define TRANSACTIONFILTERPROXY_H

#include <QSortFilterProxyModel>
#include <QDateTime>

/** Filter the transaction list according to pre-specified rules. */
class TransactionFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit TransactionFilterProxy(QObject *parent = 0);

    /** Earliest date that can be represented (far in the past) */
    static const QDateTime MIN_DATE;
    /** Last date that can be represented (far in the future) */
    static const QDateTime MAX_DATE;
    /** Type filter bit field (all types) */
    static const quint32 ALL_TYPES = 0xFFFFFFFF;

    static quint32 TYPE(int type) { return 1<<type; }

    void setDateRange(const QDateTime &from, const QDateTime &to);
    void setAddressPrefix(const QString &addrPrefix);
    /**
      @note Type filter takes a bit field created with TYPE() or ALL_TYPES
     */
    void setTypeFilter(quint32 modes);
    void setMinAmount(qint64 minimum);

    /** Set maximum number of rows returned, -1 if unlimited. */
    void setLimit(int limit);

    /** Set whether to show conflicted transactions. */
    void setShowInactive(bool showInactive);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;

private:
    QDateTime dateFrom;
    QDateTime dateTo;
    QString addrPrefix;
    quint32 typeFilter;
    qint64 minAmount;
    int limitRows;
    bool showInactive;

signals:

public slots:

};

#endif // TRANSACTIONFILTERPROXY_H

// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TRANSACTIONFILTERPROXY_H
#define BITCOIN_QT_TRANSACTIONFILTERPROXY_H

#include <consensus/amount.h>

#include <QDateTime>
#include <QSortFilterProxyModel>

#include <optional>

/** Filter the transaction list according to pre-specified rules. */
class TransactionFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TransactionFilterProxy(QObject *parent = nullptr);

    /** Type filter bit field (all types) */
    static const quint32 ALL_TYPES = 0xFFFFFFFF;

    static quint32 TYPE(int type) { return 1<<type; }

    enum WatchOnlyFilter
    {
        WatchOnlyFilter_All,
        WatchOnlyFilter_Yes,
        WatchOnlyFilter_No
    };

    /** Filter transactions between date range. Use std::nullopt for open range. */
    void setDateRange(const std::optional<QDateTime>& from, const std::optional<QDateTime>& to);
    void setSearchString(const QString &);
    /**
      @note Type filter takes a bit field created with TYPE() or ALL_TYPES
     */
    void setTypeFilter(quint32 modes);
    void setMinAmount(const CAmount& minimum);
    void setWatchOnlyFilter(WatchOnlyFilter filter);

    /** Set whether to show conflicted transactions. */
    void setShowInactive(bool showInactive);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
    std::optional<QDateTime> dateFrom;
    std::optional<QDateTime> dateTo;
    QString m_search_string;
    quint32 typeFilter;
    WatchOnlyFilter watchOnlyFilter{WatchOnlyFilter_All};
    CAmount minAmount{0};
    bool showInactive{true};
};

#endif // BITCOIN_QT_TRANSACTIONFILTERPROXY_H

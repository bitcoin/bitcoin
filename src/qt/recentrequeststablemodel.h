// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RECENTREQUESTSTABLEMODEL_H
#define RECENTREQUESTSTABLEMODEL_H

#include "walletmodel.h"

#include <QAbstractTableModel>
#include <QStringList>
#include <QDateTime>

class Bitcredit_CWallet;

class Credits_RecentRequestEntry
{
public:
    Credits_RecentRequestEntry() : nVersion(Credits_RecentRequestEntry::CURRENT_VERSION), id(0) { }

    static const int CURRENT_VERSION = 1;
    int nVersion;
    int64_t id;
    QDateTime date;
    Bitcredit_SendCoinsRecipient recipient;

    IMPLEMENT_SERIALIZE
    (
        Credits_RecentRequestEntry* pthis = const_cast<Credits_RecentRequestEntry*>(this);

        unsigned int nDate = date.toTime_t();

        READWRITE(pthis->nVersion);
        nVersion = pthis->nVersion;
        READWRITE(id);
        READWRITE(nDate);
        READWRITE(recipient);

        if (fRead)
            pthis->date = QDateTime::fromTime_t(nDate);
    )
};

class Credits_RecentRequestEntryLessThan
{
public:
    Credits_RecentRequestEntryLessThan(int nColumn, Qt::SortOrder fOrder):
        column(nColumn), order(fOrder) {}
    bool operator()(Credits_RecentRequestEntry &left, Credits_RecentRequestEntry &right) const;

private:
    int column;
    Qt::SortOrder order;
};

/** Model for list of recently generated payment requests / bitcredit: URIs.
 * Part of wallet model.
 */
class Bitcredit_RecentRequestsTableModel: public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit Bitcredit_RecentRequestsTableModel(Bitcredit_CWallet *wallet, Bitcredit_WalletModel *parent);
    ~Bitcredit_RecentRequestsTableModel();

    enum ColumnIndex {
        Date = 0,
        Label = 1,
        Message = 2,
        Amount = 3,
        NUMBER_OF_COLUMNS
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex &index) const;
    /*@}*/

    const Credits_RecentRequestEntry &entry(int row) const { return list[row]; }
    void addNewRequest(const Bitcredit_SendCoinsRecipient &recipient);
    void addNewRequest(const std::string &recipient);
    void addNewRequest(Credits_RecentRequestEntry &recipient);

public slots:
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

private:
    Bitcredit_WalletModel *walletModel;
    QStringList columns;
    QList<Credits_RecentRequestEntry> list;
    int64_t nReceiveRequestsMaxId;
};

#endif

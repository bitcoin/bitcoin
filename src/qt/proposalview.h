// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALLIST_H
#define BITCOIN_QT_PROPOSALLIST_H

#include <qt/clientmodel.h>
#include <qt/proposaltablemodel.h>

#include <QWidget>

class ClientModel;
class PlatformStyle;
class ProposalFilterProxyModel;

namespace Ui {
    class ProposalList;
}

QT_BEGIN_NAMESPACE
class QDateTimeEdit;
class QFrame;
class QItemSelection;
class QMenu;
class QModelIndex;
QT_END_NAMESPACE

class ProposalList : public QWidget
{
    Q_OBJECT

public:
    explicit ProposalList(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~ProposalList();

    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        ThisYear,
        Range
    };

    void setClientModel(ClientModel *clientModel);
    void voteFunding();
    void voteDelete();
    void voteEndorse();
    void openProposalUrl();

private:
    ClientModel *clientModel;
    Ui::ProposalList *ui;
    ProposalTableModel *proposalTableModel;
    ProposalFilterProxyModel *proposalProxyModel;

    QMenu *contextMenu;

    QFrame *dateRangeWidget;
    QDateTimeEdit *dateFrom;
    QDateTimeEdit *dateTo;

    QWidget *createDateRangeWidget();

    void vote_click_handler(const std::string voteString);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    void chooseDate(int idx);

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);
};

#endif // BITCOIN_QT_PROPOSALLIST_H

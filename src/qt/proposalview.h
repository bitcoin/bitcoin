// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALLIST_H
#define BITCOIN_QT_PROPOSALLIST_H

#include <qt/clientmodel.h>
#include <qt/columnalignedlayout.h>
#include <qt/guiutil.h>
#include <qt/proposaltablemodel.h>

#include <QWidget>
#include <QKeyEvent>
#include <QTimer>

class ClientModel;
class PlatformStyle;
class ProposalFilterProxy;


QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QFrame;
class QItemSelectionModel;
class QLineEdit;
class QMenu;
class QModelIndex;
class QSignalMapper;
class QTableView;
QT_END_NAMESPACE

#define PROPOSALLIST_UPDATE_SECONDS 30

class ProposalList : public QWidget
{
    Q_OBJECT

public:
    explicit ProposalList(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setClientModel(ClientModel *clientModel);

    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

    enum ColumnWidths {
        AMOUNT_COLUMN_WIDTH = 100,
        START_DATE_COLUMN_WIDTH = 140,
        END_DATE_COLUMN_WIDTH = 140,
        YES_VOTES_COLUMN_WIDTH = 60,
        NO_VOTES_COLUMN_WIDTH = 60,
        ABSOLUTE_YES_COLUMN_WIDTH = 60,
        PROPOSAL_COLUMN_WIDTH = 300,
        PERCENTAGE_COLUMN_WIDTH = 100,
        MINIMUM_COLUMN_WIDTH = 23
    };

private:
    ClientModel *clientModel;
    ProposalTableModel *proposalTableModel;
    ProposalFilterProxy *proposalProxyModel;
    QTableView *proposalList;
    int64_t nLastUpdate = 0;

    QLineEdit *proposalWidget;
    QComboBox *dateStartWidget;
    QComboBox *dateEndWidget;
    QTimer *timer;

    QLineEdit *yesVotesWidget;
    QLineEdit *noVotesWidget;
    QLineEdit *absoluteYesVotesWidget;
    QLineEdit *amountWidget;
    QLineEdit *percentageWidget;
    QLabel *secondsLabel;

    QMenu *contextMenu;

    QFrame *dateStartRangeWidget;
    QDateTimeEdit *dateStartFrom;
    QDateTimeEdit *dateStartTo;

    QFrame *dateEndRangeWidget;
    QDateTimeEdit *dateEndFrom;
    QDateTimeEdit *dateEndTo;
    ColumnAlignedLayout *hlayout;

    QWidget *createDateStartRangeWidget();
    QWidget *createDateEndRangeWidget();
    void vote_click_handler(const std::string voteString);

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateStartRangeChanged();
    void dateEndRangeChanged();
    void voteYes();
    void voteNo();
    void voteAbstain();
    void openProposalUrl();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

public Q_SLOTS:
    void refreshProposals();
    void changedProposal();
    void chooseStartDate(int idx);
    void chooseEndDate(int idx);
    void changedYesVotes();
    void changedNoVotes();
    void changedAbsoluteYesVotes();
    void changedPercentage();
    void changedAmount();

};

#endif // BITCOIN_QT_PROPOSALLIST_H

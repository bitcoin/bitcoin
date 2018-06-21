// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALLIST_H
#define BITCOIN_QT_PROPOSALLIST_H

#include "guiutil.h"
#include "proposaltablemodel.h"
#include "columnalignedlayout.h"
#include <QWidget>
#include <QKeyEvent>
#include <QTimer>

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

    void setModel();

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
        PROPOSAL_COLUMN_WIDTH = 380,
        START_DATE_COLUMN_WIDTH = 110,
        END_DATE_COLUMN_WIDTH = 110,
        YES_VOTES_COLUMN_WIDTH = 60,
        NO_VOTES_COLUMN_WIDTH = 60,
        ABSOLUTE_YES_COLUMN_WIDTH = 60,
        AMOUNT_COLUMN_WIDTH = 100,
        PERCENTAGE_COLUMN_WIDTH = 80,
        MINIMUM_COLUMN_WIDTH = 23
    };

private:
    ProposalTableModel *proposalTableModel;
    ProposalFilterProxy *proposalProxyModel;
    QTableView *proposalList;
    int64_t nLastUpdate = 0;

    QLineEdit *proposalWidget;
    QComboBox *startDateWidget;
    QComboBox *endDateWidget;
    QTimer *timer;

    QLineEdit *yesVotesWidget;
    QLineEdit *noVotesWidget;
    QLineEdit *absoluteYesVotesWidget;
    QLineEdit *amountWidget;
    QLineEdit *percentageWidget;
    QLabel *secondsLabel;

    QMenu *contextMenu;

    QFrame *startDateRangeWidget;
    QDateTimeEdit *proposalStartDate;

    QFrame *endDateRangeWidget;
    QDateTimeEdit *proposalEndDate;
    ColumnAlignedLayout *hlayout;

    QWidget *createStartDateRangeWidget();
    QWidget *createEndDateRangeWidget();
    void vote_click_handler(const std::string voteString);

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void startDateRangeChanged();
    void endDateRangeChanged();
    void voteYes();
    void voteNo();
    void voteAbstain();
    void openProposalUrl();
    void invalidateAlignedLayout();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

public Q_SLOTS:
    void refreshProposals(bool force = false);
    void changedProposal(const QString &proposal);
    void chooseStartDate(int idx);
    void chooseEndDate(int idx);
    void changedYesVotes(const QString &minYesVotes);
    void changedNoVotes(const QString &minNoVotes);
    void changedAbsoluteYesVotes(const QString &minAbsoluteYesVotes);
    void changedPercentage(const QString &minPercentage);
    void changedAmount(const QString &minAmount);

};

#endif // BITCOIN_QT_PROPOSALLIST_H

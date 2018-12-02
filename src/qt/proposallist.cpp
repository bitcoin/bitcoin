// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposallist.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/optionsmodel.h>
#include <qt/proposalfilterproxy.h>
#include <qt/proposalrecord.h>
#include <qt/proposaltablemodel.h>

#include <interfaces/node.h>
#include <util/system.h>
 
#include <ui_interface.h>

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QSettings>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

ProposalList::ProposalList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent), clientModel(0),
    proposalProxyModel(0), proposalList(0), columnResizingFixer(0)
{
    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);

    if (platformStyle->getUseExtraSpacing()) {
        hlayout->setSpacing(5);
        hlayout->addSpacing(26);
    } else {
        hlayout->setSpacing(0);
        hlayout->addSpacing(23);
    }

    amountWidget = new QLineEdit(this);
    amountWidget->setFixedWidth(AMOUNT_COLUMN_WIDTH);
    amountWidget->setPlaceholderText(tr("Min amount"));
    amountWidget->setValidator(new QDoubleValidator(0, 1e20, 8, this));
    amountWidget->setObjectName("amountWidget");
    hlayout->addWidget(amountWidget);

    dateStartWidget = new QComboBox(this);
    if (platformStyle->getUseExtraSpacing()) {
        dateStartWidget->setFixedWidth(START_DATE_COLUMN_WIDTH + 1);
    } else {
        dateStartWidget->setFixedWidth(START_DATE_COLUMN_WIDTH);
    }
    dateStartWidget->addItem(tr("All"), All);
    dateStartWidget->addItem(tr("Today"), Today);
    dateStartWidget->addItem(tr("This week"), ThisWeek);
    dateStartWidget->addItem(tr("This month"), ThisMonth);
    dateStartWidget->addItem(tr("Last month"), LastMonth);
    dateStartWidget->addItem(tr("This year"), ThisYear);
    dateStartWidget->addItem(tr("Range..."), Range);
    hlayout->addWidget(dateStartWidget);

    dateEndWidget = new QComboBox(this);
    if (platformStyle->getUseExtraSpacing()) {
        dateEndWidget->setFixedWidth(END_DATE_COLUMN_WIDTH + 1);
    } else {
        dateEndWidget->setFixedWidth(END_DATE_COLUMN_WIDTH);
    }
    dateEndWidget->addItem(tr("All"), All);
    dateEndWidget->addItem(tr("Today"), Today);
    dateEndWidget->addItem(tr("This week"), ThisWeek);
    dateEndWidget->addItem(tr("This month"), ThisMonth);
    dateEndWidget->addItem(tr("Last month"), LastMonth);
    dateEndWidget->addItem(tr("This year"), ThisYear);
    dateEndWidget->addItem(tr("Range..."), Range);
    hlayout->addWidget(dateEndWidget);

    yesVotesWidget = new QLineEdit(this);
    if (platformStyle->getUseExtraSpacing()) {
        yesVotesWidget->setFixedWidth(YES_VOTES_COLUMN_WIDTH + 1);
    } else {
        yesVotesWidget->setFixedWidth(YES_VOTES_COLUMN_WIDTH);
    }
    yesVotesWidget->setPlaceholderText(tr("Min yes votes"));
    yesVotesWidget->setValidator(new QIntValidator(0, INT_MAX, this));
    yesVotesWidget->setObjectName("yesVotesWidget");
    hlayout->addWidget(yesVotesWidget);

    noVotesWidget = new QLineEdit(this);
    if (platformStyle->getUseExtraSpacing()) {
        noVotesWidget->setFixedWidth(NO_VOTES_COLUMN_WIDTH + 1);
    } else {
        noVotesWidget->setFixedWidth(NO_VOTES_COLUMN_WIDTH);
    }
    noVotesWidget->setPlaceholderText(tr("Min no votes"));
    noVotesWidget->setValidator(new QIntValidator(0, INT_MAX, this));
    noVotesWidget->setObjectName("noVotesWidget");
    hlayout->addWidget(noVotesWidget);


    absoluteYesVotesWidget = new QLineEdit(this);
    if (platformStyle->getUseExtraSpacing()) {
        absoluteYesVotesWidget->setFixedWidth(ABSOLUTE_YES_COLUMN_WIDTH + 1);
    } else {
        absoluteYesVotesWidget->setFixedWidth(ABSOLUTE_YES_COLUMN_WIDTH);
    }
    absoluteYesVotesWidget->setPlaceholderText(tr("Min abs. yes votes"));
    absoluteYesVotesWidget->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
    absoluteYesVotesWidget->setObjectName("absoluteYesVotesWidget");
    hlayout->addWidget(absoluteYesVotesWidget);

    proposalWidget = new QLineEdit(this);
    proposalWidget->setPlaceholderText(tr("Enter proposal name"));
    proposalWidget->setObjectName("proposalWidget");
    hlayout->addWidget(proposalWidget);

    percentageWidget = new QLineEdit(this);
    if (platformStyle->getUseExtraSpacing()) {
        percentageWidget->setFixedWidth(PERCENTAGE_COLUMN_WIDTH + 1);
    } else {
        percentageWidget->setFixedWidth(PERCENTAGE_COLUMN_WIDTH);
    }
    percentageWidget->setPlaceholderText(tr("Min percentage"));
    percentageWidget->setValidator(new QIntValidator(-100, 100, this));
    percentageWidget->setObjectName("percentageWidget");
    hlayout->addWidget(percentageWidget);

    // Delay before filtering proposals in ms
    static const int input_filter_delay = 200;

    QTimer* amount_typing_delay = new QTimer(this);
    amount_typing_delay->setSingleShot(true);
    amount_typing_delay->setInterval(input_filter_delay);

    QTimer* prefix_typing_delay = new QTimer(this);
    prefix_typing_delay->setSingleShot(true);
    prefix_typing_delay->setInterval(input_filter_delay);

    QTimer* yes_typing_delay = new QTimer(this);
    yes_typing_delay->setSingleShot(true);
    yes_typing_delay->setInterval(input_filter_delay);

    QTimer* no_typing_delay = new QTimer(this);
    no_typing_delay->setSingleShot(true);
    no_typing_delay->setInterval(input_filter_delay);

    QTimer* abs_yes_typing_delay = new QTimer(this);
    abs_yes_typing_delay->setSingleShot(true);
    abs_yes_typing_delay->setInterval(input_filter_delay);

    QTimer* percentage_typing_delay = new QTimer(this);
    percentage_typing_delay->setSingleShot(true);
    percentage_typing_delay->setInterval(input_filter_delay);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(createDateStartRangeWidget());
    vlayout->addWidget(createDateEndRangeWidget());
    vlayout->addWidget(view);
    vlayout->setSpacing(0);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
    if (platformStyle->getUseExtraSpacing()) {
        hlayout->addSpacing(width+2);
    } else {
        hlayout->addSpacing(width);
    }
    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    view->installEventFilter(this);

    proposalList = view;
    proposalList->setObjectName("proposalList");

    QHBoxLayout *actionBar = new QHBoxLayout();
    actionBar->setSpacing(11);
    actionBar->addSpacing(25);
    actionBar->setContentsMargins(0,20,0,20);

    QPushButton *voteYesButton = new QPushButton(tr("Vote Yes"), this);
    voteYesButton->setToolTip(tr("Vote Yes on the selected proposal"));
    actionBar->addWidget(voteYesButton);

    QPushButton *voteAbstainButton = new QPushButton(tr("Vote Abstain"), this);
    voteAbstainButton->setToolTip(tr("Vote Abstain on the selected proposal"));
    actionBar->addWidget(voteAbstainButton);

    QPushButton *voteNoButton = new QPushButton(tr("Vote No"), this);
    voteNoButton->setToolTip(tr("Vote No on the selected proposal"));
    actionBar->addWidget(voteNoButton);

    secondsLabel = new QLabel();
    actionBar->addWidget(secondsLabel);
    actionBar->addStretch();

    vlayout->addLayout(actionBar);

    QAction *voteYesAction = new QAction(tr("Vote yes"), this);
    QAction *voteAbstainAction = new QAction(tr("Vote abstain"), this);
    QAction *voteNoAction = new QAction(tr("Vote no"), this);
    QAction *openUrlAction = new QAction(tr("Visit proposal website"), this);
    // QAction *openStatisticsAction = new QAction(tr("Visit statistics website"), this);

    contextMenu = new QMenu(this);
    contextMenu->addAction(voteYesAction);
    contextMenu->addAction(voteAbstainAction);
    contextMenu->addAction(voteNoAction);
    contextMenu->addSeparator();
    contextMenu->addAction(openUrlAction);

    connect(voteYesButton, &QPushButton::clicked, this, &ProposalList::voteYes);
    connect(voteAbstainButton, &QPushButton::clicked, this, &ProposalList::voteAbstain);
    connect(voteNoButton, &QPushButton::clicked, this, &ProposalList::voteNo);

    connect(proposalWidget, &QLineEdit::textChanged, prefix_typing_delay, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(prefix_typing_delay, &QTimer::timeout, this, &ProposalList::changedProposal);
    connect(dateStartWidget, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ProposalList::chooseStartDate);
    connect(dateEndWidget, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ProposalList::chooseEndDate);
    connect(yesVotesWidget, &QLineEdit::textChanged, yes_typing_delay, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(yes_typing_delay, &QTimer::timeout, this, &ProposalList::changedYesVotes);
    connect(noVotesWidget, &QLineEdit::textChanged, no_typing_delay, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(no_typing_delay, &QTimer::timeout, this, &ProposalList::changedNoVotes);
    connect(absoluteYesVotesWidget, &QLineEdit::textChanged, abs_yes_typing_delay, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(abs_yes_typing_delay, &QTimer::timeout, this, &ProposalList::changedAbsoluteYesVotes);
    connect(amountWidget, &QLineEdit::textChanged, amount_typing_delay, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(amount_typing_delay, &QTimer::timeout, this, &ProposalList::changedAmount);
    connect(percentageWidget, &QLineEdit::textChanged, percentage_typing_delay, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(percentage_typing_delay, &QTimer::timeout, this, &ProposalList::changedPercentage);

    connect(view, &QTableView::doubleClicked, this, &ProposalList::openProposalUrl);
    connect(view, &QTableView::customContextMenuRequested, this, &ProposalList::contextualMenu);

    connect(voteYesAction, &QAction::triggered, this, &ProposalList::voteYes);
    connect(voteNoAction, &QAction::triggered, this, &ProposalList::voteNo);
    connect(voteAbstainAction, &QAction::triggered, this, &ProposalList::voteAbstain);

    connect(openUrlAction, &QAction::triggered, this, &ProposalList::openProposalUrl);


    // connect(proposalList->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(computeSum()));

    setLayout(vlayout);
}

void ProposalList::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
    if (_clientModel) {
        proposalProxyModel = new ProposalFilterProxy(this);
        proposalProxyModel->setSourceModel(_clientModel->getProposalTableModel());
        proposalProxyModel->setDynamicSortFilter(true);
        proposalProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        proposalProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        proposalProxyModel->setSortRole(Qt::EditRole);

        proposalList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        proposalList->setModel(proposalProxyModel);
        proposalList->setAlternatingRowColors(true);
        proposalList->setSelectionBehavior(QAbstractItemView::SelectRows);
        proposalList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        proposalList->setSortingEnabled(true);
        proposalList->sortByColumn(ProposalTableModel::StartDate, Qt::DescendingOrder);
        proposalList->verticalHeader()->hide();

        proposalList->setColumnWidth(ProposalTableModel::Amount, AMOUNT_COLUMN_WIDTH);
        proposalList->setColumnWidth(ProposalTableModel::StartDate, START_DATE_COLUMN_WIDTH);
        proposalList->setColumnWidth(ProposalTableModel::EndDate, END_DATE_COLUMN_WIDTH);
        proposalList->setColumnWidth(ProposalTableModel::YesVotes, YES_VOTES_COLUMN_WIDTH);
        proposalList->setColumnWidth(ProposalTableModel::NoVotes, NO_VOTES_COLUMN_WIDTH);
        proposalList->setColumnWidth(ProposalTableModel::AbsoluteYesVotes, ABSOLUTE_YES_COLUMN_WIDTH);
        proposalList->setColumnWidth(ProposalTableModel::Proposal, PROPOSAL_COLUMN_WIDTH);
        proposalList->setColumnWidth(ProposalTableModel::Percentage, PERCENTAGE_COLUMN_WIDTH);

        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(proposalList, PERCENTAGE_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH, this);
    }
}

void ProposalList::refreshProposals()
{
    int64_t secondsRemaining = nLastUpdate - GetTime() + PROPOSALLIST_UPDATE_SECONDS;

    QString secOrMinutes = (secondsRemaining / 60 > 1) ? tr("minute(s)") : tr("second(s)");
    secondsLabel->setText(tr("List will be updated in %1 %2").arg((secondsRemaining > 60) ? QString::number(secondsRemaining / 60) : QString::number(secondsRemaining), secOrMinutes));

    nLastUpdate = GetTime();

    // proposalTableModel->refreshProposals();

    secondsLabel->setText(tr("List will be updated in 0 second(s)"));
}

void ProposalList::chooseStartDate(int idx)
{
    if(!proposalProxyModel) return;
    QDate current = QDate::currentDate();
    dateStartRangeWidget->setVisible(false);
    switch(dateStartWidget->itemData(idx).toInt())
    {
    case All:
        proposalProxyModel->setDateStartRange(
                ProposalFilterProxy::MIN_DATE,
                ProposalFilterProxy::MAX_DATE);
        break;
    case Today:
        proposalProxyModel->setDateStartRange(
                QDateTime(current),
                ProposalFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        proposalProxyModel->setDateStartRange(
                QDateTime(startOfWeek),
                ProposalFilterProxy::MAX_DATE);
        } break;
    case ThisMonth:
        proposalProxyModel->setDateStartRange(
                QDateTime(QDate(current.year(), current.month(), 1)),
                ProposalFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        proposalProxyModel->setDateStartRange(
                QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        proposalProxyModel->setDateStartRange(
                QDateTime(QDate(current.year(), 1, 1)),
                ProposalFilterProxy::MAX_DATE);
        break;
    case Range:
        dateStartRangeWidget->setVisible(true);
        dateStartRangeChanged();
        break;
    }
}

void ProposalList::chooseEndDate(int idx)
{
    if(!proposalProxyModel) return;
    QDate current = QDate::currentDate();
    dateEndRangeWidget->setVisible(false);
    switch(dateEndWidget->itemData(idx).toInt())
    {
    case All:
        proposalProxyModel->setDateEndRange(
                ProposalFilterProxy::MIN_DATE,
                ProposalFilterProxy::MAX_DATE);
        break;
    case Today:
        proposalProxyModel->setDateEndRange(
                QDateTime(current),
                ProposalFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        proposalProxyModel->setDateEndRange(
                QDateTime(startOfWeek),
                ProposalFilterProxy::MAX_DATE);
        } break;
    case ThisMonth:
        proposalProxyModel->setDateEndRange(
                QDateTime(QDate(current.year(), current.month(), 1)),
                ProposalFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        proposalProxyModel->setDateEndRange(
                QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        proposalProxyModel->setDateEndRange(
                QDateTime(QDate(current.year(), 1, 1)),
                ProposalFilterProxy::MAX_DATE);
        break;
    case Range:
        dateEndRangeWidget->setVisible(true);
        dateEndRangeChanged();
        break;
    }
}


void ProposalList::changedAmount()
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setMinAmount(amountWidget->text().toInt());
}

void ProposalList::changedPercentage()
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setMinPercentage(percentageWidget->text().toInt());
}

void ProposalList::changedProposal()
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setProposal(proposalWidget->text());
}

void ProposalList::changedYesVotes()
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setMinYesVotes(yesVotesWidget->text().toInt());
}

void ProposalList::changedNoVotes()
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setMinNoVotes(noVotesWidget->text().toInt());
}

void ProposalList::changedAbsoluteYesVotes()
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setMinAbsoluteYesVotes(absoluteYesVotesWidget->text().toInt());
}

void ProposalList::contextualMenu(const QPoint &point)
{
    QModelIndex index = proposalList->indexAt(point);
    QModelIndexList selection = proposalList->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    if(index.isValid())
        contextMenu->exec(QCursor::pos());
}

void ProposalList::voteYes()
{
    vote_click_handler("yes");
}

void ProposalList::voteNo()
{
    vote_click_handler("no");
}

void ProposalList::voteAbstain()
{
    vote_click_handler("abstain");
}

void ProposalList::vote_click_handler(const std::string voteString)
{
    if (!proposalList->selectionModel())
        return;

    if (!clientModel)
        return;

    QModelIndexList selection = proposalList->selectionModel()->selectedRows();
    if(selection.empty())
        return;

    QString proposalName = selection.at(0).data(ProposalTableModel::ProposalRole).toString();

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote"),
        tr("Are you sure you want to vote <strong>%1</strong> on the proposal <strong>%2</strong>?").arg(QString::fromStdString(voteString), proposalName),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    uint256 hash;
    hash.SetHex(selection.at(0).data(ProposalTableModel::ProposalHashRole).toString().toStdString());

    int nSuccessful = 0;
    int nFailed = 0;

    if (!clientModel->node().sendVoting(hash, voteString, nSuccessful, nFailed))
        QMessageBox::critical(this, tr("No Client Connection"),
            tr("There was an error connecting to the network. Please try again."));
    else
        QMessageBox::information(this, tr("Voting"),
            tr("You voted %1 %2 time(s) successfully and failed %3 time(s) on %4").arg(QString::fromStdString(voteString), QString::number(nSuccessful), QString::number(nFailed), proposalName));
}

void ProposalList::openProposalUrl()
{
    if(!proposalList || !proposalList->selectionModel())
        return;

    QModelIndexList selection = proposalList->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
         QDesktopServices::openUrl(selection.at(0).data(ProposalTableModel::ProposalUrlRole).toString());
}

QWidget *ProposalList::createDateStartRangeWidget()
{
    dateStartRangeWidget = new QFrame();
    dateStartRangeWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    dateStartRangeWidget->setContentsMargins(1,1,1,1);
    QHBoxLayout *layout = new QHBoxLayout(dateStartRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));

    dateStartFrom = new QDateTimeEdit(this);
    dateStartFrom->setDisplayFormat("dd/MM/yy");
    dateStartFrom->setCalendarPopup(true);
    dateStartFrom->setMinimumWidth(100);
    dateStartFrom->setDate(QDate::currentDate().addDays(-7));
    layout->addWidget(dateStartFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateStartTo = new QDateTimeEdit(this);
    dateStartTo->setDisplayFormat("dd/MM/yy");
    dateStartTo->setCalendarPopup(true);
    dateStartTo->setMinimumWidth(100);
    dateStartTo->setDate(QDate::currentDate());
    layout->addWidget(dateStartTo);
    layout->addStretch();

    // Hide by default
    dateStartRangeWidget->setVisible(false);

    // Notify on change
    connect(dateStartFrom, &QDateTimeEdit::dateChanged, this, &ProposalList::dateStartRangeChanged);
    connect(dateStartTo, &QDateTimeEdit::dateChanged, this, &ProposalList::dateStartRangeChanged);

    return dateStartRangeWidget;
}

QWidget *ProposalList::createDateEndRangeWidget()
{
    dateEndRangeWidget = new QFrame();
    dateEndRangeWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    dateEndRangeWidget->setContentsMargins(1,1,1,1);
    QHBoxLayout *layout = new QHBoxLayout(dateEndRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));

    dateEndFrom = new QDateTimeEdit(this);
    dateEndFrom->setDisplayFormat("dd/MM/yy");
    dateEndFrom->setCalendarPopup(true);
    dateEndFrom->setMinimumWidth(100);
    dateEndFrom->setDate(QDate::currentDate().addDays(-7));
    layout->addWidget(dateEndFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateEndTo = new QDateTimeEdit(this);
    dateEndTo->setDisplayFormat("dd/MM/yy");
    dateEndTo->setCalendarPopup(true);
    dateEndTo->setMinimumWidth(100);
    dateEndTo->setDate(QDate::currentDate());
    layout->addWidget(dateEndTo);
    layout->addStretch();

    // Hide by default
    dateEndRangeWidget->setVisible(false);

    // Notify on change
    connect(dateEndFrom, &QDateTimeEdit::dateChanged, this, &ProposalList::dateEndRangeChanged);
    connect(dateEndTo, &QDateTimeEdit::dateChanged, this, &ProposalList::dateEndRangeChanged);

    return dateEndRangeWidget;
}

void ProposalList::dateStartRangeChanged()
{
    if(!proposalProxyModel)
        return;
    proposalProxyModel->setDateStartRange(
            QDateTime(dateStartFrom->date()),
            QDateTime(dateStartTo->date()).addDays(1));
}

void ProposalList::dateEndRangeChanged()
{
    if(!proposalProxyModel)
        return;
    proposalProxyModel->setDateEndRange(
            QDateTime(dateEndFrom->date()),
            QDateTime(dateEndTo->date()).addDays(1));
}

void ProposalList::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(ProposalTableModel::Proposal);
}

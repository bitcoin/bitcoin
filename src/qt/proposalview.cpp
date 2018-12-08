// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalview.h>

#include <qt/forms/ui_proposallist.h>

#include <qt/clientmodel.h>
#include <qt/platformstyle.h>
#include <qt/proposaltablemodel.h>

#include <interfaces/node.h>
#include <util/system.h>
 
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QUrl>

class ProposalFilterProxyModel final : public QSortFilterProxyModel
{
    QDateTime dateFrom = QDateTime::fromTime_t(0);
    QDateTime dateTo  = QDateTime::fromTime_t(0xFFFFFFFF);

public:
    ProposalFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setSortCaseSensitivity(Qt::CaseInsensitive);
    }

    void setDateRange(const QDateTime &from, const QDateTime &to)
    {
        this->dateFrom = from;
        this->dateTo = to;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const
    {
        auto model = sourceModel();
        auto proposal = model->index(row, ProposalTableModel::Proposal, parent);
        auto amount = model->index(row, ProposalTableModel::Amount, parent);

        if (filterRegExp().indexIn(model->data(proposal).toString()) < 0 &&
                filterRegExp().indexIn(model->data(amount).toString()) < 0) {
            return false;
        }
        QDateTime datetime = model->data(proposal, ProposalTableModel::ProposalDateFilterRole).toDateTime();
        if (datetime < dateFrom || datetime > dateTo)
            return false;
        return true;
    }
};


ProposalList::ProposalList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    clientModel(0),
    ui(new Ui::ProposalList)
{
    ui->setupUi(this);

    if (platformStyle->getUseExtraSpacing()) {
        ui->hlayout->setSpacing(5);
        ui->hlayout->addSpacing(26);
    } else {
        ui->hlayout->setSpacing(0);
        ui->hlayout->addSpacing(23);
    }

    if (platformStyle->getUseExtraSpacing()) {
        ui->dateWidget->setFixedWidth(141);
    } else {
        ui->dateWidget->setFixedWidth(140);
    }
    ui->dateWidget->addItem(tr("All"), All);
    ui->dateWidget->addItem(tr("Ends today"), Today);
    ui->dateWidget->addItem(tr("Ends this week"), ThisWeek);
    ui->dateWidget->addItem(tr("Ends this month"), ThisMonth);
    ui->dateWidget->addItem(tr("Ends this year"), ThisYear);
    ui->dateWidget->addItem(tr("End range..."), Range);

    ui->vlayout->addWidget(createDateRangeWidget());

    QAction *voteFundingAction = new QAction(tr("Vote funding"), this);
    QAction *voteDeleteAction = new QAction(tr("Vote delete"), this);
    QAction *voteEndorseAction = new QAction(tr("Vote endorse"), this);
    QAction *openUrlAction = new QAction(tr("Visit proposal website"), this);

    contextMenu = new QMenu(this);
    contextMenu->addAction(voteFundingAction);
    contextMenu->addAction(voteDeleteAction);
    contextMenu->addAction(voteEndorseAction);
    contextMenu->addSeparator();
    contextMenu->addAction(openUrlAction);

    connect(ui->voteFundingButton, &QPushButton::clicked, this, &ProposalList::voteFunding);
    connect(ui->voteDeleteButton, &QPushButton::clicked, this, &ProposalList::voteDelete);
    connect(ui->voteEndorseButton, &QPushButton::clicked, this, &ProposalList::voteEndorse);

    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &ProposalList::contextualMenu);
    connect(ui->tableView, &QTableView::doubleClicked, this, &ProposalList::openProposalUrl);

    connect(voteFundingAction, &QAction::triggered, this, &ProposalList::voteFunding);
    connect(voteDeleteAction, &QAction::triggered, this, &ProposalList::voteDelete);

    connect(openUrlAction, &QAction::triggered, this, &ProposalList::openProposalUrl);
}

ProposalList::~ProposalList()
{
    delete ui;
}

void ProposalList::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
    if(!_clientModel)
        return;

    proposalProxyModel = new ProposalFilterProxyModel(this);
    proposalProxyModel->setSourceModel(_clientModel->getProposalTableModel());

    connect(ui->searchLineEdit, &QLineEdit::textChanged, proposalProxyModel, &QSortFilterProxyModel::setFilterWildcard);
    connect(ui->dateWidget, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ProposalList::chooseDate);

    ui->tableView->setModel(proposalProxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::Amount, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::StartDate, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::EndDate, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::YesVotes, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::NoVotes, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::AbsoluteYesVotes, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::Funding, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::Percentage, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ProposalTableModel::Proposal, QHeaderView::Stretch);
}

void ProposalList::chooseDate(int idx)
{
    if(!proposalProxyModel) return;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(ui->dateWidget->itemData(idx).toInt())
    {
    case All:
        proposalProxyModel->setDateRange(
                QDateTime::fromTime_t(0),
                QDateTime::fromTime_t(0xFFFFFFFF));
        break;
    case Today:
        proposalProxyModel->setDateRange(
                QDateTime(current),
                QDateTime(current.addDays(1)));
        break;
    case ThisWeek: {
        // Find next Monday
        QDate startOfWeek = current.addDays(7-(current.dayOfWeek()));
        proposalProxyModel->setDateRange(
                QDateTime(current),
                QDateTime(startOfWeek));
        } break;
    case ThisMonth:
        proposalProxyModel->setDateRange(
                QDateTime(current),
                QDateTime(QDate(current.year(), current.month() + 1, 1)));
        break;
    case ThisYear:
        proposalProxyModel->setDateRange(
                QDateTime(current),
                QDateTime(QDate(current.year() + 1, 1, 1)));
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
}


void ProposalList::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    if(index.isValid())
        contextMenu->exec(QCursor::pos());
}

void ProposalList::voteFunding()
{
    vote_click_handler("funding");
}

void ProposalList::voteDelete()
{
    vote_click_handler("delete");
}

void ProposalList::voteEndorse()
{
    vote_click_handler("endorsed");
}

void ProposalList::vote_click_handler(const std::string voteString)
{
    if (!ui->tableView->selectionModel())
        return;

    if (!clientModel)
        return;

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if(selection.empty())
        return;

    QString proposalName = selection.at(0).data(ProposalTableModel::Proposal).toString();

    QMessageBox msgBox;
    msgBox.setText(tr("Your vote <strong>%1</strong> on the selected proposal is ready to be sent. How would you like to vote?").arg(QString::fromStdString(voteString)));
    msgBox.setWindowTitle(tr("Confirm vote"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.addButton(tr("CANCEL"), QMessageBox::RejectRole);
    QAbstractButton* pButtonNo = msgBox.addButton(tr("No"), QMessageBox::YesRole);
    QAbstractButton* pButtonAbs = msgBox.addButton(tr("Abstain"), QMessageBox::YesRole);
    QAbstractButton* pButtonYes = msgBox.addButton(tr("Yes"), QMessageBox::YesRole);

    msgBox.setEscapeButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);

    msgBox.exec();

    std::pair<std::string, std::string> strVote;
    if (msgBox.clickedButton()==pButtonYes) strVote = std::make_pair(voteString, "yes");
    else if (msgBox.clickedButton()==pButtonAbs) strVote = std::make_pair(voteString, "abstain");
    else if (msgBox.clickedButton()==pButtonNo) strVote = std::make_pair(voteString, "no");
    else return;

    uint256 hash;
    hash.SetHex(selection.at(0).data(ProposalTableModel::ProposalHashRole).toString().toStdString());

    std::pair<int,int> nResult = std::make_pair(0, 0);

    if (!clientModel->node().sendVoting(hash, strVote, nResult))
        QMessageBox::critical(this, tr("No Client Connection"),
            tr("There was an error connecting to the network. Please try again."));
    else
        QMessageBox::information(this, tr("Voting"),
            tr("You voted %1 %2 %3 time(s) successfully and failed %4 time(s).").arg(QString::fromStdString(strVote.first), QString::fromStdString(strVote.second), QString::number(nResult.second), QString::number(nResult.first)));
}

void ProposalList::openProposalUrl()
{
    if(!ui->tableView || !ui->tableView->selectionModel())
        return;

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
         QDesktopServices::openUrl(selection.at(0).data(ProposalTableModel::ProposalUrlRole).toString());
}

QWidget *ProposalList::createDateRangeWidget()
{
    dateRangeWidget = new QFrame();
    dateRangeWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    dateRangeWidget->setContentsMargins(1,1,1,1);
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));

    dateFrom = new QDateTimeEdit(this);
    dateFrom->setDisplayFormat("dd/MM/yy");
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    dateFrom->setDate(QDate::currentDate().addDays(-7));
    layout->addWidget(dateFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateTo = new QDateTimeEdit(this);
    dateTo->setDisplayFormat("dd/MM/yy");
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    dateTo->setDate(QDate::currentDate());
    layout->addWidget(dateTo);
    layout->addStretch();

    // Hide by default
    dateRangeWidget->setVisible(false);

    // Notify on change
    connect(dateFrom, &QDateTimeEdit::dateChanged, this, &ProposalList::dateRangeChanged);
    connect(dateTo, &QDateTimeEdit::dateChanged, this, &ProposalList::dateRangeChanged);

    return dateRangeWidget;
}

void ProposalList::dateRangeChanged()
{
    if(!proposalProxyModel)
        return;
    proposalProxyModel->setDateRange(
            QDateTime(dateFrom->date()),
            QDateTime(dateTo->date()).addDays(1));
}


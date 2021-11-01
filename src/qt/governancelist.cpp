#include <qt/forms/ui_governancelist.h>
#include <qt/governancelist.h>

#include <chainparams.h>
#include <clientversion.h>
#include <coins.h>
#include <evo/deterministicmns.h>
#include <netbase.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>

#include <univalue.h>

#include <QAbstractItemView>
#include <QDesktopServices>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QUrl>
#include <QtGui/QClipboard>

///
/// Proposal wrapper
///

Proposal::Proposal(const CGovernanceObject* p, QObject* parent) :
    QObject(parent),
    pGovObj(p)
{
    UniValue prop_data;
    if (prop_data.read(pGovObj->GetDataAsPlainString())) {
        if (UniValue titleValue = find_value(prop_data, "name"); titleValue.isStr()) {
            m_title = QString::fromStdString(titleValue.get_str());
        }

        if (UniValue paymentStartValue = find_value(prop_data, "start_epoch"); paymentStartValue.isNum()) {
            m_startDate = QDateTime::fromSecsSinceEpoch(paymentStartValue.get_int64());
        }

        if (UniValue paymentEndValue = find_value(prop_data, "end_epoch"); paymentEndValue.isNum()) {
            m_endDate = QDateTime::fromSecsSinceEpoch(paymentEndValue.get_int64());
        }

        if (UniValue amountValue = find_value(prop_data, "payment_amount"); amountValue.isNum()) {
            m_paymentAmount = amountValue.get_real();
        }

        if (UniValue urlValue = find_value(prop_data, "url"); urlValue.isStr()) {
            m_url = QString::fromStdString(urlValue.get_str());
        }
    }
}

QString Proposal::title() const { return m_title; }

QString Proposal::hash() const { return QString::fromStdString(pGovObj->GetHash().ToString()); }

QDateTime Proposal::startDate() const { return m_startDate; }

QDateTime Proposal::endDate() const { return m_endDate; }

float Proposal::paymentAmount() const { return m_paymentAmount; }

QString Proposal::url() const { return m_url; }

bool Proposal::isActive() const
{
    std::string strError;
    LOCK(cs_main);
    return pGovObj->IsValidLocally(strError, false);
}

QString Proposal::votingStatus(const int nAbsVoteReq) const
{
    // Voting status...
    // TODO: determine if voting is in progress vs. funded or not funded for past proposals.
    // see CSuperblock::GetNearestSuperblocksHeights(nBlockHeight, nLastSuperblock, nNextSuperblock);
    const int absYesCount = pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING);
    QString qStatusString;
    if (absYesCount >= nAbsVoteReq) {
        // Could use pGovObj->IsSetCachedFunding here, but need nAbsVoteReq to display numbers anyway.
        return tr("Passing +%1").arg(absYesCount - nAbsVoteReq);
    } else {
        return tr("Needs additional %1 votes").arg(nAbsVoteReq - absYesCount);
    }
}

int Proposal::GetAbsoluteYesCount() const
{
    return pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING);
}

void Proposal::openUrl() const
{
    QDesktopServices::openUrl(QUrl(m_url));
}

QString Proposal::toJson() const
{
    const auto json = pGovObj->ToJson();
    return QString::fromStdString(json.write(2));
}

///
/// Proposal Model
///


int ProposalModel::rowCount(const QModelIndex& index) const
{
    return m_data.count();
}

int ProposalModel::columnCount(const QModelIndex& index) const
{
    return Column::_COUNT;
}

QVariant ProposalModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole) return {};
    const auto proposal = m_data[index.row()];
    switch(role) {
    case Qt::DisplayRole:
    {
        switch (index.column()) {
        case Column::HASH:
            return proposal->hash();
        case Column::TITLE:
            return proposal->title();
        case Column::START_DATE:
            return proposal->startDate().date();
        case Column::END_DATE:
            return proposal->endDate().date();
        case Column::PAYMENT_AMOUNT:
            return proposal->paymentAmount();
        case Column::IS_ACTIVE:
            return proposal->isActive() ? "Y" : "N";
        case Column::VOTING_STATUS:
            return proposal->votingStatus(nAbsVoteReq);
        default:
            return {};
        };
        break;
    }
    case Qt::EditRole:
    {
        // Edit role is used for sorting, so return the raw values where possible
        switch (index.column()) {
        case Column::HASH:
            return proposal->hash();
        case Column::TITLE:
            return proposal->title();
        case Column::START_DATE:
            return proposal->startDate();
        case Column::END_DATE:
            return proposal->endDate();
        case Column::PAYMENT_AMOUNT:
            return proposal->paymentAmount();
        case Column::IS_ACTIVE:
            return proposal->isActive();
        case Column::VOTING_STATUS:
            return proposal->GetAbsoluteYesCount();
        default:
            return {};
        };
        break;
    }
    };
    return {};
}

QVariant ProposalModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case Column::HASH:
        return "Hash";
    case Column::TITLE:
        return "Title";
    case Column::START_DATE:
        return "Start";
    case Column::END_DATE:
        return "End";
    case Column::PAYMENT_AMOUNT:
        return "Amount";
    case Column::IS_ACTIVE:
        return "Active";
    case Column::VOTING_STATUS:
        return "Status";
    default:
        return {};
    }
}

int ProposalModel::columnWidth(int section)
{
    switch (section) {
    case Column::HASH:
        return 80;
    case Column::TITLE:
        return 220;
    case Column::START_DATE:
    case Column::END_DATE:
    case Column::PAYMENT_AMOUNT:
        return 110;
    case Column::IS_ACTIVE:
        return 80;
    case Column::VOTING_STATUS:
        return 220;
    default:
        return 80;
    }
}

void ProposalModel::append(const Proposal* proposal)
{
    beginInsertRows({}, m_data.count(), m_data.count());
    m_data.append(proposal);
    endInsertRows();
}

void ProposalModel::remove(int row)
{
    beginRemoveRows({}, row, row);
    m_data.removeAt(row);
    endRemoveRows();
}

void ProposalModel::reconcile(const std::vector<const Proposal*>& proposals)
{
    // Vector of m_data.count() false values. Going through new proposals,
    // set keep_index true for each old proposal found in the new proposals.
    // After going through new proposals, remove any existing proposals that
    // weren't found (and are still false).
    std::vector<bool> keep_index(m_data.count(), false);
    for (const auto proposal : proposals) {
        bool found = false;
        for (unsigned int i = 0; i < m_data.count(); ++i) {
            if (m_data.at(i)->hash() == proposal->hash()) {
                found = true;
                keep_index.at(i) = true;
                break;
            }
        }
        if (!found) {
            append(proposal);
        }
    }
    for (unsigned int i = keep_index.size(); i > 0; --i) {
        if (!keep_index.at(i - 1)) {
            remove(i - 1);
        }
    }
}


void ProposalModel::setVotingParams(int newAbsVoteReq)
{
    if (this->nAbsVoteReq != newAbsVoteReq) {
        this->nAbsVoteReq = newAbsVoteReq;
        // Changing either of the voting params may change the voting status
        // column. Emit signal to force recalculation.
        Q_EMIT dataChanged(createIndex(0, Column::VOTING_STATUS), createIndex(columnCount(), Column::VOTING_STATUS));
    }
}

const Proposal* ProposalModel::getProposalAt(const QModelIndex& index) const
{
    return m_data[index.row()];
}

//
// Governance Tab main widget.
//

GovernanceList::GovernanceList(QWidget* parent) :
    QWidget(parent),
    ui(std::make_unique<Ui::GovernanceList>()),
    proposalModel(new ProposalModel(this)),
    proposalModelProxy(new QSortFilterProxyModel(this)),
    proposalContextMenu(new QMenu(this)),
    timer(new QTimer(this))
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_count_2, ui->countLabel}, GUIUtil::FontWeight::Bold, 14);
    GUIUtil::setFont({ui->label_filter_2}, GUIUtil::FontWeight::Normal, 15);

    proposalModelProxy->setSourceModel(proposalModel);
    ui->govTableView->setModel(proposalModelProxy);
    ui->govTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->govTableView->horizontalHeader()->setStretchLastSection(true);

    for (int i = 0; i < proposalModel->columnCount(); ++i) {
        ui->govTableView->setColumnWidth(i, proposalModel->columnWidth(i));
    }

    // Set up sorting.
    proposalModelProxy->setSortRole(Qt::EditRole);
    ui->govTableView->setSortingEnabled(true);
    ui->govTableView->sortByColumn(ProposalModel::Column::START_DATE, Qt::DescendingOrder);

    // Set up filtering.
    proposalModelProxy->setFilterKeyColumn(ProposalModel::Column::TITLE); // filter by title column...
    ui->filterLineEdit->setPlaceholderText(tr("Filter by Title"));
    connect(ui->filterLineEdit, &QLineEdit::textChanged, proposalModelProxy, &QSortFilterProxyModel::setFilterFixedString);

    // Changes to number of rows should update proposal count display.
    connect(proposalModelProxy, &QSortFilterProxyModel::rowsInserted, this, &GovernanceList::updateProposalCount);
    connect(proposalModelProxy, &QSortFilterProxyModel::rowsRemoved, this, &GovernanceList::updateProposalCount);
    connect(proposalModelProxy, &QSortFilterProxyModel::layoutChanged, this, &GovernanceList::updateProposalCount);

    // Enable CustomContextMenu on the table to make the view emit customContextMenuRequested signal.
    ui->govTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->govTableView, &QTableView::customContextMenuRequested, this, &GovernanceList::showProposalContextMenu);
    connect(ui->govTableView, &QTableView::doubleClicked, this, &GovernanceList::showAdditionalInfo);

    connect(timer, &QTimer::timeout, this, &GovernanceList::updateProposalList);

    GUIUtil::updateFonts();
}

GovernanceList::~GovernanceList() = default;

void GovernanceList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    updateProposalList();
}

void GovernanceList::updateProposalList()
{
    if (this->clientModel) {
        // A proposal is considered passing if (YES votes - NO votes) >= (Total Number of Masternodes / 10),
        // count total valid (ENABLED) masternodes to determine passing threshold.
        // Need to query number of masternodes here with access to clientModel.
        const int nMnCount = clientModel->getMasternodeList().GetValidMNsCount();
        const int nAbsVoteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, nMnCount / 10);
        proposalModel->setVotingParams(nAbsVoteReq);

        const std::vector<const CGovernanceObject*> govObjList = clientModel->getAllGovernanceObjects();
        std::vector<const Proposal*> newProposals;
        for (const auto pGovObj : govObjList) {
            if (pGovObj->GetObjectType() != GOVERNANCE_OBJECT_PROPOSAL) {
                continue; // Skip triggers.
            }

            newProposals.emplace_back(new Proposal(pGovObj, proposalModel));
        }
        proposalModel->reconcile(newProposals);
    }

    // Schedule next update.
    timer->start(GOVERNANCELIST_UPDATE_SECONDS * 1000);
}

void GovernanceList::updateProposalCount() const
{
    ui->countLabel->setText(QString::number(proposalModelProxy->rowCount()));
}

void GovernanceList::showProposalContextMenu(const QPoint& pos)
{
    const auto index = ui->govTableView->indexAt(pos);

    if (!index.isValid()) {
        return;
    }

    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(index));
    if (proposal == nullptr) {
        return;
    }

    // right click menu with option to open proposal url
    QAction* openProposalUrl = new QAction(proposal->url(), this);
    proposalContextMenu->clear();
    proposalContextMenu->addAction(openProposalUrl);
    connect(openProposalUrl, &QAction::triggered, proposal, &Proposal::openUrl);
    proposalContextMenu->exec(QCursor::pos());
}

void GovernanceList::showAdditionalInfo(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(index));
    if (proposal == nullptr) {
        return;
    }

    const auto windowTitle = tr("Proposal Info: %1").arg(proposal->title());
    const auto json = proposal->toJson();

    QMessageBox::information(this, windowTitle, json);
}

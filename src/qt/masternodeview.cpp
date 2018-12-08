// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodeview.h>

#include <qt/forms/ui_masternodelist.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/masternodetablemodel.h>
#include <qt/platformstyle.h>
#include <qt/qrdialog.h>
#include <qt/walletmodel.h>

#include <interfaces/node.h>

#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>

class MasternodeSortFilterProxyModel final : public QSortFilterProxyModel
{
    const QString m_type;

public:
    MasternodeSortFilterProxyModel(const QString& type, QObject* parent)
        : QSortFilterProxyModel(parent)
        , m_type(type)
    {
        setDynamicSortFilter(true);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setSortCaseSensitivity(Qt::CaseInsensitive);
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const
    {
        auto model = sourceModel();
        auto alias = model->index(row, MasternodeTableModel::Alias, parent);

        if (model->data(alias, MasternodeTableModel::TypeRole).toString() != m_type) {
            return false;
        }

        auto address = model->index(row, MasternodeTableModel::Address, parent);
        auto daemon = model->index(row, MasternodeTableModel::Daemon, parent);
        auto status = model->index(row, MasternodeTableModel::Status, parent);
        auto banscore = model->index(row, MasternodeTableModel::Score, parent);
        auto activeSeconds = model->index(row, MasternodeTableModel::Active, parent);
        auto lastSeen = model->index(row, MasternodeTableModel::Last_Seen, parent);
        auto pubKey = model->index(row, MasternodeTableModel::Payee, parent);

        if (filterRegExp().indexIn(model->data(alias).toString()) < 0 &&
                filterRegExp().indexIn(model->data(address).toString()) < 0 &&
                filterRegExp().indexIn(model->data(daemon).toString()) < 0 &&
                filterRegExp().indexIn(model->data(status).toString()) < 0 &&
                filterRegExp().indexIn(model->data(banscore).toString()) < 0 &&
                filterRegExp().indexIn(model->data(activeSeconds).toString()) < 0 &&
                filterRegExp().indexIn(model->data(lastSeen).toString()) < 0 &&
                filterRegExp().indexIn(model->data(pubKey).toString()) < 0) {
            return false;
        }

        return true;
    }
};

MasternodeList::MasternodeList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    clientModel(0),
    walletModel(0),
    ui(new Ui::MasternodeList),
    tab(MasternodeList::MyNodesTab)
{
    ui->setupUi(this);

    this->tab = static_cast<Tabs>(ui->tabWidget->currentIndex());

    switch(tab)
    {
    case MyNodesTab:
        ui->startButton->show();
        ui->startAllButton->show();
        ui->startMissingButton->show();
        break;
    case AllNodesTab:
        ui->startButton->hide();
        ui->startAllButton->hide();
        ui->startMissingButton->hide();
        break;
    }


    // Context menu actions
    startAliasAction = new QAction(tr("Start alias"), this);
    QAction *showQRAction = new QAction(tr("Show QR"), this);

    // Build context menu
    contextMenu = new QMenu(this);
    contextMenu->addAction(startAliasAction);
    contextMenu->addAction(showQRAction);
    contextMenu->addSeparator();

    // Connect signals for context menu actions
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &MasternodeList::contextualMenu);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MasternodeList::selectionChanged);
    connect(ui->tableView, &QTableView::doubleClicked, this, &MasternodeList::on_QRButton_clicked);
    connect(startAliasAction, &QAction::triggered, this, &MasternodeList::on_startButton_clicked);
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::setClientModel(ClientModel *_clientmodel)
{
    this->clientModel = _clientmodel;
    if(!_clientmodel)
        return;

    proxyModelMyNodes = new MasternodeSortFilterProxyModel(MasternodeTableModel::MyNodes, this);
    proxyModelAllNodes = new MasternodeSortFilterProxyModel(MasternodeTableModel::AllNodes, this);
    proxyModelMyNodes->setSourceModel(_clientmodel->getMasternodeTableModel());
    proxyModelAllNodes->setSourceModel(_clientmodel->getMasternodeTableModel());

    connect(ui->searchLineEdit, &QLineEdit::textChanged, proxyModelMyNodes, &QSortFilterProxyModel::setFilterWildcard);

    connect(ui->searchLineEdit, &QLineEdit::textChanged, proxyModelAllNodes, &QSortFilterProxyModel::setFilterWildcard);
    connect(clientModel, &ClientModel::strMasternodesChanged, this, &MasternodeList::UpdateCounter);

    ui->tableView->setModel(proxyModelMyNodes);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
    ui->tableView->horizontalHeader()->setSectionResizeMode(MasternodeTableModel::Alias, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(MasternodeTableModel::Address, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(MasternodeTableModel::Daemon, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(MasternodeTableModel::Status, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(MasternodeTableModel::Score, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(MasternodeTableModel::Active, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(MasternodeTableModel::Last_Seen, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(MasternodeTableModel::Payee, QHeaderView::Stretch);

    selectionChanged();
    UpdateCounter(clientModel->getMasternodeCountString());
}

void MasternodeList::setWalletModel(WalletModel *_walletmodel)
{
    this->walletModel = _walletmodel;
}

void MasternodeList::UpdateCounter(QString count)
{
    this->ui->countLabel->setText(count);
}

void MasternodeList::StartAlias(std::string strAlias)
{
    if (!clientModel)
        return;

    std::string strStatusHtml;
    strStatusHtml += "<center>Alias: " + strAlias;
    std::string strError;

    if (!clientModel->node().startMasternodeAlias(strAlias, strError)) {
        strStatusHtml += "<br>Failed to start masternode.<br>Error: " + strError;
    } else {
        strStatusHtml += "<br>Successfully started masternode.";
    }

    strStatusHtml += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(strStatusHtml));
    msg.exec();
}

void MasternodeList::StartAll(std::string strCommand)
{
    if (!clientModel)
        return;

    std::string strError = strprintf("No connection to node");
    int nCountSuccessful = 0;
    int nCountFailed = 0;

    if (!clientModel->node().startAllMasternodes(strCommand, strError, nCountSuccessful, nCountFailed)) {
        QMessageBox msg;
        msg.setText(QString::fromStdString(strError));
        msg.exec();
        return;
    }

    std::string returnObj;
    returnObj = strprintf("Successfully started %d masternodes, failed to start %d, total %d", nCountSuccessful, nCountFailed, nCountFailed + nCountSuccessful);
    if (nCountFailed > 0) {
        returnObj += strError;
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();
}

void MasternodeList::on_startButton_clicked()
{
    if(!clientModel)
        return;

    if(!ui->tableView->selectionModel())
        return;

    QString strAlias;
    {
        LOCK(cs_mnlist);
        // Find selected node alias
        QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows(0);

        if(selected.count() == 0) return;

        strAlias = selected.at(0).data(0).toString();
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm masternode start"),
        tr("Are you sure you want to start masternode %1?").arg(strAlias),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAlias(strAlias.toStdString());
        return;
    }

    StartAlias(strAlias.toStdString());
}

void MasternodeList::on_startAllButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all masternodes start"),
        tr("Are you sure you want to start ALL masternodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll();
        return;
    }

    StartAll();
}

void MasternodeList::on_startMissingButton_clicked()
{
    if (!clientModel)
        return;

    if (!clientModel->node().isMasternodelistSynced()) {
        QMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until masternode list is synced"));
        return;
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this,
        tr("Confirm missing masternodes start"),
        tr("Are you sure you want to start MISSING masternodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll("start-missing");
        return;
    }

    StartAll("start-missing");
}

void MasternodeList::selectionChanged()
{
    // Set button states based on selected tab
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    this->tab = static_cast<Tabs>(ui->tabWidget->currentIndex());

    switch(tab)
    {
    case MyNodesTab:
        this->ui->tableView->setModel(proxyModelMyNodes);
        startAliasAction->setEnabled(true);
        this->ui->startButton->show();
        this->ui->startAllButton->show();
        this->ui->startMissingButton->show();
        this->ui->tableView->setColumnHidden(MasternodeTableModel::Alias, false);
        break;
    case AllNodesTab:
        this->ui->tableView->setModel(proxyModelAllNodes);
        startAliasAction->setEnabled(false);
        this->ui->startButton->hide();
        this->ui->startAllButton->hide();
        this->ui->startMissingButton->hide();
        this->ui->tableView->setColumnHidden(MasternodeTableModel::Alias, true);
        break;
    }
}

void MasternodeList::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void MasternodeList::on_QRButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows(0);

    if(selected.count() == 0) return;

    if (tab == MyNodesTab) {
        auto hash = proxyModelMyNodes->data(selected.at(0), MasternodeTableModel::TxHashRole).toString();
        auto index = proxyModelMyNodes->data(selected.at(0), MasternodeTableModel::TxOutIndexRole).toString();
        ShowQRCode(COutPoint(uint256S(hash.toStdString()), atoi(index.toStdString().c_str())));
    } else if (tab == AllNodesTab){
        auto hash = proxyModelAllNodes->data(selected.at(0), MasternodeTableModel::TxHashRole).toString();
        auto index = proxyModelAllNodes->data(selected.at(0), MasternodeTableModel::TxOutIndexRole).toString();
        ShowQRCode(COutPoint(uint256S(hash.toStdString()), atoi(index.toStdString().c_str())));
    } else return;
}

static std::string FormatVersion(int nVersion)
{
    if (nVersion % 100 == 0)
        return strprintf("%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100);
    else
        return strprintf("%d.%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100, nVersion % 100);
}

void MasternodeList::ShowQRCode(const COutPoint& _outpoint) {

    if(!walletModel || !walletModel->getOptionsModel())
        return;

    if (!clientModel)
        return;

    interfaces::Masternode toShow = clientModel->node().getMasternode(_outpoint);

    std::string strAlias = toShow.alias;
    std::string strMNPrivKey = _("<private keys are only available for self-owned masternodes>");
    if (strAlias != "")
        strMNPrivKey = clientModel->node().getMasternodeKey(strAlias);
    std::string strCollateral = toShow.outpoint.ToStringShort();
    std::string strIP = toShow.address;

        // Title of popup window
    QString strWindowtitle = tr("Additional information for Masternode ") + QString::fromStdString(strAlias);

    // Title above QR-Code
    QString strQRCodeTitle = tr("Masternode Private Key");

    // Create dialog text as HTML
    QString strHTML = "<html><font face='verdana, arial, helvetica, sans-serif'>";
    strHTML += "<b>" + tr("Alias") +            ": </b>" + (strAlias == "" ? tr("Not my node") : GUIUtil::HtmlEscape(strAlias)) + "<br>";
    strHTML += "<b>" + tr("Private Key") +      ": </b>" + GUIUtil::HtmlEscape(strMNPrivKey) + "<br>";
    strHTML += "<b>" + tr("Collateral") +       ": </b>" + GUIUtil::HtmlEscape(strCollateral) + "<br>";
    strHTML += "<b>" + tr("IP") +               ": </b>" + GUIUtil::HtmlEscape(strIP) + "<br>";
    strHTML += "<b>" + tr("Protocol") +         ": </b>" + QString::number(toShow.protocol) + "<br>";
    strHTML += "<b>" + tr("Version") +          ": </b>" + (toShow.daemon > 0 ? GUIUtil::HtmlEscape(FormatVersion(toShow.daemon)) : tr("unknown")) + "<br>";
    strHTML += "<b>" + tr("Sentinel") +         ": </b>" + (toShow.sentinel > 0 ? GUIUtil::HtmlEscape(SafeIntVersionToString(toShow.sentinel)) : tr("unknown")) + "<br>";
    strHTML += "<b>" + tr("Status") +           ": </b>" + GUIUtil::HtmlEscape(toShow.status) + "<br>";
    strHTML += "<b>" + tr("Payee") +            ": </b>" + GUIUtil::HtmlEscape(toShow.payee) + "<br>";
    strHTML += "<b>" + tr("Active") +           ": </b>" + GUIUtil::HtmlEscape(DurationToDHMS(toShow.active)) + "<br>";
    strHTML += "<b>" + tr("Last Seen") +        ": </b>" + (toShow.last_seen > 0 ? GUIUtil::HtmlEscape(QDateTime::fromTime_t(toShow.last_seen).toString(Qt::SystemLocaleLongDate)) : tr("unknown")) + "<br>";
    strHTML += "<b>" + tr("Last Paid") +        ": </b>" + (toShow.lastpaid > 0 ? GUIUtil::HtmlEscape(QDateTime::fromTime_t(toShow.lastpaid).toString(Qt::SystemLocaleLongDate)) : tr("unknown")) + "<br>";
    strHTML += "<b>" + tr("Ban Score") +        ": </b>" + QString::number(toShow.banscore) + "<br>";

    // Open QR dialog
    QRDialog *dialog = new QRDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModel(walletModel->getOptionsModel());
    dialog->setInfo(strWindowtitle, QString::fromStdString(strMNPrivKey), strHTML, strQRCodeTitle);
    dialog->show();
}

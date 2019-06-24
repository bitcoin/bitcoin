#include "masternodelist.h"
#include "ui_masternodelist.h"

#include "activemasternode.h"
#include "clientmodel.h"
#include "clientversion.h"
#include "guiutil.h"
#include "init.h"
#include "masternode-sync.h"
#include "netbase.h"
#include "sync.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <univalue.h>

#include <QMessageBox>
#include <QTimer>
#include <QtGui/QClipboard>

int GetOffsetFromUtc()
{
#if QT_VERSION < 0x050200
    const QDateTime dateTime1 = QDateTime::currentDateTime();
    const QDateTime dateTime2 = QDateTime(dateTime1.date(), dateTime1.time(), Qt::UTC);
    return dateTime1.secsTo(dateTime2);
#else
    return QDateTime::currentDateTime().offsetFromUtc();
#endif
}

MasternodeList::MasternodeList(const PlatformStyle* platformStyle, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    clientModel(0),
    walletModel(0),
    fFilterUpdatedDIP3(true),
    nTimeFilterUpdatedDIP3(0),
    nTimeUpdatedDIP3(0),
    mnListChanged(true)
{
    ui->setupUi(this);

    int columnAddressWidth = 200;
    int columnStatusWidth = 80;
    int columnPoSeScoreWidth = 80;
    int columnRegisteredWidth = 80;
    int columnLastPaidWidth = 80;
    int columnNextPaymentWidth = 100;
    int columnPayeeWidth = 130;
    int columnOperatorRewardWidth = 130;

    ui->tableWidgetMasternodesDIP3->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(1, columnStatusWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(2, columnPoSeScoreWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(3, columnRegisteredWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(4, columnLastPaidWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(5, columnNextPaymentWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(6, columnPayeeWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(7, columnOperatorRewardWidth);

    // dummy column for proTxHash
    // TODO use a proper table model for the MN list
    ui->tableWidgetMasternodesDIP3->insertColumn(8);
    ui->tableWidgetMasternodesDIP3->setColumnHidden(8, true);

    ui->tableWidgetMasternodesDIP3->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction* copyProTxHashAction = new QAction(tr("Copy ProTx Hash"), this);
    QAction* copyCollateralOutpointAction = new QAction(tr("Copy Collateral Outpoint"), this);
    contextMenuDIP3 = new QMenu();
    contextMenuDIP3->addAction(copyProTxHashAction);
    contextMenuDIP3->addAction(copyCollateralOutpointAction);
    connect(ui->tableWidgetMasternodesDIP3, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuDIP3(const QPoint&)));
    connect(ui->tableWidgetMasternodesDIP3, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(extraInfoDIP3_clicked()));
    connect(copyProTxHashAction, SIGNAL(triggered()), this, SLOT(copyProTxHash_clicked()));
    connect(copyCollateralOutpointAction, SIGNAL(triggered()), this, SLOT(copyCollateralOutpoint_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateDIP3ListScheduled()));
    timer->start(1000);
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // try to update list when masternode count changes
        connect(clientModel, SIGNAL(masternodeListChanged()), this, SLOT(handleMasternodeListChanged()));
    }
}

void MasternodeList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
}

void MasternodeList::showContextMenuDIP3(const QPoint& point)
{
    QTableWidgetItem* item = ui->tableWidgetMasternodesDIP3->itemAt(point);
    if (item) contextMenuDIP3->exec(QCursor::pos());
}

void MasternodeList::handleMasternodeListChanged()
{
    LOCK(cs_dip3list);
    mnListChanged = true;
}

void MasternodeList::updateDIP3ListScheduled()
{
    TRY_LOCK(cs_dip3list, fLockAcquired);
    if (!fLockAcquired) return;

    if (!clientModel || ShutdownRequested()) {
        return;
    }

    // To prevent high cpu usage update only once in MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds
    // after filter was last changed unless we want to force the update.
    if (fFilterUpdatedDIP3) {
        int64_t nSecondsToWait = nTimeFilterUpdatedDIP3 - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS;
        ui->countLabelDIP3->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            fFilterUpdatedDIP3 = false;
        }
    } else if (mnListChanged) {
        int64_t nSecondsToWait = nTimeUpdatedDIP3 - GetTime() + MASTERNODELIST_UPDATE_SECONDS;

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            mnListChanged = false;
        }
    }
}

void MasternodeList::updateDIP3List()
{
    if (!clientModel || ShutdownRequested()) {
        return;
    }

    LOCK(cs_dip3list);

    QString strToFilter;
    ui->countLabelDIP3->setText("Updating...");
    ui->tableWidgetMasternodesDIP3->setSortingEnabled(false);
    ui->tableWidgetMasternodesDIP3->clearContents();
    ui->tableWidgetMasternodesDIP3->setRowCount(0);

    auto mnList = clientModel->getMasternodeList();
    nTimeUpdatedDIP3 = GetTime();

    auto projectedPayees = mnList.GetProjectedMNPayees(mnList.GetValidMNsCount());
    std::map<uint256, int> nextPayments;
    for (size_t i = 0; i < projectedPayees.size(); i++) {
        const auto& dmn = projectedPayees[i];
        nextPayments.emplace(dmn->proTxHash, mnList.GetHeight() + (int)i + 1);
    }

    std::set<COutPoint> setOutpts;
    if (walletModel && ui->checkBoxMyMasternodesOnly->isChecked()) {
        std::vector<COutPoint> vOutpts;
        walletModel->listProTxCoins(vOutpts);
        for (const auto& outpt : vOutpts) {
            setOutpts.emplace(outpt);
        }
    }

    mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
        if (walletModel && ui->checkBoxMyMasternodesOnly->isChecked()) {
            bool fMyMasternode = setOutpts.count(dmn->collateralOutpoint) ||
                walletModel->havePrivKey(dmn->pdmnState->keyIDOwner) ||
                walletModel->havePrivKey(dmn->pdmnState->keyIDVoting) ||
                walletModel->havePrivKey(dmn->pdmnState->scriptPayout) ||
                walletModel->havePrivKey(dmn->pdmnState->scriptOperatorPayout);
            if (!fMyMasternode) return;
        }
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem* addressItem = new QTableWidgetItem(QString::fromStdString(dmn->pdmnState->addr.ToString()));
        QTableWidgetItem* statusItem = new QTableWidgetItem(mnList.IsMNValid(dmn) ? tr("ENABLED") : (mnList.IsMNPoSeBanned(dmn) ? tr("POSE_BANNED") : tr("UNKNOWN")));
        QTableWidgetItem* PoSeScoreItem = new QTableWidgetItem(QString::number(dmn->pdmnState->nPoSePenalty));
        QTableWidgetItem* registeredItem = new QTableWidgetItem(QString::number(dmn->pdmnState->nRegisteredHeight));
        QTableWidgetItem* lastPaidItem = new QTableWidgetItem(QString::number(dmn->pdmnState->nLastPaidHeight));
        QTableWidgetItem* nextPaymentItem = new QTableWidgetItem(nextPayments.count(dmn->proTxHash) ? QString::number(nextPayments[dmn->proTxHash]) : tr("UNKNOWN"));

        CTxDestination payeeDest;
        QString payeeStr;
        if (ExtractDestination(dmn->pdmnState->scriptPayout, payeeDest)) {
            payeeStr = QString::fromStdString(CBitcoinAddress(payeeDest).ToString());
        } else {
            payeeStr = tr("UNKNOWN");
        }
        QTableWidgetItem* payeeItem = new QTableWidgetItem(payeeStr);

        QString operatorRewardStr;
        if (dmn->nOperatorReward) {
            operatorRewardStr += QString::number(dmn->nOperatorReward / 100.0, 'f', 2) + "% ";

            if (dmn->pdmnState->scriptOperatorPayout != CScript()) {
                CTxDestination operatorDest;
                if (ExtractDestination(dmn->pdmnState->scriptOperatorPayout, operatorDest)) {
                    operatorRewardStr += tr("to %1").arg(QString::fromStdString(CBitcoinAddress(operatorDest).ToString()));
                } else {
                    operatorRewardStr += tr("to UNKNOWN");
                }
            } else {
                operatorRewardStr += tr("but not claimed");
            }
        } else {
            operatorRewardStr = tr("NONE");
        }
        QTableWidgetItem* operatorRewardItem = new QTableWidgetItem(operatorRewardStr);
        QTableWidgetItem* proTxHashItem = new QTableWidgetItem(QString::fromStdString(dmn->proTxHash.ToString()));

        if (strCurrentFilterDIP3 != "") {
            strToFilter = addressItem->text() + " " +
                          statusItem->text() + " " +
                          PoSeScoreItem->text() + " " +
                          registeredItem->text() + " " +
                          lastPaidItem->text() + " " +
                          nextPaymentItem->text() + " " +
                          payeeItem->text() + " " +
                          operatorRewardItem->text() + " " +
                          proTxHashItem->text();
            if (!strToFilter.contains(strCurrentFilterDIP3)) return;
        }

        ui->tableWidgetMasternodesDIP3->insertRow(0);
        ui->tableWidgetMasternodesDIP3->setItem(0, 0, addressItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 1, statusItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 2, PoSeScoreItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 3, registeredItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 4, lastPaidItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 5, nextPaymentItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 6, payeeItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 7, operatorRewardItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 8, proTxHashItem);
    });

    ui->countLabelDIP3->setText(QString::number(ui->tableWidgetMasternodesDIP3->rowCount()));
    ui->tableWidgetMasternodesDIP3->setSortingEnabled(true);
}

void MasternodeList::on_filterLineEditDIP3_textChanged(const QString& strFilterIn)
{
    strCurrentFilterDIP3 = strFilterIn;
    nTimeFilterUpdatedDIP3 = GetTime();
    fFilterUpdatedDIP3 = true;
    ui->countLabelDIP3->setText(QString::fromStdString(strprintf("Please wait... %d", MASTERNODELIST_FILTER_COOLDOWN_SECONDS)));
}

void MasternodeList::on_checkBoxMyMasternodesOnly_stateChanged(int state)
{
    // no cooldown
    nTimeFilterUpdatedDIP3 = GetTime() - MASTERNODELIST_FILTER_COOLDOWN_SECONDS;
    fFilterUpdatedDIP3 = true;
}

CDeterministicMNCPtr MasternodeList::GetSelectedDIP3MN()
{
    if (!clientModel) {
        return nullptr;
    }

    std::string strProTxHash;
    {
        LOCK(cs_dip3list);

        QItemSelectionModel* selectionModel = ui->tableWidgetMasternodesDIP3->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if (selected.count() == 0) return nullptr;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        strProTxHash = ui->tableWidgetMasternodesDIP3->item(nSelectedRow, 8)->text().toStdString();
    }

    uint256 proTxHash;
    proTxHash.SetHex(strProTxHash);

    auto mnList = clientModel->getMasternodeList();
    return mnList.GetMN(proTxHash);
}

void MasternodeList::extraInfoDIP3_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    UniValue json(UniValue::VOBJ);
    dmn->ToJson(json);

    // Title of popup window
    QString strWindowtitle = tr("Additional information for DIP3 Masternode %1").arg(QString::fromStdString(dmn->proTxHash.ToString()));
    QString strText = QString::fromStdString(json.write(2));

    QMessageBox::information(this, strWindowtitle, strText);
}

void MasternodeList::copyProTxHash_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    QApplication::clipboard()->setText(QString::fromStdString(dmn->proTxHash.ToString()));
}

void MasternodeList::copyCollateralOutpoint_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    QApplication::clipboard()->setText(QString::fromStdString(dmn->collateralOutpoint.ToStringShort()));
}

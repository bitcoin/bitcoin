// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/informationwidget.h>
#include <qt/forms/ui_informationwidget.h>

#include <chainparams.h>
#include <interfaces/node.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/guiutil_font.h>

#include <QDateTime>

namespace {
constexpr QChar nonbreaking_hyphen(8209);
} // anonymous namespace

InformationWidget::InformationWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::InformationWidget)
{
    ui->setupUi(this);
    GUIUtil::setFont({ui->label_9,
                      ui->label_10,
                      ui->labelMempoolTitle,
                      ui->labelNetwork},
                     {GUIUtil::FontWeight::Bold, 16});

    for (auto* element : {ui->label_10, ui->labelNetwork, ui->labelMempoolTitle}) {
        element->setContentsMargins(0, 10, 0, 0);
    }
}

InformationWidget::~InformationWidget()
{
    delete ui;
}

void InformationWidget::setClientModel(ClientModel* model)
{
    clientModel = model;

    if (clientModel) {
        // Keep up to date with client
        setNumConnections(model->getNumConnections());
        connect(model, &ClientModel::numConnectionsChanged, this, &InformationWidget::setNumConnections);

        updateNetworkState();
        connect(model, &ClientModel::networkActiveChanged, this, &InformationWidget::setNetworkActive);

        connect(model, &ClientModel::mempoolSizeChanged, this, &InformationWidget::setMempoolSize);

        // Provide initial values
        ui->blocksDir->setText(model->blocksDir());
        ui->blocksDir->setToolTip(ui->blocksDir->toolTip().arg(QString(nonbreaking_hyphen) + "blocksdir"));

        ui->dataDir->setText(model->dataDir());
        ui->dataDir->setToolTip(ui->dataDir->toolTip().arg(QString(nonbreaking_hyphen) + "datadir"));

        ui->clientUserAgent->setText(model->formatSubVersion());
        ui->clientVersion->setText(model->formatFullVersion());
        ui->networkName->setText(QString::fromStdString(Params().NetworkIDString()));
        ui->startupTime->setText(model->formatClientStartupTime());
    }
}

void InformationWidget::updateNetworkState()
{
    if (!clientModel) return;
    QString connections = QString::number(clientModel->getNumConnections()) + " (";
    connections += tr("In:") + " " + QString::number(clientModel->getNumConnections(CONNECTIONS_IN)) + " / ";
    connections += tr("Out:") + " " + QString::number(clientModel->getNumConnections(CONNECTIONS_OUT)) + ")";

    if(!clientModel->node().getNetworkActive()) {
        connections += " (" + tr("Network activity disabled") + ")";
    }

    ui->numberOfConnections->setText(connections);

    QString local_addresses;
    std::map<CNetAddr, LocalServiceInfo> hosts = clientModel->getNetLocalAddresses();
    for (const auto& [addr, info] : hosts) {
        local_addresses += QString::fromStdString(addr.ToStringAddr());
        if (!addr.IsI2P()) local_addresses += ":" + QString::number(info.nPort);
        local_addresses += ", ";
    }
    local_addresses.chop(2); // remove last ", "
    if (local_addresses.isEmpty()) local_addresses = tr("None");

    ui->localAddresses->setText(local_addresses);
}

void InformationWidget::setNumConnections(int count)
{
    if (!clientModel)
        return;

    updateNetworkState();
}

void InformationWidget::setNetworkActive(bool networkActive)
{
    updateNetworkState();
}

void InformationWidget::setNumBlocks(int count, const QDateTime& blockDate, const QString& blockHash, double nVerificationProgress, bool headers)
{
    if (!headers) {
        ui->numberOfBlocks->setText(QString::number(count));
        ui->lastBlockTime->setText(blockDate.toString());
        ui->lastBlockHash->setText(blockHash);
    }
}

void InformationWidget::setMempoolSize(long numberOfTxs, size_t dynUsage, size_t maxUsage)
{
    ui->mempoolNumberTxs->setText(QString::number(numberOfTxs));

    const auto cur_usage_str = dynUsage < 1000000 ?
        QObject::tr("%1 kB").arg(dynUsage / 1000.0, 0, 'f', 2) :
        QObject::tr("%1 MB").arg(dynUsage / 1000000.0, 0, 'f', 2);
    const auto max_usage_str = QObject::tr("%1 MB").arg(maxUsage / 1000000.0, 0, 'f', 2);

    ui->mempoolSize->setText(cur_usage_str + " / " + max_usage_str);
}

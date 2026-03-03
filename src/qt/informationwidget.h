// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_INFORMATIONWIDGET_H
#define BITCOIN_QT_INFORMATIONWIDGET_H

#include <QWidget>

class ClientModel;
namespace Ui {
class InformationWidget;
} // namespace Ui

class InformationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InformationWidget(QWidget* parent = nullptr);
    ~InformationWidget() override;

    void setClientModel(ClientModel* model);

public Q_SLOTS:
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set network state shown in the UI */
    void setNetworkActive(bool networkActive);
    /** Set number of blocks, last block date and last block hash shown in the UI */
    void setNumBlocks(int count, const QDateTime& blockDate, const QString& blockHash, double nVerificationProgress, bool headers);
    /** Set size (number of transactions and memory usage) of the mempool in the UI */
    void setMempoolSize(long numberOfTxs, size_t dynUsage, size_t maxUsage);

private:
    /** Update UI with latest network info from model. */
    void updateNetworkState();

private:
    Ui::InformationWidget* ui;
    ClientModel* clientModel{nullptr};
};

#endif // BITCOIN_QT_INFORMATIONWIDGET_H

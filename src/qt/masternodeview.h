// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODEVIEW_H
#define BITCOIN_QT_MASTERNODEVIEW_H

#include <qt/clientmodel.h>
#include <qt/masternodetablemodel.h>

#include <util/system.h>

#include <QWidget>

class COutPoint;
class MasternodeSortFilterProxyModel;
class PlatformStyle;
class WalletModel;

namespace Ui {
    class MasternodeList;
}


QT_BEGIN_NAMESPACE
class QItemSelection;
class QMenu;
class QModelIndex;
QT_END_NAMESPACE

/** Widget that shows a list of sending or receiving addresses.
  */
class MasternodeList : public QWidget
{
    Q_OBJECT

public:
    enum Tabs {
        MyNodesTab = 0,
        AllNodesTab = 1
    };

    explicit MasternodeList(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MasternodeList();

    void setClientModel(ClientModel *_clientmodel);
    void setWalletModel(WalletModel *_walletmodel);
    void UpdateCounter(QString count);
    void ShowQRCode(const COutPoint& _outpoint);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");

private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    Ui::MasternodeList *ui;
    Tabs tab;
    MasternodeTableModel *masternodeTableModel;
    MasternodeSortFilterProxyModel *proxyModelMyNodes;
    MasternodeSortFilterProxyModel *proxyModelAllNodes;

    QMenu *contextMenu;

    QAction *startAliasAction; // to be able to explicitly disable it

    // Protects tableView
    CCriticalSection cs_mnlist;

private Q_SLOTS:
    /** QR code button clicked */
    void on_QRButton_clicked();
    /** Start button clicked */
    void on_startButton_clicked();
    /** Start All button clicked */
    void on_startAllButton_clicked();
    /** Start MISSING button clicked */
    void on_startMissingButton_clicked();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for address book entry */
    void contextualMenu(const QPoint &point);

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);
};

#endif // BITCOIN_QT_MASTERNODEVIEW_H

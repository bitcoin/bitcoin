#ifndef MASTERNODELIST_H
#define MASTERNODELIST_H

#include "platformstyle.h"
#include "primitives/transaction.h"
#include "sync.h"
#include "util.h"

#include <evo/deterministicmns.h>

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define MY_MASTERNODELIST_UPDATE_SECONDS 60
#define MASTERNODELIST_UPDATE_SECONDS 15
#define MASTERNODELIST_FILTER_COOLDOWN_SECONDS 3

namespace Ui
{
class MasternodeList;
}

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Masternode Manager page widget */
class MasternodeList : public QWidget
{
    Q_OBJECT

public:
    explicit MasternodeList(const PlatformStyle* platformStyle, QWidget* parent = 0);
    ~MasternodeList();

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);
    void ShowQRCode(std::string strAlias);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");
    CDeterministicMNCPtr GetSelectedDIP3MN();

private:
    QMenu* contextMenu;
    QMenu* contextMenuDIP3;
    int64_t nTimeFilterUpdated;
    int64_t nTimeFilterUpdatedDIP3;
    bool fFilterUpdated;
    bool fFilterUpdatedDIP3;

public Q_SLOTS:
    void updateMyMasternodeInfo(QString strAlias, QString strAddr, const COutPoint& outpoint);
    void updateMyNodeList(bool fForce = false);
    void updateNodeList();
    void updateDIP3List();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

private:
    QTimer* timer;
    Ui::MasternodeList* ui;
    ClientModel* clientModel;
    WalletModel* walletModel;

    // Protects tableWidgetMasternodes
    CCriticalSection cs_mnlist;

    // Protects tableWidgetMyMasternodes
    CCriticalSection cs_mymnlist;

    // Protects tableWidgetMasternodesDIP3
    CCriticalSection cs_dip3list;

    QString strCurrentFilter;
    QString strCurrentFilterDIP3;

private Q_SLOTS:
    void showContextMenu(const QPoint&);
    void showContextMenuDIP3(const QPoint&);
    void on_filterLineEdit_textChanged(const QString& strFilterIn);
    void on_filterLineEditDIP3_textChanged(const QString& strFilterIn);
    void on_QRButton_clicked();
    void on_startButton_clicked();
    void on_startAllButton_clicked();
    void on_startMissingButton_clicked();
    void on_tableWidgetMyMasternodes_itemSelectionChanged();
    void on_UpdateButton_clicked();
    void on_checkBoxMyMasternodesOnly_stateChanged(int state);

    void extraInfoDIP3_clicked();
    void copyProTxHash_clicked();
    void copyCollateralOutpoint_clicked();
};
#endif // MASTERNODELIST_H

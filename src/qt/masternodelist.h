#ifndef MASTERNODELIST_H
#define MASTERNODELIST_H

#include "platformstyle.h"
#include "primitives/transaction.h"
#include "sync.h"
#include "util.h"

#include "evo/deterministicmns.h"

#include <QMenu>
#include <QTimer>
#include <QWidget>

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

private:
    QMenu* contextMenuDIP3;
    int64_t nTimeFilterUpdatedDIP3;
    bool fFilterUpdatedDIP3;

    QTimer* timer;
    Ui::MasternodeList* ui;
    ClientModel* clientModel;
    WalletModel* walletModel;

    // Protects tableWidgetMasternodesDIP3
    CCriticalSection cs_dip3list;

    QString strCurrentFilterDIP3;

    CDeterministicMNCPtr GetSelectedDIP3MN();

    void updateDIP3List(bool fForce);

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

private Q_SLOTS:
    void showContextMenuDIP3(const QPoint&);
    void on_filterLineEditDIP3_textChanged(const QString& strFilterIn);
    void on_checkBoxMyMasternodesOnly_stateChanged(int state);

    void extraInfoDIP3_clicked();
    void copyProTxHash_clicked();
    void copyCollateralOutpoint_clicked();

    void updateDIP3ListScheduled();
    void updateDIP3ListForced();
};
#endif // MASTERNODELIST_H

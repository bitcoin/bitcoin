#ifndef MASTERNODELIST_H
#define MASTERNODELIST_H

#include "masternode.h"
#include "sync.h"
#include "util.h"
#include "sendcollateraldialog.h"

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define MASTERNODELIST_UPDATE_SECONDS            60
#define MY_MASTERNODELIST_UPDATE_SECONDS         15
#define MASTERNODELIST_FILTER_COOLDOWN_SECONDS   3

namespace Ui {
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
    explicit MasternodeList(QWidget *parent = 0);
    ~MasternodeList();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");
    void VoteMany(std::string strCommand);
    void selectAliasRow(const QString& alias);

private:
    QMenu *contextMenu;
    int64_t nTimeFilterUpdate;
    bool fFilterUpdated = false;
public Q_SLOTS:
    void updateMyMasternodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CMasternode *pmn);
    void updateMyNodeList(bool reset = false);
    void updateNodeList();
    void updateVoteList(bool reset = false);
    void updateNextSuperblock();
    SendCollateralDialog* getSendCollateralDialog()
    {
        return sendDialog;
    }

Q_SIGNALS:

private:
    QTimer *timer;
    Ui::MasternodeList *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    SendCollateralDialog *sendDialog;
    CCriticalSection cs_mnlistupdate;
    QString strCurrentFilter;

private Q_SLOTS:
    void showContextMenu(const QPoint &);
    void on_filterLineEdit_textChanged(const QString &filterString);
    void on_startButton_clicked();
    void on_editButton_clicked();
    void on_startAllButton_clicked();
    void on_startMissingButton_clicked();
    void on_tableWidgetMyMasternodes_itemSelectionChanged();
    void on_UpdateButton_clicked();
    
    void on_voteManyYesButton_clicked();
    void on_voteManyNoButton_clicked();
    void on_voteManyAbstainButton_clicked();
    void on_tableWidgetVoting_itemSelectionChanged();
    void on_UpdateVotesButton_clicked();
    void on_CreateNewMasternode_clicked();
};
#endif // MASTERNODELIST_H

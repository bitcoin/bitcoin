#ifndef MASTERNODELIST_H
#define MASTERNODELIST_H

#include "masternode.h"
#include "platformstyle.h"
#include "sync.h"
#include "util.h"

#include <QWidget>
#include <QTimer>

#define MASTERNODELIST_UPDATE_SECONDS            5
#define MY_MASTERNODELIST_UPDATE_SECONDS        60

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
    explicit MasternodeList(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MasternodeList();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");

public Q_SLOTS:
    void updateMyMasternodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CMasternode *pmn);
    void updateMyNodeList(bool reset = false);
    void updateNodeList();

Q_SIGNALS:

private:
    QTimer *timer;
    Ui::MasternodeList *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CCriticalSection cs_mnlistupdate;
    QString strCurrentFilter;

private Q_SLOTS:
    void on_filterLineEdit_textChanged(const QString &filterString);
    void on_startButton_clicked();
    void on_startAllButton_clicked();
    void on_startMissingButton_clicked();
    void on_tableWidgetMyMasternodes_itemSelectionChanged();
    void on_UpdateButton_clicked();
};
#endif // MASTERNODELIST_H

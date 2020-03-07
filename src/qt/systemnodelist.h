#ifndef SYSTEMNODELIST_H
#define SYSTEMNODELIST_H

#include "systemnode.h"
#include "sync.h"
#include "util.h"
#include "sendcollateraldialog.h"

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define SYSTEMNODELIST_UPDATE_SECONDS            60
#define MY_SYSTEMNODELIST_UPDATE_SECONDS         15
#define SYSTEMNODELIST_FILTER_COOLDOWN_SECONDS   3

namespace Ui {
    class SystemnodeList;
}

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Systemnode Manager page widget */
class SystemnodeList : public QWidget
{
    Q_OBJECT

public:
    explicit SystemnodeList(QWidget *parent = 0);
    ~SystemnodeList();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");
    void selectAliasRow(const QString& alias);

private:
    QMenu *contextMenu;
    int64_t nTimeFilterUpdate;
    bool fFilterUpdated = false;
public Q_SLOTS:
    void updateMySystemnodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CSystemnode *pmn);
    void updateMyNodeList(bool reset = false);
    void updateNodeList();
    SendCollateralDialog* getSendCollateralDialog()
    {
        return sendDialog;
    }

Q_SIGNALS:

private:
    QTimer *timer;
    Ui::SystemnodeList *ui;
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
    void on_tableWidgetMySystemnodes_itemSelectionChanged();
    void on_UpdateButton_clicked();
    void on_CreateNewSystemnode_clicked();
    
};

#endif // SYSTEMNODELIST_H

#ifndef THRONELIST_H
#define THRONELIST_H

#include "throne.h"
#include "sync.h"
#include "util.h"

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define THRONELIST_UPDATE_SECONDS            5
#define MY_THRONELIST_UPDATE_SECONDS        60

namespace Ui {
    class ThroneList;
}

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Throne Manager page widget */
class ThroneList : public QWidget
{
    Q_OBJECT

public:
    explicit ThroneList(QWidget *parent = 0);
    ~ThroneList();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");
    void VoteMany();

private:
    QMenu *contextMenu;

public Q_SLOTS:
    void updateMyThroneInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CThrone *pmn);
    void updateMyNodeList(bool reset = false);
    void updateNodeList();
    void updateVoteList();

Q_SIGNALS:

private:
    QTimer *timer;
    Ui::ThroneList *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CCriticalSection cs_mnlistupdate;
    QString strCurrentFilter;

private Q_SLOTS:
    void showContextMenu(const QPoint &);
    void on_filterLineEdit_textChanged(const QString &filterString);
    void on_startButton_clicked();
    void on_startAllButton_clicked();
    void on_startMissingButton_clicked();
    void on_tableWidgetMyThrones_itemSelectionChanged();
    void on_UpdateButton_clicked();
    
    void on_voteManyButton_clicked();
    void on_UpdateVotesButton_clicked();
};
#endif // THRONELIST_H

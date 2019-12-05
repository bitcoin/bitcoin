#ifndef GOVERNANCELIST_H
#define GOVERNANCELIST_H

#include "primitives/transaction.h"
#include "platformstyle.h"
#include "sync.h"
#include "util/system.h"
#include <governance/governance.h>

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define GOBJECT_UPDATE_SECONDS                 15
#define GOBJECT_COOLDOWN_SECONDS               3

namespace Ui {
    class GovernanceList;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

std::string getValue(std::string, std::string, bool);
std::string getNumericValue(std::string str, std::string key);

/** Governance Manager page widget */
class GovernanceList : public QWidget
{
    Q_OBJECT

public:
    explicit GovernanceList(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~GovernanceList();
    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void ShowGovernanceObject(uint256 gobjectSingle);
    void Vote(uint256 nHash, vote_outcome_enum_t eVoteOutcome);


private:
    QMenu *contextMenu;
    int64_t nTimeFilterUpdated;
    bool fFilterUpdated;

public Q_SLOTS:
    void updateGobjects();


Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

private:
    QTimer *timer;
    Ui::GovernanceList *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

    // Protects tableWidgetMasternodes
    CCriticalSection cs_gobjlist;

    QString strCurrentFilter;

private Q_SLOTS:
    void showContextMenu(const QPoint &);
    void on_filterLineEdit_textChanged(const QString &strFilterIn);
    void on_GovernanceButton_clicked();
    void on_UpdateButton_clicked();
    void on_voteYesButton_clicked();
    void on_voteNoButton_clicked();
    void on_voteAbstainButton_clicked();

};
#endif // GOVERNANCELIST_H

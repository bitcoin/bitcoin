#ifndef THRONEMANAGER_H
#define THRONEMANAGER_H

#include "guiutil.h"

#include <QWidget>
#include <QTimer>

namespace Ui {
    class ThroneManager;
}
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Throne Manager page widget */
class ThroneManager : public QWidget
{
    Q_OBJECT

public:
    explicit ThroneManager(QWidget *parent = 0);
    ~ThroneManager();

    void setWalletModel(WalletModel *walletModel);


public slots:
    void updateNodeList();
    void updateThrone(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, QString status);
    void on_UpdateButton_clicked();

signals:

private:
    QTimer *timer;
    Ui::ThroneManager *ui;
    WalletModel *walletModel;

private slots:
    void on_addButton_clicked();
    void on_removeButton_clicked();
    void on_startButton_clicked();
    void on_startAllButton_clicked();
    void on_tableWidget_2_itemSelectionChanged();
};
#endif // THRONEMANAGER_H
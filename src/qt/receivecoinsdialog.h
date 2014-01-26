// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RECEIVECOINSDIALOG_H
#define RECEIVECOINSDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QPoint>
#include <QVariant>

namespace Ui {
    class ReceiveCoinsDialog;
}
class WalletModel;
class OptionsModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog for requesting payment of bitcoins */
class ReceiveCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiveCoinsDialog(QWidget *parent = 0);
    ~ReceiveCoinsDialog();

    void setModel(WalletModel *model);

public slots:
    void clear();
    void reject();
    void accept();

protected:
    virtual void keyPressEvent(QKeyEvent *event);

private:
    Ui::ReceiveCoinsDialog *ui;
    WalletModel *model;
    QMenu *contextMenu;
    void copyColumnToClipboard(int column);

private slots:
    void on_receiveButton_clicked();
    void on_showRequestButton_clicked();
    void on_removeRequestButton_clicked();
    void on_recentRequestsView_doubleClicked(const QModelIndex &index);
    void updateDisplayUnit();
    void showMenu(const QPoint &);
    void copyLabel();
    void copyMessage();
    void copyAmount();
};

#endif // RECEIVECOINSDIALOG_H

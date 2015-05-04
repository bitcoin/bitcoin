// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RECEIVECOINSDIALOG_H
#define BITCOIN_RECEIVECOINSDIALOG_H

#include <QDialog>
#include <QHeaderView>
#include <QItemSelection>
#include <QKeyEvent>
#include <QMenu>
#include <QPoint>
#include <QVariant>

#include "guiutil.h"

namespace Ui {
    class Bitcoin_ReceiveCoinsDialog;
}
class OptionsModel;
class Bitcoin_WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog for requesting payment of bitcoins */
class Bitcoin_ReceiveCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    enum ColumnWidths {
        DATE_COLUMN_WIDTH = 130,
        LABEL_COLUMN_WIDTH = 120,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 160,
        MINIMUM_COLUMN_WIDTH = 130
    };

    explicit Bitcoin_ReceiveCoinsDialog(QWidget *parent = 0);
    ~Bitcoin_ReceiveCoinsDialog();

    void setModel(Bitcoin_WalletModel *model);

public slots:
    void clear();
    void reject();
    void accept();

protected:
    virtual void keyPressEvent(QKeyEvent *event);

private:
    Ui::Bitcoin_ReceiveCoinsDialog *ui;
    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;
    Bitcoin_WalletModel *model;
    QMenu *contextMenu;
    void copyColumnToClipboard(int column);
    virtual void resizeEvent(QResizeEvent *event);

private slots:
    void on_receiveButton_clicked();
    void on_showRequestButton_clicked();
    void on_removeRequestButton_clicked();
    void on_recentRequestsView_doubleClicked(const QModelIndex &index);
    void recentRequestsView_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void updateDisplayUnit();
    void showMenu(const QPoint &point);
    void copyLabel();
    void copyMessage();
    void copyAmount();
};

#endif // BITCOIN_RECEIVECOINSDIALOG_H

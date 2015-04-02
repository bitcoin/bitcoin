// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BALANCESVIEW_H
#define BALANCESVIEW_H

#include "guiutil.h"

#include <QWidget>
#include <QDialog>
#include <QString>
#include <QTableWidget>
#include <QTextEdit>
#include <QDialogButtonBox>

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
/*class QComboBox;
class QFrame;
class QLineEdit;
class QMenu;
class QModelIndex;
class QSignalMapper;
class QTableView;*/
QT_END_NAMESPACE

namespace Ui {
    class balancesDialog;
}

class BalancesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BalancesDialog(QWidget *parent = 0);
    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);
    void AddRow(std::string label, std::string address, std::string reserved, std::string available);
    void PopulateBalances(unsigned int propertyId);
    void RefreshBalances();
    void UpdatePropSelector();

    QTableWidgetItem *labelCell;
    QTableWidgetItem *addressCell;
    QTableWidgetItem *reservedCell;
    QTableWidgetItem *availableCell;

private:
    Ui::balancesDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    QMenu *contextMenu;
    QMenu *contextMenuSummary;

    GUIUtil::TableViewLastColumnResizingFixer *borrowedColumnResizingFixer;
    virtual void resizeEvent(QResizeEvent* event);

private slots:
    void contextualMenu(const QPoint &);
    void balancesCopyCol0();
    void balancesCopyCol1();
    void balancesCopyCol2();
    void balancesCopyCol3();

signals:
    void doubleClicked(const QModelIndex&);

    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);

public slots:
    void propSelectorChanged();
    void balancesUpdated();
};

#endif // BALANCESVIEW_H

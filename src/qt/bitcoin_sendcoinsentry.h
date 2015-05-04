// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SENDCOINSENTRY_H
#define BITCOIN_SENDCOINSENTRY_H

#include "bitcoin_walletmodel.h"

#include <QStackedWidget>

class Bitcoin_WalletModel;

namespace Ui {
    class Bitcoin_SendCoinsEntry;
}

/**
 * A single entry in the dialog for sending bitcoins.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class Bitcoin_SendCoinsEntry : public QStackedWidget
{
    Q_OBJECT

public:
    explicit Bitcoin_SendCoinsEntry(QWidget *parent = 0);
    ~Bitcoin_SendCoinsEntry();

    void setModel(Bitcoin_WalletModel *model);
    bool validate();
    Bitcoin_SendCoinsRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const Bitcoin_SendCoinsRecipient &value);
    void setAddress(const QString &address);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases
     *  (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public slots:
    void clear();

signals:
    void removeEntry(Bitcoin_SendCoinsEntry *entry);
    void payAmountChanged();

private slots:
    void deleteClicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    Bitcoin_SendCoinsRecipient recipient;
    Ui::Bitcoin_SendCoinsEntry *ui;
    Bitcoin_WalletModel *model;

    bool updateLabel(const QString &address);
};

#endif // BITCOIN_SENDCOINSENTRY_H

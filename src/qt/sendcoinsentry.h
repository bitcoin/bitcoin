// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SENDCOINSENTRY_H
#define SENDCOINSENTRY_H

#include "walletmodel.h"

#include <QStackedWidget>

class Bitcredit_WalletModel;

namespace Ui {
    class Credits_SendCoinsEntry;
}

/**
 * A single entry in the dialog for sending bitcoins.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class Credits_SendCoinsEntry : public QStackedWidget
{
    Q_OBJECT

public:
    explicit Credits_SendCoinsEntry(QWidget *parent = 0);
    ~Credits_SendCoinsEntry();

    void setModel(Bitcredit_WalletModel *model);
    bool validate();
    Bitcredit_SendCoinsRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const Bitcredit_SendCoinsRecipient &value);
    void setAddress(const QString &address);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases
     *  (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public slots:
    void clear();

signals:
    void removeEntry(Credits_SendCoinsEntry *entry);
    void payAmountChanged();

private slots:
    void deleteClicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    Bitcredit_SendCoinsRecipient recipient;
    Ui::Credits_SendCoinsEntry *ui;
    Bitcredit_WalletModel *model;

    bool updateLabel(const QString &address);
};

#endif // SENDCOINSENTRY_H

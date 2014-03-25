// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UNEDITABLECOINSENTRY_H
#define UNEDITABLECOINSENTRY_H

#include "walletmodel.h"

#include <QStackedWidget>

class Bitcredit_WalletModel;

namespace Ui {
    class UneditableCoinsEntry;
}

/**
 * A single entry in the dialog for sending bitcoins.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class UneditableCoinsEntry : public QStackedWidget
{
    Q_OBJECT

public:
    explicit UneditableCoinsEntry(QWidget *parent = 0);
    ~UneditableCoinsEntry();

    void setModel(Bitcredit_WalletModel *model);
    void setValue(qint64 value);
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
    void removeEntry(UneditableCoinsEntry *entry);
    void payAmountChanged();

private slots:
    void deleteClicked();
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    Bitcredit_SendCoinsRecipient recipient;
    Ui::UneditableCoinsEntry *ui;
    Bitcredit_WalletModel *model;
};

#endif // UNEDITABLECOINSENTRY_H

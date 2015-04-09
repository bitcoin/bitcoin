// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METADEXDIALOG_H
#define METADEXDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>
#include <boost/multiprecision/cpp_dec_float.hpp>

using boost::multiprecision::cpp_dec_float_100;

class OptionsModel;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class MetaDExDialog;
}

/** Dialog for looking up Master Protocol tokens */
class MetaDExDialog : public QDialog
{
    Q_OBJECT

public:
    void FullRefresh();
    void SwitchMarket();
    void UpdateSellAddressBalance();
    void UpdateBuyAddressBalance();
    void UpdateSellOffers();
    void UpdateBuyOffers();
    explicit MetaDExDialog(QWidget *parent = 0);
    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);


public slots:
    void switchButtonClicked();
    void sellAddressComboBoxChanged(int idx);
    void buyAddressComboBoxChanged(int idx);
    void sellClicked(int row, int col);
    void buyClicked(int row, int col);
    void sendTrade(bool sell);
    void OrderRefresh();

private:
    Ui::MetaDExDialog *ui;
    WalletModel *model;

private slots:
    void buyRecalc();
    void sellRecalc();
    void buyTrade();
    void sellTrade();

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // METADEXDIALOG_H

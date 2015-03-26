// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOOKUPSPDIALOG_H
#define LOOKUPSPDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>

class OptionsModel;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class LookupSPDialog;
}

/** Dialog for looking up Master Protocol tokens */
class LookupSPDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LookupSPDialog(QWidget *parent = 0);
    void searchSP();
    void updateDisplayedProperty();
    void addSPToMatchingResults(unsigned int);
    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);


public slots:
    void searchButtonClicked();
    void matchingComboBoxChanged(int);

private:
    Ui::LookupSPDialog *ui;
    WalletModel *model;

//private slots:

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // LOOKUPSPDIALOG_H

// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOOKUPSPDIALOG_H
#define LOOKUPSPDIALOG_H

#include <QDialog>

class WalletModel;

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
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
    ~LookupSPDialog();

    void searchSP();
    void updateDisplayedProperty();
    void addSPToMatchingResults(unsigned int propertyId);

public Q_SLOTS:
    void searchButtonClicked();
    void matchingComboBoxChanged(int idx);

private:
    Ui::LookupSPDialog *ui;
    WalletModel *model;

private Q_SLOTS:
    // None

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // LOOKUPSPDIALOG_H

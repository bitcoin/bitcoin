// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_QT_CREATEASSETDIALOG_H
#define RAVEN_QT_CREATEASSETDIALOG_H

#include <QDialog>

class PlatformStyle;
class WalletModel;

namespace Ui {
    class CreateAssetDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class CreateAssetDialog : public QDialog
{
Q_OBJECT

public:
    explicit CreateAssetDialog(const PlatformStyle *platformStyle, QWidget *parent = 0, WalletModel *model = NULL);
    ~CreateAssetDialog();

private:
    Ui::CreateAssetDialog *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;

    bool checkedAvailablity = false;


    void toggleIPFSText();
    void setUpValues();
    void showMessage(QString string);
    void showValidMessage(QString string);
    void hideMessage();
    void disableCreateButton();
    void enableCreateButton();
    void CheckFormState();

private Q_SLOTS:
    void ipfsStateChanged();
    void checkAvailabilityClicked();
    void onNameChanged(QString name);
    void onAddressNameChanged(QString address);
    void onIPFSHashChanged(QString hash);
    void onCloseClicked();
    void onCreateAssetClicked();
    void onUnitChanged(int value);
    void onCustomAddressClicked();
    void onChangeAddressChanged(QString changeAddress);
};

#endif // RAVEN_QT_CREATEASSETDIALOG_H

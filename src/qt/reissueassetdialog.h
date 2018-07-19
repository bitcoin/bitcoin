// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_QT_REISSUEASSETDIALOG_H
#define RAVEN_QT_REISSUEASSETDIALOG_H

#include <QDialog>

class PlatformStyle;
class WalletModel;
class CNewAsset;

namespace Ui {
    class ReissueAssetDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class ReissueAssetDialog : public QDialog
{
Q_OBJECT

public:
    explicit ReissueAssetDialog(const PlatformStyle *platformStyle, QWidget *parent = 0, WalletModel *model = NULL);
    ~ReissueAssetDialog();

private:
    Ui::ReissueAssetDialog *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;

    CNewAsset *asset;

    void toggleIPFSText();
    void setUpValues();
    void showMessage(QString string);
    void showValidMessage(QString string);
    void hideMessage();
    void disableReissueButton();
    void enableReissueButton();
    void CheckFormState();
    void disableAll();
    void enableDataEntry();
    void buildUpdatedData();

private Q_SLOTS:
    void onCloseClicked();
    void onAssetSelected(int index);
    void onQuantityChanged(double qty);
    void onIPFSStateChanged();
    void onIPFSHashChanged(QString hash);
    void onAddressNameChanged(QString address);
    void onReissueAssetClicked();
    void onChangeAddressChanged();
    void onChangeAddressTextChanged(QString address);
    void onReissueBoxChanged();
};

#endif // RAVEN_QT_REISSUEASSETDIALOG_H

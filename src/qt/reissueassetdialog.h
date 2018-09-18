// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_QT_REISSUEASSETDIALOG_H
#define RAVEN_QT_REISSUEASSETDIALOG_H

#include "walletmodel.h"

#include <QDialog>

class PlatformStyle;
class WalletModel;
class ClientModel;
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
    explicit ReissueAssetDialog(const PlatformStyle *platformStyle, QWidget *parent = 0, WalletModel *model = NULL, ClientModel *client = NULL);
    ~ReissueAssetDialog();

    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);

    QString formatGreen;
    QString formatBlack;

private:
    Ui::ReissueAssetDialog *ui;
    ClientModel *clientModel;
    WalletModel *model;
    const PlatformStyle *platformStyle;
    bool fFeeMinimized;

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
    void setDisplayedDataToNone();

    //CoinControl
    // Update the passed in CCoinControl with state from the GUI
    void updateCoinControlState(CCoinControl& ctrl);

    //Fee
    void updateFeeMinimizedLabel();
    void minimizeFeeSection(bool fMinimize);

private Q_SLOTS:
    void onCloseClicked();
    void onAssetSelected(int index);
    void onQuantityChanged(double qty);
    void onIPFSStateChanged();
    void onIPFSHashChanged(QString hash);
    void onAddressNameChanged(QString address);
    void onReissueAssetClicked();
    void onReissueBoxChanged();
    void onUnitChanged(int value);

    //CoinControl
    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();
    void coinControlUpdateLabels();

    //Fee
    void on_buttonChooseFee_clicked();
    void on_buttonMinimizeFee_clicked();
    void setMinimumFee();
    void updateFeeSectionControls();
    void updateMinFeeLabel();
    void updateSmartFeeLabel();

    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);
    void updateDisplayUnit();
};

#endif // RAVEN_QT_REISSUEASSETDIALOG_H

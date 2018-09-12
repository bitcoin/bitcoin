// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_QT_CREATEASSETDIALOG_H
#define RAVEN_QT_CREATEASSETDIALOG_H

#include "walletmodel.h"

#include <QDialog>

class PlatformStyle;
class WalletModel;
class ClientModel;

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
    explicit CreateAssetDialog(const PlatformStyle *platformStyle, QWidget *parent = 0, WalletModel *model = NULL, ClientModel *client = NULL);
    ~CreateAssetDialog();

    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);

    int type;
    QString format;

private:
    Ui::CreateAssetDialog *ui;
    ClientModel *clientModel;
    WalletModel *model;
    bool fFeeMinimized;
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
    void updatePresentedAssetName(QString name);
    QString GetSpecialCharacter();
    QString GetAssetName();
    void UpdateAssetNameMaxSize();
    void UpdateAssetNameToUpper();

    //CoinControl
    // Update the passed in CCoinControl with state from the GUI
    void updateCoinControlState(CCoinControl& ctrl);

    //Fee
    void updateFeeMinimizedLabel();
    void minimizeFeeSection(bool fMinimize);

private Q_SLOTS:
    void ipfsStateChanged();
    void checkAvailabilityClicked();
    void onNameChanged(QString name);
    void onAddressNameChanged(QString address);
    void onIPFSHashChanged(QString hash);
    void onCloseClicked();
    void onCreateAssetClicked();
    void onUnitChanged(int value);
    void onChangeAddressChanged(QString changeAddress);
    void onAssetTypeActivated(int index);
    void onAssetListActivated(int index);

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

#endif // RAVEN_QT_CREATEASSETDIALOG_H

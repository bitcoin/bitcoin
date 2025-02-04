// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_OPTIONSDIALOG_H
#define BITCOIN_QT_OPTIONSDIALOG_H

#include <QDialog>
#include <QValidator>

class BitcoinAmountField;
class ClientModel;
class OptionsModel;
class QValidatedLineEdit;

QT_BEGIN_NAMESPACE
class QBoxLayout;
class QCheckBox;
class QDataWidgetMapper;
class QDoubleSpinBox;
class QEvent;
class QLayout;
class QRadioButton;
class QSpinBox;
class QString;
class QValueComboBox;
class QWidget;
QT_END_NAMESPACE

namespace Ui {
class OptionsDialog;
}

/** Proxy address widget validator, checks for a valid proxy address.
 */
class ProxyAddressValidator : public QValidator
{
    Q_OBJECT

public:
    explicit ProxyAddressValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Preferences dialog. */
class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent, bool enableWallet);
    ~OptionsDialog();

    enum Tab {
        TAB_MAIN,
        TAB_NETWORK,
    };

    void setClientModel(ClientModel* client_model);
    void setModel(OptionsModel *model);
    void setMapper();
    void setCurrentTab(OptionsDialog::Tab tab);

private Q_SLOTS:
    void updateThemeColors();

    /* set OK button state (enabled / disabled) */
    void setOkButtonState(bool fState);
    void on_resetButton_clicked();
    void on_openBitcoinConfButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();

    void on_showTrayIcon_stateChanged(int state);

    void togglePruneWarning(bool enabled);
    void showRestartWarning(bool fPersistent = false);
    void clearStatusLabel();
    void updateProxyValidationState();
    /* query the networks, for which the default proxy is used */
    void updateDefaultProxyNets();
    void checkLineEdit();

    void incrementalrelayfee_changed();
    void blockmaxsize_changed(int);
    void blockmaxsize_increase(int);
    void blockmaxweight_changed(int);

Q_SIGNALS:
    void proxyIpChecks(QValidatedLineEdit *pUiProxyIp, uint16_t nProxyPort);
    void quitOnReset();

protected:
    void changeEvent(QEvent *e) override;

private:
    Ui::OptionsDialog *ui;
    ClientModel* m_client_model{nullptr};
    OptionsModel* model{nullptr};
    QDataWidgetMapper* mapper{nullptr};

    QWidget *prevwidget{nullptr};
    void FixTabOrder(QWidget *);
    void CreateOptionUI(QBoxLayout *, QWidget *, const QString& text, QLayout *horizontalLayout = nullptr);

    QCheckBox *walletrbf;

    QSpinBox *blockreconstructionextratxn;

    QValueComboBox *mempoolreplacement;
    QSpinBox *maxorphantx;
    BitcoinAmountField *incrementalrelayfee;
    QSpinBox *maxmempool;
    QSpinBox *mempoolexpiry;

    QCheckBox *rejectunknownscripts;
    QCheckBox *rejectspkreuse;
    BitcoinAmountField *minrelaytxfee;
    QSpinBox *bytespersigop, *bytespersigopstrict;
    QSpinBox *limitancestorcount;
    QSpinBox *limitancestorsize;
    QSpinBox *limitdescendantcount;
    QSpinBox *limitdescendantsize;
    QCheckBox *rejectbarepubkey;
    QCheckBox *rejectbaremultisig;
    QSpinBox *maxscriptsize;
    QSpinBox *datacarriersize;
    QDoubleSpinBox *datacarriercost;
    BitcoinAmountField *dustrelayfee;
    QCheckBox *dustdynamic_enable;
    QDoubleSpinBox *dustdynamic_multiplier;
    QRadioButton *dustdynamic_target;
    QSpinBox *dustdynamic_target_blocks;
    QRadioButton *dustdynamic_mempool;
    QSpinBox *dustdynamic_mempool_kvB;

    BitcoinAmountField *blockmintxfee;
    QSpinBox *blockmaxsize, *blockprioritysize, *blockmaxweight;
};

#endif // BITCOIN_QT_OPTIONSDIALOG_H

// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_OPTIONSDIALOG_H
#define BITCOIN_QT_OPTIONSDIALOG_H

#include <QDialog>
#include <QIntValidator>

class OptionsModel;
class QValidatedLineEdit;
class QLineEdit;
class QLabel;

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

namespace Ui
{
class OptionsDialog;
}

/** Ensures that one edit box is always less than another */
class LessThanValidator : public QIntValidator
{
    QLineEdit* other;
    QLabel* errorDisplay;

public:
    LessThanValidator(int minimum, int maximum, QObject* parent = 0) : QIntValidator(minimum, maximum, parent), other(NULL), errorDisplay(NULL)
    {
    }
    
    // This cannot be part of the constructor because these widgets may not be created at construction time.
    void initialize(QLineEdit* otherp, QLabel* errorDisplayp)
    {
        other = otherp;
        errorDisplay = errorDisplayp;
    }


    virtual State validate(QString& input, int& pos) const;
};

/** Preferences dialog. */
class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget* parent, bool enableWallet);
    ~OptionsDialog();

    void setModel(OptionsModel* model);
    void setMapper();

protected:
    bool eventFilter(QObject* object, QEvent* event);

private Q_SLOTS:
    /* enable OK button */
    void enableOkButton();
    /* disable OK button */
    void disableOkButton();
    /* set OK button state (enabled / disabled) */
    void setOkButtonState(bool fState);
    void on_resetButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();

    void showRestartWarning(bool fPersistent = false);
    void clearStatusLabel();
    void doProxyIpChecks(QValidatedLineEdit *pUiProxyIp, int nProxyPort);

Q_SIGNALS:
    void proxyIpChecks(QValidatedLineEdit *pUiProxyIp, int nProxyPort);

private:
    Ui::OptionsDialog *ui;
    OptionsModel *model;
    QDataWidgetMapper *mapper;
    bool fProxyIpValid;
};

#endif // BITCOIN_QT_OPTIONSDIALOG_H

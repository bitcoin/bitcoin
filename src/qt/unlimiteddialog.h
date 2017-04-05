// Copyright (c) 2011-2015 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_UNLIMITEDDIALOG_H
#define BITCOIN_QT_UNLIMITEDDIALOG_H

#include <QDialog>
#include <QIntValidator>
#include <QDataWidgetMapper>
class OptionsModel;
class QValidatedLineEdit;
class QLineEdit;
class QLabel;
class UnlimitedModel;

// Minimum time in ms to show notifications (used in BitcoinGUI not the Unlimited dialog)
#define NOTIFY_MIN_SHOW_TIME 1000

#include "ui_unlimited.h"
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

// Unlimited dialog
class UnlimitedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UnlimitedDialog(QWidget* parent,UnlimitedModel* model);
    virtual ~UnlimitedDialog();
    void setMapper();
    
private Q_SLOTS:
    void setOkButtonState(bool fState);
    void on_resetButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void validateBlockSize();
    void shapingSliderChanged();         // Pushes the traffic shaping slider changes into the traffic shaping edit boxes
    void shapingMaxEditFinished(void);   // auto-corrects cases where max is lower then average
    void shapingAveEditFinished(void);   // auto-corrects cases where max is lower then average
    void shapingEnableChanged(bool val); // Pushes the traffic shaping slider changes into the traffic shaping edit boxes
 
Q_SIGNALS:
   
private:
    Ui_UnlimitedDialog ui;
    QDataWidgetMapper mapper;
    UnlimitedModel* model;

    QIntValidator burstValidator;
    LessThanValidator sendAveValidator;
    LessThanValidator recvAveValidator;    
};

#endif

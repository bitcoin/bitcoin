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

#include "ui_unlimited.h"

// Unlimited dialog
class UnlimitedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UnlimitedDialog(QWidget* parent,UnlimitedModel* model);
    virtual ~UnlimitedDialog();
    void setMapper();
    
private Q_SLOTS:
    void on_okButton_clicked();
    void on_cancelButton_clicked();

Q_SIGNALS:
    
private:
    Ui_UnlimitedDialog ui;
    QDataWidgetMapper mapper;
    UnlimitedModel* model;
    
};

#endif

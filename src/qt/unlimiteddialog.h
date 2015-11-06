// Copyright (c) 2011-2015 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_UNLIMITEDDIALOG_H
#define BITCOIN_QT_UNLIMITEDDIALOG_H

#include <QDialog>
#include <QIntValidator>

class OptionsModel;
class QValidatedLineEdit;
class QLineEdit;
class QLabel;

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

#include "ui_unlimited.h"

// Unlimited dialog
class UnlimitedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UnlimitedDialog(QWidget* parent);
    virtual ~UnlimitedDialog();
    
private Q_SLOTS:

Q_SIGNALS:
    
private:
    Ui_UnlimitedDialog ui;
    
};

#endif

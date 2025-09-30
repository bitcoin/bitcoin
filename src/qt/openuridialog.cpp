// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/openuridialog.h>
#include <qt/forms/ui_openuridialog.h>

#include <qt/guiutil.h>
#include <qt/sendcoinsrecipient.h>

#include <QAbstractButton>
#include <QLineEdit>

OpenURIDialog::OpenURIDialog(QWidget* parent) : QDialog(parent, GUIUtil::dialog_flags),
                                                ui(new Ui::OpenURIDialog)
{
    ui->setupUi(this);
    GUIUtil::setIcon(ui->pasteButton, "editpaste");
    QObject::connect(ui->pasteButton, &QAbstractButton::clicked, ui->uriEdit, &QLineEdit::paste);

    GUIUtil::updateFonts();
    GUIUtil::disableMacFocusRect(this);
    GUIUtil::handleCloseWindowShortcut(this);
}

OpenURIDialog::~OpenURIDialog()
{
    delete ui;
}

QString OpenURIDialog::getURI()
{
    return ui->uriEdit->text();
}

void OpenURIDialog::accept()
{
    SendCoinsRecipient rcp;
    if (GUIUtil::parseBitcoinURI(getURI(), &rcp)) {
        /* Only accept value URIs */
        QDialog::accept();
    } else {
        ui->uriEdit->setValid(false);
    }
}

void OpenURIDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    if (e->type() == QEvent::StyleChange) {
        // Adjust button icon colors on theme changes
        GUIUtil::setIcon(ui->pasteButton, "editpaste");
    }
}

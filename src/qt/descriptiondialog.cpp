// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/forms/ui_descriptiondialog.h>

#include <qt/descriptiondialog.h>
#include <qt/guiutil_font.h>

#include <qt/guiutil.h>

DescriptionDialog::DescriptionDialog(const QString& title, const QString& html, QWidget* parent) :
    QDialog{parent, GUIUtil::dialog_flags},
    ui{new Ui::DescriptionDialog}
{
    ui->setupUi(this);
    setWindowTitle(title);
    GUIUtil::registerWidget(ui->detailText, html);
    GUIUtil::updateFonts();
    GUIUtil::handleCloseWindowShortcut(this);
}

DescriptionDialog::~DescriptionDialog()
{
    delete ui;
}

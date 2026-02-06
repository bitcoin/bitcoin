// Copyright (c) 2011-2020 The Bitcoin Core developers
// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_DESCRIPTIONDIALOG_H
#define BITCOIN_QT_DESCRIPTIONDIALOG_H

#include <QDialog>

namespace Ui {
class DescriptionDialog;
} // namespace Ui

/** Generic dialog showing detailed text description. */
class DescriptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DescriptionDialog(const QString& title, const QString& html, QWidget* parent = nullptr);
    ~DescriptionDialog();

private:
    Ui::DescriptionDialog* ui;
};

#endif // BITCOIN_QT_DESCRIPTIONDIALOG_H

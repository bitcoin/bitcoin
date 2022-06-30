// Copyright (c) 2011-2021 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_QT_OPENURIDIALOG_H
#define WIDECOIN_QT_OPENURIDIALOG_H

#include <QDialog>

class PlatformStyle;

namespace Ui {
    class OpenURIDialog;
}

class OpenURIDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenURIDialog(const PlatformStyle* platformStyle, QWidget* parent);
    ~OpenURIDialog();

    QString getURI();

protected Q_SLOTS:
    void accept() override;
    void changeEvent(QEvent* e) override;

private:
    Ui::OpenURIDialog* ui;

    const PlatformStyle* m_platform_style;
};

#endif // WIDECOIN_QT_OPENURIDIALOG_H

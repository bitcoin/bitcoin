// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EDITADDRESSDIALOG_H
#define EDITADDRESSDIALOG_H

#include <QDialog>

class Bitcredit_AddressTableModel;

namespace Ui {
    class Bitcredit_EditAddressDialog;
}

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

/** Dialog for editing an address and associated information.
 */
class Bitcredit_EditAddressDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewReceivingAddress,
        NewSendingAddress,
        EditReceivingAddress,
        EditSendingAddress
    };

    explicit Bitcredit_EditAddressDialog(Mode mode, QWidget *parent);
    ~Bitcredit_EditAddressDialog();

    void setModel(Bitcredit_AddressTableModel *model);
    void loadRow(int row);

    QString getAddress() const;
    void setAddress(const QString &address);

public slots:
    void accept();

private:
    bool saveCurrentRow();

    Ui::Bitcredit_EditAddressDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    Bitcredit_AddressTableModel *model;

    QString address;
};

#endif // EDITADDRESSDIALOG_H

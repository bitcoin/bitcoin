// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EDITADDRESSDIALOG_H
#define BITCOIN_EDITADDRESSDIALOG_H

#include <QDialog>

class Bitcoin_AddressTableModel;

namespace Ui {
    class Bitcoin_EditAddressDialog;
}

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

/** Dialog for editing an address and associated information.
 */
class Bitcoin_EditAddressDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewReceivingAddress,
        NewSendingAddress,
        EditReceivingAddress,
        EditSendingAddress
    };

    explicit Bitcoin_EditAddressDialog(Mode mode, QWidget *parent);
    ~Bitcoin_EditAddressDialog();

    void setModel(Bitcoin_AddressTableModel *model);
    void loadRow(int row);

    QString getAddress() const;
    void setAddress(const QString &address);

public slots:
    void accept();

private:
    bool saveCurrentRow();

    Ui::Bitcoin_EditAddressDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    Bitcoin_AddressTableModel *model;

    QString address;
};

#endif // BITCOIN_EDITADDRESSDIALOG_H

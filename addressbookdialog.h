#ifndef ADDRESSBOOKDIALOG_H
#define ADDRESSBOOKDIALOG_H

#include <QDialog>

namespace Ui {
    class AddressBookDialog;
}

class AddressBookDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddressBookDialog(QWidget *parent = 0);
    ~AddressBookDialog();

    enum {
        SendingTab = 0,
        ReceivingTab = 1
    } Tabs;

    void setTab(int tab);
private:
    Ui::AddressBookDialog *ui;
};

#endif // ADDRESSBOOKDIALOG_H

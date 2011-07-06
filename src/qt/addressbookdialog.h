#ifndef ADDRESSBOOKDIALOG_H
#define ADDRESSBOOKDIALOG_H

#include <QDialog>

namespace Ui {
    class AddressBookDialog;
}
class AddressTableModel;

QT_BEGIN_NAMESPACE
class QTableView;
QT_END_NAMESPACE

class AddressBookDialog : public QDialog
{
    Q_OBJECT

public:
    enum Tabs {
        SendingTab = 0,
        ReceivingTab = 1
    };

    enum Mode {
        ForSending, // Pick address for sending
        ForEditing  // Open address book for editing
    };

    explicit AddressBookDialog(Mode mode, QWidget *parent = 0);
    ~AddressBookDialog();

    void setModel(AddressTableModel *model);
    void setTab(int tab);
    const QString &getReturnValue() const { return returnValue; }
private:
    Ui::AddressBookDialog *ui;
    AddressTableModel *model;
    Mode mode;
    QString returnValue;

    QTableView *getCurrentTable();

private slots:
    void on_buttonBox_accepted();
    void on_deleteButton_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_newAddressButton_clicked();
    void on_copyToClipboard_clicked();
};

#endif // ADDRESSBOOKDIALOG_H

#ifndef EDITADDRESSDIALOG_H
#define EDITADDRESSDIALOG_H

#include <QDialog>

namespace Ui {
    class EditAddressDialog;
}

class EditAddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditAddressDialog(QWidget *parent = 0);
    ~EditAddressDialog();

private:
    Ui::EditAddressDialog *ui;
};

#endif // EDITADDRESSDIALOG_H

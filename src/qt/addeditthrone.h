#ifndef ADDEDITTHRONE_H
#define ADDEDITTHRONE_H

#include <QDialog>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem/fstream.hpp>

namespace Ui {
    class AddEditThrone;
}


class AddEditThrone : public QDialog
{
    Q_OBJECT

public:
    explicit AddEditThrone(QWidget *parent = 0);
    ~AddEditThrone();

protected:

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void on_AddEditAddressPasteButton_clicked();
    void on_AddEditPrivkeyPasteButton_clicked();
    void on_AddEditTxhashPasteButton_clicked();

signals:

private:
    Ui::AddEditThrone *ui;
};

#endif // ADDEDITTHRONE_H
#ifndef CREATESYSTEMNODEDIALOG_H
#define CREATESYSTEMNODEDIALOG_H

#include <QDialog>

namespace Ui {
class CreateSystemnodeDialog;
}

class CreateSystemnodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateSystemnodeDialog(QWidget *parent = 0);
    ~CreateSystemnodeDialog();

public:
    QString getAlias();
    QString getIP();

private:
    Ui::CreateSystemnodeDialog *ui;
};

#endif // CREATESYSTEMNODEDIALOG_H

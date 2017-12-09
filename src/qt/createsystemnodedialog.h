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
    QString getLabel();
    void setAlias(QString alias);
    void setIP(QString ip);
    void setNoteLabel(QString text);
    void done(int);
    void setEditMode();

private:
    bool editMode;
    QString startAlias;
    Ui::CreateSystemnodeDialog *ui;
};

#endif // CREATESYSTEMNODEDIALOG_H

#ifndef CREATENODEDIALOG_H
#define CREATENODEDIALOG_H

#include <QDialog>

namespace Ui {
class CreateNodeDialog;
}

class CreateNodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateNodeDialog(QWidget *parent = 0);
    ~CreateNodeDialog();

public:
    QString getAlias();
    QString getIP();
    QString getLabel();
    void setAlias(QString alias);
    void setIP(QString ip);
    void setNoteLabel(QString text);
    void setEditMode();

protected slots:
    void accept();

private:
    virtual bool aliasExists(QString alias) = 0;
    bool CheckAlias();
    bool CheckIP();

private:
    bool editMode;
    QString startAlias;
    Ui::CreateNodeDialog *ui;
};

#endif // CREATENODEDIALOG_H

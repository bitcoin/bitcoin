#ifndef CREATEMASTERNODEDIALOG_H
#define CREATEMASTERNODEDIALOG_H

#include <QDialog>

namespace Ui {
class CreateMasternodeDialog;
}

class CreateMasternodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateMasternodeDialog(QWidget *parent = 0);
    ~CreateMasternodeDialog();

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
    Ui::CreateMasternodeDialog *ui;
};

#endif // CREATEMASTERNODEDIALOG_H

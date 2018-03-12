#ifndef STARTMISSINGDIALOG_H
#define STARTMISSINGDIALOG_H

#include <QDialog>

namespace Ui {
class StartMissingDialog;
}

class StartMissingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartMissingDialog(QWidget *parent = 0);
    ~StartMissingDialog();

public:
    void setText(const QString& text);
    void setCheckboxText(const QString& text);
    void setWarningText(const QString& text);
    bool checkboxChecked();

private slots:
    void CheckBoxStateChanged(int state);

private:
    Ui::StartMissingDialog *ui;
};

#endif // STARTMISSINGDIALOG_H

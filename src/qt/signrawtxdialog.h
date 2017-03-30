#ifndef SIGNRAWTXDIALOG_H
#define SIGNRAWTXDIALOG_H

#include <QDialog>

namespace Ui {
    class SignRawTxDialog;
}

/** Dialog for editing an address and associated information.
 */
class SignRawTxDialog : public QDialog
{
    Q_OBJECT

public:

    explicit SignRawTxDialog(QWidget *parent = 0);
    ~SignRawTxDialog();
private:
	bool saveCurrentRow();
	Ui::SignRawTxDialog *ui;
	void setRawTxEdit();
	void setRawSysTxEdit();

public Q_SLOTS:
    void on_okButton_clicked();
	void on_cancelButton_clicked();
	void rawTxChanged();
};

#endif // SIGNRAWTXDIALOG_H

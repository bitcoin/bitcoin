#ifndef MANAGEESCROWDIALOG_H
#define MANAGEESCROWDIALOG_H

#include <QDialog>

namespace Ui {
    class ManageEscrowDialog;
}

/** Dialog for editing an address and associated information.
 */
class ManageEscrowDialog : public QDialog
{
    Q_OBJECT

public:
    enum EscrowType {
        Buyer,
        Seller,
		Arbiter,
		None
    };
    explicit ManageEscrowDialog(const QString &escrow, const QString &buyer, const QString &seller, const QString &arbiter, const QString &status, const QString &offertitle, const QString &total,QWidget *parent = 0);
    ~ManageEscrowDialog();

	bool isYourAlias(const QString &alias);
	ManageEscrowDialog::EscrowType findYourEscrowRoleFromAliases(const QString &buyer, const QString &seller, const QString &arbiter);

public Q_SLOTS:
	void on_releaseButton_clicked();
	void on_refundButton_clicked();
	void on_cancelButton_clicked();
private:
    Ui::ManageEscrowDialog *ui;
	QString escrow;
	QString buyer;
	QString seller;
	QString arbiter;
	QString status;
	QString offertitle;
	QString total;
};

#endif // MANAGEESCROWDIALOG_H

#ifndef OFFERFEEDBACKDIALOG_H
#define OFFERFEEDBACKDIALOG_H

#include <QDialog>
class WalletModel;
namespace Ui {
    class OfferFeedbackDialog;
}

/** Dialog for editing an address and associated information.
 */
class OfferFeedbackDialog : public QDialog
{
    Q_OBJECT

public:
    enum OfferType {
        Buyer,
        Seller,
		None
    };
    explicit OfferFeedbackDialog(WalletModel* model, const QString &offer, const QString &accept, QWidget *parent = 0);
    ~OfferFeedbackDialog();
public Q_SLOTS:
	void on_feedbackButton_clicked();
	void on_cancelButton_clicked();
private:
	QString offer;
	QString acceptGUID;
	bool isYourAlias(const QString &alias);
	bool lookup(const QString &offer, const QString &accept, QString &buyer,QString &seller,QString &offertitle,QString &currency,QString &total,QString &systotal);
	OfferFeedbackDialog::OfferType findYourOfferRoleFromAliases(const QString &buyer, const QString &seller);
	WalletModel* walletModel;
    Ui::OfferFeedbackDialog *ui;
	QString escrow;
};

#endif // OFFERFEEDBACKDIALOG_H

#ifndef OFFERACCEPTDIALOG_H
#define OFFERACCEPTDIALOG_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class OfferAcceptDialog;
}
class OfferAcceptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OfferAcceptDialog(const PlatformStyle *platformStyle, QString aliaspeg, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString strPrice, QString sellerAlias, QString address, QWidget *parent=0);
    ~OfferAcceptDialog();

    bool getPaymentStatus();

private:
	const PlatformStyle *platformStyle;
	void setupEscrowCheckboxState();
    Ui::OfferAcceptDialog *ui;
	QString quantity;
	QString alias;
	QString offer;
	QString notes;
	QString price;
	QString fprice;
	QString title;
	QString currency;
	QString seller;
	QString address;
	QString aliaspeg;
	bool offerPaid; 
	

private Q_SLOTS:
	void acceptPayment();
	void onEscrowCheckBoxChanged(bool);
	void on_cancelButton_clicked();
    void acceptOffer();
	void acceptEscrow();
	void acceptBTCPayment();
};

#endif // OFFERACCEPTDIALOG_H

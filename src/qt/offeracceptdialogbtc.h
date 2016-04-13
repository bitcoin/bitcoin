#ifndef OFFERACCEPTDIALOGBTC_H
#define OFFERACCEPTDIALOGBTC_H
#include "walletmodel.h"
#include <QDialog>
#include <QImage>
#include <QLabel>
#include "amount.h"
class PlatformStyle;
QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE
namespace Ui {
    class OfferAcceptDialogBTC;
}
class OfferAcceptDialogBTC : public QDialog
{
    Q_OBJECT

public:
    explicit OfferAcceptDialogBTC(const PlatformStyle *platformStyle, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString strPrice, QString sellerAlias, QString address, QWidget *parent=0);
    ~OfferAcceptDialogBTC();
	void CheckPaymentInBTC();
    bool getPaymentStatus();

private:
	const PlatformStyle *platformStyle;
    Ui::OfferAcceptDialogBTC *ui;
	SendCoinsRecipient info;
	QString quantity;
	QString notes;
	QString price;
	QString title;
	QString offer;
	QString acceptGuid;
	QString sellerAlias;
	QString address;
	QString alias;
	QString m_buttonText;
	double dblPrice;
	bool offerPaid; 
	

private Q_SLOTS:
	void on_cancelButton_clicked();
    void tryAcceptOffer();
	void acceptOffer();
	void openBTCWallet();
	void slotConfirmedFinished(QNetworkReply *);
	
};

#endif // OFFERACCEPTDIALOGBTC_H

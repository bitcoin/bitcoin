#ifndef OFFERACCEPTDIALOGBTC_H
#define OFFERACCEPTDIALOGBTC_H
#include "walletmodel.h"
#include <QDialog>
#include <QImage>
#include <QLabel>
class PlatformStyle;
class QSslError;
class QNetworkReply;
namespace Ui {
    class OfferAcceptDialogBTC;
}
class OfferAcceptDialogBTC : public QDialog
{
    Q_OBJECT

public:
    explicit OfferAcceptDialogBTC(const PlatformStyle *platformStyle, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString strPrice, QString sellerAlias, QString address, QWidget *parent=0);
    ~OfferAcceptDialogBTC();

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
	QString sellerAlias;
	QString address;
	QString alias;
	bool offerPaid; 
	

private Q_SLOTS:
	void acceptPayment();
	void on_cancelButton_clicked();
    void acceptOffer();
	void openBTCWallet();
	void onIgnoreSSLErrors(QNetworkReply *reply, QList<QSslError> error);  
	bool CheckPaymentInBTC(const QString &strBTCTxId, const QString& address, const QString& price, int& height, long& time);
	bool CheckUnconfirmedPaymentInBTC(const QString &strBTCTxId, const QString& address, const QString& price);
	bool lookup(const QString &lookupid, QString& address, QString& price);
};

#endif // OFFERACCEPTDIALOGBTC_H

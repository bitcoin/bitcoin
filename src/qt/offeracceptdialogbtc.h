#ifndef OFFERACCEPTDIALOGBTC_H
#define OFFERACCEPTDIALOGBTC_H
#include "walletmodel.h"
#include <QDialog>
#include <QSslError>
#include <QImage>
#include <QLabel>
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
	bool CheckPaymentInBTC(const QString &strBTCTxId, const QString& price);
	bool CheckUnconfirmedPaymentInBTC(const QString &strBTCTxId, const QString& price);
	bool lookup(const QString &lookupid, QString& price);
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
	QString m_strBTCTxId;
	bool offerPaid; 
	

private Q_SLOTS:
	void on_cancelButton_clicked();
    void tryAcceptOffer();
	void acceptOffer();
	void openBTCWallet();
	void onIgnoreSSLErrors(QNetworkReply *reply, QList<QSslError> error);  
	void slotUnconfirmedFinished(QNetworkReply *);
	void slotConfirmedFinished(QNetworkReply *);
	
};

#endif // OFFERACCEPTDIALOGBTC_H

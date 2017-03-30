#ifndef OFFERACCEPTDIALOGBTC_H
#define OFFERACCEPTDIALOGBTC_H
#include "walletmodel.h"
#include <QDialog>
#include <QImage>
#include <QLabel>
#include "amount.h"
class PlatformStyle;
class WalletModel;
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
    explicit OfferAcceptDialogBTC(WalletModel* model, const PlatformStyle *platformStyle, QString strAliasPeg, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString sysPrice, QString sellerAlias, QString address, QString arbiter, QWidget *parent=0);
    ~OfferAcceptDialogBTC();
	void CheckPaymentInBTC();
    bool getPaymentStatus();
	void SetupQRCode(const QString&price);
private:
	bool setupEscrowCheckboxState(bool state);
	WalletModel* walletModel;
	const PlatformStyle *platformStyle;
    Ui::OfferAcceptDialogBTC *ui;
	SendCoinsRecipient info;
	QString quantity;
	QString notes;
	QString qstrPrice;
	QString title;
	QString offer;
	QString acceptGuid;
	QString sellerAlias;
	QString arbiter;
	QString address;
	QString multisigaddress;
	QString alias;
	QString m_buttonText;
	QString m_address;
	bool offerPaid; 
	QString m_redeemScript;	
	QString priceBtc;
	qint64 m_height;

private Q_SLOTS:
	void on_cancelButton_clicked();
    void tryAcceptOffer();
	void onEscrowCheckBoxChanged(bool);
    void acceptOffer();
	void acceptEscrow();
	void openBTCWallet();
	void slotConfirmedFinished(QNetworkReply *);
	void on_escrowEdit_textChanged(const QString & text);
};

#endif // OFFERACCEPTDIALOGBTC_H

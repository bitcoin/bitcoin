#ifndef OFFERACCEPTDIALOGZEC_H
#define OFFERACCEPTDIALOGZEC_H
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
    class OfferAcceptDialogZEC;
}
class OfferAcceptDialogZEC : public QDialog
{
    Q_OBJECT

public:
    explicit OfferAcceptDialogZEC(WalletModel* model, const PlatformStyle *platformStyle, QString strAliasPeg, QString alias, QString offer, QString quantity, QString notes, QString title, QString currencyCode, QString sysPrice, QString sellerAlias, QString address, QString arbiter, QWidget *parent=0);
    ~OfferAcceptDialogZEC();
	void CheckPaymentInZEC();
    bool getPaymentStatus();
	void SetupQRCode(const QString&price);
	void convertAddress();
private:
	bool setupEscrowCheckboxState(bool state);
	WalletModel* walletModel;
	const PlatformStyle *platformStyle;
    Ui::OfferAcceptDialogZEC *ui;
	SendCoinsRecipient info;
	QString quantity;
	QString notes;
	QString qstrPrice;
	QString title;
	QString offer;
	QString arbiter;
	QString acceptGuid;
	QString sellerAlias;
	QString address;
	QString zaddress;
	QString multisigaddress;
	QString alias;
	QString m_buttonText;
	QString m_address;
	double dblPrice;
	bool offerPaid; 
	QString m_redeemScript;	
	QString priceZec;
	qint64 m_height;

private Q_SLOTS:
	void on_cancelButton_clicked();
    void tryAcceptOffer();
	void onEscrowCheckBoxChanged(bool);
    void acceptOffer();
	void acceptEscrow();
	void openZECWallet();
	void slotConfirmedFinished(QNetworkReply *);
	void on_escrowEdit_textChanged(const QString & text);
	
};

#endif // OFFERACCEPTDIALOGZEC_H

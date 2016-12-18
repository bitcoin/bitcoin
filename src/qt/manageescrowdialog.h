#ifndef MANAGEESCROWDIALOG_H
#define MANAGEESCROWDIALOG_H

#include <QDialog>
class WalletModel;
namespace Ui {
    class ManageEscrowDialog;
}
QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE
/** Dialog for editing an address and associated information.
 */
 enum EscrowRoleType {
    Buyer,
    Seller,
	ReSeller,
	Arbiter,
	None
};
class ManageEscrowDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManageEscrowDialog(WalletModel* model, const QString &escrow, QWidget *parent = 0);
    ~ManageEscrowDialog();
	void SendRawTxBTC();
	void SendRawTxZEC();
	void CheckPaymentInBTC();
	void CheckPaymentInZEC();
	bool isYourAlias(const QString &alias);
	bool CompleteEscrowRefund();
	bool CompleteEscrowRelease();
	void onRelease();
	void onRefund();
	void doRefund(const QString &rawTx="");
	void doRelease(const QString &rawTx="");
	bool loadEscrow(const QString &escrow, QString &buyer, QString &seller, QString &reseller, QString &arbiter, QString &status, QString &offertitle, QString &total, QString &exttxid, QString &paymentOption, QString &redeemtxid);
	QString EscrowRoleTypeToString(const EscrowRoleType& escrowType);
	EscrowRoleType findYourEscrowRoleFromAliases(const QString &buyer, const QString &seller, const QString &reseller, const QString &arbiter);
	EscrowRoleType escrowRoleType;
public Q_SLOTS:
	void on_releaseButton_clicked();
	void on_extButton_clicked();
	void on_refundButton_clicked();
	void on_cancelButton_clicked();
	void slotConfirmedFinished(QNetworkReply *);
	void slotConfirmedFinishedCheck(QNetworkReply *);
private:
	void onLeaveFeedback();
	WalletModel* walletModel;
    Ui::ManageEscrowDialog *ui;
	QString escrow;
	QString m_exttxid;
	QString m_checkTxId;
	QString m_redeemTxId;
	QString m_rawTx;
	QString refundWarningStr;
	QString releaseWarningStr;
	QString m_buttontext;
    QString m_paymentOption;
	bool m_bRelease;
	bool m_bRefund;
};

#endif // MANAGEESCROWDIALOG_H

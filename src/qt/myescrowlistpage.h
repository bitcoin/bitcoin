#ifndef MYESCROWLISTPAGE_H
#define MYESCROWLISTPAGE_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class MyEscrowListPage;
}
class EscrowTableModel;
class OptionsModel;
class ClientModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
class QNetworkReply;
QT_END_NAMESPACE

/** Widget that shows a list of owned certes.
  */
class MyEscrowListPage : public QDialog
{
    Q_OBJECT

public:


    explicit MyEscrowListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MyEscrowListPage();

    void setModel(WalletModel*, EscrowTableModel *model);
    void setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void showEvent ( QShowEvent * event );
	void loadAliasList();
	bool lookup(const QString &escrow, QString& address, QString& price, QString& extTxId, QString& redeemTxId, QString& paymentOption);
	void CheckPaymentInBTC(const QString &strExtTxId, const QString& address, const QString& price);
	void CheckPaymentInZEC(const QString &strExtTxId, const QString& address, const QString& price);
private:
	ClientModel* clientModel;
	WalletModel *walletModel;
    Ui::MyEscrowListPage *ui;
    EscrowTableModel *model;
    OptionsModel *optionsModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QString newEscrowToSelect;
	const PlatformStyle *platformStyle;
	QString m_buttonText;
	QString m_strExtTxId;
	QString m_strAddress;
	QString m_paymentOption;
	double dblPrice;
private Q_SLOTS:
	void slotConfirmedFinished(QNetworkReply *);
	void onToggleShowComplete(bool toggled);
    void on_copyEscrow_clicked();
	void on_copyOffer_clicked();
    /** Export button clicked */
    void on_exportButton_clicked();
	void on_refreshButton_clicked();
	void on_buyerMessageButton_clicked();
	void on_sellerMessageButton_clicked();
	void on_arbiterMessageButton_clicked();
	void on_extButton_clicked();
	void on_manageButton_clicked();
	void on_detailButton_clicked();
    /** Spawn contextual menu (right mouse menu) for cert book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to cert table */
    void selectNewEscrow(const QModelIndex &parent, int begin, int /*end*/);
	void on_ackButton_clicked();
	void displayListChanged(const QString& alias);

};

#endif // MYESCROWLISTPAGE_H

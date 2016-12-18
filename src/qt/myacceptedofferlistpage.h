#ifndef MYACCEPTEDOFFERLISTPAGE_H
#define MYACCEPTEDOFFERLISTPAGE_H

#include <QDialog>
#include "amount.h"
class PlatformStyle;
namespace Ui {
    class MyAcceptedOfferListPage;
}
class OfferAcceptTableModel;
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
class MyAcceptedOfferListPage : public QDialog
{
    Q_OBJECT

public:


    explicit MyAcceptedOfferListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MyAcceptedOfferListPage();

    void setModel(WalletModel*, OfferAcceptTableModel *model);
    void setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void showEvent ( QShowEvent * event );
	bool lookup(const QString &lookupid, const QString &acceptid, QString& address, QString& price, QString& extTxId, QString& paymentOption);
	void CheckPaymentInBTC(const QString &strExtTxId, const QString& address, const QString& price);
	void CheckPaymentInZEC(const QString &strExtTxId, const QString& address, const QString& price);
	void loadAliasList();
	QString convertAddress(const QString &sysAddress);

public Q_SLOTS:
    void done(int retval);
private:
	const PlatformStyle *platformStyle;
	ClientModel* clientModel;
	WalletModel *walletModel;
    Ui::MyAcceptedOfferListPage *ui;
    OfferAcceptTableModel *model;
    OptionsModel *optionsModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newOfferToSelect;
	QString m_buttonText;
	QString m_strExtTxId;
	QString m_strAddress;
	QString m_paymentOption;
	double dblPrice;
private Q_SLOTS:
	void slotConfirmedFinished(QNetworkReply *);
    void on_copyOffer_clicked();
    void onCopyOfferValueAction();
    /** Export button clicked */
    void on_exportButton_clicked();
	void on_refreshButton_clicked();
	void on_messageButton_clicked();
	void on_feedbackButton_clicked();
	void on_extButton_clicked();
    void selectionChanged();
    void contextualMenu(const QPoint &point);
    void selectNewOffer(const QModelIndex &parent, int begin, int /*end*/);
	void on_detailButton_clicked();
	void on_ackButton_clicked();
	void displayListChanged(const QString& alias);

};

#endif // MYACCEPTEDOFFERLISTPAGE_H

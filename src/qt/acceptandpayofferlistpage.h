#ifndef ACCEPTANDPAYOFFERLISTPAGE_H
#define ACCEPTANDPAYOFFERLISTPAGE_H

#include <QDialog>
class PlatformStyle;
#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#else
#include <QUrl>
#endif
namespace Ui {
    class AcceptandPayOfferListPage;
}
class JSONRequest;

class OptionsModel;
class WalletModel;
class COffer;
QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
class QUrl;
class QNetworkAccessManager;
class QPixmap;
class QUrl;
class QStringList;
QT_END_NAMESPACE

class SendCoinsRecipient;
/** Widget that shows a list of owned offeres.
  */
class AcceptandPayOfferListPage : public QDialog
{
    Q_OBJECT

public:


    explicit AcceptandPayOfferListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~AcceptandPayOfferListPage();

    const QString &getReturnValue() const { return returnValue; }
	bool handlePaymentRequest(const SendCoinsRecipient *rv);
	bool getProfileData(QString& publicData, QString& privateData);
	void setValue(const QString& strAlias, const QString& strRand, const QString& strSold,  const QString& strRating, COffer &offer, QString price, QString address, QString aliasPeg);
	void updateCaption();
	void OpenPayDialog();
	void OpenBTCPayDialog();
	void OpenZECPayDialog();
	void RefreshImage();
	void loadAliases();
	void setModel(WalletModel* model);
public Q_SLOTS:
	void on_pubProfile();
	void on_privProfile();
    void acceptOffer();
	bool lookup(const QString &id = QString(""));
	void resetState();
	void netwManagerFinished();
	void on_imageButton_clicked();
    /** Spawn contextual menu (right mouse menu) for escrow book entry */
    void contextualMenu(const QPoint &point);
private:
	WalletModel* walletModel;
	const PlatformStyle *platformStyle;
    Ui::AcceptandPayOfferListPage *ui;
	bool URIHandled;
    QString returnValue;
	bool offerPaid;
	QMenu *contextMenu;
	QNetworkAccessManager* m_netwManager;
	QPixmap m_placeholderImage;
	QUrl m_url;
	QStringList m_imageList;
	QString sAddress;
	unsigned char paymentOptions;
	bool usedProfileInfo;
	bool isOfferCert;
};

#endif // ACCEPTANDPAYOFFERLISTPAGE_H

#include "acceptandpayofferlistpage.h"
#include "ui_acceptandpayofferlistpage.h"
#include "init.h"
#include "main.h"
#include "util.h"
#include "offeracceptdialog.h"
#include "offeracceptdialogbtc.h"
#include "offeracceptdialogzec.h"
#include "offer.h"

#include "syscoingui.h"

#include "guiutil.h"
#include "platformstyle.h"
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include <QString>
#include <QByteArray>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegExp>
#include <QStringList>
#include <QDesktopServices>
#include "rpc/server.h"
#include "alias.h"
#include "walletmodel.h"
#include <QSettings>
using namespace std;

extern CRPCTable tableRPC;

AcceptandPayOfferListPage::AcceptandPayOfferListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent), platformStyle(platformStyle),
    ui(new Ui::AcceptandPayOfferListPage)
{	
	sAddress = "";
	isOfferCert = false;
	paymentOptions = 0;
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->lookupButton->setIcon(QIcon());
		ui->acceptButton->setIcon(QIcon());
	}
	else
	{
		ui->lookupButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
		ui->acceptButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/cart"));
	}
	ui->imageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/imageplaceholder"));
	this->offerPaid = false;
	this->usedProfileInfo = false;
	this->URIHandled = false;	
    ui->labelExplanation->setText(tr("Purchase an offer, coins will be used from your balance to complete the transaction"));
    connect(ui->acceptButton, SIGNAL(clicked()), this, SLOT(acceptOffer()));
	connect(ui->lookupButton, SIGNAL(clicked()), this, SLOT(lookup()));
	connect(ui->offeridEdit, SIGNAL(textChanged(QString)), this, SLOT(resetState()));
	ui->notesEdit->setStyleSheet("color: rgb(0, 0, 0); background-color: rgb(255, 255, 255)");
	ui->aliasDisclaimer->setText(QString("<font color='blue'>") + tr("Select an Alias. You may right-click on the notes section and include your public or private profile information from this alias for the merchant") + QString("</font>"));
	m_netwManager = new QNetworkAccessManager(this);
	m_placeholderImage.load(":/images/" + theme + "/imageplaceholder");

	ui->imageButton->setToolTip(tr("Click to open image in browser..."));
    // Build context menu
	QAction *pubProfileAction = new QAction(tr("Use Public Profile"), this);
	QAction *privProfileAction = new QAction(tr("Use Private Profile"), this);
    contextMenu = ui->notesEdit->createStandardContextMenu();
	contextMenu->addSeparator();
    contextMenu->addAction(pubProfileAction);
    contextMenu->addAction(privProfileAction);
    // Connect signals for context menu actions
    connect(pubProfileAction, SIGNAL(triggered()), this, SLOT(on_pubProfile()));
    connect(privProfileAction, SIGNAL(triggered()), this, SLOT(on_privProfile()));
	ui->notesEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->notesEdit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

	RefreshImage();

}
void AcceptandPayOfferListPage::contextualMenu(const QPoint &point)
{
    contextMenu->exec(ui->notesEdit->mapToGlobal(point));
}
void AcceptandPayOfferListPage::setModel(WalletModel* model)
{
	walletModel = model;
}
void AcceptandPayOfferListPage::on_pubProfile()
{
	if(this->usedProfileInfo)
	{
        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm Public Profile Inclusion"),
                 tr("Warning: You have already appended profile information to the notes for this purchase!") + "<br><br>" + tr("Are you sure you wish to continue?"),
                 QMessageBox::Yes|QMessageBox::Cancel,
                 QMessageBox::Cancel);
        if(retval == QMessageBox::Cancel)
			return;
	}
	QString pubProfile, privProfile;
	if(getProfileData(pubProfile, privProfile))
	{
		ui->notesEdit->appendPlainText(pubProfile);
		this->usedProfileInfo = true;
	}
}
void AcceptandPayOfferListPage::on_privProfile()
{
	if(this->usedProfileInfo)
	{
        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm Private Profile Inclusion"),
                 tr("Warning: You have already appended profile information to the notes for this purchase!") + "<br><br>" + tr("Are you sure you wish to continue?"),
                 QMessageBox::Yes|QMessageBox::Cancel,
                 QMessageBox::Cancel);
        if(retval == QMessageBox::Cancel)
			return;
	}
	QString pubProfile, privProfile;
	if(getProfileData(pubProfile, privProfile))
	{
		ui->notesEdit->appendPlainText(privProfile);
		this->usedProfileInfo = true;
	}
}
bool AcceptandPayOfferListPage::getProfileData(QString& publicData, QString& privateData)
{
	string strMethod = string("aliasinfo");
    UniValue params(UniValue::VARR); 
	UniValue result ;
	params.push_back(ui->aliasEdit->currentText().toStdString());

	publicData = "";
	privateData = "";
	try {
		result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VOBJ)
		{	
			const UniValue &o = result.get_obj();	
			const UniValue& pub_value = find_value(o, "value");
			if (pub_value.type() == UniValue::VSTR)
				publicData = QString::fromStdString(pub_value.get_str());
			const UniValue& priv_value = find_value(o, "privatevalue");
			if (priv_value.type() == UniValue::VSTR)
				privateData = QString::fromStdString(priv_value.get_str());				
			return true;					
		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could get alias profile data: ") + QString::fromStdString(strError),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to get the alias profile data: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}  
	QMessageBox::critical(this, windowTitle(),
		tr("Couldn't find alias in the database: ") + ui->aliasEdit->currentText(),
			QMessageBox::Ok, QMessageBox::Ok);
	return false;
}
void AcceptandPayOfferListPage::loadAliases()
{
	ui->aliasEdit->clear();
	string strMethod = string("aliaslist");
    UniValue params(UniValue::VARR); 
	UniValue result ;
	string name_str;
	int expired = 0;
	
	try {
		result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VARR)
		{
			name_str = "";
			expired = 0;


	
			const UniValue &arr = result.get_array();
		    for (unsigned int idx = 0; idx < arr.size(); idx++) {
			    const UniValue& input = arr[idx];
				if (input.type() != UniValue::VOBJ)
					continue;
				const UniValue& o = input.get_obj();
				name_str = "";

				expired = 0;


		
				const UniValue& name_value = find_value(o, "name");
				if (name_value.type() == UniValue::VSTR)
					name_str = name_value.get_str();		
				const UniValue& expired_value = find_value(o, "expired");
				if (expired_value.type() == UniValue::VNUM)
					expired = expired_value.get_int();
				
				if(expired == 0)
				{
					QString name = QString::fromStdString(name_str);
					ui->aliasEdit->addItem(name);					
				}
				
			}
		}
	}
	catch (UniValue& objError)
	{
		string strError = find_value(objError, "message").get_str();
		QMessageBox::critical(this, windowTitle(),
			tr("Could not refresh alias list: ") + QString::fromStdString(strError),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to refresh the alias list: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}       
	QSettings settings;
 	QString defaultOfferAlias = settings.value("defaultAlias", "").toString();
	int aliasIndex = ui->aliasEdit->findText(defaultOfferAlias);
	if(aliasIndex >= 0)
		ui->aliasEdit->setCurrentIndex(aliasIndex);
}

void AcceptandPayOfferListPage::on_imageButton_clicked()
{
	if(m_url.isValid())
		QDesktopServices::openUrl(QUrl(m_url.toString(),QUrl::TolerantMode));
}
void AcceptandPayOfferListPage::netwManagerFinished()
{
	QNetworkReply* reply = (QNetworkReply*)sender();
	if(!reply)
		return;
	if (reply->error() != QNetworkReply::NoError) {
		return;
	}

	QByteArray imageData = reply->readAll();
	QPixmap pixmap;
	pixmap.loadFromData(imageData);
	QIcon ButtonIcon(pixmap);
	ui->imageButton->setIcon(ButtonIcon);


	reply->deleteLater();
}
AcceptandPayOfferListPage::~AcceptandPayOfferListPage()
{
    delete ui;
	this->URIHandled = false;
	delete contextMenu;
}
void AcceptandPayOfferListPage::resetState()
{
	this->offerPaid = false;
	this->URIHandled = false;
	this->usedProfileInfo = false;
	updateCaption();
}
void AcceptandPayOfferListPage::updateCaption()
{	
	if(this->offerPaid)
	{
		ui->labelExplanation->setText(QString("<font color='green'>") + tr("You have successfully paid for this offer!") + QString("</font>"));
	}
	else
	{
		ui->labelExplanation->setText(tr("Purchase this offer, coins will be used from your balance to complete the transaction"));
	}	
}
void AcceptandPayOfferListPage::OpenPayDialog()
{
	if(!walletModel)
		return;
	OfferAcceptDialog dlg(walletModel, platformStyle, ui->aliasPegEdit->text(), ui->aliasEdit->currentText(), ui->offeridEdit->text(), ui->qtyEdit->text(), ui->notesEdit->toPlainText(), ui->infoTitle->text(), ui->infoCurrency->text(), ui->infoPrice->text(), ui->sellerEdit->text(), sAddress, paymentOptions, this);
	if(dlg.exec())
	{
		this->offerPaid = dlg.getPaymentStatus();
	}
	updateCaption();
}
void AcceptandPayOfferListPage::OpenBTCPayDialog()
{
	if(!walletModel)
		return;
	int sysprecision = 0;
	double dblPrice = ui->infoPrice->text().toDouble();
	CAmount sysPrice = convertCurrencyCodeToSyscoin(vchFromString(ui->aliasPegEdit->text().toStdString()), vchFromString(ui->infoCurrency->text().toStdString()), dblPrice, chainActive.Tip()->nHeight, sysprecision);
	if(sysPrice == 0)
	{
        QMessageBox::critical(this, windowTitle(),
			tr("Could not find currency in the rates peg for this offer. Currency: ") + ui->infoCurrency->text()
                ,QMessageBox::Ok, QMessageBox::Ok);
		return;
	}	
	QString strSYSPrice = QString::fromStdString(strprintf("%.*f", 8, ValueFromAmount(sysPrice).get_real()));
	OfferAcceptDialogBTC dlg(walletModel, platformStyle, ui->aliasPegEdit->text(), ui->aliasEdit->currentText(), ui->offeridEdit->text(), ui->qtyEdit->text(), ui->notesEdit->toPlainText(), ui->infoTitle->text(), ui->infoCurrency->text(), strSYSPrice, ui->sellerEdit->text(), sAddress, "",this);
	if(dlg.exec())
	{
		this->offerPaid = dlg.getPaymentStatus();
	}
	updateCaption();
}
void AcceptandPayOfferListPage::OpenZECPayDialog()
{
	if(!walletModel)
		return;

	int sysprecision = 0;
	double dblPrice = ui->infoPrice->text().toDouble();
	CAmount sysPrice = convertCurrencyCodeToSyscoin(vchFromString(ui->aliasPegEdit->text().toStdString()), vchFromString(ui->infoCurrency->text().toStdString()), dblPrice, chainActive.Tip()->nHeight, sysprecision);
	if(sysPrice == 0)
	{
        QMessageBox::critical(this, windowTitle(),
			tr("Could not find currency in the rates peg for this offer. Currency: ") + ui->infoCurrency->text()
                ,QMessageBox::Ok, QMessageBox::Ok);
		return;
	}	
	QString strSYSPrice = QString::fromStdString(strprintf("%.*f", 8, ValueFromAmount(sysPrice).get_real()));
	OfferAcceptDialogZEC dlg(walletModel, platformStyle, ui->aliasPegEdit->text(), ui->aliasEdit->currentText(), ui->offeridEdit->text(), ui->qtyEdit->text(), ui->notesEdit->toPlainText(), ui->infoTitle->text(), ui->infoCurrency->text(), strSYSPrice, ui->sellerEdit->text(), sAddress, "",this);
	
	if(dlg.exec())
	{
		this->offerPaid = dlg.getPaymentStatus();
	}
	updateCaption();
}
// send offeraccept with offer guid/qty as params and then send offerpay with wtxid (first param of response) as param, using RPC commands.
void AcceptandPayOfferListPage::acceptOffer()
{
	if(ui->qtyEdit->text().toUInt() <= 0)
	{
		QMessageBox::information(this, windowTitle(),
			tr("Invalid quantity when trying to accept this offer!"),
			QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
	if(ui->notesEdit->toPlainText().size() <= 0 && !isOfferCert)
	{
		QMessageBox::information(this, windowTitle(),
			tr("Please enter pertinent information required to the offer in the 'Notes' field (address, e-mail address, shipping notes, etc)."),
			QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
	if(ui->aliasEdit->currentText().size() <= 0)
	{
		QMessageBox::information(this, windowTitle(),
			tr("Please choose an alias before purchasing this offer."),
			QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
	
	this->offerPaid = false;
	ui->labelExplanation->setText(tr("Waiting for confirmation on the purchase of this offer"));
	if(paymentOptions == PAYMENTOPTION_BTC)
		OpenBTCPayDialog();
	else if(paymentOptions == PAYMENTOPTION_ZEC)
		OpenZECPayDialog();
	else
		OpenPayDialog();
}

bool AcceptandPayOfferListPage::lookup(const QString &lookupid)
{
	QString id = lookupid;
	if(id == QString(""))
	{
		id = ui->offeridEdit->text();
	}
	string strError;
	string strMethod = string("offerinfo");
	UniValue params(UniValue::VARR);
	UniValue result;
	params.push_back(id.toStdString());

    try {
        result = tableRPC.execute(strMethod, params);

		if (result.type() == UniValue::VOBJ)
		{
			const UniValue &offerObj = result.get_obj();
			COffer offerOut;
			const string &strRand = find_value(offerObj, "offer").get_str();
			const string &strAddress = find_value(offerObj, "address").get_str();
			offerOut.vchCert = vchFromString(find_value(offerObj, "cert").get_str());
			string alias = find_value(offerObj, "alias").get_str();
			offerOut.sTitle = vchFromString(find_value(offerObj, "title").get_str());
			offerOut.sCategory = vchFromString(find_value(offerObj, "category").get_str());
			offerOut.sCurrencyCode = vchFromString(find_value(offerObj, "currency").get_str());
			string strAliasPeg = find_value(offerObj, "alias_peg").get_str();
			const QString &strSold = QString::number(find_value(offerObj, "offers_sold").get_int());
			const QString &strRating = QString::number(find_value(offerObj, "alias_rating").get_int());
			if(find_value(offerObj, "quantity").get_str() == "unlimited")
				offerOut.nQty = -1;
			else
				offerOut.nQty = QString::fromStdString(find_value(offerObj, "quantity").get_str()).toUInt();	
			offerOut.paymentOptions = find_value(offerObj, "paymentoptions").get_int();	
			string descString = find_value(offerObj, "description").get_str();

			offerOut.sDescription = vchFromString(descString);
			UniValue outerDescValue(UniValue::VSTR);
			bool read = outerDescValue.read(descString);
			if (read)
			{
				if(outerDescValue.type() == UniValue::VOBJ)
				{
					const UniValue &outerDescObj = outerDescValue.get_obj();
					const UniValue &descValue = find_value(outerDescObj, "description");
					if (descValue.type() == UniValue::VSTR)
					{
						offerOut.sDescription = vchFromString(descValue.get_str());
					}
				}

			}
			setValue(QString::fromStdString(alias), QString::fromStdString(strRand), strSold, strRating, offerOut, QString::fromStdString(find_value(offerObj, "price").get_str()), QString::fromStdString(strAddress), QString::fromStdString(strAliasPeg));
			return true;
		}
		 

	}
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("Could not find this offer, please ensure the offer has been confirmed by the blockchain: ") + id,
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to locate this offer, please ensure the offer has been confirmed by the blockchain: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	return false;


}
bool AcceptandPayOfferListPage::handlePaymentRequest(const SendCoinsRecipient *rv)
{	
	if(this->URIHandled)
	{
		QMessageBox::critical(this, windowTitle(),
		tr("URI has been already handled"),
			QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}

	ui->qtyEdit->setText(QString::number(rv->amount));
	ui->notesEdit->setPlainText(rv->message);

	if(lookup(rv->address))
	{
	
		this->URIHandled = true;
		acceptOffer();
		this->URIHandled = false;
	}
    return true;
}
void AcceptandPayOfferListPage::setValue(const QString& strAlias, const QString& strRand, const QString& strSold,  const QString& strRating, COffer &offer, QString price, QString address, QString aliasPeg)
{
	loadAliases();
    ui->offeridEdit->setText(strRand);
	if(!offer.vchCert.empty())
	{
		isOfferCert = true;
	}
	else
	{
		isOfferCert = false;	
	}
	ui->sellerEdit->setText(strAlias);
	ui->infoTitle->setText(QString::fromStdString(stringFromVch(offer.sTitle)));
	ui->infoCategory->setText(QString::fromStdString(stringFromVch(offer.sCategory)));
	ui->infoCurrency->setText(QString::fromStdString(stringFromVch(offer.sCurrencyCode)));
	ui->aliasPegEdit->setText(aliasPeg);
	ui->sellerRatingEdit->setText(QString("%1 %2").arg(strRating).arg(tr("Stars")));
	ui->infoPrice->setText(price);
	if(offer.nQty == -1)
		ui->infoQty->setText(tr("unlimited"));
	else
		ui->infoQty->setText(QString::number(offer.nQty));
	ui->infoSold->setText(strSold);
	ui->infoDescription->setPlainText(QString::fromStdString(stringFromVch(offer.sDescription)));
	ui->qtyEdit->setText(QString("1"));
	ui->notesEdit->setPlainText(QString(""));
	paymentOptions = offer.paymentOptions;
	sAddress = address;
	QRegExp rx("(?:https?|ftp)://\\S+");

    rx.indexIn(QString::fromStdString(stringFromVch(offer.sDescription)));
    m_imageList = rx.capturedTexts();
	RefreshImage();

}

void AcceptandPayOfferListPage::RefreshImage()
{
	QIcon ButtonIcon(m_placeholderImage);
	ui->imageButton->setIcon(ButtonIcon);
	m_url.clear();
	if(m_imageList.size() > 0 && m_imageList.at(0) != QString(""))
	{
		QString parsedURL = m_imageList.at(0).simplified();
		m_url = QUrl(parsedURL);
		if(m_url.isValid())
		{
			QNetworkRequest request(m_url);
			request.setRawHeader("Accept", "q=0.9,image/webp,*/*;q=0.8");
			request.setRawHeader("Cache-Control", "no-cache");
			request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.71 Safari/537.36");
			QSslConfiguration conf = request.sslConfiguration();
			conf.setPeerVerifyMode(QSslSocket::VerifyNone);
			conf.setProtocol(QSsl::TlsV1_0);
			request.setSslConfiguration(conf);
			QNetworkReply *reply = m_netwManager->get(request);
			reply->ignoreSslErrors();
			connect(reply, SIGNAL(finished()), this, SLOT(netwManagerFinished()));
		}
	}
}

#include "acceptandpayofferlistpage.h"
#include "ui_acceptandpayofferlistpage.h"
#include "init.h"
#include "util.h"
#include "offeracceptdialog.h"
#include "offeracceptdialogbtc.h"
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
#include "rpcserver.h"
#include "alias.h"
#include "walletmodel.h"
using namespace std;

extern const CRPCTable tableRPC;

AcceptandPayOfferListPage::AcceptandPayOfferListPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent), platformStyle(platformStyle),
    ui(new Ui::AcceptandPayOfferListPage)
{	
	sAddress = "";
	bOnlyAcceptBTC = false;
    ui->setupUi(this);
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->lookupButton->setIcon(QIcon());
		ui->acceptButton->setIcon(QIcon());
		ui->imageButton->setIcon(QIcon());
	}
	else
	{
		ui->lookupButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
		ui->acceptButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/cart"));
	}
	ui->imageButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/imageplaceholder"));
	this->offerPaid = false;
	this->URIHandled = false;	
    ui->labelExplanation->setText(tr("Purchase an offer, Syscoin will be used from your balance to complete the transaction"));
    connect(ui->acceptButton, SIGNAL(clicked()), this, SLOT(acceptOffer()));
	connect(ui->lookupButton, SIGNAL(clicked()), this, SLOT(lookup()));
	connect(ui->offeridEdit, SIGNAL(textChanged(const QString &)), this, SLOT(resetState()));
	ui->notesEdit->setStyleSheet("color: rgb(0, 0, 0); background-color: rgb(255, 255, 255)");
	ui->aliasDisclaimer->setText(tr("<font color='blue'>Select an Alias</font>"));
	m_netwManager = new QNetworkAccessManager(this);
	m_placeholderImage.load(":/images/" + theme + "/imageplaceholder");

	ui->imageButton->setToolTip(tr("Click to open image in browser..."));
	ui->infoCert->setVisible(false);
	ui->certLabel->setVisible(false);
	RefreshImage();

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
			tr("Could not refresh cert list: %1").arg(QString::fromStdString(strError)),
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to refresh the cert list: ") + QString::fromStdString(e.what()),
				QMessageBox::Ok, QMessageBox::Ok);
	}         
 
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
			QMessageBox::critical(this, windowTitle(),
				reply->errorString(),
				QMessageBox::Ok, QMessageBox::Ok);
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
}
void AcceptandPayOfferListPage::resetState()
{
		this->offerPaid = false;
		this->URIHandled = false;
		updateCaption();
}
void AcceptandPayOfferListPage::updateCaption()
{
		
		if(this->offerPaid)
		{
			ui->labelExplanation->setText(tr("<font color='green'>You have successfully paid for this offer!</font>"));
		}
		else
		{
			ui->labelExplanation->setText(tr("Purchase this offer, Syscoin will be used from your balance to complete the transaction"));
		}
		
}
void AcceptandPayOfferListPage::OpenPayDialog()
{
	OfferAcceptDialog dlg(platformStyle, ui->aliasPegEdit->text(), ui->aliasEdit->currentText(), ui->offeridEdit->text(), ui->qtyEdit->text(), ui->notesEdit->toPlainText(), ui->infoTitle->text(), ui->infoCurrency->text(), ui->infoPrice->text(), ui->sellerEdit->text(), sAddress, this);
	if(dlg.exec())
	{
		this->offerPaid = dlg.getPaymentStatus();
	}
	updateCaption();
}
void AcceptandPayOfferListPage::OpenBTCPayDialog()
{
	OfferAcceptDialogBTC dlg(platformStyle, ui->aliasEdit->currentText(), ui->offeridEdit->text(), ui->qtyEdit->text(), ui->notesEdit->toPlainText(), ui->infoTitle->text(), ui->infoCurrency->text(), ui->infoPrice->text(), ui->sellerEdit->text(), sAddress, this);
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
	if(ui->notesEdit->toPlainText().size() <= 0 && ui->infoCert->text().size() <= 0)
	{
		QMessageBox::information(this, windowTitle(),
			tr("Please enter pertinent information required to the offer in the <b>Notes</b> field (address, e-mail address, shipping notes, etc)."),
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
	if(bOnlyAcceptBTC)
		OpenBTCPayDialog();
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
			offerOut.vchAliasPeg = vchFromString(find_value(offerObj, "alias_peg").get_str());
			if(find_value(offerObj, "quantity").get_str() == "unlimited")
				offerOut.nQty = -1;
			else
				offerOut.nQty = QString::fromStdString(find_value(offerObj, "quantity").get_str()).toUInt();	
			offerOut.bOnlyAcceptBTC = find_value(offerObj, "btconly").get_str() == "Yes"? true: false;	
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
			setValue(QString::fromStdString(alias), QString::fromStdString(strRand), offerOut, QString::fromStdString(find_value(offerObj, "price").get_str()), QString::fromStdString(strAddress));
			return true;
		}
		 

	}
	catch (UniValue& objError)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("Could not find this offer, please check the offer ID and that it has been confirmed by the blockchain: ") + id,
				QMessageBox::Ok, QMessageBox::Ok);

	}
	catch(std::exception& e)
	{
		QMessageBox::critical(this, windowTitle(),
			tr("There was an exception trying to locate this offer, please check the offer ID and that it has been confirmed by the blockchain: ") + QString::fromStdString(e.what()),
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
void AcceptandPayOfferListPage::setValue(const QString& strAlias, const QString& strRand, COffer &offer, QString price, QString address)
{
	loadAliases();
    ui->offeridEdit->setText(strRand);
	if(!offer.vchCert.empty())
	{
		ui->infoCert->setVisible(true);
		ui->certLabel->setVisible(true);
		ui->infoCert->setText(QString::fromStdString(stringFromVch(offer.vchCert)));
	}
	else
	{
		ui->infoCert->setVisible(false);
		ui->infoCert->setText("");
		ui->certLabel->setVisible(false);
	}
	ui->sellerEdit->setText(strAlias);
	ui->infoTitle->setText(QString::fromStdString(stringFromVch(offer.sTitle)));
	ui->infoCategory->setText(QString::fromStdString(stringFromVch(offer.sCategory)));
	ui->infoCurrency->setText(QString::fromStdString(stringFromVch(offer.sCurrencyCode)));
	ui->aliasPegEdit->setText(QString::fromStdString(stringFromVch(offer.vchAliasPeg)));
	ui->infoPrice->setText(price);
	if(offer.nQty == -1)
		ui->infoQty->setText(tr("unlimited"));
	else
		ui->infoQty->setText(QString::number(offer.nQty));
	ui->infoDescription->setPlainText(QString::fromStdString(stringFromVch(offer.sDescription)));
	ui->qtyEdit->setText(QString("1"));
	ui->notesEdit->setPlainText(QString(""));
	bOnlyAcceptBTC = offer.bOnlyAcceptBTC;
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
			QNetworkReply *reply = m_netwManager->get(request);
			reply->ignoreSslErrors();
			connect(reply, SIGNAL(finished()), this, SLOT(netwManagerFinished()));
		}
	}
}

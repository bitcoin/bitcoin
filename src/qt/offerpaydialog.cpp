#include "init.h"
#include "util.h"
#include "platformstyle.h"
#include "guiutil.h"
#include "offerpaydialog.h"
#include "ui_offerpaydialog.h"
OfferPayDialog::OfferPayDialog(const PlatformStyle *platformStyle, QString title, QString quantity, QString price, QString currency,QWidget *parent) :
    QDialog(parent), 
	ui(new Ui::OfferPayDialog)
{
    ui->setupUi(this);
	connect(ui->finishButton, SIGNAL(clicked()), this, SLOT(accept()));
	ui->payMessage->setText(QString("<p>") + tr("You've purchased") + QString(" %1 ").arg(quantity) + tr("of") + QString(" '%1' ").arg(title) + tr("for") + QString(" %3 %4 ").arg(price).arg(currency) + QString("</p><p><font color='green'><b>") + tr("Your payment is complete!") + QString("</b></font></p><p>") + tr("The merchant has been sent your delivery information and your item should arrive shortly. The merchant may follow-up with further information through a private message (please check your inbox regularly).") + QString("</p><br>"));				
	ui->purchaseHint->setText(tr("Please click Finish"));
	QString theme = GUIUtil::getThemeName();
    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/" + theme + "/synced");
	if (!platformStyle->getImagesOnButtons())
		ui->finishButton->setIcon(QIcon());
	else
		ui->finishButton->setIcon(icon);
}
OfferPayDialog::~OfferPayDialog()
{
    delete ui;
}



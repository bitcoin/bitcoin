#include "init.h"
#include "util.h"
#include "platformstyle.h"
#include "guiutil.h"
#include "offerescrowdialog.h"
#include "ui_offerescrowdialog.h"
OfferEscrowDialog::OfferEscrowDialog(const PlatformStyle *platformStyle, QString title, QString quantity, QString price, QString currency, QWidget *parent) :
    QDialog(parent), 
	ui(new Ui::OfferEscrowDialog)
{
    ui->setupUi(this);
	connect(ui->finishButton, SIGNAL(clicked()), this, SLOT(accept()));
	ui->payMessage->setText(QString("<p>") + tr("You've created an escrow for") + QString(" %1 ").arg(quantity) + tr("of") + QString(" '%1' ").arg(title) + tr("for") + QString(" %3 %4 ").arg(price).arg(currency) + tr("(includes a 0.05% escrow arbiter fee which is returned to you if the arbiter is not involved in the escrow)") + QString("</p><p><font color='green'><b>") + tr("Your payment is in escrow!") + QString("</b></font></p><p>") + tr("The merchant and arbiter have been sent an escrow notification. The merchant may follow-up with further information.") + QString("</p><br>"));			
	ui->purchaseHint->setText(tr("Please click Finish"));
	QString theme = GUIUtil::getThemeName();  
	if (!platformStyle->getImagesOnButtons())
	{
		ui->finishButton->setIcon(QIcon());
	}
	else
	{
		ui->finishButton->setIcon(platformStyle->SingleColorIcon(":/icons/" + theme + "/synced"));
	}
}

OfferEscrowDialog::~OfferEscrowDialog()
{
    delete ui;
}



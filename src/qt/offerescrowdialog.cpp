#include "init.h"
#include "util.h"
#include "platformstyle.h"
#include "guiutil.h"
#include "offerescrowdialog.h"
#include "ui_offerescrowdialog.h"
OfferEscrowDialog::OfferEscrowDialog(const PlatformStyle *platformStyle, QString title, QString quantity, QString price, QWidget *parent) :
    QDialog(parent), 
	ui(new Ui::OfferEscrowDialog)
{
    ui->setupUi(this);
	connect(ui->finishButton, SIGNAL(clicked()), this, SLOT(accept()));
	ui->payMessage->setText(tr("<p>You've created an escrow for %1 of '%2' for %3 SYS!</p><p><FONT COLOR='green'><b>Your payment is in escrow!</b></FONT></p><p>The merchant and arbiter have been sent an escrow notification. The merchant may follow-up with further information.</p><br>").arg(quantity).arg(title).arg(price));			
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



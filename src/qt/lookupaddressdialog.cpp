// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "lookupaddressdialog.h"
#include "ui_lookupaddressdialog.h"

#include "guiutil.h"

#include "omnicore/omnicore.h"
#include "omnicore/sp.h"
#include "omnicore/wallettxs.h"

#include "base58.h"

#include <stdint.h>
#include <sstream>
#include <string>

#include <QAction>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDrag>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QString>
#include <QWidget>

#if defined(HAVE_CONFIG_H)
#include "bitcoin-config.h" /* for USE_QRCODE */
#endif

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

using std::ostringstream;
using std::string;

using namespace mastercore;

MPQRImageWidget::MPQRImageWidget(QWidget *parent):
    QLabel(parent), contextMenu(0)
{
    contextMenu = new QMenu();
    QAction *saveImageAction = new QAction(tr("&Save Image..."), this);
    connect(saveImageAction, SIGNAL(triggered()), this, SLOT(saveImage()));
    contextMenu->addAction(saveImageAction);
    QAction *copyImageAction = new QAction(tr("&Copy Image"), this);
    connect(copyImageAction, SIGNAL(triggered()), this, SLOT(copyImage()));
    contextMenu->addAction(copyImageAction);
}

QImage MPQRImageWidget::exportImage()
{
    if(!pixmap())
        return QImage();
    return pixmap()->toImage().scaled(256,256);
}

void MPQRImageWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && pixmap())
    {
        event->accept();
        QMimeData *mimeData = new QMimeData;
        mimeData->setImageData(exportImage());

        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec();
    } else {
        QLabel::mousePressEvent(event);
    }
}

void MPQRImageWidget::saveImage()
{
    if(!pixmap())
        return;
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Image (*.png)"), NULL);
    if (!fn.isEmpty())
    {
        exportImage().save(fn);
    }
}

void MPQRImageWidget::copyImage()
{
    if(!pixmap())
        return;
    QApplication::clipboard()->setImage(exportImage());
}

void MPQRImageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if(!pixmap())
        return;
    contextMenu->exec(event->globalPos());
}

LookupAddressDialog::LookupAddressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LookupAddressDialog)
{
    ui->setupUi(this);

#if QT_VERSION >= 0x040700
    ui->searchLineEdit->setPlaceholderText("Search address");
#endif

    // connect actions
    connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(searchButtonClicked()));

    // hide balance labels
    QLabel* balances[] = { ui->propertyLabel1, ui->propertyLabel2, ui->propertyLabel3, ui->propertyLabel4, ui->propertyLabel5, ui->propertyLabel6, ui->propertyLabel7, ui->propertyLabel8, ui->propertyLabel9, ui->propertyLabel10 };
    QLabel* labels[] = { ui->property1, ui->property2, ui->property3, ui->property4, ui->property5, ui->property6, ui->property7, ui->property8, ui->property9, ui->property10 };
    int pItem = 0;
    for (pItem = 1; pItem < 11; pItem++)
    {
        labels[pItem-1]->setVisible(false);
        balances[pItem-1]->setVisible(false);
    }
    ui->onlyLabel->setVisible(false);
    ui->frame->setVisible(false);
}

LookupAddressDialog::~LookupAddressDialog()
{
    delete ui;
}

void LookupAddressDialog::searchAddress()
{
    // search function to lookup address
    string searchText = ui->searchLineEdit->text().toStdString();

    // first let's check if we have a searchText, if not do nothing
    if (searchText.empty()) return;

    // lets see if the string is a valid bitcoin address
    CBitcoinAddress address;
    address.SetString(searchText); // no null check on searchText required we've already checked it's not empty above
    if (address.IsValid()) //do what?
    {
        // update top fields
        ui->addressLabel->setText(QString::fromStdString(searchText));
        if ((searchText.substr(0,1) == "1") || (searchText.substr(0,1) == "m") || (searchText.substr(0,1) == "n")) ui->addressTypeLabel->setText("Public Key Hash");
        if ((searchText.substr(0,1) == "2") || (searchText.substr(0,1) == "3")) ui->addressTypeLabel->setText("Pay to Script Hash");
        if (IsMyAddress(searchText)) { ui->isMineLabel->setText("Yes"); } else { ui->isMineLabel->setText("No"); }
        ui->balanceLabel->setText(QString::fromStdString(FormatDivisibleMP(getUserAvailableMPbalance(searchText, 1)) + " OMNI"));
        // QR
        #ifdef USE_QRCODE
        ui->QRCode->setText("");
        QRcode *code = QRcode_encodeString(QString::fromStdString(searchText).toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!code)
        {
            ui->QRCode->setText(tr("Error encoding address into QR Code."));
        }
        else
        {
            QImage myImage = QImage(code->width + 4, code->width + 4, QImage::Format_RGB32);
            myImage.fill(0xffffff);
            unsigned char *p = code->data;
            for (int y = 0; y < code->width; y++)
            {
                for (int x = 0; x < code->width; x++)
                {
                    myImage.setPixel(x + 2, y + 2, ((*p & 1) ? 0x0 : 0xffffff));
                    p++;
                }
            }
            QRcode_free(code);
            ui->QRCode->setPixmap(QPixmap::fromImage(myImage).scaled(96, 96));
        }
        #endif

        //scrappy way to do this, find a more efficient way of interacting with labels
        //show first 10 SPs with balances - needs to be converted to listwidget or something
        unsigned int propertyId;
        unsigned int lastFoundPropertyIdMainEco = 1;
        unsigned int lastFoundPropertyIdTestEco = 1;
        string pName[12]; // TODO: enough slots?
        uint64_t pBal[12];
        bool pDivisible[12];
        bool pFound[12];
        unsigned int pItem;
        bool foundProperty = false;
        for (pItem = 1; pItem < 12; pItem++)
        {
            pFound[pItem] = false;
            for (propertyId = lastFoundPropertyIdMainEco+1; propertyId<10000; propertyId++)
            {
                foundProperty=false;
                if (getUserAvailableMPbalance(searchText, propertyId) > 0)
                {
                    lastFoundPropertyIdMainEco = propertyId;
                    foundProperty=true;
                    pName[pItem] = getPropertyName(propertyId).c_str();
                    if(pName[pItem].size()>32) pName[pItem]=pName[pItem].substr(0,32)+"...";
                    pName[pItem] += strprintf(" (#%d)", propertyId);
                    pBal[pItem] = getUserAvailableMPbalance(searchText, propertyId);
                    pDivisible[pItem] = isPropertyDivisible(propertyId);
                    pFound[pItem] = true;
                    break;
                }
            }

            // have we found a property in main eco?  If not let's try test eco
            if (!foundProperty)
            {
                for (propertyId = lastFoundPropertyIdTestEco+1; propertyId<10000; propertyId++)
                {
                    if (getUserAvailableMPbalance(searchText, propertyId+2147483647) > 0)
                    {
                        lastFoundPropertyIdTestEco = propertyId;
                        foundProperty=true;
                        pName[pItem] = getPropertyName(propertyId+2147483647).c_str();
                        if(pName[pItem].size()>32) pName[pItem]=pName[pItem].substr(0,32)+"...";
                        pName[pItem] += strprintf(" (#%d)", propertyId+2147483647);
                        pBal[pItem] = getUserAvailableMPbalance(searchText, propertyId+2147483647);
                        pDivisible[pItem] = isPropertyDivisible(propertyId+2147483647);
                        pFound[pItem] = true;
                        break;
                    }
                }
            }
        }

        // set balance info
        ui->frame->setVisible(true);
        QLabel* balances[] = { ui->propertyLabel1, ui->propertyLabel2, ui->propertyLabel3, ui->propertyLabel4, ui->propertyLabel5, ui->propertyLabel6, ui->propertyLabel7, ui->propertyLabel8, ui->propertyLabel9, ui->propertyLabel10 };
        QLabel* labels[] = { ui->property1, ui->property2, ui->property3, ui->property4, ui->property5, ui->property6, ui->property7, ui->property8, ui->property9, ui->property10 };
        for (pItem = 1; pItem < 11; pItem++)
        {
            if (pFound[pItem])
            {
                labels[pItem-1]->setVisible(true);
                balances[pItem-1]->setVisible(true);
                labels[pItem-1]->setText(pName[pItem].c_str());
                string tokenLabel = " SPT";
                if (pName[pItem]=="Test Omni (#2)") { tokenLabel = " TOMNI"; }
                if (pDivisible[pItem])
                {
                    balances[pItem-1]->setText(QString::fromStdString(FormatDivisibleMP(pBal[pItem]) + tokenLabel));
                }
                else
                {
                    string balText = strprintf("%d", pBal[pItem]);
                    balText += tokenLabel;
                    balances[pItem-1]->setText(balText.c_str());
                }
            }
            else
            {
                labels[pItem-1]->setVisible(false);
                balances[pItem-1]->setVisible(false);
            }
        }
        if (pFound[11]) { ui->onlyLabel->setVisible(true); } else { ui->onlyLabel->setVisible(false); }
    }
    else
    {
         // hide balance labels
        QLabel* balances[] = { ui->propertyLabel1, ui->propertyLabel2, ui->propertyLabel3, ui->propertyLabel4, ui->propertyLabel5, ui->propertyLabel6, ui->propertyLabel7, ui->propertyLabel8, ui->propertyLabel9, ui->propertyLabel10 };
        QLabel* labels[] = { ui->property1, ui->property2, ui->property3, ui->property4, ui->property5, ui->property6, ui->property7, ui->property8, ui->property9, ui->property10 };
        int pItem = 0;
        for (pItem = 1; pItem < 11; pItem++)
        {
            labels[pItem-1]->setVisible(false);
            balances[pItem-1]->setVisible(false);
        }
        ui->addressLabel->setText("N/A");
        ui->addressTypeLabel->setText("N/A");
        ui->isMineLabel->setText("N/A");
        ui->frame->setVisible(false);
        // show error message
        string strText = "The address entered was not valid.";
        QString strQText = QString::fromStdString(strText);
        QMessageBox errorDialog;
        errorDialog.setIcon(QMessageBox::Critical);
        errorDialog.setWindowTitle("Address error");
        errorDialog.setText(strQText);
        errorDialog.setStandardButtons(QMessageBox::Ok);
        errorDialog.setDefaultButton(QMessageBox::Ok);
        if(errorDialog.exec() == QMessageBox::Ok) { } // no other button to choose, acknowledged
    }
}

void LookupAddressDialog::searchButtonClicked()
{
    searchAddress();
}

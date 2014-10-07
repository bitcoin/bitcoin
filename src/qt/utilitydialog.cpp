// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "utilitydialog.h"

#include "ui_paperwalletdialog.h"
#include "ui_helpmessagedialog.h"

#include "bitcoinunits.h"

#ifdef ENABLE_WALLET
#include "sendcoinsdialog.h"
#include "sendcoinsentry.h"
#include "coincontrol.h"
#include "coincontroldialog.h"
#endif

#include "optionsmodel.h"
#include "bitcoingui.h"
#include "clientmodel.h"
#include "guiutil.h"

#include "init.h"
#include "version.h"
#include "random.h"

#include <stdio.h>

#include <QCloseEvent>
#include <QLabel>
#include <QFont>
#include <QRegExp>
#include <QVBoxLayout>
#include <QInputDialog>

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

#if QT_VERSION < 0x050000
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#else
// Use QT5's new modular classes
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#endif
#include <QPainter>
#include "walletmodel.h"


/** "PaperWallet" dialog box */
PaperWalletDialog::PaperWalletDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PaperWalletDialog)
{
    ui->setupUi(this);

    ui->buttonBox->addButton(tr("Close"), QDialogButtonBox::RejectRole);

    // Begin with a small bold monospace font for the textual version of the key and address.
    QFont font("Monospace");
    font.setBold(true);
    font.setStyleHint(QFont::TypeWriter);
    font.setPixelSize(1);
    ui->addressText->setFont(font);
    ui->privateKeyText->setFont(font);
    ui->addressText->setAlignment(Qt::AlignJustify);
    ui->privateKeyText->setAlignment(Qt::AlignJustify);

}

void PaperWalletDialog::setModel(WalletModel *model)
{
    RandAddSeed();
    this->model = model;
    this->on_getNewAddress_clicked();
}

PaperWalletDialog::~PaperWalletDialog()
{
    delete ui;
}

void PaperWalletDialog::on_getNewAddress_clicked()
{
    // Create a new private key
    CKey privKey;
    privKey.MakeNewKey(true);

    // Derive the public key
    CPubKey pubkey = privKey.GetPubKey();

    // Derive the public key hash
    CBitcoinAddress pubkeyhash;
    pubkeyhash.Set(pubkey.GetID());

    // Create String versions of each
    std::string myPrivKey = CBitcoinSecret(privKey).ToString();
    std::string myPubKey = HexStr(pubkey.begin(), pubkey.end());
    std::string myAddress = pubkeyhash.ToString();


#ifdef USE_QRCODE
    // Generate the address QR code
    QRcode *code = QRcode_encodeString(myAddress.c_str(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!code)
    {
        ui->addressQRCode->setText(tr("Error encoding Address into QR Code."));
        return;
    }
    QImage myImage = QImage(code->width, code->width, QImage::Format_ARGB32);
    myImage.fill(QColor(0,0,0,0));
    unsigned char *p = code->data;
    for (int y = 0; y < code->width; y++)
    {
        for (int x = 0; x < code->width; x++)
        {
            myImage.setPixel(x, y, ((*p & 1) ? 0xff000000 : 0x0));
            p++;
        }
    }
    QRcode_free(code);


    // Generate the private key QR code
    code = QRcode_encodeString(myPrivKey.c_str(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!code)
    {
        ui->privateKeyQRCode->setText(tr("Error encoding private key into QR Code."));
        return;
    }
    QImage myImagePriv = QImage(code->width, code->width, QImage::Format_ARGB32);
    myImagePriv.fill(QColor(0,0,0,0));
    p = code->data;
    for (int y = 0; y < code->width; y++)
    {
        for (int x = 0; x < code->width; x++)
        {
            myImagePriv.setPixel(x, y, ((*p & 1) ? 0xff000000 : 0x0));
            p++;
        }
    }
    QRcode_free(code);

    // Populate the QR Codes
    ui->addressQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(ui->addressQRCode->width(), ui->addressQRCode->height()));
    ui->privateKeyQRCode->setPixmap(QPixmap::fromImage(myImagePriv).scaled(ui->privateKeyQRCode->width(), ui->privateKeyQRCode->height()));
#endif

    // Populate the Texts
    ui->addressText->setText(myAddress.c_str());
    ui->privateKeyText->setText(tr(myPrivKey.c_str()));

    ui->publicKey->setHtml(myPubKey.c_str());

    // Update the fonts to fit the height of the wallet.
    // This should only really trigger the first time since the font size persists.
    double paperHeight = (double) ui->paperTemplate->height();
    double maxTextWidth = paperHeight * 0.99;   
    double minTextWidth = paperHeight * 0.95;
    int pixelSizeStep = 1;

    int addressTextLength = ui->addressText->fontMetrics().boundingRect(ui->addressText->text()).width();
    QFont font = ui->addressText->font();
    for(int i = 0; i < PAPER_WALLET_READJUST_LIMIT; i++) {
        if ( addressTextLength < minTextWidth) {
            font.setPixelSize(font.pixelSize() + pixelSizeStep);
            ui->addressText->setFont(font);
            addressTextLength = ui->addressText->fontMetrics().boundingRect(ui->addressText->text()).width();
        } else {
            break;
        }

    }
    if ( addressTextLength > maxTextWidth ) {
        font.setPixelSize(font.pixelSize() - pixelSizeStep);
        ui->addressText->setFont(font);
        addressTextLength = ui->addressText->fontMetrics().boundingRect(ui->addressText->text()).width();
    }

    int privateKeyTextLength = ui->privateKeyText->fontMetrics().boundingRect(ui->privateKeyText->text()).width();
    font = ui->privateKeyText->font();
    for(int i = 0; i < PAPER_WALLET_READJUST_LIMIT; i++) {
        if ( privateKeyTextLength < minTextWidth) {
            font.setPixelSize(font.pixelSize() + pixelSizeStep);
            ui->privateKeyText->setFont(font);
            privateKeyTextLength = ui->privateKeyText->fontMetrics().boundingRect(ui->privateKeyText->text()).width();
        } else {
            break;
        }
    }
    if ( privateKeyTextLength > maxTextWidth ) {
        font.setPixelSize(font.pixelSize() - pixelSizeStep);
        ui->privateKeyText->setFont(font);
        privateKeyTextLength = ui->privateKeyText->fontMetrics().boundingRect(ui->privateKeyText->text()).width();
    }

}

void PaperWalletDialog::on_printButton_clicked()
{

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *qpd = new QPrintDialog(&printer, this);

    qpd->setPrintRange(QAbstractPrintDialog::AllPages);

    QList<QString> recipientPubKeyHashes;

    if ( qpd->exec() != QDialog::Accepted ) {
        return;
    }

    // Hardcode these values
    printer.setOrientation(QPrinter::Portrait);
    printer.setPaperSize(QPrinter::A4);
    printer.setFullPage(true);

    QPainter painter;
    if (! painter.begin(&printer)) { // failed to open file
        QMessageBox::critical(this, "Printing Error", tr("failed to open file, is it writable?"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    int walletCount = ui->walletCount->currentIndex() + 1;
    int walletsPerPage = 4;

    int pageHeight = printer.pageRect().height() - PAPER_WALLET_PAGE_MARGIN;
    int walletHeight = ui->paperTemplate->height();
    double computedWalletHeight = 0.9 * pageHeight / walletsPerPage;
    double scale = computedWalletHeight / walletHeight;
    double walletPadding = pageHeight * 0.05 / (walletsPerPage - 1) / scale;

    QRegion walletRegion = QRegion(ui->paperTemplate->x(), ui->paperTemplate->y(),
    ui->paperTemplate->width(), ui->paperTemplate->height());
        painter.scale(scale, scale);

    for(int i = 0; i < walletCount; i++) {

        this->on_getNewAddress_clicked();
        QPoint point = QPoint(PAPER_WALLET_PAGE_MARGIN, (PAPER_WALLET_PAGE_MARGIN / 2) + ( i % walletsPerPage ) * (walletHeight + walletPadding));
        this->render(&painter, point, walletRegion);
        recipientPubKeyHashes.append(ui->addressText->text());

        if ( i % walletsPerPage == ( walletsPerPage - 1 ) ) {

            printer.newPage();

        }

    }

    painter.end();

#ifdef ENABLE_WALLET
    QStringList formatted;

    WalletModelTransaction *tx;
    while( true ) {
        bool ok;

        // Ask for an amount to send to each paper wallet. It might be better to try to use the BitcoinAmountField, but this works fine.
        double amountInput = QInputDialog::getDouble(this, tr("Load Paper Wallets"), tr("Please wait for wallets to print and verify readability.<br/>Enter the number of BTC you wish to send to each wallet:"), 0, 0, 2147483647, 8, &ok);

        if(!ok) {
            return;
        }


        WalletModel::UnlockContext ctx(this->model->requestUnlock());
        if(!ctx.isValid())
        {
            return;
        }

        QList<SendCoinsRecipient> recipients;
        quint64 amount = (quint64) ( amountInput * COIN );
        foreach(const QString &dest, recipientPubKeyHashes)
        {

            recipients.append(SendCoinsRecipient(dest,tr("Paper wallet %1").arg(dest), amount,""));
            formatted.append(tr("<b>%1</b> to Paper Wallet <span style='font-family: monospace;'>%2</span>").arg(QString::number(amountInput, 'f', 8), GUIUtil::HtmlEscape(dest)));

        }

        tx = new WalletModelTransaction(recipients);

        WalletModel::SendCoinsReturn prepareStatus;
        if (this->model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
            prepareStatus = this->model->prepareTransaction(*tx, CoinControlDialog::coinControl);
        else
            prepareStatus = this->model->prepareTransaction(*tx);

        if (prepareStatus.status == WalletModel::InvalidAddress) {
            QMessageBox::critical(this, tr("Send Coins"), tr("The recipient address is not valid, please recheck."), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::InvalidAmount) {
            QMessageBox::critical(this, tr("Send Coins"), tr("The amount to pay must be larger than 0"), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::AmountExceedsBalance) {
            QMessageBox::critical(this, tr("Send Coins"), tr("The amount exceeds your balance."), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::AmountWithFeeExceedsBalance) {
            QMessageBox::critical(this, tr("Send Coins"), tr("The total exceeds your balance when the transaction fee is included"), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::DuplicateAddress) {
            QMessageBox::critical(this, tr("Send Coins"), tr("Duplicate address found, can only send to each address once per send operation."), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::TransactionCreationFailed) {
            QMessageBox::critical(this, tr("Send Coins"), tr("Transaction creation failed!"), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::OK) {
            break;
        } else {
            delete tx;
            return;
        }

    }

   // Stolen from sendcoinsdialog.cpp
    qint64 txFee = tx->getTransactionFee();
    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if(txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    qint64 totalAmount = tx->getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    foreach(BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcoinUnits::formatWithUnit(u, totalAmount));
    }

    questionString.append(tr("Total Amount %1 (= %2)")
        .arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount))
        .arg(alternativeUnits.join(" " + tr("or") + " ")));

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins"),
        questionString.arg(formatted.join("<br />")),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        delete tx;
        return;
    }

    WalletModel::SendCoinsReturn sendStatus = this->model->sendCoins(*tx);

    if (sendStatus.status == WalletModel::TransactionCommitFailed) {
        QMessageBox::critical(this, tr("Send Coins"), tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."), QMessageBox::Ok, QMessageBox::Ok);
    }
    delete tx;
#endif
    return;

}

/** "Help message" or "About" dialog box */
HelpMessageDialog::HelpMessageDialog(QWidget *parent, bool about) :
    QDialog(parent),
    ui(new Ui::HelpMessageDialog)
{
    ui->setupUi(this);
    GUIUtil::restoreWindowGeometry("nHelpMessageDialogWindow", this->size(), this);

    QString version = tr("Bitcoin Core") + " " + tr("version") + " " + QString::fromStdString(FormatFullVersion());
    /* On x86 add a bit specifier to the version so that users can distinguish between
     * 32 and 64 bit builds. On other architectures, 32/64 bit may be more ambigious.
     */
#if defined(__x86_64__)
    version += " " + tr("(%1-bit)").arg(64);
#elif defined(__i386__ )
    version += " " + tr("(%1-bit)").arg(32);
#endif

    if (about)
    {
        setWindowTitle(tr("About Bitcoin Core"));

        /// HTML-format the license message from the core
        QString licenseInfo = QString::fromStdString(LicenseInfo());
        QString licenseInfoHTML = licenseInfo;
        // Make URLs clickable
        QRegExp uri("<(.*)>", Qt::CaseSensitive, QRegExp::RegExp2);
        uri.setMinimal(true); // use non-greedy matching
        licenseInfoHTML.replace(uri, "<a href=\"\\1\">\\1</a>");
        // Replace newlines with HTML breaks
        licenseInfoHTML.replace("\n\n", "<br><br>");

        ui->helpMessageLabel->setTextFormat(Qt::RichText);
        ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        text = version + "\n" + licenseInfo;
        ui->helpMessageLabel->setText(version + "<br><br>" + licenseInfoHTML);
        ui->helpMessageLabel->setWordWrap(true);
    } else {
        setWindowTitle(tr("Command-line options"));
        QString header = tr("Usage:") + "\n" +
            "  bitcoin-qt [" + tr("command-line options") + "]                     " + "\n";

        QString coreOptions = QString::fromStdString(HelpMessage(HMM_BITCOIN_QT));

        QString uiOptions = tr("UI options") + ":\n" +
            "  -choosedatadir            " + tr("Choose data directory on startup (default: 0)") + "\n" +
            "  -lang=<lang>              " + tr("Set language, for example \"de_DE\" (default: system locale)") + "\n" +
            "  -min                      " + tr("Start minimized") + "\n" +
            "  -rootcertificates=<file>  " + tr("Set SSL root certificates for payment request (default: -system-)") + "\n" +
            "  -splash                   " + tr("Show splash screen on startup (default: 1)");

        ui->helpMessageLabel->setFont(GUIUtil::bitcoinAddressFont());
        text = version + "\n" + header + "\n" + coreOptions + "\n" + uiOptions;
        ui->helpMessageLabel->setText(text);
    }
}

HelpMessageDialog::~HelpMessageDialog()
{
    GUIUtil::saveWindowGeometry("nHelpMessageDialogWindow", this);
    delete ui;
}

void HelpMessageDialog::printToConsole()
{
    // On other operating systems, the expected action is to print the message to the console.
    fprintf(stdout, "%s\n", qPrintable(text));
}

void HelpMessageDialog::showOrPrint()
{
#if defined(WIN32)
    // On Windows, show a message box, as there is no stderr/stdout in windowed applications
    exec();
#else
    // On other operating systems, print help text to console
    printToConsole();
#endif
}

void HelpMessageDialog::on_okButton_accepted()
{
    close();
}


/** "Shutdown" window */
ShutdownWindow::ShutdownWindow(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(new QLabel(
        tr("Bitcoin Core is shutting down...") + "<br /><br />" +
        tr("Do not shut down the computer until this window disappears.")));
    setLayout(layout);
}

void ShutdownWindow::showShutdownWindow(BitcoinGUI *window)
{
    if (!window)
        return;

    // Show a simple window indicating shutdown status
    QWidget *shutdownWindow = new ShutdownWindow();
    // We don't hold a direct pointer to the shutdown window after creation, so use
    // Qt::WA_DeleteOnClose to make sure that the window will be deleted eventually.
    shutdownWindow->setAttribute(Qt::WA_DeleteOnClose);
    shutdownWindow->setWindowTitle(window->windowTitle());

    // Center shutdown window at where main window was
    const QPoint global = window->mapToGlobal(window->rect().center());
    shutdownWindow->move(global.x() - shutdownWindow->width() / 2, global.y() - shutdownWindow->height() / 2);
    shutdownWindow->show();
}

void ShutdownWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

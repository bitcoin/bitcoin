<<<<<<< HEAD
// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
=======
// Copyright (c) 2011-2014 The Crowncoin developers
>>>>>>> origin/dirty-merge-dash-0.11.0
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "utilitydialog.h"

#include "ui_helpmessagedialog.h"

#include "crowncoingui.h"
#include "clientmodel.h"
#include "guiutil.h"

#include "clientversion.h"
#include "init.h"

#include <stdio.h>

#include <QCloseEvent>
#include <QLabel>
#include <QRegExp>
#include <QVBoxLayout>

/** "Help message" or "About" dialog box */
HelpMessageDialog::HelpMessageDialog(QWidget *parent, bool about) :
    QDialog(parent),
    ui(new Ui::HelpMessageDialog)
{
    ui->setupUi(this);
    GUIUtil::restoreWindowGeometry("nHelpMessageDialogWindow", this->size(), this);

<<<<<<< HEAD
    QString version = tr("Dash Core") + " " + tr("version") + " " + QString::fromStdString(FormatFullVersion());
    /* On x86 add a bit specifier to the version so that users can distinguish between
     * 32 and 64 bit builds. On other architectures, 32/64 bit may be more ambigious.
     */
=======
    // Set current copyright year
    ui->copyrightLabel->setText(tr("Copyright") + QString(" &copy; 2009-%1 ").arg(COPYRIGHT_YEAR) + tr("The Crowncoin developers"));
}

void AboutDialog::setModel(ClientModel *model)
{
    if(model)
    {
        QString version = model->formatFullVersion();
        /* On x86 add a bit specifier to the version so that users can distinguish between
         * 32 and 64 bit builds. On other architectures, 32/64 bit may be more ambigious.
         */
>>>>>>> origin/dirty-merge-dash-0.11.0
#if defined(__x86_64__)
    version += " " + tr("(%1-bit)").arg(64);
#elif defined(__i386__ )
    version += " " + tr("(%1-bit)").arg(32);
#endif

<<<<<<< HEAD
    if (about)
    {
        setWindowTitle(tr("About Dash Core"));

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
            "  dash-qt [" + tr("command-line options") + "]                     " + "\n";

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
=======
AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_buttonBox_accepted()
{
    close();
}

/** "Help message" dialog box */
HelpMessageDialog::HelpMessageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpMessageDialog)
{
    ui->setupUi(this);
    GUIUtil::restoreWindowGeometry("nHelpMessageDialogWindow", this->size(), this);

    header = tr("Crowncoin") + " " + tr("version") + " " +
        QString::fromStdString(FormatFullVersion()) + "\n\n" +
        tr("Usage:") + "\n" +
        "  crowncoin-qt [" + tr("command-line options") + "]                     " + "\n";

    coreOptions = QString::fromStdString(HelpMessage(HMM_CROWNCOIN_QT));

    uiOptions = tr("UI options") + ":\n" +
        "  -choosedatadir            " + tr("Choose data directory on startup (default: 0)") + "\n" +
        "  -lang=<lang>              " + tr("Set language, for example \"de_DE\" (default: system locale)") + "\n" +
        "  -min                      " + tr("Start minimized") + "\n" +
        "  -rootcertificates=<file>  " + tr("Set SSL root certificates for payment request (default: -system-)") + "\n" +
        "  -splash                   " + tr("Show splash screen on startup (default: 1)");

    ui->helpMessageLabel->setFont(GUIUtil::crowncoinAddressFont());

    // Set help message text
    ui->helpMessageLabel->setText(header + "\n" + coreOptions + "\n" + uiOptions);
>>>>>>> origin/dirty-merge-dash-0.11.0
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
<<<<<<< HEAD
ShutdownWindow::ShutdownWindow(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(new QLabel(
        tr("Dash Core is shutting down...") + "<br /><br />" +
        tr("Do not shut down the computer until this window disappears.")));
    setLayout(layout);
}

void ShutdownWindow::showShutdownWindow(BitcoinGUI *window)
=======
void ShutdownWindow::showShutdownWindow(CrowncoinGUI *window)
>>>>>>> origin/dirty-merge-dash-0.11.0
{
    if (!window)
        return;

    // Show a simple window indicating shutdown status
<<<<<<< HEAD
    QWidget *shutdownWindow = new ShutdownWindow();
    // We don't hold a direct pointer to the shutdown window after creation, so use
    // Qt::WA_DeleteOnClose to make sure that the window will be deleted eventually.
    shutdownWindow->setAttribute(Qt::WA_DeleteOnClose);
    shutdownWindow->setWindowTitle(window->windowTitle());
=======
    QWidget *shutdownWindow = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(new QLabel(
        tr("Crowncoin is shutting down...") + "<br /><br />" +
        tr("Do not shut down the computer until this window disappears.")));
    shutdownWindow->setLayout(layout);
>>>>>>> origin/dirty-merge-dash-0.11.0

    // Center shutdown window at where main window was
    const QPoint global = window->mapToGlobal(window->rect().center());
    shutdownWindow->move(global.x() - shutdownWindow->width() / 2, global.y() - shutdownWindow->height() / 2);
    shutdownWindow->show();
}

void ShutdownWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

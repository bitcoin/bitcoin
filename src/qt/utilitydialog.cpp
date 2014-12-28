// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "utilitydialog.h"

#include "ui_helpmessagedialog.h"

#include "bitcoingui.h"
#include "clientmodel.h"
#include "guiutil.h"

#include "clientversion.h"
#include "init.h"

#include <stdio.h>

#include <QCloseEvent>
#include <QLabel>
#include <QRegExp>
#include <QTextTable>
#include <QTextCursor>
#include <QVBoxLayout>

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

        ui->aboutMessage->setTextFormat(Qt::RichText);
        ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        text = version + "\n" + licenseInfo;
        ui->aboutMessage->setText(version + "<br><br>" + licenseInfoHTML);
        ui->aboutMessage->setWordWrap(true);
        ui->helpMessage->setVisible(false);
    } else {
        setWindowTitle(tr("Command-line options"));
        QTextCursor cursor(ui->helpMessage->document());
        cursor.insertText(version);
        cursor.insertBlock();
        cursor.insertText(tr("Usage:") + '\n' +
            "  bitcoin-qt [" + tr("command-line options") + "]\n");

        cursor.insertBlock();
        QTextTableFormat tf;
        tf.setBorderStyle(QTextFrameFormat::BorderStyle_None);
        tf.setCellPadding(2);
        QVector<QTextLength> widths;
        widths << QTextLength(QTextLength::PercentageLength, 20);
        widths << QTextLength(QTextLength::PercentageLength, 80);
        tf.setColumnWidthConstraints(widths);
        QTextTable *table = cursor.insertTable(2, 2, tf);

        QString coreOptions = QString::fromStdString(HelpMessage(HMM_BITCOIN_QT));
        bool first = true;
        QTextCharFormat bold;
        bold.setFontWeight(QFont::Bold);
        // note that coreOptions is not translated.
        foreach (const QString &line, coreOptions.split('\n')) {
            if (!first) {
                table->appendRows(1);
                cursor.movePosition(QTextCursor::NextRow);
            }
            first = false;

            if (line.startsWith("  ")) {
                int index = line.indexOf(' ', 3);
                if (index > 0) {
                    cursor.insertText(line.left(index).trimmed());
                    cursor.movePosition(QTextCursor::NextCell);
                    cursor.insertText(line.mid(index).trimmed());
                    continue;
                }
            }
            cursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor);
            table->mergeCells(cursor);
            cursor.insertText(line.trimmed(), bold);
        }

        table->appendRows(6);
        cursor.movePosition(QTextCursor::NextRow);
        cursor.insertText(tr("UI options") + ":", bold);
        cursor.movePosition(QTextCursor::NextRow);
        cursor.insertText("-choosedatadir");
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText(tr("Choose data directory on startup (default: 0)"));
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText("-lang=<lang>");
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText(tr("Set language, for example \"de_DE\" (default: system locale)"));
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText("-min");
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText(tr("Start minimized"));
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText("-rootcertificates=<file>");
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText(tr("Set SSL root certificates for payment request (default: -system-)"));
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText("-splash");
        cursor.movePosition(QTextCursor::NextCell);
        cursor.insertText(tr("Show splash screen on startup (default: 1)"));

        ui->helpMessage->moveCursor(QTextCursor::Start);
        ui->scrollArea->setVisible(false);
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

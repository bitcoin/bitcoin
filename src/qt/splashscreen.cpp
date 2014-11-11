// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "splashscreen.h"

#include "clientversion.h"
#include "init.h"
#include "networkstyle.h"
#include "ui_interface.h"
#include "util.h"
#include "version.h"

#ifdef ENABLE_WALLET
#include "wallet.h"
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPainter>

SplashScreen::SplashScreen(Qt::WindowFlags f, const NetworkStyle *networkStyle) :
    QWidget(0, f), curAlignment(0)
{
    // set reference point, paddings
    int paddingRight            = 50;
    int paddingTop              = 50;
    int titleVersionVSpace      = 17;
    int titleCopyrightVSpace    = 40;

    float fontFactor            = 1.0;

    // define text to place
    QString titleText       = tr("Bitcoin Core");
    QString versionText     = QString("Version %1").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText   = QChar(0xA9)+QString(" 2009-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Bitcoin Core developers"));
    QString titleAddText    = networkStyle->getTitleAddText();

    QString font            = "Arial";

    // load the bitmap for writing some text over it
    pixmap     = networkStyle->getSplashImage();

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(QColor(100,100,100));

    // check font size and drawing with
    pixPaint.setFont(QFont(font, 33*fontFactor));
    QFontMetrics fm = pixPaint.fontMetrics();
    int titleTextWidth  = fm.width(titleText);
    if(titleTextWidth > 160) {
        // strange font rendering, Arial probably not found
        fontFactor = 0.75;
    }

    pixPaint.setFont(QFont(font, 33*fontFactor));
    fm = pixPaint.fontMetrics();
    titleTextWidth  = fm.width(titleText);
    pixPaint.drawText(pixmap.width()-titleTextWidth-paddingRight,paddingTop,titleText);

    pixPaint.setFont(QFont(font, 15*fontFactor));

    // if the version string is to long, reduce size
    fm = pixPaint.fontMetrics();
    int versionTextWidth  = fm.width(versionText);
    if(versionTextWidth > titleTextWidth+paddingRight-10) {
        pixPaint.setFont(QFont(font, 10*fontFactor));
        titleVersionVSpace -= 5;
    }
    pixPaint.drawText(pixmap.width()-titleTextWidth-paddingRight+2,paddingTop+titleVersionVSpace,versionText);

    // draw copyright stuff
    pixPaint.setFont(QFont(font, 10*fontFactor));
    pixPaint.drawText(pixmap.width()-titleTextWidth-paddingRight,paddingTop+titleCopyrightVSpace,copyrightText);

    // draw additional text if special network
    if(!titleAddText.isEmpty()) {
        QFont boldFont = QFont(font, 10*fontFactor);
        boldFont.setWeight(QFont::Bold);
        pixPaint.setFont(boldFont);
        fm = pixPaint.fontMetrics();
        int titleAddTextWidth  = fm.width(titleAddText);
        pixPaint.drawText(pixmap.width()-titleAddTextWidth-10,15,titleAddText);
    }

    pixPaint.end();

    // Set window title
    setWindowTitle(titleText + " " + titleAddText);

    // Resize window and move to center of desktop, disallow resizing
    QRect r(QPoint(), pixmap.size());
    resize(r.size());
    setFixedSize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());

    subscribeToCoreSignals();
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();
}

void SplashScreen::slotFinish(QWidget *mainWin)
{
    Q_UNUSED(mainWin);
    hide();
}

void SplashScreen::InitMessage(const std::string &message)
{
    QMetaObject::invokeMethod(this, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom|Qt::AlignHCenter),
        Q_ARG(QColor, QColor(55,55,55)));
}

void SplashScreen::ShowProgress(const std::string &title, int nProgress)
{
    InitMessage(title + strprintf("%d", nProgress) + "%");
}

#ifdef ENABLE_WALLET
void SplashScreen::ConnectWallet(CWallet* wallet)
{
    wallet->ShowProgress.Connect(this, & SplashScreen::ShowProgress);
}
#endif

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.InitMessage.Connect(this, &SplashScreen::InitMessage);
    uiInterface.ShowProgress.Connect(this, &SplashScreen::ShowProgress);
#ifdef ENABLE_WALLET
    uiInterface.LoadWallet.Connect(this, &SplashScreen::ConnectWallet);
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.InitMessage.Disconnect(this, &SplashScreen::InitMessage);
    uiInterface.ShowProgress.Disconnect(this, &SplashScreen::ShowProgress);
#ifdef ENABLE_WALLET
	uiInterface.LoadWallet.Disconnect(this, &SplashScreen::ConnectWallet);
    if(pwalletMain)
		pwalletMain->ShowProgress.Disconnect(this, &SplashScreen::ShowProgress);
#endif
}

void SplashScreen::showMessage(const QString &message, int alignment, const QColor &color)
{
    curMessage = message;
    curAlignment = alignment;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);
    QRect r = rect().adjusted(5, 5, -5, -5);
    painter.setPen(curColor);
    painter.drawText(r, curAlignment, curMessage);
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
    StartShutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}

#include "splashscreen.h"
#include "clientversion.h"
#include "util.h"

#include <QPainter>
#include <QApplication>

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f) :
    QSplashScreen(pixmap, f)
{
    // set reference point, paddings
    int paddingRight            = 50;
    int paddingTop              = 50;
    int titleVersionVSpace      = 17;
    int titleCopyrightVSpace    = 40;

    // define text to place
    QString titleText       = QString(QApplication::applicationName()).replace(QString("-testnet"), QString(""), Qt::CaseSensitive);
    QString versionText     = QString("Version %1").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText   = QChar(0xA9)+QString(" 2009-%1\n").arg(COPYRIGHT_YEAR) + QString(tr("The Bitcoin developers"));
    QString testnetAddText  = QString(tr("[testnet]"));

    // load the bitmap for writing some text over it
    QPixmap newPixmap = this->pixmap();

    QPainter pixPaint(&newPixmap);
    pixPaint.setPen(QColor(100,100,100));

    pixPaint.setFont(QFont("Helvetica", 33));
    QFontMetrics fm = pixPaint.fontMetrics();
    int titleTextWidth  = fm.width(titleText);
    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingRight,paddingTop,titleText);

    pixPaint.setFont(QFont("Helvetica", 15));

    // if the version string is to long, reduce size
    fm = pixPaint.fontMetrics();
    int versionTextWidth  = fm.width(versionText);
    if(versionTextWidth > titleTextWidth+paddingRight-10) {
        pixPaint.setFont(QFont("Helvetica", 10));
        titleVersionVSpace -= 5;
    }
    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingRight+2,paddingTop+titleVersionVSpace,versionText);

    // draw copyright stuff
    pixPaint.setFont(QFont("Helvetica", 10));
    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingRight,paddingTop+titleCopyrightVSpace,copyrightText);

    // draw testnet string if -testnet is on
    if(QApplication::applicationName().contains(QString("-testnet"))) {
        // draw copyright stuff
        QFont boldFont = QFont("Helvetica", 10);
        boldFont.setWeight(QFont::Bold);
        pixPaint.setFont(boldFont);
        fm = pixPaint.fontMetrics();
        int testnetAddTextWidth  = fm.width(testnetAddText);
        pixPaint.drawText(newPixmap.width()-testnetAddTextWidth-10,15,testnetAddText);
    }

    pixPaint.end();

    this->setPixmap(newPixmap);
}

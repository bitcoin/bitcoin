#include "splashscreen.h"
#include "clientversion.h"


#include <QPainter>
#include <QApplication>

#include "util.h"
SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f) :
    QSplashScreen(pixmap, f)
{
    QPixmap newPixmap;
    if(GetBoolArg("-testnet")) {
        newPixmap     = QPixmap(":/images/splash_testnet");
    }
    else {
        newPixmap     = QPixmap(":/images/splash");
    }

    this->setPixmap(newPixmap);
}

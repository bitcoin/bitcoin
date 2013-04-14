#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>

class SplashScreen : public QSplashScreen
{
    Q_OBJECT
public:
    explicit SplashScreen(const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = 0);
signals:
    
public slots:
    
};

#endif // SPLASHSCREEN_H

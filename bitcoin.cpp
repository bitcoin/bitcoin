/*
 * W.J. van der Laan 2011
 */
#include "bitcoingui.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    BitcoinGUI window;

    window.show();

    /* Depending on settings: QApplication::setQuitOnLastWindowClosed(false); */

    return app.exec();
}

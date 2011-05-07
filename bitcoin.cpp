/*
 * W.J. van der Laan 2011
 */
#include "BitcoinGUI.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    BitcoinGUI window;

    window.show();

    return app.exec();
}

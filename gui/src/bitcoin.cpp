/*
 * W.J. van der Laan 2011
 */
#include "bitcoingui.h"
#include "util.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    /* Testing on testnet */
    fTestNet = true;

    BitcoinGUI window;
    window.setBalance(1234.567890);
    window.setNumConnections(4);
    window.setNumTransactions(4);
    window.setNumBlocks(33);
    window.setAddress("123456789");

    window.show();

    /* Depending on settings: QApplication::setQuitOnLastWindowClosed(false); */

    return app.exec();
}

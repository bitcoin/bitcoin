/*
 * W.J. van der Laan 2011
 */
#include "bitcoingui.h"
#include "clientmodel.h"
#include "util.h"
#include "init.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    try {
        if(AppInit2(argc, argv))
        {
            ClientModel model;
            BitcoinGUI window;
            window.setModel(&model);

            window.show();

            /* Depending on settings: QApplication::setQuitOnLastWindowClosed(false); */
            int retval = app.exec();

            Shutdown(NULL);

            return retval;
        }
        else
        {
            return 1;
        }
    } catch (std::exception& e) {
        PrintException(&e, "Runaway exception");
    } catch (...) {
        PrintException(NULL, "Runaway exception");
    }
}

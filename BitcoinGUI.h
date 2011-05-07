#ifndef H_BITCOINGUI
#define H_BITCOINGUI

#include <QMainWindow>

class BitcoinGUI : public QMainWindow
{
    Q_OBJECT
public:
    BitcoinGUI(QWidget *parent = 0);
    
    /* Transaction table tab indices */
    enum {
        ALL_TRANSACTIONS = 0,
        SENT_RECEIVED = 1,
        SENT = 2,
        RECEIVED = 3
    } TabIndex;
private slots:
    void currentChanged(int tab);
};

#endif

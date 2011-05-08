#ifndef BITCOINGUI_H
#define BITCOINGUI_H

#include <QMainWindow>

class BitcoinGUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit BitcoinGUI(QWidget *parent = 0);
    
    /* Transaction table tab indices */
    enum {
        AllTransactions = 0,
        SentReceived = 1,
        Sent = 2,
        Received = 3
    } TabIndex;
private slots:
    void currentChanged(int tab);
};

#endif

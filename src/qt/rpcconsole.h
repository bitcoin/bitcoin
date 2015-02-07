#ifndef RPCCONSOLE_H
#define RPCCONSOLE_H

#include <QWidget>

namespace Ui {
    class RPCConsole;
}
class ClientModel;

/** Local Bitcoin RPC console. */
class RPCConsole: public QWidget
{
    Q_OBJECT

public:
    explicit RPCConsole(QWidget *parent = 0);
    ~RPCConsole();

    void setClientModel(ClientModel *model);

    enum MessageClass {
        MC_ERROR,
        MC_DEBUG,
        CMD_REQUEST,
        CMD_REPLY,
        CMD_ERROR
    };

protected:
    virtual bool eventFilter(QObject* obj, QEvent *event);
    void keyPressEvent(QKeyEvent *);

private slots:
    void on_lineEdit_returnPressed();
    void on_tabWidget_currentChanged(int index);
    /** open the debug.log from the current datadir */
    void on_openDebugLogfileButton_clicked();
    /** open the novacoin.conf from the current datadir */
    void on_openConfigurationfileButton_clicked();
    /** change the time range of the network traffic graph */
    void on_sldGraphRange_valueChanged(int value);
    /** update traffic statistics */
    void updateTrafficStats(quint64 totalBytesIn, quint64 totalBytesOut);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    /** display messagebox with program parameters (same as bitcoin-qt --help) */
    void on_showCLOptionsButton_clicked();

public slots:
    void clear();
    void message(int category, const QString &message, bool html = false);
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set number of blocks shown in the UI */
    void setNumBlocks(int count, int countOfPeers);
    /** Go forward or back in history */
    void browseHistory(int offset);
    /** Scroll console view to end */
    void scrollToEnd();
signals:
    // For RPC command executor
    void stopExecutor();
    void cmdRequest(const QString &command);

private:
    static QString FormatBytes(quint64 bytes);
    void setTrafficGraphRange(int mins);
    /** show detailed information on ui about selected node */

    enum ColumnWidths
    {
        ADDRESS_COLUMN_WIDTH = 200,
        SUBVERSION_COLUMN_WIDTH = 100,
        PING_COLUMN_WIDTH = 80
    };

    Ui::RPCConsole *ui;
    ClientModel *clientModel;
    QStringList history;
    int historyPtr;

    void startExecutor();
};

#endif // RPCCONSOLE_H

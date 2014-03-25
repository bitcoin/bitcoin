// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_rpcconsole.h"
#include "ui_bitcoin_rpcconsole.h"

#include "clientmodel.h"
#include "guiutil.h"
#include "peertablemodel.h"

#include "main.h"
#include "rpcserver.h"
#include "rpcclient.h"
#include "util.h"

#include "json/json_spirit_value.h"
#include <openssl/crypto.h>
#include <QKeyEvent>
#include <QScrollBar>
#include <QThread>
#include <QTime>

#if QT_VERSION < 0x050000
#include <QUrl>
#endif

const QSize ICON_SIZE(24, 24);

const int INITIAL_TRAFFIC_GRAPH_MINS = 30;

/* Object for executing console RPC commands in a separate thread.
*/
class Bitcoin_RPCExecutor : public QObject
{
    Q_OBJECT

public slots:
    void request(const QString &command);

signals:
    void reply(int category, const QString &command);
};

#include "bitcoin_rpcconsole.moc"

/**
 * Split shell command line into a list of arguments. Aims to emulate \c bash and friends.
 *
 * - Arguments are delimited with whitespace
 * - Extra whitespace at the beginning and end and between arguments will be ignored
 * - Text can be "double" or 'single' quoted
 * - The backslash \c \ is used as escape character
 *   - Outside quotes, any character can be escaped
 *   - Within double quotes, only escape \c " and backslashes before a \c " or another backslash
 *   - Within single quotes, no escaping is possible and no special interpretation takes place
 *
 * @param[out]   args        Parsed arguments will be appended to this list
 * @param[in]    strCommand  Command line to split
 */
bool bitcoin_parseCommandLine(std::vector<std::string> &args, const std::string &strCommand)
{
    enum CmdParseState
    {
        STATE_EATING_SPACES,
        STATE_ARGUMENT,
        STATE_SINGLEQUOTED,
        STATE_DOUBLEQUOTED,
        STATE_ESCAPE_OUTER,
        STATE_ESCAPE_DOUBLEQUOTED
    } state = STATE_EATING_SPACES;
    std::string curarg;
    foreach(char ch, strCommand)
    {
        switch(state)
        {
        case STATE_ARGUMENT: // In or after argument
        case STATE_EATING_SPACES: // Handle runs of whitespace
            switch(ch)
            {
            case '"': state = STATE_DOUBLEQUOTED; break;
            case '\'': state = STATE_SINGLEQUOTED; break;
            case '\\': state = STATE_ESCAPE_OUTER; break;
            case ' ': case '\n': case '\t':
                if(state == STATE_ARGUMENT) // Space ends argument
                {
                    args.push_back(curarg);
                    curarg.clear();
                }
                state = STATE_EATING_SPACES;
                break;
            default: curarg += ch; state = STATE_ARGUMENT;
            }
            break;
        case STATE_SINGLEQUOTED: // Single-quoted string
            switch(ch)
            {
            case '\'': state = STATE_ARGUMENT; break;
            default: curarg += ch;
            }
            break;
        case STATE_DOUBLEQUOTED: // Double-quoted string
            switch(ch)
            {
            case '"': state = STATE_ARGUMENT; break;
            case '\\': state = STATE_ESCAPE_DOUBLEQUOTED; break;
            default: curarg += ch;
            }
            break;
        case STATE_ESCAPE_OUTER: // '\' outside quotes
            curarg += ch; state = STATE_ARGUMENT;
            break;
        case STATE_ESCAPE_DOUBLEQUOTED: // '\' in double-quoted text
            if(ch != '"' && ch != '\\') curarg += '\\'; // keep '\' for everything but the quote and '\' itself
            curarg += ch; state = STATE_DOUBLEQUOTED;
            break;
        }
    }
    switch(state) // final state
    {
    case STATE_EATING_SPACES:
        return true;
    case STATE_ARGUMENT:
        args.push_back(curarg);
        return true;
    default: // ERROR to end in one of the other states
        return false;
    }
}

void Bitcoin_RPCExecutor::request(const QString &command)
{
    std::vector<std::string> args;
    if(!bitcoin_parseCommandLine(args, command.toStdString()))
    {
        emit reply(Bitcoin_RPCConsole::CMD_ERROR, QString("Parse error: unbalanced ' or \""));
        return;
    }
    if(args.empty())
        return; // Nothing to do
    try
    {
        std::string strPrint;
        // Convert argument list to JSON objects in method-dependent way,
        // and pass it along with the method name to the dispatcher.
        json_spirit::Value result = tableRPC.execute(
            args[0],
            RPCConvertValues(args[0], std::vector<std::string>(args.begin() + 1, args.end())));

        // Format result reply
        if (result.type() == json_spirit::null_type)
            strPrint = "";
        else if (result.type() == json_spirit::str_type)
            strPrint = result.get_str();
        else
            strPrint = write_string(result, true);

        emit reply(Bitcoin_RPCConsole::CMD_REPLY, QString::fromStdString(strPrint));
    }
    catch (json_spirit::Object& objError)
    {
        try // Nice formatting for standard-format error
        {
            int code = find_value(objError, "code").get_int();
            std::string message = find_value(objError, "message").get_str();
            emit reply(Bitcoin_RPCConsole::CMD_ERROR, QString::fromStdString(message) + " (code " + QString::number(code) + ")");
        }
        catch(std::runtime_error &) // raised when converting to invalid type, i.e. missing code or message
        {   // Show raw JSON object
            emit reply(Bitcoin_RPCConsole::CMD_ERROR, QString::fromStdString(write_string(json_spirit::Value(objError), false)));
        }
    }
    catch (std::exception& e)
    {
        emit reply(Bitcoin_RPCConsole::CMD_ERROR, QString("Error: ") + QString::fromStdString(e.what()));
    }
}

Bitcoin_RPCConsole::Bitcoin_RPCConsole(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Bitcoin_RPCConsole),
    clientModel(0)
{
    detailNodeStats = CNodeCombinedStats();
    detailNodeStats.nodestats.nodeid = -1;
    detailNodeStats.statestats.nMisbehavior = -1;

    ui->setupUi(this);
    GUIUtil::restoreWindowGeometry("nBitcoin_RPCConsoleWindow", this->size(), this);

    connect(ui->btnClearTrafficGraph, SIGNAL(clicked()), ui->trafficGraph, SLOT(clear()));

    // set OpenSSL version label
    ui->openSSLVersion->setText(SSLeay_version(SSLEAY_VERSION));

    startExecutor();
    setTrafficGraphRange(INITIAL_TRAFFIC_GRAPH_MINS);
    ui->detailWidget->hide();
}

Bitcoin_RPCConsole::~Bitcoin_RPCConsole()
{
    GUIUtil::saveWindowGeometry("nBitcoin_RPCConsoleWindow", this);
    emit stopExecutor();
    delete ui;
}

void Bitcoin_RPCConsole::setClientModel(ClientModel *model)
{
    clientModel = model;
    ui->trafficGraph->setClientModel(model);
    if(model)
    {
        // Keep up to date with client
        setNumConnections(model->getNumConnections());
        connect(model, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(model->getNumBlocks());
        connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

        updateTrafficStats(model->getTotalBytesRecv(), model->getTotalBytesSent());
        connect(model, SIGNAL(bytesChanged(quint64,quint64)), this, SLOT(updateTrafficStats(quint64, quint64)));

        // set up peer table
        ui->peerWidget->setModel(model->getPeerTableModel());
        ui->peerWidget->verticalHeader()->hide();
        ui->peerWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->peerWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->peerWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->peerWidget->setColumnWidth(PeerTableModel::Address, ADDRESS_COLUMN_WIDTH);
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(ui->peerWidget, MINIMUM_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH);

        // connect the peerWidget's selection model to our peerSelected() handler
        QItemSelectionModel *peerSelectModel = ui->peerWidget->selectionModel();
        connect(peerSelectModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
             this, SLOT(peerSelected(const QItemSelection &, const QItemSelection &)));
        connect(model->getPeerTableModel(), SIGNAL(layoutChanged()), this, SLOT(peerLayoutChanged()));

        // Provide initial values
        ui->clientVersion->setText(model->bitcoin_formatVersion());
        ui->clientName->setText(model->bitcoin_clientName());

        ui->networkName->setText(model->getNetworkName());
    }
}

void Bitcoin_RPCConsole::reject()
{
    // Ignore escape keypress if this is not a seperate window
    if(windowType() != Qt::Widget)
        QDialog::reject();
}

void Bitcoin_RPCConsole::setNumConnections(int count)
{
    if (!clientModel)
        return;

    QString connections = QString::number(count) + " (";
    connections += tr("In:") + " " + QString::number(clientModel->getNumConnections(CONNECTIONS_IN)) + " / ";
    connections += tr("Out:") + " " + QString::number(clientModel->getNumConnections(CONNECTIONS_OUT)) + ")";

    ui->numberOfConnections->setText(connections);
}

void Bitcoin_RPCConsole::setNumBlocks(int count)
{
    ui->numberOfBlocks->setText(QString::number(count));
    if(clientModel)
        ui->lastBlockTime->setText(clientModel->getLastBlockDate().toString());
}

void Bitcoin_RPCConsole::startExecutor()
{
    QThread *thread = new QThread;
    Bitcoin_RPCExecutor *executor = new Bitcoin_RPCExecutor();
    executor->moveToThread(thread);

    // Replies from executor object must go to this object
    connect(executor, SIGNAL(reply(int,QString)), this, SLOT(message(int,QString)));
    // Requests from this object must go to executor
    connect(this, SIGNAL(cmdRequest(QString)), executor, SLOT(request(QString)));

    // On stopExecutor signal
    // - queue executor for deletion (in execution thread)
    // - quit the Qt event loop in the execution thread
    connect(this, SIGNAL(stopExecutor()), executor, SLOT(deleteLater()));
    connect(this, SIGNAL(stopExecutor()), thread, SLOT(quit()));
    // Queue the thread for deletion (in this thread) when it is finished
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    // Default implementation of QThread::run() simply spins up an event loop in the thread,
    // which is what we want.
    thread->start();
}

void Bitcoin_RPCConsole::on_sldGraphRange_valueChanged(int value)
{
    const int multiplier = 5; // each position on the slider represents 5 min
    int mins = value * multiplier;
    setTrafficGraphRange(mins);
}

QString Bitcoin_RPCConsole::FormatBytes(quint64 bytes)
{
    if(bytes < 1024)
        return QString(tr("%1 B")).arg(bytes);
    if(bytes < 1024 * 1024)
        return QString(tr("%1 KB")).arg(bytes / 1024);
    if(bytes < 1024 * 1024 * 1024)
        return QString(tr("%1 MB")).arg(bytes / 1024 / 1024);

    return QString(tr("%1 GB")).arg(bytes / 1024 / 1024 / 1024);
}

void Bitcoin_RPCConsole::setTrafficGraphRange(int mins)
{
    ui->trafficGraph->setGraphRangeMins(mins);
    ui->lblGraphRange->setText(GUIUtil::formatDurationStr(mins * 60));
}

void Bitcoin_RPCConsole::updateTrafficStats(quint64 totalBytesIn, quint64 totalBytesOut)
{
    ui->lblBytesIn->setText(FormatBytes(totalBytesIn));
    ui->lblBytesOut->setText(FormatBytes(totalBytesOut));
}

void Bitcoin_RPCConsole::peerSelected(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    if (selected.indexes().isEmpty())
        return;

    // mark the cached banscore as unknown
    detailNodeStats.statestats.nMisbehavior = -1;

    const CNodeCombinedStats *stats = clientModel->getPeerTableModel()->getNodeStats(selected.indexes().first().row());

    if (stats)
    {
        detailNodeStats.nodestats.nodeid = stats->nodestats.nodeid;
        updateNodeDetail(stats);
        ui->detailWidget->show();
        ui->detailWidget->setDisabled(false);
    }
}

void Bitcoin_RPCConsole::peerLayoutChanged()
{
    const CNodeCombinedStats *stats = NULL;
    bool fUnselect = false, fReselect = false, fDisconnected = false;

    if (detailNodeStats.nodestats.nodeid == -1)
        // no node selected yet
        return;

    // find the currently selected row
    int selectedRow;
    QModelIndexList selectedModelIndex = ui->peerWidget->selectionModel()->selectedIndexes();
    if (selectedModelIndex.isEmpty())
        selectedRow = -1;
    else
        selectedRow = selectedModelIndex.first().row();

    // check if our detail node has a row in the table (it may not necessarily
    // be at selectedRow since its position can change after a layout change)
    int detailNodeRow = clientModel->getPeerTableModel()->getRowByNodeId(detailNodeStats.nodestats.nodeid);

    if (detailNodeRow < 0)
    {
        // detail node dissapeared from table (node disconnected)
        fUnselect = true;
        fDisconnected = true;
        detailNodeStats.nodestats.nodeid = 0;
    }
    else
    {
        if (detailNodeRow != selectedRow)
        {
            // detail node moved position
            fUnselect = true;
            fReselect = true;
        }

        // get fresh stats on the detail node.
        stats = clientModel->getPeerTableModel()->getNodeStats(detailNodeRow);
    }

    if (fUnselect && selectedRow >= 0)
    {
        ui->peerWidget->selectionModel()->select(QItemSelection(selectedModelIndex.first(), selectedModelIndex.last()),
                                                 QItemSelectionModel::Deselect);
    }

    if (fReselect)
    {
        ui->peerWidget->selectRow(detailNodeRow);
    }

    if (stats)
        updateNodeDetail(stats);

    if (fDisconnected)
    {
        ui->peerHeading->setText(QString(tr("Peer Disconnected")));
        ui->detailWidget->setDisabled(true);
        QDateTime dt = QDateTime::fromTime_t(detailNodeStats.nodestats.nLastSend);
        if (detailNodeStats.nodestats.nLastSend)
            ui->peerLastSend->setText(dt.toString("yyyy-MM-dd hh:mm:ss"));
        dt.setTime_t(detailNodeStats.nodestats.nLastRecv);
        if (detailNodeStats.nodestats.nLastRecv)
            ui->peerLastRecv->setText(dt.toString("yyyy-MM-dd hh:mm:ss"));
        dt.setTime_t(detailNodeStats.nodestats.nTimeConnected);
        ui->peerConnTime->setText(dt.toString("yyyy-MM-dd hh:mm:ss"));
    }
}

void Bitcoin_RPCConsole::updateNodeDetail(const CNodeCombinedStats *combinedStats)
{
    CNodeStats stats = combinedStats->nodestats;

    // keep a copy of timestamps, used to display dates upon disconnect
    detailNodeStats.nodestats.nLastSend = stats.nLastSend;
    detailNodeStats.nodestats.nLastRecv = stats.nLastRecv;
    detailNodeStats.nodestats.nTimeConnected = stats.nTimeConnected;

    // update the detail ui with latest node information
    ui->peerHeading->setText(QString("<b>%1</b>").arg(tr("Node Detail")));
    ui->peerAddr->setText(QString(stats.addrName.c_str()));
    ui->peerServices->setText(GUIUtil::formatServicesStr(stats.nServices));
    ui->peerLastSend->setText(stats.nLastSend ? GUIUtil::formatDurationStr(GetTime() - stats.nLastSend) : tr("never"));
    ui->peerLastRecv->setText(stats.nLastRecv ? GUIUtil::formatDurationStr(GetTime() - stats.nLastRecv) : tr("never"));
    ui->peerBytesSent->setText(FormatBytes(stats.nSendBytes));
    ui->peerBytesRecv->setText(FormatBytes(stats.nRecvBytes));
    ui->peerConnTime->setText(GUIUtil::formatDurationStr(GetTime() - stats.nTimeConnected));
    ui->peerPingTime->setText(stats.dPingTime == 0 ? tr("N/A") : QString(tr("%1 secs")).arg(QString::number(stats.dPingTime, 'f', 3)));
    ui->peerVersion->setText(QString("%1").arg(stats.nVersion));
    ui->peerSubversion->setText(QString(stats.cleanSubVer.c_str()));
    ui->peerDirection->setText(stats.fInbound ? tr("Inbound") : tr("Outbound"));
    ui->peerHeight->setText(QString("%1").arg(stats.nStartingHeight));
    ui->peerSyncNode->setText(stats.fSyncNode ? tr("Yes") : tr("No"));

    // if we can, display the peer's ban score
    CNodeStateStats statestats = combinedStats->statestats;
    if (statestats.nMisbehavior >= 0)
    {
        // we have a new nMisbehavor value - update the cache
        detailNodeStats.statestats.nMisbehavior = statestats.nMisbehavior;
    }

    // pull the ban score from cache.  -1 means it hasn't been retrieved yet (lock busy).
    if (detailNodeStats.statestats.nMisbehavior >= 0)
        ui->peerBanScore->setText(QString("%1").arg(detailNodeStats.statestats.nMisbehavior));
    else
        ui->peerBanScore->setText(tr("Fetching..."));
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void Bitcoin_RPCConsole::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(PeerTableModel::Address);
}

void Bitcoin_RPCConsole::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // peerWidget needs a resize in case the dialog has non-default geometry
    columnResizingFixer->stretchColumnWidth(PeerTableModel::Address);

    // start PeerTableModel auto refresh
    clientModel->getPeerTableModel()->startAutoRefresh(1000);
}

void Bitcoin_RPCConsole::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    // stop PeerTableModel auto refresh
    clientModel->getPeerTableModel()->stopAutoRefresh();
}

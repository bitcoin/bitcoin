#include "rpcconsole.h"
#include "ui_rpcconsole.h"

#include "clientmodel.h"
#include "bitcoinrpc.h"
#include "guiutil.h"

#include <QTime>
#include <QTimer>
#include <QThread>
#include <QTextEdit>
#include <QKeyEvent>
#include <QUrl>
#include <QScrollBar>

#include <boost/tokenizer.hpp>
#include <openssl/crypto.h>

// TODO: make it possible to filter out categories (esp debug messages when implemented)
// TODO: receive errors and debug messages through ClientModel

const int CONSOLE_SCROLLBACK = 50;
const int CONSOLE_HISTORY = 50;

const QSize ICON_SIZE(24, 24);

const struct {
    const char *url;
    const char *source;
} ICON_MAPPING[] = {
    {"cmd-request", ":/icons/tx_input"},
    {"cmd-reply", ":/icons/tx_output"},
    {"cmd-error", ":/icons/tx_output"},
    {"misc", ":/icons/tx_inout"},
    {NULL, NULL}
};

/* Object for executing console RPC commands in a separate thread.
*/
class RPCExecutor: public QObject
{
    Q_OBJECT
public slots:
    void start();
    void request(const QString &command);
signals:
    void reply(int category, const QString &command);
};

#include "rpcconsole.moc"

void RPCExecutor::start()
{
   // Nothing to do
}

void RPCExecutor::request(const QString &command)
{
    // Parse shell-like command line into separate arguments
    std::string strMethod;
    std::vector<std::string> strParams;
    try {
        boost::escaped_list_separator<char> els('\\',' ','\"');
        std::string strCommand = command.toStdString();
        boost::tokenizer<boost::escaped_list_separator<char> > tok(strCommand, els);

        int n = 0;
        for(boost::tokenizer<boost::escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end();++beg,++n)
        {
            if(n == 0) // First parameter is the command
                strMethod = *beg;
            else
                strParams.push_back(*beg);
        }
    }
    catch(boost::escaped_list_error &e)
    {
        emit reply(RPCConsole::CMD_ERROR, QString("Parse error"));
        return;
    }

    try {
        std::string strPrint;
        json_spirit::Value result = tableRPC.execute(strMethod, RPCConvertValues(strMethod, strParams));

        // Format result reply
        if (result.type() == json_spirit::null_type)
            strPrint = "";
        else if (result.type() == json_spirit::str_type)
            strPrint = result.get_str();
        else
            strPrint = write_string(result, true);

        emit reply(RPCConsole::CMD_REPLY, QString::fromStdString(strPrint));
    }
    catch (json_spirit::Object& objError)
    {
        emit reply(RPCConsole::CMD_ERROR, QString::fromStdString(write_string(json_spirit::Value(objError), false)));
    }
    catch (std::exception& e)
    {
        emit reply(RPCConsole::CMD_ERROR, QString("Error: ") + QString::fromStdString(e.what()));
    }
}

RPCConsole::RPCConsole(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RPCConsole),
    historyPtr(0)
{
    ui->setupUi(this);

#ifndef Q_WS_MAC
    ui->openDebugLogfileButton->setIcon(QIcon(":/icons/export"));
    ui->showCLOptionsButton->setIcon(QIcon(":/icons/options"));
#endif

    // Install event filter for up and down arrow
    ui->lineEdit->installEventFilter(this);

    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // set OpenSSL version label
    ui->openSSLVersion->setText(SSLeay_version(SSLEAY_VERSION));

    startExecutor();

    clear();
}

RPCConsole::~RPCConsole()
{
    emit stopExecutor();
    delete ui;
}

bool RPCConsole::eventFilter(QObject* obj, QEvent *event)
{
    if(obj == ui->lineEdit)
    {
        if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent *key = static_cast<QKeyEvent*>(event);
            switch(key->key())
            {
            case Qt::Key_Up: browseHistory(-1); return true;
            case Qt::Key_Down: browseHistory(1); return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void RPCConsole::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Subscribe to information, replies, messages, errors
        connect(model, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));
        connect(model, SIGNAL(numBlocksChanged(int,int)), this, SLOT(setNumBlocks(int,int)));

        // Provide initial values
        ui->clientVersion->setText(model->formatFullVersion());
        ui->clientName->setText(model->clientName());
        ui->buildDate->setText(model->formatBuildDate());
        ui->startupTime->setText(model->formatClientStartupTime());

        setNumConnections(model->getNumConnections());
        ui->isTestNet->setChecked(model->isTestNet());

        setNumBlocks(model->getNumBlocks(), model->getNumBlocksOfPeers());
    }
}

static QString categoryClass(int category)
{
    switch(category)
    {
    case RPCConsole::CMD_REQUEST:  return "cmd-request"; break;
    case RPCConsole::CMD_REPLY:    return "cmd-reply"; break;
    case RPCConsole::CMD_ERROR:    return "cmd-error"; break;
    default:                       return "misc";
    }
}

void RPCConsole::clear()
{
    ui->messagesWidget->clear();
    ui->lineEdit->clear();
    ui->lineEdit->setFocus();

    // Add smoothly scaled icon images.
    // (when using width/height on an img, Qt uses nearest instead of linear interpolation)
    for(int i=0; ICON_MAPPING[i].url; ++i)
    {
        ui->messagesWidget->document()->addResource(
                    QTextDocument::ImageResource,
                    QUrl(ICON_MAPPING[i].url),
                    QImage(ICON_MAPPING[i].source).scaled(ICON_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    // Set default style sheet
    ui->messagesWidget->document()->setDefaultStyleSheet(
                "table { }"
                "td.time { color: #808080; padding-top: 3px; } "
                "td.message { font-family: Monospace; font-size: 12px; } "
                "td.cmd-request { color: #006060; } "
                "td.cmd-error { color: red; } "
                "b { color: #006060; } "
                );

    message(CMD_REPLY, (tr("Welcome to the Bitcoin RPC console.") + "<br>" +
                        tr("Use up and down arrows to navigate history, and <b>Ctrl-L</b> to clear screen.") + "<br>" +
                        tr("Type <b>help</b> for an overview of available commands.")), true);
}

void RPCConsole::message(int category, const QString &message, bool html)
{
    QTime time = QTime::currentTime();
    QString timeString = time.toString();
    QString out;
    out += "<table><tr><td class=\"time\" width=\"65\">" + timeString + "</td>";
    out += "<td class=\"icon\" width=\"32\"><img src=\"" + categoryClass(category) + "\"></td>";
    out += "<td class=\"message " + categoryClass(category) + "\" valign=\"middle\">";
    if(html)
        out += message;
    else
        out += GUIUtil::HtmlEscape(message, true);
    out += "</td></tr></table>";
    ui->messagesWidget->append(out);
}

void RPCConsole::setNumConnections(int count)
{
    ui->numberOfConnections->setText(QString::number(count));
}

void RPCConsole::setNumBlocks(int count, int countOfPeers)
{
    ui->numberOfBlocks->setText(QString::number(count));
    ui->totalBlocks->setText(QString::number(countOfPeers));
    if(clientModel)
    {
        // If there is no current number available display N/A instead of 0, which can't ever be true
        ui->totalBlocks->setText(clientModel->getNumBlocksOfPeers() == 0 ? tr("N/A") : QString::number(clientModel->getNumBlocksOfPeers()));
        ui->lastBlockTime->setText(clientModel->getLastBlockDate().toString());
    }
}

void RPCConsole::on_lineEdit_returnPressed()
{
    QString cmd = ui->lineEdit->text();
    ui->lineEdit->clear();

    if(!cmd.isEmpty())
    {
        message(CMD_REQUEST, cmd);
        emit cmdRequest(cmd);
        // Truncate history from current position
        history.erase(history.begin() + historyPtr, history.end());
        // Append command to history
        history.append(cmd);
        // Enforce maximum history size
        while(history.size() > CONSOLE_HISTORY)
            history.removeFirst();
        // Set pointer to end of history
        historyPtr = history.size();
        // Scroll console view to end
        scrollToEnd();
    }
}

void RPCConsole::browseHistory(int offset)
{
    historyPtr += offset;
    if(historyPtr < 0)
        historyPtr = 0;
    if(historyPtr > history.size())
        historyPtr = history.size();
    QString cmd;
    if(historyPtr < history.size())
        cmd = history.at(historyPtr);
    ui->lineEdit->setText(cmd);
}

void RPCConsole::startExecutor()
{
    QThread* thread = new QThread;
    RPCExecutor *executor = new RPCExecutor();
    executor->moveToThread(thread);

    // Notify executor when thread started (in executor thread)
    connect(thread, SIGNAL(started()), executor, SLOT(start()));
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

void RPCConsole::on_tabWidget_currentChanged(int index)
{
    if(ui->tabWidget->widget(index) == ui->tab_console)
    {
        ui->lineEdit->setFocus();
    }
}

void RPCConsole::on_openDebugLogfileButton_clicked()
{
    GUIUtil::openDebugLogfile();
}

void RPCConsole::scrollToEnd()
{
    QScrollBar *scrollbar = ui->messagesWidget->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void RPCConsole::on_showCLOptionsButton_clicked()
{
    GUIUtil::HelpMessageBox help;
    help.exec();
}

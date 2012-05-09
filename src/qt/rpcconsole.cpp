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

#include <boost/tokenizer.hpp>

// TODO: make it possible to filter out categories (esp debug messages when implemented)
// TODO: receive errors and debug messages through ClientModel

const int CONSOLE_SCROLLBACK = 50;
const int CONSOLE_HISTORY = 50;

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
    boost::escaped_list_separator<char> els('\\',' ','\"');
    std::string strCommand = command.toStdString();
    boost::tokenizer<boost::escaped_list_separator<char> > tok(strCommand, els);

    std::string strMethod;
    std::vector<std::string> strParams;
    int n = 0;
    for(boost::tokenizer<boost::escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end();++beg,++n)
    {
        if(n == 0) // First parameter is the command
            strMethod = *beg;
        else
            strParams.push_back(*beg);
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
    firstLayout(true),
    historyPtr(0)
{
    ui->setupUi(this);
    ui->messagesWidget->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    ui->messagesWidget->setContextMenuPolicy(Qt::ActionsContextMenu);

    // Install event filter for up and down arrow
    ui->lineEdit->installEventFilter(this);

    // Add "Copy message" to context menu explicitly
    QAction *copyMessageAction = new QAction(tr("&Copy"), this);
    copyMessageAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    copyMessageAction->setShortcutContext(Qt::WidgetShortcut);
    connect(copyMessageAction, SIGNAL(triggered()), this, SLOT(copyMessage()));
    ui->messagesWidget->addAction(copyMessageAction);

    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

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
        connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

        // Provide initial values
        ui->clientVersion->setText(model->formatFullVersion());
        ui->clientName->setText(model->clientName());
        ui->buildDate->setText(model->formatBuildDate());

        setNumConnections(model->getNumConnections());
        ui->isTestNet->setChecked(model->isTestNet());

        setNumBlocks(model->getNumBlocks());
    }
}

static QColor categoryColor(int category)
{
    switch(category)
    {
    case RPCConsole::MC_ERROR:     return QColor(255,0,0); break;
    case RPCConsole::MC_DEBUG:     return QColor(192,192,192); break;
    case RPCConsole::CMD_REQUEST:  return QColor(128,128,128); break;
    case RPCConsole::CMD_REPLY:    return QColor(128,255,128); break;
    case RPCConsole::CMD_ERROR:    return QColor(255,128,128); break;
    default:           return QColor(0,0,0);
    }
}

void RPCConsole::clear()
{
    ui->messagesWidget->clear();
    ui->messagesWidget->setRowCount(0);
    ui->lineEdit->clear();
    ui->lineEdit->setFocus();

    message(CMD_REPLY, tr("Welcome to the bitcoin RPC console.")+"\n"+
                       tr("Use up and down arrows to navigate history, and Ctrl-L to clear screen.")+"\n"+
                       tr("Type \"help\" for an overview of available commands."));
}

void RPCConsole::message(int category, const QString &message)
{
    // Add row to messages widget
    int row = ui->messagesWidget->rowCount();
    ui->messagesWidget->setRowCount(row+1);

    QTime time = QTime::currentTime();
    QTableWidgetItem *newTime = new QTableWidgetItem(time.toString());
    newTime->setData(Qt::DecorationRole, categoryColor(category));
    newTime->setForeground(QColor(128,128,128));
    newTime->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled); // make non-editable

    int numLines = message.count("\n") + 1;
    // As Qt doesn't like very tall cells (they break scrolling) keep only short messages in
    // the cell text, longer messages trigger a display widget with scroll bar
    if(numLines < 5)
    {
        QTableWidgetItem *newItem = new QTableWidgetItem(message);
        newItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled); // make non-editable
        if(category == CMD_ERROR) // Coloring error messages in red
            newItem->setForeground(QColor(255,16,16));
        ui->messagesWidget->setItem(row, 1, newItem);
    } else {
        QTextEdit *newWidget = new QTextEdit;
        newWidget->setText(message);
        newWidget->setMaximumHeight(100);
        newWidget->setReadOnly(true);
        ui->messagesWidget->setCellWidget(row, 1, newWidget);
    }

    ui->messagesWidget->setItem(row, 0, newTime);
    ui->messagesWidget->resizeRowToContents(row);
    // Preserve only limited scrollback buffer
    while(ui->messagesWidget->rowCount() > CONSOLE_SCROLLBACK)
        ui->messagesWidget->removeRow(0);
    // Scroll to bottom after table is updated
    QTimer::singleShot(0, ui->messagesWidget, SLOT(scrollToBottom()));
}

void RPCConsole::setNumConnections(int count)
{
    ui->numberOfConnections->setText(QString::number(count));
}

void RPCConsole::setNumBlocks(int count)
{
    ui->numberOfBlocks->setText(QString::number(count));
    if(clientModel)
    {
        ui->totalBlocks->setText(QString::number(clientModel->getNumBlocksOfPeers()));
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

void RPCConsole::copyMessage()
{
    GUIUtil::copyEntryData(ui->messagesWidget, 1, Qt::EditRole);
}

void RPCConsole::on_tabWidget_currentChanged(int index)
{
    if(ui->tabWidget->widget(index) == ui->tab_console)
    {
        if(firstLayout)
        {
            // Work around QTableWidget issue:
            // Call resizeRowsToContents on first Layout request with widget visible,
            // to make sure multiline messages that were added before the console was shown
            // have the right height.
            firstLayout = false;
            ui->messagesWidget->resizeRowsToContents();
        }
        ui->lineEdit->setFocus();
    }
}

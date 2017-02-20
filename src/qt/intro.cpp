// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "intro.h"
#include "ui_intro.h"

#include "guiutil.h"

#include "util.h"

#include <boost/filesystem.hpp>

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

#include <cmath>

static const uint64_t GB_BYTES = 1000000000LL;
/* Minimum free space (in GB) needed for data directory */
static const uint64_t BLOCK_CHAIN_SIZE = 120;
/* Minimum free space (in GB) needed for data directory when pruned; Does not include prune target */
static const uint64_t CHAIN_STATE_SIZE = 2;
/* Total required space (in GB) depending on user choice (prune, not prune) */
static uint64_t requiredSpace;

/* Check free space asynchronously to prevent hanging the UI thread.

   Up to one request to check a path is in flight to this thread; when the check()
   function runs, the current path is requested from the associated Intro object.
   The reply is sent back through a signal.

   This ensures that no queue of checking requests is built up while the user is
   still entering the path, and that always the most recently entered path is checked as
   soon as the thread becomes available.
*/
class FreespaceChecker : public QObject
{
    Q_OBJECT

public:
    FreespaceChecker(Intro *intro);

    enum Status {
        ST_OK,
        ST_ERROR
    };

public Q_SLOTS:
    void check();

Q_SIGNALS:
    void reply(int status, const QString &message, quint64 available);

private:
    Intro *intro;
};

#include "intro.moc"

FreespaceChecker::FreespaceChecker(Intro *_intro)
{
    this->intro = _intro;
}

void FreespaceChecker::check()
{
    namespace fs = boost::filesystem;
    QString dataDirStr = intro->getPathToCheck();
    fs::path dataDir = GUIUtil::qstringToBoostPath(dataDirStr);
    uint64_t freeBytesAvailable = 0;
    int replyStatus = ST_OK;
    QString replyMessage = tr("A new data directory will be created.");

    /* Find first parent that exists, so that fs::space does not fail */
    fs::path parentDir = dataDir;
    fs::path parentDirOld = fs::path();
    while(parentDir.has_parent_path() && !fs::exists(parentDir))
    {
        parentDir = parentDir.parent_path();

        /* Check if we make any progress, break if not to prevent an infinite loop here */
        if (parentDirOld == parentDir)
            break;

        parentDirOld = parentDir;
    }

    try {
        freeBytesAvailable = fs::space(parentDir).available;
        if(fs::exists(dataDir))
        {
            if(fs::is_directory(dataDir))
            {
                QString separator = "<code>" + QDir::toNativeSeparators("/") + tr("name") + "</code>";
                replyStatus = ST_OK;
                replyMessage = tr("Directory already exists. Add %1 if you intend to create a new directory here.").arg(separator);
            } else {
                replyStatus = ST_ERROR;
                replyMessage = tr("Path already exists, and is not a directory.");
            }
        }
    } catch (const fs::filesystem_error&)
    {
        /* Parent directory does not exist or is not accessible */
        replyStatus = ST_ERROR;
        replyMessage = tr("Cannot create data directory here.");
    }
    Q_EMIT reply(replyStatus, replyMessage, freeBytesAvailable);
}


Intro::Intro(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Intro),
    thread(0),
    signalled(false)
{
    ui->setupUi(this);
    ui->welcomeLabel->setText(ui->welcomeLabel->text().arg(tr(PACKAGE_NAME)));
    ui->storageLabel->setText(ui->storageLabel->text().arg(tr(PACKAGE_NAME)));

    ui->lblExplanation1->setText(ui->lblExplanation1->text()
        .arg(tr(PACKAGE_NAME))
        .arg(BLOCK_CHAIN_SIZE)
        .arg(2009)
        .arg(tr("Bitcoin"))
    );
    ui->lblExplanation2->setText(ui->lblExplanation2->text().arg(tr(PACKAGE_NAME)));

    uint64_t pruneTarget = std::max<int64_t>(0, GetArg("-prune", 0));
    requiredSpace = BLOCK_CHAIN_SIZE;
    QString storageRequiresMsg = tr("At least %1 GB of data will be stored in this directory, and it will grow over time.");
    if (pruneTarget) {
        uint64_t prunedGBs = std::ceil(pruneTarget * 1024 * 1024.0 / GB_BYTES);
        if (prunedGBs <= requiredSpace) {
            requiredSpace = prunedGBs;
            storageRequiresMsg = tr("Approximately %1 GB of data will be stored in this directory.");
        }
        ui->lblExplanation3->setVisible(true);
    } else {
        ui->lblExplanation3->setVisible(false);
    }
    requiredSpace += CHAIN_STATE_SIZE;
    ui->sizeWarningLabel->setText(
        tr("%1 will download and store a copy of the Bitcoin block chain.").arg(tr(PACKAGE_NAME)) + " " +
        storageRequiresMsg.arg(requiredSpace) + " " +
        tr("The wallet will also be stored in this directory.")
    );
    startThread();
}

Intro::~Intro()
{
    delete ui;
    /* Ensure thread is finished before it is deleted */
    Q_EMIT stopThread();
    thread->wait();
}

QString Intro::getDataDirectory()
{
    return ui->dataDirectory->text();
}

void Intro::setDataDirectory(const QString &dataDir)
{
    ui->dataDirectory->setText(dataDir);
    if(dataDir == getDefaultDataDirectory())
    {
        ui->dataDirDefault->setChecked(true);
        ui->dataDirectory->setEnabled(false);
        ui->ellipsisButton->setEnabled(false);
    } else {
        ui->dataDirCustom->setChecked(true);
        ui->dataDirectory->setEnabled(true);
        ui->ellipsisButton->setEnabled(true);
    }
}

QString Intro::getDefaultDataDirectory()
{
    return GUIUtil::boostPathToQString(GetDefaultDataDir());
}

bool Intro::pickDataDirectory()
{
    namespace fs = boost::filesystem;
    QSettings settings;
    /* If data directory provided on command line, no need to look at settings
       or show a picking dialog */
    if(!GetArg("-datadir", "").empty())
        return true;
    /* 1) Default data directory for operating system */
    QString dataDir = getDefaultDataDirectory();
    /* 2) Allow QSettings to override default dir */
    dataDir = settings.value("strDataDir", dataDir).toString();

    if(!fs::exists(GUIUtil::qstringToBoostPath(dataDir)) || GetBoolArg("-choosedatadir", DEFAULT_CHOOSE_DATADIR) || settings.value("fReset", false).toBool() || GetBoolArg("-resetguisettings", false))
    {
        /* If current default data directory does not exist, let the user choose one */
        Intro intro;
        intro.setDataDirectory(dataDir);
        intro.setWindowIcon(QIcon(":icons/bitcoin"));

        while(true)
        {
            if(!intro.exec())
            {
                /* Cancel clicked */
                return false;
            }
            dataDir = intro.getDataDirectory();
            try {
                TryCreateDirectory(GUIUtil::qstringToBoostPath(dataDir));
                break;
            } catch (const fs::filesystem_error&) {
                QMessageBox::critical(0, tr(PACKAGE_NAME),
                    tr("Error: Specified data directory \"%1\" cannot be created.").arg(dataDir));
                /* fall through, back to choosing screen */
            }
        }

        settings.setValue("strDataDir", dataDir);
        settings.setValue("fReset", false);
    }
    /* Only override -datadir if different from the default, to make it possible to
     * override -datadir in the bitcoin.conf file in the default data directory
     * (to be consistent with bitcoind behavior)
     */
    if(dataDir != getDefaultDataDirectory())
        SoftSetArg("-datadir", GUIUtil::qstringToBoostPath(dataDir).string()); // use OS locale for path setting
    return true;
}

void Intro::setStatus(int status, const QString &message, quint64 bytesAvailable)
{
    switch(status)
    {
    case FreespaceChecker::ST_OK:
        ui->errorMessage->setText(message);
        ui->errorMessage->setStyleSheet("");
        break;
    case FreespaceChecker::ST_ERROR:
        ui->errorMessage->setText(tr("Error") + ": " + message);
        ui->errorMessage->setStyleSheet("QLabel { color: #800000 }");
        break;
    }
    /* Indicate number of bytes available */
    if(status == FreespaceChecker::ST_ERROR)
    {
        ui->freeSpace->setText("");
    } else {
        QString freeString = tr("%n GB of free space available", "", bytesAvailable/GB_BYTES);
        if(bytesAvailable < requiredSpace * GB_BYTES)
        {
            freeString += " " + tr("(of %n GB needed)", "", requiredSpace);
            ui->freeSpace->setStyleSheet("QLabel { color: #800000 }");
        } else {
            ui->freeSpace->setStyleSheet("");
        }
        ui->freeSpace->setText(freeString + ".");
    }
    /* Don't allow confirm in ERROR state */
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(status != FreespaceChecker::ST_ERROR);
}

void Intro::on_dataDirectory_textChanged(const QString &dataDirStr)
{
    /* Disable OK button until check result comes in */
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    checkPath(dataDirStr);
}

void Intro::on_ellipsisButton_clicked()
{
    QString dir = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(0, "Choose data directory", ui->dataDirectory->text()));
    if(!dir.isEmpty())
        ui->dataDirectory->setText(dir);
}

void Intro::on_dataDirDefault_clicked()
{
    setDataDirectory(getDefaultDataDirectory());
}

void Intro::on_dataDirCustom_clicked()
{
    ui->dataDirectory->setEnabled(true);
    ui->ellipsisButton->setEnabled(true);
}

void Intro::startThread()
{
    thread = new QThread(this);
    FreespaceChecker *executor = new FreespaceChecker(this);
    executor->moveToThread(thread);

    connect(executor, SIGNAL(reply(int,QString,quint64)), this, SLOT(setStatus(int,QString,quint64)));
    connect(this, SIGNAL(requestCheck()), executor, SLOT(check()));
    /*  make sure executor object is deleted in its own thread */
    connect(this, SIGNAL(stopThread()), executor, SLOT(deleteLater()));
    connect(this, SIGNAL(stopThread()), thread, SLOT(quit()));

    thread->start();
}

void Intro::checkPath(const QString &dataDir)
{
    mutex.lock();
    pathToCheck = dataDir;
    if(!signalled)
    {
        signalled = true;
        Q_EMIT requestCheck();
    }
    mutex.unlock();
}

QString Intro::getPathToCheck()
{
    QString retval;
    mutex.lock();
    retval = pathToCheck;
    signalled = false; /* new request can be queued now */
    mutex.unlock();
    return retval;
}

// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparams.h>
#include <fs.h>
#include <qt/intro.h>
#include <qt/forms/ui_intro.h>

#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>

#include <interfaces/node.h>
#include <util/system.h>
#include <validation.h>

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

#include <cmath>

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
    explicit FreespaceChecker(Intro *intro);

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

#include <qt/intro.moc>

FreespaceChecker::FreespaceChecker(Intro *_intro)
{
    this->intro = _intro;
}

void FreespaceChecker::check()
{
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

namespace {
//! Return pruning size that will be used if automatic pruning is enabled.
int GetPruneTargetGB()
{
    int64_t prune_target_mib = gArgs.GetIntArg("-prune", 0);
    // >1 means automatic pruning is enabled by config, 1 means manual pruning, 0 means no pruning.
    return prune_target_mib > 1 ? PruneMiBtoGB(prune_target_mib) : DEFAULT_PRUNE_TARGET_GB;
}
} // namespace

Intro::Intro(QWidget *parent, int64_t blockchain_size_gb, int64_t chain_state_size_gb) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::Intro),
    thread(nullptr),
    signalled(false),
    m_blockchain_size_gb(blockchain_size_gb),
    m_chain_state_size_gb(chain_state_size_gb),
    m_prune_target_gb{GetPruneTargetGB()}
{
    ui->setupUi(this);
    ui->welcomeLabel->setText(ui->welcomeLabel->text().arg(PACKAGE_NAME));
    ui->storageLabel->setText(ui->storageLabel->text().arg(PACKAGE_NAME));

    ui->lblExplanation1->setText(ui->lblExplanation1->text()
        .arg(PACKAGE_NAME)
        .arg(m_blockchain_size_gb)
        .arg(2009)
        .arg(tr("Bitcoin"))
    );
    ui->lblExplanation2->setText(ui->lblExplanation2->text().arg(PACKAGE_NAME));

    const int min_prune_target_GB = std::ceil(MIN_DISK_SPACE_FOR_BLOCK_FILES / 1e9);
    ui->pruneGB->setRange(min_prune_target_GB, std::numeric_limits<int>::max());
    if (gArgs.GetIntArg("-prune", 0) > 1) { // -prune=1 means enabled, above that it's a size in MiB
        ui->prune->setChecked(true);
        ui->prune->setEnabled(false);
    }
    ui->pruneGB->setValue(m_prune_target_gb);
    ui->pruneGB->setToolTip(ui->prune->toolTip());
    ui->lblPruneSuffix->setToolTip(ui->prune->toolTip());
    UpdatePruneLabels(ui->prune->isChecked());

    connect(ui->prune, &QCheckBox::toggled, [this](bool prune_checked) {
        UpdatePruneLabels(prune_checked);
        UpdateFreeSpaceLabel();
    });
    connect(ui->pruneGB, qOverload<int>(&QSpinBox::valueChanged), [this](int prune_GB) {
        m_prune_target_gb = prune_GB;
        UpdatePruneLabels(ui->prune->isChecked());
        UpdateFreeSpaceLabel();
    });

    startThread();
}

Intro::~Intro()
{
    delete ui;
    /* Ensure thread is finished before it is deleted */
    thread->quit();
    thread->wait();
}

QString Intro::getDataDirectory()
{
    return ui->dataDirectory->text();
}

void Intro::setDataDirectory(const QString &dataDir)
{
    ui->dataDirectory->setText(dataDir);
    if(dataDir == GUIUtil::getDefaultDataDirectory())
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

int64_t Intro::getPruneMiB() const
{
    switch (ui->prune->checkState()) {
    case Qt::Checked:
        return PruneGBtoMiB(m_prune_target_gb);
    case Qt::Unchecked: default:
        return 0;
    }
}

bool Intro::showIfNeeded(bool& did_show_intro, int64_t& prune_MiB)
{
    did_show_intro = false;

    QSettings settings;
    /* If data directory provided on command line, no need to look at settings
       or show a picking dialog */
    if(!gArgs.GetArg("-datadir", "").empty())
        return true;
    /* 1) Default data directory for operating system */
    QString dataDir = GUIUtil::getDefaultDataDirectory();
    /* 2) Allow QSettings to override default dir */
    dataDir = settings.value("strDataDir", dataDir).toString();

    if(!fs::exists(GUIUtil::qstringToBoostPath(dataDir)) || gArgs.GetBoolArg("-choosedatadir", DEFAULT_CHOOSE_DATADIR) || settings.value("fReset", false).toBool() || gArgs.GetBoolArg("-resetguisettings", false))
    {
        /* Use selectParams here to guarantee Params() can be used by node interface */
        try {
            SelectParams(gArgs.GetChainName());
        } catch (const std::exception&) {
            return false;
        }

        /* If current default data directory does not exist, let the user choose one */
        Intro intro(0, Params().AssumedBlockchainSize(), Params().AssumedChainStateSize());
        intro.setDataDirectory(dataDir);
        intro.setWindowIcon(QIcon(":icons/bitcoin"));
        did_show_intro = true;

        while(true)
        {
            if(!intro.exec())
            {
                /* Cancel clicked */
                return false;
            }
            dataDir = intro.getDataDirectory();
            try {
                if (TryCreateDirectories(GUIUtil::qstringToBoostPath(dataDir))) {
                    // If a new data directory has been created, make wallets subdirectory too
                    TryCreateDirectories(GUIUtil::qstringToBoostPath(dataDir) / "wallets");
                }
                break;
            } catch (const fs::filesystem_error&) {
                QMessageBox::critical(nullptr, PACKAGE_NAME,
                    tr("Error: Specified data directory \"%1\" cannot be created.").arg(dataDir));
                /* fall through, back to choosing screen */
            }
        }

        // Additional preferences:
        prune_MiB = intro.getPruneMiB();

        settings.setValue("strDataDir", dataDir);
        settings.setValue("fReset", false);
    }
    /* Only override -datadir if different from the default, to make it possible to
     * override -datadir in the bitcoin.conf file in the default data directory
     * (to be consistent with bitcoind behavior)
     */
    if(dataDir != GUIUtil::getDefaultDataDirectory()) {
        gArgs.SoftSetArg("-datadir", fs::PathToString(GUIUtil::qstringToBoostPath(dataDir))); // use OS locale for path setting
    }
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
        m_bytes_available = bytesAvailable;
        if (ui->prune->isEnabled()) {
            ui->prune->setChecked(m_bytes_available < (m_blockchain_size_gb + m_chain_state_size_gb + 10) * GB_BYTES);
        }
        UpdateFreeSpaceLabel();
    }
    /* Don't allow confirm in ERROR state */
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(status != FreespaceChecker::ST_ERROR);
}

void Intro::UpdateFreeSpaceLabel()
{
    QString freeString = tr("%1 GB of free space available").arg(m_bytes_available / GB_BYTES);
    if (m_bytes_available < m_required_space_gb * GB_BYTES) {
        freeString += " " + tr("(of %1 GB needed)").arg(m_required_space_gb);
        ui->freeSpace->setStyleSheet("QLabel { color: #800000 }");
    } else if (m_bytes_available / GB_BYTES - m_required_space_gb < 10) {
        freeString += " " + tr("(%1 GB needed for full chain)").arg(m_required_space_gb);
        ui->freeSpace->setStyleSheet("QLabel { color: #999900 }");
    } else {
        ui->freeSpace->setStyleSheet("");
    }
    ui->freeSpace->setText(freeString + ".");
}

void Intro::on_dataDirectory_textChanged(const QString &dataDirStr)
{
    /* Disable OK button until check result comes in */
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    checkPath(dataDirStr);
}

void Intro::on_ellipsisButton_clicked()
{
    QString dir = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(nullptr, "Choose data directory", ui->dataDirectory->text()));
    if(!dir.isEmpty())
        ui->dataDirectory->setText(dir);
}

void Intro::on_dataDirDefault_clicked()
{
    setDataDirectory(GUIUtil::getDefaultDataDirectory());
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

    connect(executor, &FreespaceChecker::reply, this, &Intro::setStatus);
    connect(this, &Intro::requestCheck, executor, &FreespaceChecker::check);
    /*  make sure executor object is deleted in its own thread */
    connect(thread, &QThread::finished, executor, &QObject::deleteLater);

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

void Intro::UpdatePruneLabels(bool prune_checked)
{
    m_required_space_gb = m_blockchain_size_gb + m_chain_state_size_gb;
    QString storageRequiresMsg = tr("At least %1 GB of data will be stored in this directory, and it will grow over time.");
    if (prune_checked && m_prune_target_gb <= m_blockchain_size_gb) {
        m_required_space_gb = m_prune_target_gb + m_chain_state_size_gb;
        storageRequiresMsg = tr("Approximately %1 GB of data will be stored in this directory.");
    }
    ui->lblExplanation3->setVisible(prune_checked);
    ui->pruneGB->setEnabled(prune_checked);
    static constexpr uint64_t nPowTargetSpacing = 10 * 60;  // from chainparams, which we don't have at this stage
    static constexpr uint32_t expected_block_data_size = 2250000;  // includes undo data
    const uint64_t expected_backup_days = m_prune_target_gb * 1e9 / (uint64_t(expected_block_data_size) * 86400 / nPowTargetSpacing);
    ui->lblPruneSuffix->setText(
        //: Explanatory text on the capability of the current prune target.
        tr("(sufficient to restore backups %n day(s) old)", "", expected_backup_days));
    ui->sizeWarningLabel->setText(
        tr("%1 will download and store a copy of the Bitcoin block chain.").arg(PACKAGE_NAME) + " " +
        storageRequiresMsg.arg(m_required_space_gb) + " " +
        tr("The wallet will also be stored in this directory.")
    );
    this->adjustSize();
}

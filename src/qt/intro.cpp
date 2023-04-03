// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <chainparams.h>
#include <qt/intro.h>
#include <qt/forms/ui_intro.h>
#include <util/chaintype.h>
#include <util/fs.h>

#include <qt/freespacechecker.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>

#include <common/args.h>
#include <interfaces/node.h>
#include <util/fs_helpers.h>
#include <validation.h>

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

#include <cmath>
#include <fstream>

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
    m_blockchain_size_gb(blockchain_size_gb),
    m_chain_state_size_gb(chain_state_size_gb),
    m_prune_target_gb{GetPruneTargetGB()}
{
    ui->setupUi(this);
    ui->welcomeLabel->setText(ui->welcomeLabel->text().arg(CLIENT_NAME));
    ui->storageLabel->setText(ui->storageLabel->text().arg(CLIENT_NAME));

    ui->lblExplanation1->setText(ui->lblExplanation1->text()
        .arg(CLIENT_NAME)
        .arg(m_blockchain_size_gb)
        .arg(2009)
        .arg(tr("Bitcoin"))
    );
    ui->lblExplanation2->setText(ui->lblExplanation2->text().arg(CLIENT_NAME));

    const int min_prune_target_GB = std::ceil(MIN_DISK_SPACE_FOR_BLOCK_FILES / 1e9);
    ui->pruneGB->setRange(min_prune_target_GB, std::numeric_limits<int>::max());
    if (const auto arg{gArgs.GetIntArg("-prune")}) {
        m_prune_checkbox_is_default = false;
        ui->prune->setChecked(*arg >= 1);
        ui->prune->setEnabled(false);
    }
    ui->pruneGB->setValue(m_prune_target_gb);
    ui->pruneGB->setToolTip(ui->prune->toolTip());
    ui->lblPruneSuffix->setToolTip(ui->prune->toolTip());
    UpdatePruneLabels(ui->prune->isChecked());

    connect(ui->prune, &QCheckBox::toggled, [this](bool prune_checked) {
        m_prune_checkbox_is_default = false;
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

// TODO move to common/init
// TODO write new file before renaming old so less fragile
// TODO choose better unique filename
bool SetInitialDataDir(const fs::path& default_datadir, const fs::path& datadir, std::string& error)
{
    assert(default_datadir.is_absolute());
    assert(datadir.is_absolute());
    const bool link_datadir{datadir == default_datadir};
    std::error_code ec;
    fs::file_status status{fs::symlink_status(default_datadir, ec)};
    if (ec) {
        error = strprintf("Could not read %s: %s", fs::quoted(fs::PathToString(default_datadir)), ec.message());
        return false;
    }
    if (status.type() != fs::file_type::not_found && (link_datadir || status.type() != fs::file_type::directory)) {
        fs::path prev_datadir{default_datadir};
        prev_datadir += strprintf(".%d.bak", GetTime());
        fs::rename(default_datadir, prev_datadir, ec);
        if (ec) {
            error = strprintf("Could not rename %s to %s: %s", fs::quoted(fs::PathToString(default_datadir)),
                              fs::quoted(fs::PathToString(prev_datadir)), ec.message());
            return false;
        }
    }
    if (link_datadir) {
        fs::create_directory_symlink(datadir, default_datadir, ec);
        if (ec) {
            if (ec != std::errc::operation_not_permitted) {
                LogPrintf("Could not create symlink to %s at %s: %s", fs::quoted(fs::PathToString(datadir)),
                          fs::quoted(fs::PathToString(default_datadir)), ec.message());
            }
            std::ofstream file;
            file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try {
                file.open(datadir);
                file << fs::PathToString(datadir) << std::endl;
            } catch (std::system_error& e) {
                ec = e.code();
            }
            if (ec) {
                error = strprintf("Could not write %s to %s: %s", fs::quoted(fs::PathToString(datadir)),
                                  fs::quoted(fs::PathToString(default_datadir)), ec.message());
                return false;
            }
        }
    } else {
        if (!CreateDataDir(datadir, error)) return false;
    }
    return true;
}

// TODO move low level code out of showIfNeeded to this function
// TODO move common/init, consolidate arguments/return value
fs::path GetInitialDataDir(const ArgsManager& args, bool& new_datadir, bool& custom_datadir, std::string& error)
{
    return {};
}

bool Intro::showIfNeeded(bool& did_show_intro, fs::path& datadir, int64_t& prune_MiB)
{
    assert(datadir.empty());

    // Show intro dialog if requested by settings or command line arguments.
    // Intro dialog will still be skipped, however, if this function is never
    // called, which will happen if an explicit -datadir value was passed on the
    // command line, and also if a -conf value with an absolute path was passed
    // on the command line and the configuration file contains a datadir= line.
    QSettings settings;
    bool show_intro{gArgs.GetBoolArg("-choosedatadir", DEFAULT_CHOOSE_DATADIR) ||
                    gArgs.GetBoolArg("-resetguisettings", false) || settings.value("fReset", false).toBool()};

    fs::path default_datadir = GetDefaultDataDir();
    std::error_code ec;
    fs::file_status status = fs::symlink_status(default_datadir, ec);
    if (ec) LogPrintf("Warning: could not read %s: %s", fs::quoted(fs::PathToString(default_datadir)), ec.message());
    enum DirType { DIR, LINK, NOT_FOUND } datadir_type{NOT_FOUND};
    if (status.type() == fs::file_type::directory) {
        datadir = default_datadir;
        datadir_type = DIR;
    } else if (status.type() == fs::file_type::symlink) {
        datadir = fs::read_symlink(default_datadir, ec);
        datadir_type = LINK;
        if (ec)
            LogPrintf("Warning: could not read symlink %s: %s", fs::quoted(fs::PathToString(default_datadir)),
                      ec.message());
    } else if (status.type() == fs::file_type::regular) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        std::string line;
        try {
            file.open(default_datadir);
            std::getline(file, line);
        } catch (std::system_error& e) {
            ec = e.code();
            LogPrintf("Warning: could not read file %s: %s", fs::quoted(fs::PathToString(default_datadir)),
                      ec.message());
        }
        datadir = fs::PathFromString(line);
        datadir_type = LINK;
    }
    if (datadir.empty()) {
        datadir = default_datadir;
        datadir_type = NOT_FOUND;
        if (!ec && status.type() != fs::file_type::not_found) ec = make_error_code(std::errc::not_a_directory);
    }

    // Check if there is a legacy QSettings "strDataDir" setting that should be
    // migrated.
    QVariant legacy_datadir_str{settings.value("strDataDir")};
    bool remove_legacy_setting{false};
    if (legacy_datadir_str.isValid()) {
        fs::path legacy_datadir{fs::PathFromString(legacy_datadir_str.toString().toStdString()).lexically_normal()};
        if (legacy_datadir.empty() || legacy_datadir == datadir || legacy_datadir == default_datadir) {
            // If the legacy datadir string is empty, or the same as the current
            // datadir, just discard the legacy value.
            remove_legacy_setting = true;
        } else if (datadir_type == NOT_FOUND) {
            // If there is no current datadir, use the legacy datadir.
            datadir = legacy_datadir;
            // If showing intro dialog, legacy setting will be shown in the
            // dialog and saved in the dialog is completed. If not showing
            // intro, try to save legacy datadir as default now. If it fails to
            // save, just log a warning. It will still be used this session, and
            // the legacy setting will be kept so there is a chance to retry the
            // next session.
            std::string error;
            if (show_intro) {
                remove_legacy_setting = true;
            } else if (SetInitialDataDir(default_datadir, datadir, error)) {
                remove_legacy_setting = true;
            } else {
                LogPrintf("Warning: failed to set %s as default data directory: %s",
                          fs::quoted(fs::PathToString(datadir)), error);
            }
        } else if (show_intro) {
            // If legacy datadir conflicts with current datadir, but the intro
            // dialog is going to be shown, just discard the legacy datadir if
            // the intro dialog is completed. instead of showing an extra dialog
            // before the intro.
            remove_legacy_setting = true;
        } else {
            // Show a dialog to choose between the legacy and current datadirs.
            QString gui_datadir{QString::fromStdString(fs::PathToString(legacy_datadir))};
            QString cli_datadir{QString::fromStdString(fs::PathToString(datadir.empty() ? default_datadir : datadir))};

            QMessageBox messagebox;
            messagebox.setWindowTitle(CLIENT_NAME);
            messagebox.setTextFormat(Qt::RichText);
            /*: Dialog text shown when a previous version of the GUI sets a
                different datadir path that the default CLI datadir path,
                letting the user choose which of the two datadirs to use. Newer
                version fo the GUI set a single shared datadir, so this dialog
                will only be shown when upgrading a legacy installation. */
            messagebox.setText(
                tr("The %1 graphical interface (GUI) is configured to use a different data directory than %1 "
                   "command line (CLI) tools.")
                    .arg(CLIENT_NAME));
            messagebox.setInformativeText(
                tr("<dl><dt>The GUI data directory is:</dt><dd>%1</dd></dl>"
                   "<dt>The CLI data directory is:</dt><dd>%2</dd></dl>"
                   "<p>Previous versions of the %3 GUI used the GUI default directory and ignored the CLI default "
                   "directory. This version allows choosing which directory to use. It is recommended to set a common "
                   "default so the GUI and CLI tools such as <code>%4</code>, <code>%5</code>, and <code>%6</code> "
                   "can "
                   "interoperate and this prompt can be avoided.</p>")
                    .arg(gui_datadir.toHtmlEscaped())
                    .arg(cli_datadir.toHtmlEscaped())
                    .arg(CLIENT_NAME)
                    .arg("bitcoind")
                    .arg("bitcoin-cli")
                    .arg("bitcoin-wallet"));
            QPushButton* use_gui =
                messagebox.addButton(tr("Use GUI data directory (legacy behavior)"), QMessageBox::AcceptRole);
            QPushButton* set_gui_default =
                messagebox.addButton(tr("Use GUI data directory and set as default"), QMessageBox::AcceptRole);
            QPushButton* use_cli = messagebox.addButton(tr("Use CLI data directory"), QMessageBox::AcceptRole);
            QPushButton* set_cli_default =
                messagebox.addButton(tr("Use CLI data directory and set as default"), QMessageBox::AcceptRole);
            QPushButton* choose_datadir = messagebox.addButton(
                tr("Choose another data directory and set as default..."), QMessageBox::AcceptRole);
            QPushButton* quit = messagebox.addButton(tr("Quit"), QMessageBox::AcceptRole);
            messagebox.findChild<QDialogButtonBox*>()->setOrientation(Qt::Vertical);
            messagebox.findChild<QDialogButtonBox*>()->setCenterButtons(true);
            messagebox.setStyleSheet("QPushButton { text-align: left; padding: .5em; } "
                                     "QDialogButtonBox { background-color: red; } ");
            messagebox.setDefaultButton(use_gui);
            messagebox.exec();
            QAbstractButton* clicked = messagebox.clickedButton();
            if (clicked == use_gui) {
                datadir = legacy_datadir;
                datadir_type = LINK;
            } else if (clicked == use_cli) {
                // Keep cli datadir.
            } else if (clicked == set_gui_default) {
                datadir = legacy_datadir;
                datadir_type = LINK;
                std::string error;
                if (SetInitialDataDir(default_datadir, datadir, error)) {
                    remove_legacy_setting = true;
                } else {
                    LogPrintf("Warning: failed to set %s as default data directory: %s",
                              fs::quoted(fs::PathToString(datadir)), error);
                }
            } else if (clicked == set_cli_default) {
                remove_legacy_setting = true;
            } else if (clicked == choose_datadir) {
                show_intro = true;
            } else {
                assert(clicked == quit);
                return false;
            }
        }
    }

    // If a default or explicit datadir does not exist just show the intro
    // dialog to confirm it should be created. But if a custom datadir that was
    // previously selected in the GUI no longer exists, show a dialog to notify
    // about the problem, since it could happen when an external drive is not
    // attached, and choosing a new datadirectory would not be desirable.
    std::string message;
    if (datadir_type == LINK) {
        if (datadir.is_absolute()) {
            fs::file_status status = fs::status(datadir, ec);
            while (status.type() != fs::file_type::directory) {
                if (!ec) ec = std::make_error_code(std::errc::not_a_directory);
                QMessageBox messagebox;
                messagebox.setIcon(QMessageBox::Critical);
                messagebox.setWindowTitle(CLIENT_NAME);
                messagebox.setTextFormat(Qt::RichText);
                messagebox.setText(tr("%1 data directory path %2 no longer exists or is not a directory")
                                       .arg(CLIENT_NAME)
                                       .arg(QString::fromStdString(fs::PathToString(datadir))));
                messagebox.setInformativeText(
                    tr("Do you want to retry accessing the same data directory, choose a different data "
                       "directory, or abort without making changes?"));
                messagebox.setDetailedText(QString::fromStdString(ec.message()));
                QPushButton* retry = messagebox.addButton(QMessageBox::Retry);
                QPushButton* choose_datadir =
                    messagebox.addButton(tr("Choose a different data directory..."), QMessageBox::AcceptRole);
                QPushButton* abort = messagebox.addButton(QMessageBox::Abort);
                messagebox.exec();
                QAbstractButton* clicked = messagebox.clickedButton();
                if (clicked == retry) {
                    status = fs::status(datadir, ec);
                    // Do nothing and loop
                } else if (clicked == choose_datadir) {
                    show_intro = true;
                    break;
                } else {
                    assert(clicked == abort);
                    return false;
                }
            }
        } else {
            // Error will be displayed in intro dialog
            ec = std::make_error_code(std::errc::not_a_directory);
        }
    }

    did_show_intro = false;

    if (show_intro) {
        /* Use selectParams here to guarantee Params() can be used by node interface */
        SelectParams(gArgs.GetChainType());

        /* If current default data directory does not exist, let the user choose one */
        Intro intro(nullptr, Params().AssumedBlockchainSize(), Params().AssumedChainStateSize());
        intro.setDataDirectory(QString::fromStdString(fs::PathToString(datadir)));
        intro.setWindowIcon(QIcon(":icons/bitcoin"));
        if (ec) intro.setStatus(FreespaceChecker::ST_ERROR, QString::fromStdString(ec.message()), 0);
        did_show_intro = true;

        while(show_intro)
        {
            if(!intro.exec())
            {
                /* Cancel clicked */
                return false;
            }
            datadir = fs::PathFromString(intro.getDataDirectory().toStdString());
            std::string error;
            if (!datadir.is_absolute()) {
                intro.setStatus(FreespaceChecker::ST_ERROR,
                                QString::fromStdString("Data directory is not an absolute path."), 0);
            } else if (!CreateDataDir(datadir, error)) {
                intro.setStatus(FreespaceChecker::ST_ERROR,
                                QString::fromStdString(strprintf("Could not create data directory: %s", error)), 0);
            } else if (!SetInitialDataDir(default_datadir, datadir, error)) {
                intro.setStatus(FreespaceChecker::ST_ERROR,
                                QString::fromStdString(strprintf("Could not set default datadirectory: %s", error)),
                                0);
            } else {
                show_intro = false;
            }
        }

        // Additional preferences:
        prune_MiB = intro.getPruneMiB();
    }

    settings.setValue("fReset", false);
    if (remove_legacy_setting) settings.remove("strDataDir");

    assert(!datadir.empty());
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
        if (ui->prune->isEnabled() && m_prune_checkbox_is_default) {
            ui->prune->setChecked(m_bytes_available < (m_blockchain_size_gb + m_chain_state_size_gb + 10) * GB_BYTES);
        }
        UpdateFreeSpaceLabel();
    }
    /* Don't allow confirm in ERROR state */
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(status != FreespaceChecker::ST_ERROR);
}

void Intro::UpdateFreeSpaceLabel()
{
    QString freeString = tr("%n GB of space available", "", m_bytes_available / GB_BYTES);
    if (m_bytes_available < m_required_space_gb * GB_BYTES) {
        freeString += " " + tr("(of %n GB needed)", "", m_required_space_gb);
        ui->freeSpace->setStyleSheet("QLabel { color: #800000 }");
    } else if (m_bytes_available / GB_BYTES - m_required_space_gb < 10) {
        freeString += " " + tr("(%n GB needed for full chain)", "", m_required_space_gb);
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
    QString dir = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(nullptr, tr("Choose data directory"), ui->dataDirectory->text()));
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
        tr("%1 will download and store a copy of the Bitcoin block chain.").arg(CLIENT_NAME) + " " +
        storageRequiresMsg.arg(m_required_space_gb) + " " +
        tr("The wallet will also be stored in this directory.")
    );
    this->adjustSize();
}

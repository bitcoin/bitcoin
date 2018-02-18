#include "updatedialog.h"
#include "ui_updatedialog.h"
#include "util.h"
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>

UpdateDialog::UpdateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog),
    finished(false),
    iconSize(40)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->osComboBox->lineEdit()->setReadOnly(true);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Download");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Not Now");
    connect(ui->osComboBox , SIGNAL(currentIndexChanged(int)),this,SLOT(selectionChanged(int)));

    setPossibleOS();
    setProgressValue(0);
    setIcon();
}

UpdateDialog* UpdateDialog::instance = 0;

UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::setUpdateVersion(QString text)
{
    ui->topLabel->setText(ui->topLabel->text().arg(text));
}

void UpdateDialog::setCurrentVersion(QString text)
{
    ui->currentVersionLabel->setText(ui->currentVersionLabel->text().arg(text));
}

QString UpdateDialog::getOSString(boost::optional<Updater::OS> os)
{
    QString osString = "Unknown";
    if (os) {
        switch(os.get()) {
            case Updater::LINUX_32:
                osString = "Linux 32";
                break;
            case Updater::LINUX_64:
                osString = "Linux 64";
                break;
            case Updater::WINDOWS_32:
                osString = "Windows 32";
                break;
            case Updater::WINDOWS_64:
                osString = "Windows 64";
                break;
            case Updater::MAC_OS:
                osString = "Mac OS";
                break;
            default:
                osString = "Unknown";
        }
    }
    return osString;
}

void UpdateDialog::setOS(Updater::OS os)
{
    ui->osComboBox->setCurrentIndex(os);
}

Updater::OS UpdateDialog::getOS()
{
    return static_cast<Updater::OS>(ui->osComboBox->currentIndex());
}

void UpdateDialog::setPossibleOS()
{
    ui->osComboBox->addItem(getOSString(Updater::LINUX_32));
    ui->osComboBox->addItem(getOSString(Updater::LINUX_64));
    ui->osComboBox->addItem(getOSString(Updater::WINDOWS_32));
    ui->osComboBox->addItem(getOSString(Updater::WINDOWS_64));
    ui->osComboBox->addItem(getOSString(Updater::MAC_OS));
}

void UpdateProgressBar(curl_off_t now, curl_off_t total)
{
    QCoreApplication::processEvents();
    if (now != 0 && total != 0)
    {
        UpdateDialog::GetInstance()->setProgressMaximum(total);
        UpdateDialog::GetInstance()->setProgressValue(now);
    }
}

void UpdateDialog::shaError(const QString& error)
{
    QMessageBox::warning(this, "Error", tr(qPrintable(error)),
        QMessageBox::Ok, QMessageBox::Ok);
    QFile(fileName).remove();
}

void UpdateDialog::downloadFinished()
{
    if (!finished)
    {
        finished = true;
        std::string newSha = updater.GetDownloadSha256Sum(getOS());
        try
        {
            std::string sha = Sha256Sum(fileName.toStdString());
            if (!sha.empty()) {
                if (newSha.compare(sha) == 0) {
                    QMessageBox::information(this, tr("Download Complete"),
                        tr("Package has been successfully downloaded!\nPath - %1").arg(fileName));
                } else {
                    shaError("Download Failed. \nSHA-256 Checksum is not valid.");
                }
            } else {
                shaError("An error occurred during SHA-256 verification.");
            }
        } catch(std::runtime_error &e) {
            LogPrintf("%s\n", e.what());
            shaError("An error occurred during SHA-256 verification.");
        }
        QDialog::done(true);
    }
}

void UpdateDialog::setProgressMaximum(int max)
{
    ui->progressBar->setMaximum(max);
}

void UpdateDialog::setProgressValue(int value)
{
    ui->progressBar->setValue(value);
}

void UpdateDialog::downloadVersion()
{
    finished = false;
    std::string url = updater.GetDownloadUrl(getOS());
    QFileInfo fileInfo(QString::fromStdString(url));

    fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
            QDir::homePath() + QDir::separator() + fileInfo.fileName(),
            tr("Archives (*.zip *.tar.gz)"));
    if (!fileName.trimmed().isEmpty())
    {
        ui->progressBar->setVisible(true);
        this->resize(this->width(), this->height() + 45);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Cancel");
        CURLcode res = updater.DownloadFile(url, fileName.toStdString(), &UpdateProgressBar);
        if (res == CURLE_OK) {
            downloadFinished();
        } else {
            // This error code appears when canceling download
            if (res != CURLE_ABORTED_BY_CALLBACK)
            {
                QMessageBox::warning(this, "Error",
                    tr("Failed to download file - %1 \nCheck debug.log for more information.")
                    .arg(QString::fromStdString(url)),
                    QMessageBox::Ok, QMessageBox::Ok);
            }
            QDialog::done(true);
        }
    }
}

UpdateDialog* UpdateDialog::GetInstance()
{
    if (NULL == instance)
    {
        instance = new UpdateDialog();
    }
    return instance;
}

void UpdateDialog::done(int r)
{
    if (QDialog::Accepted == r)
    {
        downloadVersion();
        //QDialog::done(r);
        return;
    }
    else
    {
        updater.StopDownload();
        QDialog::done(r);
        return;
    }
}

void UpdateDialog::setIcon() const
{
    QIcon icon = this->style()->standardIcon(QStyle::SP_MessageBoxInformation);
    ui->iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
    ui->iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

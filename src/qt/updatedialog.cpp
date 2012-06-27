#include "updatedialog.h"
#include "ui_updatedialog.h"
#include "optionsmodel.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QLayout>

#include <QtNetwork/QNetworkAccessManager>

#include <boost/filesystem.hpp>

UpdateDialog::UpdateDialog(ClientModel& clientModelIn, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);

    clientModel = &clientModelIn;

    std::string basePath = QCoreApplication::applicationDirPath().toStdString();;
    std::string gitianFolder = basePath + "\\gitian-updater";
    std::string oldGitian = basePath + "\\..\\gitian-updater";
    if (boost::filesystem::exists(gitianFolder) && boost::filesystem::exists(oldGitian))
    {
        printf("Moving new gitian-updater into place.\n");

        if (!boost::filesystem::exists(oldGitian + "\\gitian-updater.exe")
            || boost::filesystem::remove(oldGitian + "\\gitian-updater.exe"))
        {
            boost::filesystem::remove_all(oldGitian);
            boost::filesystem::rename(gitianFolder, oldGitian);
        }
        else
            printf("Failed to update gitian-updater!\n");
    }

    mNetworkManager = new QNetworkAccessManager(this);
    mNetworkManager->setProxy(clientModel->getOptionsModel()->getProxy());
    connect(mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleURLReply(QNetworkReply*)));

    QUrl url("http://bitcoin.org/latestversion.txt");
    mNetworkManager->get(QNetworkRequest(url));
}

void UpdateDialog::handleURLReply(QNetworkReply* reply)
{
    if(reply->error() == QNetworkReply::NoError && reply->isReadable() &&
       reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200)
    {
        // File is three lines:
        //  first line is the current highest version in CLIENT_VERSION format
        //  second line is the current highest version in human-readable format
        //  third line is the minimum version considered "secure" in CLIENT_VERSION format
        QList<QByteArray> list = reply->read(100).split('\n');
        if (list.size() > 2 && clientModel->clientVersion() < list[0].toInt())
        {
            ui->securityUpdateLabel->setVisible(list[2].toInt() > clientModel->clientVersion());
            ui->currentVersionLabel->setText(clientModel->formatFullVersion());
            ui->latestVersionLabel->setText(list[1]);
            this->exec();
        }
    }

    reply->deleteLater();
    mNetworkManager->deleteLater();
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::accept()
{
    QString basePath = QCoreApplication::applicationDirPath();
    QFileInfo gitianFile(basePath + "\\..\\gitian-updater\\gitian-updater.exe");
    if (!gitianFile.isExecutable())
    {
        QMessageBox::critical(this, tr("Error updating"),
                              tr("Could not find gitian-updater.exe at ") + gitianFile.filePath());
        QDialog::accept();
        return;
    }
    QFileInfo gpgFile(basePath + "\\..\\gitian-updater\\GnuPG\\gpg2.exe");
    if (!gpgFile.isExecutable())
    {
        QMessageBox::critical(this, tr("Error updating"),
                              tr("Could not find gpg2.exe at ") + gpgFile.filePath());
        QDialog::accept();
        return;
    }
    process = new QProcess();
    QStringList args;
    args << "-g" << gpgFile.filePath() << "-c" << basePath + "\\src\\contrib\\gitian-downloader\\win32-download-config" << "-d" << basePath;
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finishedUpgrade(int, QProcess::ExitStatus)));
    QMessageBox::warning(this, tr("Updating"),
                         tr("Bitcoin-Qt is now updating in the background."), QMessageBox::Ok);
    process->start(gitianFile.filePath(), args);
    QDialog::accept();
}

void UpdateDialog::finishedUpgrade(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0)
    {
        QMessageBox::warning(this, tr("Updated!"),
                             tr("Successfully updated, Bitcoin-Qt will now exit, you may then restart it."), QMessageBox::Ok);
        QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
    }
    else
    {
        QMessageBox::critical(this, tr("Error updating"),
                              tr("gitian-updater failed with error code: ") + QString::number(exitCode));
    }
    delete process;
    delete this;
}

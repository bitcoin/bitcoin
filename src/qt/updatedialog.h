#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include "clientmodel.h"

#include <QDialog>
#include <QProcess>
#include <QtNetwork/QNetworkReply>

namespace Ui {
    class UpdateDialog;
}

/** Bitcoin Update Dialog. */
class UpdateDialog: public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(ClientModel& clientModelIn, QWidget *parent = 0);
    ~UpdateDialog();

public slots:
    /** update */
    void accept();

private slots:
    void handleURLReply(QNetworkReply* reply);
    void finishedUpgrade(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::UpdateDialog *ui;
    QProcess* process;
    QNetworkAccessManager* mNetworkManager;
    ClientModel* clientModel;
};

#endif // UPDATEDIALOG_H

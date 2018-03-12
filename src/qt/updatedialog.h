#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QThread>
#include "updater.h"
#include <curl/curl.h>

namespace Ui {
    class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    void setUpdateVersion(QString text);
    void setCurrentVersion(QString text);
    QString getLabel();
    void setOS(Updater::OS os);
    Updater::OS getOS();
    QString getOSString(boost::optional<Updater::OS> os = boost::none);
    void downloadVersion();
    void downloadFinished();
    static UpdateDialog* GetInstance();
    void setProgressMaximum(int max);
    void done(int);
    int exec();
    void setProgressValue(int value);
    ~UpdateDialog();

private:
    explicit UpdateDialog(QWidget *parent = 0);
    void setPossibleOS();
    void shaError(const QString& error);
    void setIcon() const;
    void reset();

private:
    Ui::UpdateDialog *ui;
    bool finished;
    static UpdateDialog *instance;
    QString fileName;
    const int iconSize;
    QSize originalSize;
};

#endif // UPDATEDIALOG_H

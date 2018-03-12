#ifndef CREATEMASTERNODEDIALOG_H
#define CREATEMASTERNODEDIALOG_H

#include "createnodedialog.h"
#include "masternodeconfig.h"
#include <QDialog>

class CreateMasternodeDialog : public CreateNodeDialog
{
    Q_OBJECT

public:
    explicit CreateMasternodeDialog(QWidget *parent = 0)
        : CreateNodeDialog(parent)
    {
        setWindowTitle("Create a new Masternode");
    }
private:
    virtual bool aliasExists(QString alias)
    {
        return masternodeConfig.aliasExists(alias.toStdString());
    }
};

#endif // CREATEMASTERNODEDIALOG_H

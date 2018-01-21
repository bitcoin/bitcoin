#ifndef CREATESYSTEMNODEDIALOG_H
#define CREATESYSTEMNODEDIALOG_H

#include "createnodedialog.h"
#include "systemnodeconfig.h"
#include <QDialog>

class CreateSystemnodeDialog : public CreateNodeDialog
{
    Q_OBJECT

public:
    explicit CreateSystemnodeDialog(QWidget *parent = 0)
        : CreateNodeDialog(parent)
    {
        setWindowTitle("Create a new Systemnode");
    }
private:
    virtual bool aliasExists(QString alias)
    {
        return systemnodeConfig.aliasExists(alias.toStdString());
    }
};

#endif // CREATESYSTEMNODEDIALOG_H

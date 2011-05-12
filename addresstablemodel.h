#ifndef ADDRESSTABLEMODEL_H
#define ADDRESSTABLEMODEL_H

#include <QAbstractTableModel>

class AddressTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AddressTableModel(QObject *parent = 0);

signals:

public slots:

};

#endif // ADDRESSTABLEMODEL_H

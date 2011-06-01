#ifndef OPTIONSMODEL_H
#define OPTIONSMODEL_H

#include <QAbstractListModel>

/* Interface from QT to configuration data structure for bitcoin client */
class OptionsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit OptionsModel(QObject *parent = 0);

    enum OptionID {
        StartAtStartup,
        MinimizeToTray,
        MapPortUPnP,
        MinimizeOnClose,
        ConnectSOCKS4,
        ProxyIP,
        ProxyPort,
        Fee,
        OptionIDRowCount
    };

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    /* Explicit getters */
    qint64 getTransactionFee();
    bool getMinimizeToTray();
    bool getMinimizeOnClose();
signals:

public slots:

};

#endif // OPTIONSMODEL_H

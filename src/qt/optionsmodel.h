#ifndef OPTIONSMODEL_H
#define OPTIONSMODEL_H

#include <QAbstractListModel>
#include <QStringList>

/** Interface from Qt to configuration data structure for Bitcoin client.
   To Qt, the options are presented as a list with the different options
   laid out vertically.
   This can be changed to a tree once the settings become sufficiently
   complex.
 */
class OptionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit OptionsModel(QObject *parent = 0);

    enum OptionID {
        /// Main Options
        Fee,                 /**< Transaction Fee. qint64 - Optional transaction fee per kB that helps make sure your transactions are processed quickly.
                               *< Most transactions are 1 kB. Fee 0.01 recommended. 0.0001 GAMEUNITS */
        ReserveBalance,      /**< Reserve Balance. qint64 - Reserved amount does not participate in staking and is therefore spendable at any time. */
        StartAtStartup,      /**< Default Transaction Fee. bool */
        DetachDatabases,     /**< Default Transaction Fee. bool */
        Staking,             /**< Default Transaction Fee. bool */
        MinStakeInterval,
        SecureMessaging,     /**< Default Transaction Fee. bool */
        ThinMode,            /**< Default Transaction Fee. bool */
        ThinFullIndex,
        ThinIndexWindow,
        AutoRingSize,        /**< Default Transaction Fee. bool */
        AutoRedeemGameunitsX,    /**< Default Transaction Fee. bool */
        MinRingSize,         /**< Default Transaction Fee. int */
        MaxRingSize,         /**< Default Transaction Fee. int */
        /// Network Related Options
        MapPortUPnP,         /**< Default Transaction Fee. bool */
        ProxyUse,            /**< Default Transaction Fee. bool */
        ProxyIP,             // QString
        ProxyPort,           // int
        ProxySocksVersion,   // int
        /// Window Options
        MinimizeToTray,      /**< Default Transaction Fee. bool */
        MinimizeOnClose,     /**< Default Transaction Fee. bool */
        /// Display Options
        Language,            // QString
        DisplayUnit,         // Bitcoinnits::Unit
        DisplayAddresses,    /**< Default Transaction Fee. bool */
        RowsPerPage,         // int
        Notifications,       // QStringList
        VisibleTransactions, // QStringList
        OptionIDRowCount,
    };

    QString optionIDName(int row);
    int optionNameID(QString name);

    void Init();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    /* Explicit getters */
    qint64 getTransactionFee();
    qint64 getReserveBalance();
    bool getMinimizeToTray();
    bool getMinimizeOnClose();
    bool getDisplayAddresses();
    bool getAutoRingSize();
    bool getAutoRedeemGameunitsX();
    int getDisplayUnit();
    int getRowsPerPage();
    int getMinRingSize();
    int getMaxRingSize();
    QStringList getNotifications();
    QStringList getVisibleTransactions();
    QString getLanguage() { return language; }

private:
    int nDisplayUnit;
    int nRowsPerPage;
    int nMinRingSize;
    int nMaxRingSize;
    bool bDisplayAddresses;
    bool fMinimizeToTray;
    bool fMinimizeOnClose;
    bool fAutoRingSize;
    bool fAutoRedeemGameunitsX;
    QString language;
    QStringList notifications;
    QStringList visibleTransactions;

signals:
    void displayUnitChanged(int unit);
    void transactionFeeChanged(qint64);
    void reserveBalanceChanged(qint64);
    void rowsPerPageChanged(int);
    void visibleTransactionsChanged(QStringList);
};

#endif // OPTIONSMODEL_H

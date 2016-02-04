// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_UNLIMITEDMODEL_H
#define BITCOIN_QT_UNLIMITEDMODEL_H

#include "amount.h"
#include "unlimited.h"
#include <QAbstractListModel>

QT_BEGIN_NAMESPACE
class QNetworkProxy;
QT_END_NAMESPACE

/** Interface from Qt to configuration data structure for Bitcoin client.
   To Qt, the options are presented as a list with the different options
   laid out vertically.
   This can be changed to a tree once the settings become sufficiently
   complex.
 */
class UnlimitedModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit UnlimitedModel(QObject *parent = 0);

    enum UOptionID {
        MaxGeneratedBlock,      // uint64_t
        ExcessiveBlockSize,
        ExcessiveAcceptDepth,
        UseReceiveShaping,      // bool
        UseSendShaping,         // bool
        ReceiveBurst,           // int
        ReceiveAve,             // int
        SendBurst,              // int
        SendAve,                // int
        UOptIDRowCount,
    };

    void Init();
    void Reset();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    /* Options setters */
    void setMaxGeneratedBlock(const QVariant& value);
    

    /* Explicit getters */
    uint64_t getMaxGeneratedBlock() 
    {
        return ::maxGeneratedBlock;
    }
    
    
    const QString& getOverriddenByCommandLine() { return strOverriddenByCommandLine; }

    /* Restart flag helper */
    void setRestartRequired(bool fRequired);
    bool isRestartRequired();
    
private:
    /* settings that were overriden by command-line */
    QString strOverriddenByCommandLine;

    /// Add option to list of GUI options overridden through command line/config file
    void addOverriddenOption(const std::string &option);

Q_SIGNALS:

};

#endif // BITCOIN_QT_OPTIONSMODEL_H

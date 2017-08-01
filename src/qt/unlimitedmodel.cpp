// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/lexical_cast.hpp>
#include <limits>

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "unlimitedmodel.h"

#include "bitcoinunits.h"
#include "guiutil.h"

#include "amount.h"
#include "init.h"
#include "main.h" // For DEFAULT_SCRIPTCHECK_THREADS
#include "net.h"
#include "tweak.h"
#include "txdb.h" // for -dbcache defaults

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#endif

#include <QNetworkProxy>
#include <QSettings>
#include <QStringList>

extern CTweakRef<uint64_t> miningBlockSize;
extern CTweakRef<uint64_t> ebTweak;

UnlimitedModel::UnlimitedModel(QObject *parent) : QAbstractListModel(parent) { Init(); }
void UnlimitedModel::addOverriddenOption(const std::string &option)
{
    strOverriddenByCommandLine += QString::fromStdString(option) + "=" + QString::fromStdString(mapArgs[option]) + " ";
}

// Writes all missing QSettings with their default values
void UnlimitedModel::Init()
{
    QSettings settings;

    // Ensure restart flag is unset on client startup
    setRestartRequired(false);

    unsigned int tmpExcessiveBlockSize = excessiveBlockSize;
    unsigned int tmpMaxGeneratedBlock = maxGeneratedBlock;

    if (!settings.contains("excessiveBlockSize"))
        settings.setValue("excessiveBlockSize", QString::number(excessiveBlockSize));
    else
        tmpExcessiveBlockSize = settings.value("excessiveBlockSize").toInt();

    if (!settings.contains("excessiveAcceptDepth"))
        settings.setValue("excessiveAcceptDepth", QString::number(excessiveAcceptDepth));
    else
        excessiveAcceptDepth = settings.value("excessiveAcceptDepth").toInt();

    if (!settings.contains("maxGeneratedBlock"))
        settings.setValue("maxGeneratedBlock", QString::number(maxGeneratedBlock));
    else
        tmpMaxGeneratedBlock = settings.value("maxGeneratedBlock").toInt();

    if (!MiningAndExcessiveBlockValidatorRule(tmpExcessiveBlockSize, tmpMaxGeneratedBlock))
    {
        std::ostringstream emsg;
        emsg << "Sorry, your configured maximum mined block (" << tmpMaxGeneratedBlock
             << ") is larger than your configured excessive size (" << tmpExcessiveBlockSize
             << ").  This would cause you to orphan your own blocks.";
        LogPrintf(emsg.str().c_str());
    }
    else
    {
        miningBlockSize.Set(tmpMaxGeneratedBlock);
        ebTweak.Set(tmpExcessiveBlockSize);
    }

    if (!SoftSetArg("-excessiveblocksize", boost::lexical_cast<std::string>(excessiveBlockSize)))
        addOverriddenOption("-excessiveblocksize");
    if (!SoftSetArg("-excessiveacceptdepth", boost::lexical_cast<std::string>(excessiveAcceptDepth)))
        addOverriddenOption("-excessiveacceptdepth");

    bool inUse = settings.value("fUseReceiveShaping").toBool();
    int64_t burstKB = settings.value("nReceiveBurst").toLongLong();
    int64_t aveKB = settings.value("nReceiveAve").toLongLong();

    std::string avg = QString::number(inUse ? aveKB : std::numeric_limits<int64_t>::max()).toStdString();
    std::string burst = QString::number(inUse ? burstKB : std::numeric_limits<int64_t>::max()).toStdString();

    if (!SoftSetArg("-receiveavg", avg))
        addOverriddenOption("-receiveavg");
    if (!SoftSetArg("-receiveburst", burst))
        addOverriddenOption("-receiveburst");

    inUse = settings.value("fUseSendShaping").toBool();
    burstKB = settings.value("nSendBurst").toLongLong();
    aveKB = settings.value("nSendAve").toLongLong();

    avg = boost::lexical_cast<std::string>(inUse ? aveKB : std::numeric_limits<int64_t>::max());
    burst = boost::lexical_cast<std::string>(inUse ? burstKB : std::numeric_limits<int64_t>::max());

    if (!SoftSetArg("-sendavg", avg))
        addOverriddenOption("-sendavg");
    if (!SoftSetArg("-sendburst", burst))
        addOverriddenOption("-sendburst");
}

void UnlimitedModel::Reset()
{
    QSettings settings;

    // Remove all entries from our QSettings object
    settings.clear();

    // default setting for UnlimitedModel::StartAtStartup - disabled
    if (GUIUtil::GetStartOnSystemStartup())
        GUIUtil::SetStartOnSystemStartup(false);
}

int UnlimitedModel::rowCount(const QModelIndex &parent) const { return UOptIDRowCount; }
// read QSettings values and return them
QVariant UnlimitedModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::EditRole)
    {
        QSettings settings;
        switch (index.row())
        {
        case MaxGeneratedBlock:
            return QVariant((qulonglong)maxGeneratedBlock);
        case ExcessiveBlockSize:
            return QVariant((qulonglong)excessiveBlockSize);
        case ExcessiveAcceptDepth:
            return QVariant(excessiveAcceptDepth);
        case UseReceiveShaping:
            return settings.value("fUseReceiveShaping");
        case UseSendShaping:
            return settings.value("fUseSendShaping");
        case ReceiveBurst:
            return settings.value("nReceiveBurst");
        case ReceiveAve:
            return settings.value("nReceiveAve");
        case SendBurst:
            return settings.value("nSendBurst");
        case SendAve:
            return settings.value("nSendAve");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// write QSettings values
bool UnlimitedModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool successful = true; /* set to false on parse error */
    bool changeSendShaper = false;
    bool changeReceiveShaper = false;
    if (role == Qt::EditRole)
    {
        QSettings settings;
        switch (index.row())
        {
        case MaxGeneratedBlock:
        {
            unsigned int mgb = value.toUInt(&successful);
            if (successful && (settings.value("maxGeneratedBlock") != value))
            {
                settings.setValue("maxGeneratedBlock", value);
                miningBlockSize.Set(mgb);
            }
        }
        break;
        case ExcessiveBlockSize:
        {
            unsigned int ebs = value.toUInt(&successful);
            if (successful && (settings.value("excessiveBlockSize") != value))
            {
                settings.setValue("excessiveBlockSize", value);
                ebTweak.Set(ebs); // equivalant to: excessiveBlockSize = ebs;
            }
        }
        break;
        case ExcessiveAcceptDepth:
        {
            unsigned int ead = value.toUInt(&successful);
            if (successful && settings.value("excessiveAcceptDepth") != value)
            {
                settings.setValue("excessiveAcceptDepth", value);
                excessiveAcceptDepth = ead;
            }
        }
        break;
        case UseReceiveShaping:
            if (settings.value("fUseReceiveShaping") != value)
            {
                settings.setValue("fUseReceiveShaping", value);
                changeReceiveShaper = true;
            }
            break;
        case UseSendShaping:
            if (settings.value("fUseSendShaping") != value)
            {
                settings.setValue("fUseSendShaping", value);
                changeSendShaper = true;
            }
            break;
        case ReceiveBurst:
            if (settings.value("nReceiveBurst") != value)
            {
                settings.setValue("nReceiveBurst", value);
                changeReceiveShaper = true;
            }
            break;
        case ReceiveAve:
            if (settings.value("nReceiveAve") != value)
            {
                settings.setValue("nReceiveAve", value);
                changeReceiveShaper = true;
            }
            break;
        case SendBurst:
            if (settings.value("nSendBurst") != value)
            {
                settings.setValue("nSendBurst", value);
                changeSendShaper = true;
            }
            break;
        case SendAve:
            if (settings.value("nSendAve") != value)
            {
                settings.setValue("nSendAve", value);
                changeSendShaper = true;
            }
            break;
        default:
            break;
        }


        if (changeReceiveShaper)
        {
            if (settings.value("fUseReceiveShaping").toBool())
            {
                int64_t burst = 1024 * settings.value("nReceiveBurst").toLongLong();
                int64_t ave = 1024 * settings.value("nReceiveAve").toLongLong();
                receiveShaper.set(burst, ave);
            }
            else
                receiveShaper.disable();
        }

        if (changeSendShaper)
        {
            if (settings.value("fUseSendShaping").toBool())
            {
                int64_t burst = 1024 * settings.value("nSendBurst").toLongLong();
                int64_t ave = 1024 * settings.value("nSendAve").toLongLong();
                sendShaper.set(burst, ave);
            }
            else
                sendShaper.disable();
        }
    }

    Q_EMIT dataChanged(index, index);

    return successful;
}

void UnlimitedModel::setMaxGeneratedBlock(const QVariant &value)
{
    if (!value.isNull())
    {
        QSettings settings;
        maxGeneratedBlock = value.toInt();
        settings.setValue("maxGeneratedBlock", static_cast<qlonglong>(maxGeneratedBlock));
        // Q_EMIT your signal if you need one
    }
}


void UnlimitedModel::setRestartRequired(bool fRequired)
{
    QSettings settings;
    return settings.setValue("fRestartRequired", fRequired);
}

bool UnlimitedModel::isRestartRequired()
{
    QSettings settings;
    return settings.value("fRestartRequired", false).toBool();
}

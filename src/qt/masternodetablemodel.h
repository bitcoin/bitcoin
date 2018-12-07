// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODETABLEMODEL_H
#define BITCOIN_QT_MASTERNODETABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class MasternodeTablePriv;
class ClientModel;
class COutPoint;

namespace interfaces {
class Node;
}

/**
   Qt model of the masternode list. This allows remote start and monitoring of masternodes.
 */
class MasternodeTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MasternodeTableModel(ClientModel *parent = 0);
    ~MasternodeTableModel();

    enum ColumnIndex {
        Alias = 0,
        Address = 1,
        Daemon = 2,
        Status = 3,
        Score = 4,
        Active = 5,
        Last_Seen = 6,
        Payee = 7,
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole, /**< Type of list (#mine or #all) */
        TxHashRole,
        TxOutIndexRole
    };

    static const QString MyNodes;      /**< Specifies My Nodes */
    static const QString AllNodes;   /**< Specifies All Nodes */

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

private:
    ClientModel* const clientModel;
    MasternodeTablePriv *priv = nullptr;
    QStringList columns;

    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    /* New Masternode, or Masternode changed status */
    void updateMasternode(const QString &_outpoint, const int &_n, int status);

    friend class MasternodeTablePriv;
};

#endif // BITCOIN_QT_MASTERNODETABLEMODEL_H

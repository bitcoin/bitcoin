// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SCRIPTPUBKEYMAN_H
#define BITCOIN_WALLET_SCRIPTPUBKEYMAN_H

#include <logging.h>            // Pour LogInfo
#include <util/formatstring.h>  // Pour ConstevalFormatString
#include <tinyformat.h>         // Pour tfm::format
#include <wallet/walletutil.h>  // Hypothèse : contient WalletStorage

namespace wallet {

class WalletStorage;

/**
 * La classe ScriptPubKeyMan gère les scriptPubKeys dans un portefeuille.
 * Elle sert de classe de base pour les portefeuilles Legacy et Descriptor.
 */
class ScriptPubKeyMan
{
protected:
    WalletStorage& m_storage;

public:
    explicit ScriptPubKeyMan(WalletStorage& storage) : m_storage(storage) {}
    virtual ~ScriptPubKeyMan() = default;

    /** Retourne vrai si la dérivation déterministe hiérarchique (HD) est activée. */
    virtual bool IsHDEnabled() const { return false; }

    /** Retourne vrai si le portefeuille peut générer de nouvelles adresses. */
    virtual bool CanGetAddresses(bool internal = false) const { return false; }

    /** * Ajoute le nom du portefeuille au début de la sortie de log pour faciliter le débogage.
     * Cette méthode utilise des templates variadiques pour accepter un nombre indéfini d'arguments.
     */
    template <typename... Params>
    void WalletLogPrintf(util::ConstevalFormatString<sizeof...(Params)> wallet_fmt, const Params&... params) const
    {
        // Utilisation de m_storage.LogName() pour identifier la source du message
        LogInfo("[%s] %s", m_storage.LogName(), tfm::format(wallet_fmt, params...));
    }

    // Ajoute ici les autres méthodes virtuelles pures si nécessaire
};

} // namespace wallet

#endif // BITCOIN_WALLET_SCRIPTPUBKEYMAN_H

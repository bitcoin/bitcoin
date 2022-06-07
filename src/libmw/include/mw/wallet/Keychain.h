#pragma once

#include <mw/models/crypto/SecretKey.h>
#include <mw/models/tx/Output.h>
#include <mw/models/wallet/Coin.h>
#include <mw/models/wallet/StealthAddress.h>
#include <memory>

// Forward Declarations
class LegacyScriptPubKeyMan;

MW_NAMESPACE

class Keychain
{
public:
    using Ptr = std::shared_ptr<Keychain>;

    Keychain(const LegacyScriptPubKeyMan& spk_man, SecretKey scan_secret, SecretKey spend_secret)
        : m_spk_man(spk_man),
        m_scanSecret(std::move(scan_secret)),
        m_spendSecret(std::move(spend_secret)) { }

    // If keychain is locked or watch-only (m_spendSecret is null),
    // this will still identify outputs belonging to the wallet, but
    // will not be able to calculate the coin's output key.
    // It will still calculate the shared_secret though, which can be
    // used to calculate the spend key when the wallet becomes unlocked.
    bool RewindOutput(const Output& output, mw::Coin& coin) const;

    // Calculates the output secret key for the given coin.
    // If the address index is known, it calculates from the keychain's master spend key.
    // If not, it attempts to lookup the spend key in the database.
    // Returns boost::empty when keychain is locked or watch-only.
    boost::optional<SecretKey> CalculateOutputKey(const mw::Coin& coin) const;

    // Calculates the StealthAddress at the given index.
    // Requires that keychain be unlocked and not watch-only.
    StealthAddress GetStealthAddress(const uint32_t index) const;

    // Requires that keychain be unlocked and not watch-only.
    SecretKey GetSpendKey(const uint32_t index) const;

    const SecretKey& GetScanSecret() const noexcept { return m_scanSecret; }
    const SecretKey& GetSpendSecret() const noexcept { return m_spendSecret; }

    bool HasSpendSecret() const noexcept { return !m_spendSecret.IsNull(); }

	// Clears the spend secret from memory, effectively making this a watch-only keychain.
    void Lock() { m_spendSecret = SecretKey::Null(); }
	
	// Reassigns the spend secret. To be used when unlocking the wallet.
    void Unlock(const SecretKey& spend_secret) { m_spendSecret = spend_secret; }
    
private:
    const LegacyScriptPubKeyMan& m_spk_man;
    SecretKey m_scanSecret;
    SecretKey m_spendSecret;
};

END_NAMESPACE
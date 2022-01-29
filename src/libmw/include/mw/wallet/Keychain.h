#pragma once

#include <mw/models/crypto/SecretKey.h>
#include <mw/models/tx/Output.h>
#include <mw/models/wallet/Coin.h>
#include <mw/models/wallet/StealthAddress.h>
#include <memory>

// Forward Declarations
class ScriptPubKeyMan;

MW_NAMESPACE

class Keychain
{
public:
    using Ptr = std::shared_ptr<Keychain>;

    Keychain(const ScriptPubKeyMan& spk_man, SecretKey scan_secret, SecretKey spend_secret)
        : m_spk_man(spk_man),
        m_scanSecret(std::move(scan_secret)),
        m_spendSecret(std::move(spend_secret)) { }

    bool RewindOutput(const Output& output, mw::Coin& coin) const;

    StealthAddress GetStealthAddress(const uint32_t index) const;
    SecretKey GetSpendKey(const uint32_t index) const;

    const SecretKey& GetScanSecret() const noexcept { return m_scanSecret; }
    const SecretKey& GetSpendSecret() const noexcept { return m_spendSecret; }
    
private:
    const ScriptPubKeyMan& m_spk_man;
    SecretKey m_scanSecret;
    SecretKey m_spendSecret;
};

END_NAMESPACE
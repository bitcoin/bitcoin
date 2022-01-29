#pragma once

#include <mw/common/Traits.h>
#include <mw/util/StringUtil.h>
#include <script/script.h>
#include <util/strencodings.h>
#include <amount.h>

//
// Represents coins being pegged out, i.e. moved from the extension block to the canonical chain.
//
class PegOutCoin : public Traits::ISerializable, public Traits::IPrintable
{
public:
    PegOutCoin() = default;
    PegOutCoin(const CAmount amount, CScript scriptPubKey)
        : m_amount(amount), m_scriptPubKey(std::move(scriptPubKey)) { }

    bool operator<(const PegOutCoin& rhs) const noexcept
    {
        if (m_amount != rhs.m_amount) {
            return m_amount < rhs.m_amount;
        }

        return m_scriptPubKey < rhs.m_scriptPubKey;
    }

    bool operator!=(const PegOutCoin& rhs) const noexcept
    {
        return m_amount != rhs.m_amount || m_scriptPubKey != rhs.m_scriptPubKey;
    }

    bool operator==(const PegOutCoin& rhs) const noexcept
    {
        return m_amount == rhs.m_amount && m_scriptPubKey == rhs.m_scriptPubKey;
    }

    CAmount GetAmount() const noexcept { return m_amount; }
    const CScript& GetScriptPubKey() const noexcept { return m_scriptPubKey; }

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZABLE(PegOutCoin, obj)
    {
        READWRITE(VARINT_MODE(obj.m_amount, VarIntMode::NONNEGATIVE_SIGNED));
        READWRITE(obj.m_scriptPubKey);

        if (ser_action.ForRead()) {
            if (obj.m_scriptPubKey.empty()) {
                throw std::ios_base::failure("Pegout scriptPubKey must not be empty");
            }
        }
    }

    std::string Format() const noexcept final
    {
        return StringUtil::Format("PegOutCoin(scriptPubKey:{}, amount:{})", HexStr(m_scriptPubKey), m_amount);
    }

private:
    CAmount m_amount;
    CScript m_scriptPubKey;
};
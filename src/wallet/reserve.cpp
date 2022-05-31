#include <wallet/reserve.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>

bool ReserveDestination::GetReservedDestination(CTxDestination& dest, bool internal)
{
    m_spk_man = pwallet->GetScriptPubKeyMan(type, internal);
    if (!m_spk_man) {
        return false;
    }

    if (nIndex == -1) {
        m_spk_man->TopUp();

        CKeyPool keypool;
        int64_t reserved_index;
        if (!m_spk_man->GetReservedDestination(type, internal, address, reserved_index, keypool)) {
            return false;
        }
        nIndex = reserved_index;
        fInternal = keypool.fInternal;
    }
    dest = address;
    return true;
}

void ReserveDestination::KeepDestination()
{
    if (nIndex != -1) {
        m_spk_man->KeepDestination(nIndex, type);
    }
    nIndex = -1;
    address = CNoDestination();
}

void ReserveDestination::ReturnDestination()
{
    if (nIndex != -1) {
        KeyPurpose purpose = (type == OutputType::MWEB) ? KeyPurpose::MWEB : (fInternal ? KeyPurpose::INTERNAL : KeyPurpose::EXTERNAL);
        m_spk_man->ReturnDestination(nIndex, purpose, address);
    }
    nIndex = -1;
    address = CNoDestination();
}
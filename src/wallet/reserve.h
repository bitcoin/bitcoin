#pragma once

#include <outputtype.h>
#include <script/standard.h>

// Forward Declarations
class CWallet;
class ScriptPubKeyMan;

/** A wrapper to reserve an address from a wallet
 *
 * ReserveDestination is used to reserve an address.
 * It is currently only used inside of CreateTransaction.
 *
 * Instantiating a ReserveDestination does not reserve an address. To do so,
 * GetReservedDestination() needs to be called on the object. Once an address has been
 * reserved, call KeepDestination() on the ReserveDestination object to make sure it is not
 * returned. Call ReturnDestination() to return the address so it can be re-used (for
 * example, if the address was used in a new transaction
 * and that transaction was not completed and needed to be aborted).
 *
 * If an address is reserved and KeepDestination() is not called, then the address will be
 * returned when the ReserveDestination goes out of scope.
 */
class ReserveDestination
{
protected:
    //! The wallet to reserve from
    const CWallet* const pwallet;
    //! The ScriptPubKeyMan to reserve from. Based on type when GetReservedDestination is called
    ScriptPubKeyMan* m_spk_man{nullptr};
    OutputType const type;
    //! The index of the address's key in the keypool
    int64_t nIndex{-1};
    //! The destination
    CTxDestination address;
    //! Whether this is from the internal (change output) keypool
    bool fInternal{false};

public:
    //! Construct a ReserveDestination object. This does NOT reserve an address yet
    explicit ReserveDestination(CWallet* pwallet, OutputType type)
        : pwallet(pwallet), type(type) {}

    ReserveDestination(const ReserveDestination&) = delete;
    ReserveDestination& operator=(const ReserveDestination&) = delete;

    //! Destructor. If a key has been reserved and not KeepKey'ed, it will be returned to the keypool
    ~ReserveDestination()
    {
        ReturnDestination();
    }

    //! Reserve an address
    bool GetReservedDestination(CTxDestination& pubkey, bool internal);
    //! Return reserved address
    void ReturnDestination();
    //! Keep the address. Do not return it's key to the keypool when this object goes out of scope
    void KeepDestination();
};
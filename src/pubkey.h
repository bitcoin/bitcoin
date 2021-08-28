// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_PUBKEY_H
#define SYSCOIN_PUBKEY_H

#include <hash.h>
#include <serialize.h>
#include <span.h>
#include <uint256.h>

#include <cstring>
#include <optional>
#include <vector>

const unsigned int BIP32_EXTKEY_SIZE = 74;

/** A reference to a CKey: the Hash160 of its serialized public key */
class CKeyID : public uint160
{
public:
    CKeyID() : uint160() {}
    explicit CKeyID(const uint160& in) : uint160(in) {}
};

typedef uint256 ChainCode;

/** An encapsulated public key. */
class CPubKey
{
public:
    /**
     * secp256k1:
     */
    static constexpr unsigned int SIZE                   = 65;
    static constexpr unsigned int COMPRESSED_SIZE        = 33;
    static constexpr unsigned int SIGNATURE_SIZE         = 72;
    static constexpr unsigned int COMPACT_SIGNATURE_SIZE = 65;
    /**
     * see www.keylength.com
     * script supports up to 75 for single byte push
     */
    static_assert(
        SIZE >= COMPRESSED_SIZE,
        "COMPRESSED_SIZE is larger than SIZE");

private:

    /**
     * Just store the serialized data.
     * Its length can very cheaply be computed from the first byte.
     */
    unsigned char vch[SIZE];

    //! Compute the length of a pubkey with a given first byte.
    unsigned int static GetLen(unsigned char chHeader)
    {
        if (chHeader == 2 || chHeader == 3)
            return COMPRESSED_SIZE;
        if (chHeader == 4 || chHeader == 6 || chHeader == 7)
            return SIZE;
        return 0;
    }

    //! Set this key data to be invalid
    void Invalidate()
    {
        vch[0] = 0xFF;
    }

public:

    bool static ValidSize(const std::vector<unsigned char> &vch) {
      return vch.size() > 0 && GetLen(vch[0]) == vch.size();
    }

    //! Construct an invalid public key.
    CPubKey()
    {
        Invalidate();
    }

    //! Initialize a public key using begin/end iterators to byte data.
    template <typename T>
    void Set(const T pbegin, const T pend)
    {
        int len = pend == pbegin ? 0 : GetLen(pbegin[0]);
        if (len && len == (pend - pbegin))
            memcpy(vch, (unsigned char*)&pbegin[0], len);
        else
            Invalidate();
    }

    //! Construct a public key using begin/end iterators to byte data.
    template <typename T>
    CPubKey(const T pbegin, const T pend)
    {
        Set(pbegin, pend);
    }

    //! Construct a public key from a byte vector.
    explicit CPubKey(Span<const uint8_t> _vch)
    {
        Set(_vch.begin(), _vch.end());
    }

    //! Simple read-only vector-like interface to the pubkey data.
    unsigned int size() const { return GetLen(vch[0]); }
    const unsigned char* data() const { return vch; }
    const unsigned char* begin() const { return vch; }
    const unsigned char* end() const { return vch + size(); }
    const unsigned char& operator[](unsigned int pos) const { return vch[pos]; }

    //! Comparator implementation.
    friend bool operator==(const CPubKey& a, const CPubKey& b)
    {
        return a.vch[0] == b.vch[0] &&
               memcmp(a.vch, b.vch, a.size()) == 0;
    }
    friend bool operator!=(const CPubKey& a, const CPubKey& b)
    {
        return !(a == b);
    }
    friend bool operator<(const CPubKey& a, const CPubKey& b)
    {
        return a.vch[0] < b.vch[0] ||
               (a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) < 0);
    }

    //! Implement serialization, as if this was a byte vector.
    template <typename Stream>
    void Serialize(Stream& s) const
    {
        unsigned int len = size();
        ::WriteCompactSize(s, len);
        s.write((char*)vch, len);
    }
    template <typename Stream>
    void Unserialize(Stream& s)
    {
        unsigned int len = ::ReadCompactSize(s);
        if (len <= SIZE) {
            s.read((char*)vch, len);
            if (len != size()) {
                Invalidate();
            }
        } else {
            // invalid pubkey, skip available data
            char dummy;
            while (len--)
                s.read(&dummy, 1);
            Invalidate();
        }
    }

    //! Get the KeyID of this public key (hash of its serialization)
    CKeyID GetID() const
    {
        return CKeyID(Hash160(MakeSpan(vch).first(size())));
    }

    //! Get the 256-bit hash of this public key.
    uint256 GetHash() const
    {
        return Hash(MakeSpan(vch).first(size()));
    }

    /*
     * Check syntactic correctness.
     *
     * When setting a pubkey (Set()) or deserializing fails (its header bytes
     * don't match the length of the data), the size is set to 0. Thus,
     * by checking size, one can observe whether Set() or deserialization has
     * failed.
     *
     * This does not check for more than that. In particular, it does not verify
     * that the coordinates correspond to a point on the curve (see IsFullyValid()
     * for that instead).
     *
     * Note that this is consensus critical as CheckECDSASignature() calls it!
     */
    bool IsValid() const
    {
        return size() > 0;
    }

    //! fully validate whether this is a valid public key (more expensive than IsValid())
    bool IsFullyValid() const;

    //! Check whether this is a compressed public key.
    bool IsCompressed() const
    {
        return size() == COMPRESSED_SIZE;
    }

    /**
     * Verify a DER signature (~72 bytes).
     * If this public key is not fully valid, the return value will be false.
     */
    bool Verify(const uint256& hash, const std::vector<unsigned char>& vchSig) const;

    /**
     * Check whether a signature is normalized (lower-S).
     */
    static bool CheckLowS(const std::vector<unsigned char>& vchSig);

    //! Recover a public key from a compact signature.
    bool RecoverCompact(const uint256& hash, const std::vector<unsigned char>& vchSig);

    //! Turn this public key into an uncompressed public key.
    bool Decompress();

    //! Derive BIP32 child pubkey.
    bool Derive(CPubKey& pubkeyChild, ChainCode &ccChild, unsigned int nChild, const ChainCode& cc) const;
};

class XOnlyPubKey
{
private:
    uint256 m_keydata;

public:
    /** Construct an empty x-only pubkey. */
    XOnlyPubKey() = default;

    XOnlyPubKey(const XOnlyPubKey&) = default;
    XOnlyPubKey& operator=(const XOnlyPubKey&) = default;

    /** Determine if this pubkey is fully valid. This is true for approximately 50% of all
     *  possible 32-byte arrays. If false, VerifySchnorr and CreatePayToContract will always
     *  fail. */
    bool IsFullyValid() const;

    /** Test whether this is the 0 key (the result of default construction). This implies
     *  !IsFullyValid(). */
    bool IsNull() const { return m_keydata.IsNull(); }

    /** Construct an x-only pubkey from exactly 32 bytes. */
    explicit XOnlyPubKey(Span<const unsigned char> bytes);

    /** Construct an x-only pubkey from a normal pubkey. */
    explicit XOnlyPubKey(const CPubKey& pubkey) : XOnlyPubKey(Span<const unsigned char>(pubkey.begin() + 1, pubkey.begin() + 33)) {}

    /** Verify a Schnorr signature against this public key.
     *
     * sigbytes must be exactly 64 bytes.
     */
    bool VerifySchnorr(const uint256& msg, Span<const unsigned char> sigbytes) const;

    /** Compute the Taproot tweak as specified in BIP341, with *this as internal
     * key:
     *  - if merkle_root == nullptr: H_TapTweak(xonly_pubkey)
     *  - otherwise:                 H_TapTweak(xonly_pubkey || *merkle_root)
     *
     * Note that the behavior of this function with merkle_root != nullptr is
     * consensus critical.
     */
    uint256 ComputeTapTweakHash(const uint256* merkle_root) const;

    /** Verify that this is a Taproot tweaked output point, against a specified internal key,
     *  Merkle root, and parity. */
    bool CheckTapTweak(const XOnlyPubKey& internal, const uint256& merkle_root, bool parity) const;

    /** Construct a Taproot tweaked output point with this point as internal key. */
    std::optional<std::pair<XOnlyPubKey, bool>> CreateTapTweak(const uint256* merkle_root) const;

    const unsigned char& operator[](int pos) const { return *(m_keydata.begin() + pos); }
    const unsigned char* data() const { return m_keydata.begin(); }
    static constexpr size_t size() { return decltype(m_keydata)::size(); }
    const unsigned char* begin() const { return m_keydata.begin(); }
    const unsigned char* end() const { return m_keydata.end(); }
    unsigned char* begin() { return m_keydata.begin(); }
    unsigned char* end() { return m_keydata.end(); }
    bool operator==(const XOnlyPubKey& other) const { return m_keydata == other.m_keydata; }
    bool operator!=(const XOnlyPubKey& other) const { return m_keydata != other.m_keydata; }
    bool operator<(const XOnlyPubKey& other) const { return m_keydata < other.m_keydata; }
};

struct CExtPubKey {
    unsigned char nDepth;
    unsigned char vchFingerprint[4];
    unsigned int nChild;
    ChainCode chaincode;
    CPubKey pubkey;

    friend bool operator==(const CExtPubKey &a, const CExtPubKey &b)
    {
        return a.nDepth == b.nDepth &&
            memcmp(a.vchFingerprint, b.vchFingerprint, sizeof(vchFingerprint)) == 0 &&
            a.nChild == b.nChild &&
            a.chaincode == b.chaincode &&
            a.pubkey == b.pubkey;
    }

    friend bool operator!=(const CExtPubKey &a, const CExtPubKey &b)
    {
        return !(a == b);
    }

    void Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const;
    void Decode(const unsigned char code[BIP32_EXTKEY_SIZE]);
    bool Derive(CExtPubKey& out, unsigned int nChild) const;
};

/** Users of this module must hold an ECCVerifyHandle. The constructor and
 *  destructor of these are not allowed to run in parallel, though. */
class ECCVerifyHandle
{
    static int refcount;

public:
    ECCVerifyHandle();
    ~ECCVerifyHandle();
};

typedef struct secp256k1_context_struct secp256k1_context;

/** Access to the internal secp256k1 context used for verification. Only intended to be used
 *  by key.cpp. */
const secp256k1_context* GetVerifyContext();

#endif // SYSCOIN_PUBKEY_H

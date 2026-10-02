// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include "pq_shield_core.h"

#include <oqs/sig.h>

#include <sys/stat.h>

#include <fstream>

namespace pq_shield {

namespace {
struct SigCtx {
    OQS_SIG* sig = nullptr;
    SigCtx() { sig = OQS_SIG_new(ALG); }
    ~SigCtx() { if (sig) OQS_SIG_free(sig); }
    bool Ok() const { return sig != nullptr; }
};
} // namespace

// Allocate buffers from liboqs' advertised lengths so key sizes track the enabled algorithm.
bool Keygen(std::vector<uint8_t>& public_key, std::vector<uint8_t>& secret_key)
{
    SigCtx ctx;
    if (!ctx.Ok()) return false;
    public_key.resize(ctx.sig->length_public_key);
    secret_key.resize(ctx.sig->length_secret_key);
    return OQS_SIG_keypair(ctx.sig, public_key.data(), secret_key.data()) == OQS_SUCCESS;
}

// Buffer must hold worst-case signature size; actual length is written back to the caller.
bool Sign(std::vector<uint8_t>& signature, const std::vector<uint8_t>& message,
          const std::vector<uint8_t>& secret_key)
{
    SigCtx ctx;
    if (!ctx.Ok()) return false;
    signature.resize(ctx.sig->length_signature);
    size_t sig_len = 0;
    if (OQS_SIG_sign(ctx.sig, signature.data(), &sig_len, message.data(), message.size(),
                     secret_key.data()) != OQS_SUCCESS) {
        return false;
    }
    signature.resize(sig_len);
    return true;
}

// Fail closed: any liboqs error is treated as an invalid signature.
bool Verify(const std::vector<uint8_t>& message,
            const std::vector<uint8_t>& signature,
            const std::vector<uint8_t>& public_key)
{
    SigCtx ctx;
    if (!ctx.Ok()) return false;
    return OQS_SIG_verify(ctx.sig, message.data(), message.size(), signature.data(),
                          signature.size(), public_key.data()) == OQS_SUCCESS;
}

// Audit depends on this to reject truncated or corrupt keys.
size_t PublicKeyLen() { SigCtx c; return c.Ok() ? c.sig->length_public_key : 0; }
size_t SecretKeyLen() { SigCtx c; return c.Ok() ? c.sig->length_secret_key : 0; }
size_t SignatureMaxLen() { SigCtx c; return c.Ok() ? c.sig->length_signature : 0; }

// Private keys are written with mode 0600; the audit tool rejects looser modes.
bool WriteFile(const std::string& path, const std::vector<uint8_t>& data, bool secret)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out.good()) return false;
    out.close();
    if (secret && chmod(path.c_str(), 0600) != 0) return false;
    return true;
}

bool ReadFile(const std::string& path, std::vector<uint8_t>& data)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return in.good() || in.eof();
}

// Size and permission checks together detect truncated keys and over-exposed private keys.
bool AuditFile(const std::string& path, bool expect_secret, std::string& error)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        error = "cannot stat file";
        return false;
    }
    size_t expected = expect_secret ? SecretKeyLen() : PublicKeyLen();
    if (static_cast<size_t>(st.st_size) != expected) {
        error = "unexpected file size";
        return false;
    }
    if (expect_secret) {
        mode_t mode = st.st_mode & 0777;
        if (mode & ~0600) {
            error = "private key permissions too open";
            return false;
        }
    }
    return true;
}

} // namespace pq_shield

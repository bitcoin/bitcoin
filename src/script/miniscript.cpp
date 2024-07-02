// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include <script/script.h>
#include <script/miniscript.h>
#include <serialize.h>

#include <assert.h>

namespace miniscript {
namespace internal {

Type SanitizeType(Type e) {
    int num_types = (e << "K"_mst) + (e << "V"_mst) + (e << "B"_mst) + (e << "W"_mst);
    if (num_types == 0) return ""_mst; // No valid type, don't care about the rest
    Assume(num_types == 1); // K, V, B, W all conflict with each other
    Assume(!(e << "z"_mst) || !(e << "o"_mst)); // z conflicts with o
    Assume(!(e << "n"_mst) || !(e << "z"_mst)); // n conflicts with z
    Assume(!(e << "n"_mst) || !(e << "W"_mst)); // n conflicts with W
    Assume(!(e << "V"_mst) || !(e << "d"_mst)); // V conflicts with d
    Assume(!(e << "K"_mst) ||  (e << "u"_mst)); // K implies u
    Assume(!(e << "V"_mst) || !(e << "u"_mst)); // V conflicts with u
    Assume(!(e << "e"_mst) || !(e << "f"_mst)); // e conflicts with f
    Assume(!(e << "e"_mst) ||  (e << "d"_mst)); // e implies d
    Assume(!(e << "V"_mst) || !(e << "e"_mst)); // V conflicts with e
    Assume(!(e << "d"_mst) || !(e << "f"_mst)); // d conflicts with f
    Assume(!(e << "V"_mst) ||  (e << "f"_mst)); // V implies f
    Assume(!(e << "K"_mst) ||  (e << "s"_mst)); // K implies s
    Assume(!(e << "z"_mst) ||  (e << "m"_mst)); // z implies m
    return e;
}

Type ComputeType(Fragment fragment, Type x, Type y, Type z, const std::vector<Type>& sub_types, uint32_t k,
                 size_t data_size, size_t n_subs, size_t n_keys, MiniscriptContext ms_ctx) {
    // Sanity check on data
    if (fragment == Fragment::SHA256 || fragment == Fragment::HASH256) {
        Assume(data_size == 32);
    } else if (fragment == Fragment::RIPEMD160 || fragment == Fragment::HASH160) {
        Assume(data_size == 20);
    } else {
        Assume(data_size == 0);
    }
    // Sanity check on k
    if (fragment == Fragment::OLDER || fragment == Fragment::AFTER) {
        Assume(k >= 1 && k < 0x80000000UL);
    } else if (fragment == Fragment::MULTI || fragment == Fragment::MULTI_A) {
        Assume(k >= 1 && k <= n_keys);
    } else if (fragment == Fragment::THRESH) {
        Assume(k >= 1 && k <= n_subs);
    } else {
        Assume(k == 0);
    }
    // Sanity check on subs
    if (fragment == Fragment::AND_V || fragment == Fragment::AND_B || fragment == Fragment::OR_B ||
        fragment == Fragment::OR_C || fragment == Fragment::OR_I || fragment == Fragment::OR_D) {
        Assume(n_subs == 2);
    } else if (fragment == Fragment::ANDOR) {
        Assume(n_subs == 3);
    } else if (fragment == Fragment::WRAP_A || fragment == Fragment::WRAP_S || fragment == Fragment::WRAP_C ||
               fragment == Fragment::WRAP_D || fragment == Fragment::WRAP_V || fragment == Fragment::WRAP_J ||
               fragment == Fragment::WRAP_N) {
        Assume(n_subs == 1);
    } else if (fragment != Fragment::THRESH) {
        Assume(n_subs == 0);
    }
    // Sanity check on keys
    if (fragment == Fragment::PK_K || fragment == Fragment::PK_H) {
        Assume(n_keys == 1);
    } else if (fragment == Fragment::MULTI) {
        Assume(n_keys >= 1 && n_keys <= MAX_PUBKEYS_PER_MULTISIG);
        Assume(!IsTapscript(ms_ctx));
    } else if (fragment == Fragment::MULTI_A) {
        Assume(n_keys >= 1 && n_keys <= MAX_PUBKEYS_PER_MULTI_A);
        Assume(IsTapscript(ms_ctx));
    } else {
        Assume(n_keys == 0);
    }

    // Below is the per-fragment logic for computing the expression types.
    // It heavily relies on Type's << operator (where "X << a_mst" means
    // "X has all properties listed in a").
    switch (fragment) {
        case Fragment::PK_K: return "Konudemsxk"_mst;
        case Fragment::PK_H: return "Knudemsxk"_mst;
        case Fragment::OLDER: return
            "g"_mst.If(k & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) |
            "h"_mst.If(!(k & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG)) |
            "Bzfmxk"_mst;
        case Fragment::AFTER: return
            "i"_mst.If(k >= LOCKTIME_THRESHOLD) |
            "j"_mst.If(k < LOCKTIME_THRESHOLD) |
            "Bzfmxk"_mst;
        case Fragment::SHA256: return "Bonudmk"_mst;
        case Fragment::RIPEMD160: return "Bonudmk"_mst;
        case Fragment::HASH256: return "Bonudmk"_mst;
        case Fragment::HASH160: return "Bonudmk"_mst;
        case Fragment::JUST_1: return "Bzufmxk"_mst;
        case Fragment::JUST_0: return "Bzudemsxk"_mst;
        case Fragment::WRAP_A: return
            "W"_mst.If(x << "B"_mst) | // W=B_x
            (x & "ghijk"_mst) | // g=g_x, h=h_x, i=i_x, j=j_x, k=k_x
            (x & "udfems"_mst) | // u=u_x, d=d_x, f=f_x, e=e_x, m=m_x, s=s_x
            "x"_mst; // x
        case Fragment::WRAP_S: return
            "W"_mst.If(x << "Bo"_mst) | // W=B_x*o_x
            (x & "ghijk"_mst) | // g=g_x, h=h_x, i=i_x, j=j_x, k=k_x
            (x & "udfemsx"_mst); // u=u_x, d=d_x, f=f_x, e=e_x, m=m_x, s=s_x, x=x_x
        case Fragment::WRAP_C: return
            "B"_mst.If(x << "K"_mst) | // B=K_x
            (x & "ghijk"_mst) | // g=g_x, h=h_x, i=i_x, j=j_x, k=k_x
            (x & "ondfem"_mst) | // o=o_x, n=n_x, d=d_x, f=f_x, e=e_x, m=m_x
            "us"_mst; // u, s
        case Fragment::WRAP_D: return
            "B"_mst.If(x << "Vz"_mst) | // B=V_x*z_x
            "o"_mst.If(x << "z"_mst) | // o=z_x
            "e"_mst.If(x << "f"_mst) | // e=f_x
            (x & "ghijk"_mst) | // g=g_x, h=h_x, i=i_x, j=j_x, k=k_x
            (x & "ms"_mst) | // m=m_x, s=s_x
            // NOTE: 'd:' is 'u' under Tapscript but not P2WSH as MINIMALIF is only a policy rule there.
            "u"_mst.If(IsTapscript(ms_ctx)) |
            "ndx"_mst; // n, d, x
        case Fragment::WRAP_V: return
            "V"_mst.If(x << "B"_mst) | // V=B_x
            (x & "ghijk"_mst) | // g=g_x, h=h_x, i=i_x, j=j_x, k=k_x
            (x & "zonms"_mst) | // z=z_x, o=o_x, n=n_x, m=m_x, s=s_x
            "fx"_mst; // f, x
        case Fragment::WRAP_J: return
            "B"_mst.If(x << "Bn"_mst) | // B=B_x*n_x
            "e"_mst.If(x << "f"_mst) | // e=f_x
            (x & "ghijk"_mst) | // g=g_x, h=h_x, i=i_x, j=j_x, k=k_x
            (x & "oums"_mst) | // o=o_x, u=u_x, m=m_x, s=s_x
            "ndx"_mst; // n, d, x
        case Fragment::WRAP_N: return
            (x & "ghijk"_mst) | // g=g_x, h=h_x, i=i_x, j=j_x, k=k_x
            (x & "Bzondfems"_mst) | // B=B_x, z=z_x, o=o_x, n=n_x, d=d_x, f=f_x, e=e_x, m=m_x, s=s_x
            "ux"_mst; // u, x
        case Fragment::AND_V: return
            (y & "KVB"_mst).If(x << "V"_mst) | // B=V_x*B_y, V=V_x*V_y, K=V_x*K_y
            (x & "n"_mst) | (y & "n"_mst).If(x << "z"_mst) | // n=n_x+z_x*n_y
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) | // o=o_x*z_y+z_x*o_y
            (x & y & "dmz"_mst) | // d=d_x*d_y, m=m_x*m_y, z=z_x*z_y
            ((x | y) & "s"_mst) | // s=s_x+s_y
            "f"_mst.If((y << "f"_mst) || (x << "s"_mst)) | // f=f_y+s_x
            (y & "ux"_mst) | // u=u_y, x=x_y
            ((x | y) & "ghij"_mst) | // g=g_x+g_y, h=h_x+h_y, i=i_x+i_y, j=j_x+j_y
            "k"_mst.If(((x & y) << "k"_mst) &&
                !(((x << "g"_mst) && (y << "h"_mst)) ||
                ((x << "h"_mst) && (y << "g"_mst)) ||
                ((x << "i"_mst) && (y << "j"_mst)) ||
                ((x << "j"_mst) && (y << "i"_mst)))); // k=k_x*k_y*!(g_x*h_y + h_x*g_y + i_x*j_y + j_x*i_y)
        case Fragment::AND_B: return
            (x & "B"_mst).If(y << "W"_mst) | // B=B_x*W_y
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) | // o=o_x*z_y+z_x*o_y
            (x & "n"_mst) | (y & "n"_mst).If(x << "z"_mst) | // n=n_x+z_x*n_y
            (x & y & "e"_mst).If((x & y) << "s"_mst) | // e=e_x*e_y*s_x*s_y
            (x & y & "dzm"_mst) | // d=d_x*d_y, z=z_x*z_y, m=m_x*m_y
            "f"_mst.If(((x & y) << "f"_mst) || (x << "sf"_mst) || (y << "sf"_mst)) | // f=f_x*f_y + f_x*s_x + f_y*s_y
            ((x | y) & "s"_mst) | // s=s_x+s_y
            "ux"_mst | // u, x
            ((x | y) & "ghij"_mst) | // g=g_x+g_y, h=h_x+h_y, i=i_x+i_y, j=j_x+j_y
            "k"_mst.If(((x & y) << "k"_mst) &&
                !(((x << "g"_mst) && (y << "h"_mst)) ||
                ((x << "h"_mst) && (y << "g"_mst)) ||
                ((x << "i"_mst) && (y << "j"_mst)) ||
                ((x << "j"_mst) && (y << "i"_mst)))); // k=k_x*k_y*!(g_x*h_y + h_x*g_y + i_x*j_y + j_x*i_y)
        case Fragment::OR_B: return
            "B"_mst.If(x << "Bd"_mst && y << "Wd"_mst) | // B=B_x*d_x*W_x*d_y
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) | // o=o_x*z_y+z_x*o_y
            (x & y & "m"_mst).If((x | y) << "s"_mst && (x & y) << "e"_mst) | // m=m_x*m_y*e_x*e_y*(s_x+s_y)
            (x & y & "zse"_mst) | // z=z_x*z_y, s=s_x*s_y, e=e_x*e_y
            "dux"_mst | // d, u, x
            ((x | y) & "ghij"_mst) | // g=g_x+g_y, h=h_x+h_y, i=i_x+i_y, j=j_x+j_y
            (x & y & "k"_mst); // k=k_x*k_y
        case Fragment::OR_D: return
            (y & "B"_mst).If(x << "Bdu"_mst) | // B=B_y*B_x*d_x*u_x
            (x & "o"_mst).If(y << "z"_mst) | // o=o_x*z_y
            (x & y & "m"_mst).If(x << "e"_mst && (x | y) << "s"_mst) | // m=m_x*m_y*e_x*(s_x+s_y)
            (x & y & "zs"_mst) | // z=z_x*z_y, s=s_x*s_y
            (y & "ufde"_mst) | // u=u_y, f=f_y, d=d_y, e=e_y
            "x"_mst | // x
            ((x | y) & "ghij"_mst) | // g=g_x+g_y, h=h_x+h_y, i=i_x+i_y, j=j_x+j_y
            (x & y & "k"_mst); // k=k_x*k_y
        case Fragment::OR_C: return
            (y & "V"_mst).If(x << "Bdu"_mst) | // V=V_y*B_x*u_x*d_x
            (x & "o"_mst).If(y << "z"_mst) | // o=o_x*z_y
            (x & y & "m"_mst).If(x << "e"_mst && (x | y) << "s"_mst) | // m=m_x*m_y*e_x*(s_x+s_y)
            (x & y & "zs"_mst) | // z=z_x*z_y, s=s_x*s_y
            "fx"_mst | // f, x
            ((x | y) & "ghij"_mst) | // g=g_x+g_y, h=h_x+h_y, i=i_x+i_y, j=j_x+j_y
            (x & y & "k"_mst); // k=k_x*k_y
        case Fragment::OR_I: return
            (x & y & "VBKufs"_mst) | // V=V_x*V_y, B=B_x*B_y, K=K_x*K_y, u=u_x*u_y, f=f_x*f_y, s=s_x*s_y
            "o"_mst.If((x & y) << "z"_mst) | // o=z_x*z_y
            ((x | y) & "e"_mst).If((x | y) << "f"_mst) | // e=e_x*f_y+f_x*e_y
            (x & y & "m"_mst).If((x | y) << "s"_mst) | // m=m_x*m_y*(s_x+s_y)
            ((x | y) & "d"_mst) | // d=d_x+d_y
            "x"_mst | // x
            ((x | y) & "ghij"_mst) | // g=g_x+g_y, h=h_x+h_y, i=i_x+i_y, j=j_x+j_y
            (x & y & "k"_mst); // k=k_x*k_y
        case Fragment::ANDOR: return
            (y & z & "BKV"_mst).If(x << "Bdu"_mst) | // B=B_x*d_x*u_x*B_y*B_z, K=B_x*d_x*u_x*K_y*K_z, V=B_x*d_x*u_x*V_y*V_z
            (x & y & z & "z"_mst) | // z=z_x*z_y*z_z
            ((x | (y & z)) & "o"_mst).If((x | (y & z)) << "z"_mst) | // o=o_x*z_y*z_z+z_x*o_y*o_z
            (y & z & "u"_mst) | // u=u_y*u_z
            (z & "f"_mst).If((x << "s"_mst) || (y << "f"_mst)) | // f=(s_x+f_y)*f_z
            (z & "d"_mst) | // d=d_z
            (z & "e"_mst).If(x << "s"_mst || y << "f"_mst) | // e=e_z*(s_x+f_y)
            (x & y & z & "m"_mst).If(x << "e"_mst && (x | y | z) << "s"_mst) | // m=m_x*m_y*m_z*e_x*(s_x+s_y+s_z)
            (z & (x | y) & "s"_mst) | // s=s_z*(s_x+s_y)
            "x"_mst | // x
            ((x | y | z) & "ghij"_mst) | // g=g_x+g_y+g_z, h=h_x+h_y+h_z, i=i_x+i_y+i_z, j=j_x+j_y_j_z
            "k"_mst.If(((x & y & z) << "k"_mst) &&
                !(((x << "g"_mst) && (y << "h"_mst)) ||
                ((x << "h"_mst) && (y << "g"_mst)) ||
                ((x << "i"_mst) && (y << "j"_mst)) ||
                ((x << "j"_mst) && (y << "i"_mst)))); // k=k_x*k_y*k_z* !(g_x*h_y + h_x*g_y + i_x*j_y + j_x*i_y)
        case Fragment::MULTI: {
            return "Bnudemsk"_mst;
        }
        case Fragment::MULTI_A: {
            return "Budemsk"_mst;
        }
        case Fragment::THRESH: {
            bool all_e = true;
            bool all_m = true;
            uint32_t args = 0;
            uint32_t num_s = 0;
            Type acc_tl = "k"_mst;
            for (size_t i = 0; i < sub_types.size(); ++i) {
                Type t = sub_types[i];
                static constexpr auto WDU{"Wdu"_mst}, BDU{"Bdu"_mst};
                if (!(t << (i ? WDU : BDU))) return ""_mst; // Require Bdu, Wdu, Wdu, ...
                if (!(t << "e"_mst)) all_e = false;
                if (!(t << "m"_mst)) all_m = false;
                if (t << "s"_mst) num_s += 1;
                args += (t << "z"_mst) ? 0 : (t << "o"_mst) ? 1 : 2;
                acc_tl = ((acc_tl | t) & "ghij"_mst) |
                    // Thresh contains a combination of timelocks if it has threshold > 1 and
                    // it contains two different children that have different types of timelocks
                    // Note how if any of the children don't have "k", the parent also does not have "k"
                    "k"_mst.If(((acc_tl & t) << "k"_mst) && ((k <= 1) ||
                        ((k > 1) && !(((acc_tl << "g"_mst) && (t << "h"_mst)) ||
                        ((acc_tl << "h"_mst) && (t << "g"_mst)) ||
                        ((acc_tl << "i"_mst) && (t << "j"_mst)) ||
                        ((acc_tl << "j"_mst) && (t << "i"_mst))))));
            }
            return "Bdu"_mst |
                   "z"_mst.If(args == 0) | // z=all z
                   "o"_mst.If(args == 1) | // o=all z except one o
                   "e"_mst.If(all_e && num_s == n_subs) | // e=all e and all s
                   "m"_mst.If(all_e && all_m && num_s >= n_subs - k) | // m=all e, >=(n-k) s
                   "s"_mst.If(num_s >= n_subs - k + 1) |  // s= >=(n-k+1) s
                   acc_tl; // timelock info
            }
    }
    assert(false);
}

size_t ComputeScriptLen(Fragment fragment, Type sub0typ, size_t subsize, uint32_t k, size_t n_subs,
                        size_t n_keys, MiniscriptContext ms_ctx) {
    switch (fragment) {
        case Fragment::JUST_1:
        case Fragment::JUST_0: return 1;
        case Fragment::PK_K: return IsTapscript(ms_ctx) ? 33 : 34;
        case Fragment::PK_H: return 3 + 21;
        case Fragment::OLDER:
        case Fragment::AFTER: return 1 + BuildScript(k).size();
        case Fragment::HASH256:
        case Fragment::SHA256: return 4 + 2 + 33;
        case Fragment::HASH160:
        case Fragment::RIPEMD160: return 4 + 2 + 21;
        case Fragment::MULTI: return 1 + BuildScript(n_keys).size() + BuildScript(k).size() + 34 * n_keys;
        case Fragment::MULTI_A: return (1 + 32 + 1) * n_keys + BuildScript(k).size() + 1;
        case Fragment::AND_V: return subsize;
        case Fragment::WRAP_V: return subsize + (sub0typ << "x"_mst);
        case Fragment::WRAP_S:
        case Fragment::WRAP_C:
        case Fragment::WRAP_N:
        case Fragment::AND_B:
        case Fragment::OR_B: return subsize + 1;
        case Fragment::WRAP_A:
        case Fragment::OR_C: return subsize + 2;
        case Fragment::WRAP_D:
        case Fragment::OR_D:
        case Fragment::OR_I:
        case Fragment::ANDOR: return subsize + 3;
        case Fragment::WRAP_J: return subsize + 4;
        case Fragment::THRESH: return subsize + n_subs + BuildScript(k).size();
    }
    assert(false);
}

InputStack& InputStack::SetAvailable(Availability avail) {
    available = avail;
    if (avail == Availability::NO) {
        stack.clear();
        size = std::numeric_limits<size_t>::max();
        has_sig = false;
        malleable = false;
        non_canon = false;
    }
    return *this;
}

InputStack& InputStack::SetWithSig() {
    has_sig = true;
    return *this;
}

InputStack& InputStack::SetNonCanon() {
    non_canon = true;
    return *this;
}

InputStack& InputStack::SetMalleable(bool x) {
    malleable = x;
    return *this;
}

InputStack operator+(InputStack a, InputStack b) {
    a.stack = Cat(std::move(a.stack), std::move(b.stack));
    if (a.available != Availability::NO && b.available != Availability::NO) a.size += b.size;
    a.has_sig |= b.has_sig;
    a.malleable |= b.malleable;
    a.non_canon |= b.non_canon;
    if (a.available == Availability::NO || b.available == Availability::NO) {
        a.SetAvailable(Availability::NO);
    } else if (a.available == Availability::MAYBE || b.available == Availability::MAYBE) {
        a.SetAvailable(Availability::MAYBE);
    }
    return a;
}

InputStack operator|(InputStack a, InputStack b) {
    // If only one is invalid, pick the other one. If both are invalid, pick an arbitrary one.
    if (a.available == Availability::NO) return b;
    if (b.available == Availability::NO) return a;
    // If only one of the solutions has a signature, we must pick the other one.
    if (!a.has_sig && b.has_sig) return a;
    if (!b.has_sig && a.has_sig) return b;
    if (!a.has_sig && !b.has_sig) {
        // If neither solution requires a signature, the result is inevitably malleable.
        a.malleable = true;
        b.malleable = true;
    } else {
        // If both options require a signature, prefer the non-malleable one.
        if (b.malleable && !a.malleable) return a;
        if (a.malleable && !b.malleable) return b;
    }
    // Between two malleable or two non-malleable solutions, pick the smaller one between
    // YESes, and the bigger ones between MAYBEs. Prefer YES over MAYBE.
    if (a.available == Availability::YES && b.available == Availability::YES) {
        return std::move(a.size <= b.size ? a : b);
    } else if (a.available == Availability::MAYBE && b.available == Availability::MAYBE) {
        return std::move(a.size >= b.size ? a : b);
    } else if (a.available == Availability::YES) {
        return a;
    } else {
        return b;
    }
}

std::optional<std::vector<Opcode>> DecomposeScript(const CScript& script)
{
    std::vector<Opcode> out;
    CScript::const_iterator it = script.begin(), itend = script.end();
    while (it != itend) {
        std::vector<unsigned char> push_data;
        opcodetype opcode;
        if (!script.GetOp(it, opcode, push_data)) {
            return {};
        } else if (opcode >= OP_1 && opcode <= OP_16) {
            // Deal with OP_n (GetOp does not turn them into pushes).
            push_data.assign(1, CScript::DecodeOP_N(opcode));
        } else if (opcode == OP_CHECKSIGVERIFY) {
            // Decompose OP_CHECKSIGVERIFY into OP_CHECKSIG OP_VERIFY
            out.emplace_back(OP_CHECKSIG, std::vector<unsigned char>());
            opcode = OP_VERIFY;
        } else if (opcode == OP_CHECKMULTISIGVERIFY) {
            // Decompose OP_CHECKMULTISIGVERIFY into OP_CHECKMULTISIG OP_VERIFY
            out.emplace_back(OP_CHECKMULTISIG, std::vector<unsigned char>());
            opcode = OP_VERIFY;
        } else if (opcode == OP_EQUALVERIFY) {
            // Decompose OP_EQUALVERIFY into OP_EQUAL OP_VERIFY
            out.emplace_back(OP_EQUAL, std::vector<unsigned char>());
            opcode = OP_VERIFY;
        } else if (opcode == OP_NUMEQUALVERIFY) {
            // Decompose OP_NUMEQUALVERIFY into OP_NUMEQUAL OP_VERIFY
            out.emplace_back(OP_NUMEQUAL, std::vector<unsigned char>());
            opcode = OP_VERIFY;
        } else if (IsPushdataOp(opcode)) {
            if (!CheckMinimalPush(push_data, opcode)) return {};
        } else if (it != itend && (opcode == OP_CHECKSIG || opcode == OP_CHECKMULTISIG || opcode == OP_EQUAL || opcode == OP_NUMEQUAL) && (*it == OP_VERIFY)) {
            // Rule out non minimal VERIFY sequences
            return {};
        }
        out.emplace_back(opcode, std::move(push_data));
    }
    std::reverse(out.begin(), out.end());
    return out;
}

std::optional<int64_t> ParseScriptNumber(const Opcode& in) {
    if (in.first == OP_0) {
        return 0;
    }
    if (!in.second.empty()) {
        if (IsPushdataOp(in.first) && !CheckMinimalPush(in.second, in.first)) return {};
        try {
            return CScriptNum(in.second, true).GetInt64();
        } catch(const scriptnum_error&) {}
    }
    return {};
}

int FindNextChar(Span<const char> sp, const char m)
{
    for (int i = 0; i < (int)sp.size(); ++i) {
        if (sp[i] == m) return i;
        // We only search within the current parentheses
        if (sp[i] == ')') break;
    }
    return -1;
}

void SanityCheckSatisfaction(Type typ, const InputResult& ret)
{
    // Do a consistency check between the satisfaction code and the type checker
    // (the actual satisfaction code in ProduceInputHelper does not use GetType)

    // For 'z' nodes, available satisfactions/dissatisfactions must have stack size 0.
    if (typ << "z"_mst && ret.nsat.available != Availability::NO) Assume(ret.nsat.stack.size() == 0);
    if (typ << "z"_mst && ret.sat.available != Availability::NO) Assume(ret.sat.stack.size() == 0);

    // For 'o' nodes, available satisfactions/dissatisfactions must have stack size 1.
    if (typ << "o"_mst && ret.nsat.available != Availability::NO) Assume(ret.nsat.stack.size() == 1);
    if (typ << "o"_mst && ret.sat.available != Availability::NO) Assume(ret.sat.stack.size() == 1);

    // For 'n' nodes, available satisfactions/dissatisfactions must have stack size 1 or larger. For satisfactions,
    // the top element cannot be 0.
    if (typ << "n"_mst && ret.sat.available != Availability::NO) Assume(ret.sat.stack.size() >= 1);
    if (typ << "n"_mst && ret.nsat.available != Availability::NO) Assume(ret.nsat.stack.size() >= 1);
    if (typ << "n"_mst && ret.sat.available != Availability::NO) Assume(!ret.sat.stack.back().empty());

    // For 'd' nodes, a dissatisfaction must exist, and they must not need a signature. If it is non-malleable,
    // it must be canonical.
    if (typ << "d"_mst) Assume(ret.nsat.available != Availability::NO);
    if (typ << "d"_mst) Assume(!ret.nsat.has_sig);
    if (typ << "d"_mst && !ret.nsat.malleable) Assume(!ret.nsat.non_canon);

    // For 'f'/'s' nodes, dissatisfactions/satisfactions must have a signature.
    if (typ << "f"_mst && ret.nsat.available != Availability::NO) Assume(ret.nsat.has_sig);
    if (typ << "s"_mst && ret.sat.available != Availability::NO) Assume(ret.sat.has_sig);

    // For non-malleable 'e' nodes, a non-malleable dissatisfaction must exist.
    if (typ << "me"_mst) Assume(ret.nsat.available != Availability::NO);
    if (typ << "me"_mst) Assume(!ret.nsat.malleable);

    // For 'm' nodes, if a satisfaction exists, it must be non-malleable.
    if (typ << "m"_mst && ret.sat.available != Availability::NO) Assume(!ret.sat.malleable);

    // If a non-malleable satisfaction exists, it must be canonical.
    if (ret.sat.available != Availability::NO && !ret.sat.malleable) Assume(!ret.sat.non_canon);
}

} // namespace internal
} // namespace miniscript

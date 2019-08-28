// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include <script/script.h>
#include <script/miniscript.h>

#include <assert.h>

namespace miniscript {
namespace internal {

Type SanitizeType(Type e) {
    int num_types = (e << "K"_mst) + (e << "V"_mst) + (e << "B"_mst) + (e << "W"_mst);
    if (num_types == 0) return ""_mst; // No valid type, don't care about the rest
    assert(num_types == 1); // K, V, B, W all conflict with each other
    bool ok = // Work around a GCC 4.8 bug that breaks user-defined literals in macro calls.
        (!(e << "z"_mst) || !(e << "o"_mst)) && // z conflicts with o
        (!(e << "n"_mst) || !(e << "z"_mst)) && // n conflicts with z
        (!(e << "V"_mst) || !(e << "d"_mst)) && // V conflicts with d
        (!(e << "K"_mst) ||  (e << "u"_mst)) && // K implies u
        (!(e << "V"_mst) || !(e << "u"_mst)) && // V conflicts with u
        (!(e << "e"_mst) || !(e << "f"_mst)) && // e conflicts with f
        (!(e << "e"_mst) ||  (e << "d"_mst)) && // e implies d
        (!(e << "V"_mst) || !(e << "e"_mst)) && // V conflicts with e
        (!(e << "d"_mst) || !(e << "f"_mst)) && // d conflicts with f
        (!(e << "V"_mst) ||  (e << "f"_mst)) && // V implies f
        (!(e << "K"_mst) ||  (e << "s"_mst)) && // K implies s
        (!(e << "z"_mst) ||  (e << "m"_mst)); // z implies m
    assert(ok);
    return e;
}

Type ComputeType(NodeType nodetype, Type x, Type y, Type z, const std::vector<Type>& sub_types, uint32_t k, size_t data_size, size_t n_subs, size_t n_keys) {
    // Sanity check on data
    if (nodetype == NodeType::SHA256 || nodetype == NodeType::HASH256) {
        assert(data_size == 32);
    } else if (nodetype == NodeType::RIPEMD160 || nodetype == NodeType::HASH160) {
        assert(data_size == 20);
    } else {
        assert(data_size == 0);
    }
    // Sanity check on k
    if (nodetype == NodeType::OLDER || nodetype == NodeType::AFTER) {
        assert(k >= 1 && k < 0x80000000UL);
    } else if (nodetype == NodeType::THRESH_M) {
        assert(k >= 1 && k <= n_keys);
    } else if (nodetype == NodeType::THRESH) {
        assert(k > 1 && k < n_subs);
    } else {
        assert(k == 0);
    }
    // Sanity check on subs
    if (nodetype == NodeType::AND_V || nodetype == NodeType::AND_B || nodetype == NodeType::OR_B ||
        nodetype == NodeType::OR_C || nodetype == NodeType::OR_I || nodetype == NodeType::OR_D) {
        assert(n_subs == 2);
    } else if (nodetype == NodeType::ANDOR) {
        assert(n_subs == 3);
    } else if (nodetype == NodeType::WRAP_A || nodetype == NodeType::WRAP_S || nodetype == NodeType::WRAP_C ||
               nodetype == NodeType::WRAP_D || nodetype == NodeType::WRAP_V || nodetype == NodeType::WRAP_J ||
               nodetype == NodeType::WRAP_N) {
        assert(n_subs == 1);
    } else if (nodetype != NodeType::THRESH) {
        assert(n_subs == 0);
    }
    // Sanity check on keys
    if (nodetype == NodeType::PK || nodetype == NodeType::PK_H) {
        assert(n_keys == 1);
    } else if (nodetype == NodeType::THRESH_M) {
        assert(n_keys >= 1 && n_keys <= 20);
    } else {
        assert(n_keys == 0);
    }

    // Below is the per-nodetype logic for computing the expression types.
    // It heavily relies on Type's << operator (where "X << a_mst" means
    // "X has all properties listed in a").
    switch (nodetype) {
        case NodeType::PK: return "Konudemsx"_mst;
        case NodeType::PK_H: return "Knudemsx"_mst;
        case NodeType::OLDER: return "Bzfmx"_mst;
        case NodeType::AFTER: return "Bzfmx"_mst;
        case NodeType::SHA256: return "Bonudm"_mst;
        case NodeType::RIPEMD160: return "Bonudm"_mst;
        case NodeType::HASH256: return "Bonudm"_mst;
        case NodeType::HASH160: return "Bonudm"_mst;
        case NodeType::JUST_1: return "Bzufmx"_mst;
        case NodeType::JUST_0: return "Bzudemsx"_mst;
        case NodeType::WRAP_A: return
            "W"_mst.If(x << "B"_mst) | // W=B_x
            (x & "udfems"_mst) | // u=u_x, d=d_x, f=f_x, e=e_x, m=m_x, s=s_x
            "x"_mst; // x
        case NodeType::WRAP_S: return
            "W"_mst.If(x << "Bo"_mst) | // W=B_x*o_x
            (x & "udfemsx"_mst); // u=u_x, d=d_x, f=f_x, e=e_x, m=m_x, s=s_x, x=x_x
        case NodeType::WRAP_C: return
            "B"_mst.If(x << "K"_mst) | // B=K_x
             (x & "ondfem"_mst) | // o=o_x, n=n_x, d=d_x, f=f_x, e=e_x, m=m_x
             "us"_mst; // u, s
        case NodeType::WRAP_D: return
            "B"_mst.If(x << "Vz"_mst) | // B=V_x*z_x
            "o"_mst.If(x << "z"_mst) | // o=z_x
            "e"_mst.If(x << "f"_mst) | // e=f_x
            (x & "ms"_mst) | // m=m_x, s=s_x
            "nudx"_mst; // n, u, d, x
        case NodeType::WRAP_V: return
            "V"_mst.If(x << "B"_mst) | // V=B_x
            (x & "zonms"_mst) | // z=z_x, o=o_x, n=n_x, m=m_x, s=s_x
            "fx"_mst; // f, x
        case NodeType::WRAP_J: return
            "B"_mst.If(x << "Bn"_mst) | // B=B_x*n_x
            "e"_mst.If(x << "f"_mst) | // e=f_x
            (x & "oums"_mst) | // o=o_x, u=u_x, m=m_x, s=s_x
            "ndx"_mst; // n, d, x
        case NodeType::WRAP_N: return
            (x & "Bzondfems"_mst) | // B=B_x, z=z_x, o=o_x, n=n_x, d=d_x, f=f_x, e=e_x, m=m_x, s=s_x
            "ux"_mst; // u, x
        case NodeType::AND_V: return
            (y & "KVB"_mst).If(x << "V"_mst) | // B=V_x*B_y, V=V_x*V_y, K=V_x*K_y
            (x & "n"_mst) | (y & "n"_mst).If(x << "z"_mst) | // n=n_x+z_x*n_y
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) | // o=o_x*z_y+z_x*o_y
            (x & y & "dmz"_mst) | // d=d_x*d_y, m=m_x*m_y, z=z_x*z_y
            ((x | y) & "s"_mst) | // s=s_x+s_y
            "f"_mst.If((y << "f"_mst) || (x << "s"_mst)) | // f=f_y+s_x
            (y & "ux"_mst); // u=u_y, x=x_y
        case NodeType::AND_B: return
            (x & "B"_mst).If(y << "W"_mst) | // B=B_x*W_y
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) | // o=o_x*z_y+z_x*o_y
            (x & "n"_mst) | (y & "n"_mst).If(x << "z"_mst) | // n=n_x+z_x*n_y
            (x & y & "e"_mst).If((x & y) << "s"_mst) | // e=e_x*e_y*s_x*s_y
            (x & y & "dzm"_mst) | // d=d_x*d_y, z=z_x*z_y, m=m_x*m_y
            "f"_mst.If(((x & y) << "f"_mst) || (x << "sf"_mst) || (y << "sf"_mst)) | // f=f_x*f_y + f_x*s_x + f_y*s_y
            ((x | y) & "s"_mst) | // s=s_x+s_y
            "ux"_mst; // u, x
        case NodeType::OR_B: return
            "B"_mst.If(x << "Bd"_mst && y << "Wd"_mst) | // B=B_x*d_x*W_x*d_y
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) | // o=o_x*z_y+z_x*o_y
            (x & y & "m"_mst).If((x | y) << "s"_mst && (x & y) << "e"_mst) | // m=m_x*m_y*e_x*e_y*(s_x+s_y)
            (x & y & "zse"_mst) | // z=z_x*z_y, s=s_x*s_y, e=e_x*e_y
            "dux"_mst; // d, u, x
        case NodeType::OR_D: return
            (y & "B"_mst).If(x << "Bdu"_mst) | // B=B_y*B_x*d_x*u_x
            (x & "o"_mst).If(y << "z"_mst) | // o=o_x*z_y
            (x & y & "m"_mst).If(x << "e"_mst && (x | y) << "s"_mst) | // m=m_x*m_y*e_x*(s_x+s_y)
            (x & y & "zes"_mst) | // z=z_x*z_y, e=e_x*e_y, s=s_x*s_y
            (y & "ufd"_mst) | // u=u_y, f=f_y, d=d_y
            "x"_mst; // x
        case NodeType::OR_C: return
            (y & "V"_mst).If(x << "Bdu"_mst) | // V=V_y*B_x*u_x*d_x
            (x & "o"_mst).If(y << "z"_mst) | // o=o_x*z_y
            (x & y & "m"_mst).If(x << "e"_mst && (x | y) << "s"_mst) | // m=m_x*m_y*e_x*(s_x*s_y)
            (x & y & "zs"_mst) | // z=z_x*z_y, s=s_x*s_y
            "fx"_mst; // f, x
        case NodeType::OR_I: return
            (x & y & "VBKufs"_mst) | // V=V_x*V_y, B=B_x*B_y, K=K_x*K_y, u=u_x*u_y, f=f_x*f_y, s=s_x*s_y
            "o"_mst.If((x & y) << "z"_mst) | // o=z_x*z_y
            ((x | y) & "e"_mst).If((x | y) << "f"_mst) | // e=e_x*f_y+f_x*e_y
            (x & y & "m"_mst).If((x | y) << "s"_mst) | // m=m_x*m_y*(s_x+s_y)
            ((x | y) & "d"_mst) | // d=d_x+d_y
            "x"_mst; // x
        case NodeType::ANDOR: return
            (y & z & "BKV"_mst).If(x << "Bdu"_mst) | // B=B_x*d_x*u_x*B_y*B_z, K=B_x*d_x*u_x*K_y*K_z, V=B_x*d_x*u_x*V_y*V_z
            (x & y & z & "z"_mst) | // z=z_x*z_y*z_z
            ((x | (y & z)) & "o"_mst).If((x | (y & z)) << "z"_mst) | // o=o_x*z_y*z_z+z_x+o_y*o_z
            (y & z & "u"_mst) | // f=f_y*f_z, u=u_y*u_z
            (z & "f"_mst).If((x << "s"_mst) || (y << "f"_mst)) |
            (z & "d"_mst) | // d=d_x
            (x & z & "e"_mst).If(x << "s"_mst || y << "f"_mst) | // e=e_x*e_z*(s_x+s_y)
            (x & y & z & "m"_mst).If(x << "e"_mst && (x | y | z) << "s"_mst) | // m=m_x*m_y*m_z*e_x*(s_x+s_y+s_z)
            (z & (x | y) & "s"_mst) | // s=s_z*(s_x+s_y)
            "x"_mst; // x
        case NodeType::THRESH_M: return "Bnudems"_mst;
        case NodeType::THRESH: {
            bool all_e = true;
            bool all_m = true;
            uint32_t args = 0;
            uint32_t num_s = 0;
            for (size_t i = 0; i < sub_types.size(); ++i) {
                Type t = sub_types[i];
                if (!(t << (i ? "Wdu"_mst : "Bdu"_mst))) return ""_mst; // Require Bdu, Wdu, Wdu, ...
                if (!(t << "e"_mst)) all_e = false;
                if (!(t << "m"_mst)) all_m = false;
                if (t << "s"_mst) num_s += 1;
                args += (t << "z"_mst) ? 0 : (t << "o"_mst) ? 1 : 2;
            }
            return "Bdu"_mst |
                   "z"_mst.If(args == 0) | // z=all z
                   "o"_mst.If(args == 1) | // o=all z except one o
                   "e"_mst.If(all_e && num_s == n_subs) | // e=all e and all s
                   "m"_mst.If(all_e && all_m && num_s >= n_subs - k) | // m=all e, >=(n-k) s
                   "s"_mst.If(num_s >= n_subs - k + 1); // s= >=(n-k+1) s
            }
    }
    assert(false);
    return ""_mst;
}

size_t ComputeScriptLen(NodeType nodetype, Type sub0typ, size_t subsize, uint32_t k, size_t n_subs, size_t n_keys) {
    switch (nodetype) {
        case NodeType::PK: return subsize + 34;
        case NodeType::PK_H: return subsize + 3 + 21;
        case NodeType::OLDER: return subsize + 1 + (CScript() << k).size();
        case NodeType::AFTER: return subsize + 1 + (CScript() << k).size();
        case NodeType::HASH256: return subsize + 4 + 2 + 33;
        case NodeType::HASH160: return subsize + 4 + 2 + 21;
        case NodeType::SHA256: return subsize + 4 + 2 + 33;
        case NodeType::RIPEMD160: return subsize + 4 + 2 + 21;
        case NodeType::WRAP_A: return subsize + 2;
        case NodeType::WRAP_S: return subsize + 1;
        case NodeType::WRAP_C: return subsize + 1;
        case NodeType::WRAP_D: return subsize + 3;
        case NodeType::WRAP_V: return subsize + (sub0typ << "x"_mst);
        case NodeType::WRAP_J: return subsize + 4;
        case NodeType::WRAP_N: return subsize + 1;
        case NodeType::JUST_1: return 1;
        case NodeType::JUST_0: return 1;
        case NodeType::AND_V: return subsize;
        case NodeType::AND_B: return subsize + 1;
        case NodeType::OR_B: return subsize + 1;
        case NodeType::OR_D: return subsize + 3;
        case NodeType::OR_C: return subsize + 2;
        case NodeType::OR_I: return subsize + 3;
        case NodeType::ANDOR: return subsize + 3;
        case NodeType::THRESH: return subsize + n_subs + 1;
        case NodeType::THRESH_M: return subsize + 3 + (n_keys > 16) + (k > 16) + 34 * n_keys;
    }
    assert(false);
    return 0;
}

} // namespace internal
} // namespace miniscript

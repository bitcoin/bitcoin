// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/set_mem_proof/set_mem_proof.h>
#include <streams.h>

template <typename Stream>
void SetMemProof::Serialize(Stream& st) const
{
    st
    // st << phi
    //    << A1
    //    << A2
    //    << S1
    //    << S2
    //    << S3
    //    << T1
    //    << T2
    //    << tau_x
    //    << mu
    //    << z_alpha
    //    << z_tau
    //    << z_beta
    //    << t
       << Ls
    //    << Rs
    //    << a
    //    << b
    //    << omega
    //    << c_factor;
    ;
}
template
void SetMemProof::Serialize(CDataStream& st) const;

template <typename Stream>
void SetMemProof::Unserialize(Stream& st)
{
    st
    //    >> phi
    //    >> A1
    //    >> A2
    //    >> S1
    //    >> S2
    //    >> S3
    //    >> T1
    //    >> T2
    //    >> tau_x
    //    >> mu
    //    >> z_alpha
    //    >> z_tau
    //    >> z_beta
    //    >> t
       >> Ls
    //    >> Rs
    //    >> a
    //    >> b
    //    >> omega
    //    >> c_factor;
    ;
}
template
void SetMemProof::Unserialize(CDataStream& st);
// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/set_mem_proof/set_mem_proof.h>
#include <streams.h>

bool SetMemProof::operator==(const SetMemProof& other) const
{
    return phi == other.phi
        && A1 == other.A1
        && A2 == other.A2
        && S1 == other.S1
        && S2 == other.S2
        && S3 == other.S3
        && T1 == other.T1
        && T2 == other.T2
        && tau_x == other.tau_x
        && mu == other.mu
        && z_alpha == other.z_alpha
        && z_tau == other.z_tau
        && z_beta == other.z_beta
        && t == other.t
        && Ls == other.Ls
        && Rs == other.Rs
        && a == other.a
        && b == other.b
        && omega == other.omega;
}

bool SetMemProof::operator!=(const SetMemProof& other) const
{
    return !operator==(other);
}

template <typename Stream>
void SetMemProof::Serialize(Stream& st) const
{
    st << phi
       << A1
       << A2
       << S1
       << S2
       << S3
       << T1
       << T2
       << tau_x
       << mu
       << z_alpha
       << z_tau
       << z_beta
       << t
       << Ls
       << Rs
       << a
       << b
       << omega;
}
template
void SetMemProof::Serialize(CDataStream& st) const;

template <typename Stream>
void SetMemProof::Unserialize(Stream& st)
{
    st >> phi
       >> A1
       >> A2
       >> S1
       >> S2
       >> S3
       >> T1
       >> T2
       >> tau_x
       >> mu
       >> z_alpha
       >> z_tau
       >> z_beta
       >> t
       >> Ls
       >> Rs
       >> a
       >> b
       >> omega;
}
template
void SetMemProof::Unserialize(CDataStream& st);
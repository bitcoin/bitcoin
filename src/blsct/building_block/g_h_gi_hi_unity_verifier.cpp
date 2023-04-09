#include <blsct/building_block/g_h_gi_hi_unity_verifier.h>
#include <blsct/arith/mcl/mcl.h>

template <typename T>
using Scalar = typename G_H_Gi_Hi_UnityVerifier<T>::Scalar;

template <typename T>
using Point = typename G_H_Gi_Hi_UnityVerifier<T>::Point;

template <typename T>
using Points = typename G_H_Gi_Hi_UnityVerifier<T>::Points;

template <typename T>
void G_H_Gi_Hi_UnityVerifier<T>::AddPoint(const LazyPoint<T>& p)
{
    m_points.Add(p);
}
template
void G_H_Gi_Hi_UnityVerifier<Mcl>::AddPoint(const LazyPoint<Mcl>& p);

template <typename T>
void G_H_Gi_Hi_UnityVerifier<T>::AddPositiveG(const Scalar& exp)
{
    m_g_pos_exp = m_g_pos_exp + exp;
}
template
void G_H_Gi_Hi_UnityVerifier<Mcl>::AddPositiveG(const Scalar& exp);

template <typename T>
void G_H_Gi_Hi_UnityVerifier<T>::AddPositiveH(const Scalar& exp)
{
    m_h_pos_exp = m_h_pos_exp + exp;
}
template
void G_H_Gi_Hi_UnityVerifier<Mcl>::AddPositiveH(const Scalar& exp);

template <typename T>
void G_H_Gi_Hi_UnityVerifier<T>::AddNegativeG(const Scalar& exp)
{
    m_g_neg_exp = m_g_neg_exp + exp;
}
template
void G_H_Gi_Hi_UnityVerifier<Mcl>::AddNegativeG(const Scalar& exp);

template <typename T>
void G_H_Gi_Hi_UnityVerifier<T>::AddNegativeH(const Scalar& exp)
{
    m_h_neg_exp = m_h_neg_exp + exp;
}
template
void G_H_Gi_Hi_UnityVerifier<Mcl>::AddNegativeH(const Scalar& exp);

template <typename T>
void G_H_Gi_Hi_UnityVerifier<T>::SetGiExp(const size_t& i, const Scalar& s)
{
    m_gi_exps[i] = s;
}
template
void G_H_Gi_Hi_UnityVerifier<Mcl>::SetGiExp(const size_t& i, const Scalar& s);

template <typename T>
void G_H_Gi_Hi_UnityVerifier<T>::SetHiExp(const size_t& i, const Scalar& s)
{
    m_hi_exps[i] = s;
}
template
void G_H_Gi_Hi_UnityVerifier<Mcl>::SetHiExp(const size_t& i, const Scalar& s);

template <typename T>
bool G_H_Gi_Hi_UnityVerifier<T>::Verify(const Point& g, const Point&h, const Points& Gi, const Points& Hi)
{
    LazyPoints<T> points(m_points);
    points.Add(LazyPoint<T>(g, m_g_pos_exp - m_g_neg_exp));
    points.Add(LazyPoint<T>(h, m_h_pos_exp - m_h_neg_exp));

    for (size_t i=0; i<m_n; ++i) {
        points.Add(LazyPoint<T>(Gi[i], m_gi_exps[i]));
        points.Add(LazyPoint<T>(Hi[i], m_hi_exps[i]));
    }

    return points.Sum().IsUnity();
}
template
bool G_H_Gi_Hi_UnityVerifier<Mcl>::Verify(const Point& g, const Point&h, const Points& Gi, const Points& Hi);

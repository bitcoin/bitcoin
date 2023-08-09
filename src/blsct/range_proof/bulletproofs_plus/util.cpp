#include <blsct/range_proof/bulletproofs_plus/util.h>

namespace bulletproofs_plus {

template <typename T>
Elements<typename T::Scalar> Util<T>::GetYPows(
    const typename T::Scalar& y,
    const size_t& n,
    CHashWriter& fiat_shamir
) {
    using Scalars = Elements<typename T::Scalar>;

    fiat_shamir << "wipa v1";
    Scalars y_pows = Scalars::FirstNPow(y, n, 1);
    for (size_t i=0; i<y_pows.Size(); ++i) {
        fiat_shamir << y_pows[i];
    }
    fiat_shamir << y_pows.Size();

    return y_pows;
}
template Elements<Mcl::Scalar> Util<Mcl>::GetYPows(
    const Mcl::Scalar& y,
    const size_t& n,
    CHashWriter& fiat_shamir
);

} // namespace bulletproofs_plus
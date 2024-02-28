#include <blsct/arith/mcl/mcl.h>
#include <blsct/building_block/fiat_shamir.h>
#include <blsct/building_block/imp_inner_prod_arg.h>
#include <blsct/building_block/lazy_points.h>
#include <blsct/common.h>
#include <blsct/range_proof/setup.h>

template <typename T>
std::optional<ImpInnerProdArgResult<T>> ImpInnerProdArg::Run(
    const size_t& N,
    Elements<typename T::Point>& Gi,
    Elements<typename T::Point>& Hi,
    const typename T::Point& u,
    Elements<typename T::Scalar>& a,
    Elements<typename T::Scalar>& b,
    const typename T::Scalar& c_factor,
    const typename T::Scalar& y,
    HashWriter& fiat_shamir
) {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Scalars = Elements<Scalar>;

    const Scalars y_inv_pows = Scalars::FirstNPow(y.Invert(), N);
    size_t n = N;
    size_t rounds = 0;
    ImpInnerProdArgResult<T> res;

    while (n > 1) {
        n /= 2;

        Point L = (
            LazyPoints<T>(Gi.From(n), a.To(n)) +
            LazyPoints<T>(Hi.To(n), rounds == 0 ? b.From(n) * y_inv_pows.To(n) : b.From(n)) +
            LazyPoint<T>(u, (a.To(n) * b.From(n)).Sum() * c_factor)
        ).Sum();
        Point R = (
            LazyPoints<T>(Gi.To(n), a.From(n)) +
            LazyPoints<T>(Hi.From(n), rounds == 0 ? b.To(n) * y_inv_pows.From(n) : b.To(n)) +
            LazyPoint<T>(u, (a.From(n) * b.To(n)).Sum() * c_factor)
        ).Sum();

        fiat_shamir << L;
        fiat_shamir << R;
        res.Ls.Add(L);
        res.Rs.Add(R);

        // verifier chooses random x and sends to prover
        GEN_FIAT_SHAMIR_VAR(x, fiat_shamir, retry);

        Scalar x_inv = x.Invert();

        // update Gi, Hi for the next iteration
        if (n > 1) { // if the last loop, there is no need to update Gi and Hi
            Gi = (Gi.To(n) * x_inv) + (Gi.From(n) * x);
            if (rounds == 0) {
                Hi = (Hi.To(n) * y_inv_pows.To(n) * x) + (Hi.From(n) * y_inv_pows.From(n) * x_inv);
            } else {
                Hi = (Hi.To(n) * x) + (Hi.From(n) * x_inv);
            }
        }

        // update a, b for the next iteration
        a = (a.To(n) * x) + (a.From(n) * x_inv);
        b = (b.To(n) * x_inv) + (b.From(n) * x);

        ++rounds;
    }

    res.a = a[0];
    res.b = b[0];

    return res;

retry:
    return std::nullopt;
}
template
std::optional<ImpInnerProdArgResult<Mcl>> ImpInnerProdArg::Run(
    const size_t& N,
    Elements<typename Mcl::Point>& Gi,
    Elements<typename Mcl::Point>& Hi,
    const typename Mcl::Point& u,
    Elements<typename Mcl::Scalar>& a,
    Elements<typename Mcl::Scalar>& b,
    const typename Mcl::Scalar& c_factor,
    const typename Mcl::Scalar& y,
    HashWriter& fiat_shamir
);

template <typename T>
std::vector<typename T::Scalar> ImpInnerProdArg::GenGeneratorExponents(
    const size_t& num_rounds,
    const Elements<typename T::Scalar>& xs
) {
    using Scalar = typename T::Scalar;
    using Scalars = Elements<Scalar>;

    Scalars x_invs = xs.Invert();
    std::vector<Scalar> acc_xs(1ull << num_rounds, 1);
    acc_xs[0] = x_invs[0];
    acc_xs[1] = xs[0];

    for (size_t i = 1; i < num_rounds; ++i) {
        const size_t sl = 1ull << (i + 1);
        for (long signed int s = sl - 1; s > 0; s -= 2) {
            acc_xs[s] = acc_xs[s / 2] * xs[i];
            acc_xs[s - 1] = acc_xs[s / 2] * x_invs[i];
        }
    }
    return acc_xs;
};
template
std::vector<Mcl::Scalar> ImpInnerProdArg::GenGeneratorExponents<Mcl>(
    const size_t& num_rounds,
    const Elements<Mcl::Scalar>& xs
);

template <typename T>
void ImpInnerProdArg::LoopWithYPows(
    const size_t& num_loops,
    const typename T::Scalar& y,
    std::function<void(const size_t&, const typename T::Scalar&, const typename T::Scalar&)> f
) {
    typename T::Scalar y_inv_pow(1);
    typename T::Scalar y_pow(1);
    typename T::Scalar y_inv = y.Invert();

    for (size_t i = 0; i < num_loops; ++i) {
        f(i, y_pow, y_inv_pow);
        y_inv_pow = y_inv_pow * y_inv;
        y_pow = y_pow * y;
    };
}
template
void ImpInnerProdArg::LoopWithYPows<Mcl>(
    const size_t& num_loops,
    const Mcl::Scalar& y,
    std::function<void(const size_t&, const Mcl::Scalar&, const Mcl::Scalar&)>
);

template <typename T>
std::optional<Elements<typename T::Scalar>> ImpInnerProdArg::GenAllRoundXs(
    const Elements<typename T::Point>& Ls,
    const Elements<typename T::Point>& Rs,
    HashWriter& fiat_shamir)
{
    using Scalar = typename T::Scalar;
    using Scalars = Elements<Scalar>;

    Scalars xs;

    for (size_t i = 0; i < Ls.Size(); ++i) {
        fiat_shamir << Ls[i];
        fiat_shamir << Rs[i];
        GEN_FIAT_SHAMIR_VAR(x, fiat_shamir, retry);
        xs.Add(x);
    }

    return xs;

retry:
    return std::nullopt;
}
template std::optional<Elements<Mcl::Scalar>> ImpInnerProdArg::GenAllRoundXs<Mcl>(
    const Elements<Mcl::Point>& Ls,
    const Elements<Mcl::Point>& Rs,
    HashWriter& fiat_shamir);

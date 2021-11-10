// Copyright Nick Thompson, 2019
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MATH_QUADRATURE_DETAIL_OOURA_FOURIER_INTEGRALS_DETAIL_HPP
#define BOOST_MATH_QUADRATURE_DETAIL_OOURA_FOURIER_INTEGRALS_DETAIL_HPP
#include <utility> // for std::pair.
#include <vector>
#include <iostream>
#include <boost/math/special_functions/expm1.hpp>
#include <boost/math/special_functions/sin_pi.hpp>
#include <boost/math/special_functions/cos_pi.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/config.hpp>

#ifdef BOOST_HAS_THREADS
#include <mutex>
#include <atomic>
#endif

namespace boost { namespace math { namespace quadrature { namespace detail {

// Ooura and Mori, A robust double exponential formula for Fourier-type integrals,
// eta is the argument to the exponential in equation 3.3:
template<class Real>
std::pair<Real, Real> ooura_eta(Real x, Real alpha) {
    using std::expm1;
    using std::exp;
    using std::abs;
    Real expx = exp(x);
    Real eta_prime = 2 + alpha/expx + expx/4;
    Real eta;
    // This is the fast branch:
    if (abs(x) > 0.125) {
        eta = 2*x - alpha*(1/expx - 1) + (expx - 1)/4;
    }
    else {// this is the slow branch using expm1 for small x:
        eta = 2*x - alpha*expm1(-x) + expm1(x)/4;
    }
    return {eta, eta_prime};
}

// Ooura and Mori, A robust double exponential formula for Fourier-type integrals,
// equation 3.6:
template<class Real>
Real calculate_ooura_alpha(Real h)
{
    using boost::math::constants::pi;
    using std::log1p;
    using std::sqrt;
    Real x = sqrt(16 + 4*log1p(pi<Real>()/h)/h);
    return 1/x;
}

template<class Real>
std::pair<Real, Real> ooura_sin_node_and_weight(long n, Real h, Real alpha)
{
    using std::expm1;
    using std::exp;
    using std::abs;
    using boost::math::constants::pi;
    using std::isnan;

    if (n == 0) {
        // Equation 44 of https://arxiv.org/pdf/0911.4796.pdf
        // Fourier Transform of the Stretched Exponential Function: Analytic Error Bounds,
        // Double Exponential Transform, and Open-Source Implementation,
        // Joachim Wuttke, 
        // The C library libkww provides functions to compute the Kohlrausch-Williams-Watts function, 
        // the Laplace-Fourier transform of the stretched (or compressed) exponential function exp(-t^beta)
        // for exponent beta between 0.1 and 1.9 with sixteen decimal digits accuracy.

        Real eta_prime_0 = Real(2) + alpha + Real(1)/Real(4);
        Real node = pi<Real>()/(eta_prime_0*h);
        Real weight = pi<Real>()*boost::math::sin_pi(1/(eta_prime_0*h));
        Real eta_dbl_prime = -alpha + Real(1)/Real(4);
        Real phi_prime_0 = (1 - eta_dbl_prime/(eta_prime_0*eta_prime_0))/2;
        weight *= phi_prime_0;
        return {node, weight};
    }
    Real x = n*h;
    auto p = ooura_eta(x, alpha);
    auto eta = p.first;
    auto eta_prime = p.second;

    Real expm1_meta = expm1(-eta);
    Real exp_meta = exp(-eta);
    Real node = -n*pi<Real>()/expm1_meta;


    // I have verified that this is not a significant source of inaccuracy in the weight computation:
    Real phi_prime = -(expm1_meta + x*exp_meta*eta_prime)/(expm1_meta*expm1_meta);

    // The main source of inaccuracy is in computation of sin_pi.
    // But I've agonized over this, and I think it's as good as it can get:
    Real s = pi<Real>();
    Real arg;
    if(eta > 1) {
        arg = n/( 1/exp_meta - 1 );
        s *= boost::math::sin_pi(arg);
        if (n&1) {
            s *= -1;
        }
    }
    else if (eta < -1) {
        arg = n/(1-exp_meta);
        s *= boost::math::sin_pi(arg);
    }
    else {
        arg = -n*exp_meta/expm1_meta;
        s *= boost::math::sin_pi(arg);
        if (n&1) {
            s *= -1;
        }
    }

    Real weight = s*phi_prime;
    return {node, weight};
}

#ifdef BOOST_MATH_INSTRUMENT_OOURA
template<class Real>
void print_ooura_estimate(size_t i, Real I0, Real I1, Real omega) {
    using std::abs;
    std::cout << std::defaultfloat
              << std::setprecision(std::numeric_limits<Real>::digits10)
              << std::fixed;
    std::cout << "h = " << Real(1)/Real(1<<i) << ", I_h = " << I0/omega
              << " = " << std::hexfloat << I0/omega << ", absolute error estimate = "
              << std::defaultfloat << std::scientific << abs(I0-I1)  << std::endl;
}
#endif


template<class Real>
std::pair<Real, Real> ooura_cos_node_and_weight(long n, Real h, Real alpha)
{
    using std::expm1;
    using std::exp;
    using std::abs;
    using boost::math::constants::pi;

    Real x = h*(n-Real(1)/Real(2));
    auto p = ooura_eta(x, alpha);
    auto eta = p.first;
    auto eta_prime = p.second;
    Real expm1_meta = expm1(-eta);
    Real exp_meta = exp(-eta);
    Real node = pi<Real>()*(Real(1)/Real(2)-n)/expm1_meta;

    Real phi_prime = -(expm1_meta + x*exp_meta*eta_prime)/(expm1_meta*expm1_meta);

    // Takuya Ooura and Masatake Mori,
    // Journal of Computational and Applied Mathematics, 112 (1999) 229-241.
    // A robust double exponential formula for Fourier-type integrals.
    // Equation 4.6
    Real s = pi<Real>();
    Real arg;
    if (eta < -1) {
        arg = -(n-Real(1)/Real(2))/expm1_meta;
        s *= boost::math::cos_pi(arg);
    }
    else {
        arg = -(n-Real(1)/Real(2))*exp_meta/expm1_meta;
        s *= boost::math::sin_pi(arg);
        if (n&1) {
            s *= -1;
        }
    }

    Real weight = s*phi_prime;
    return {node, weight};
}


template<class Real>
class ooura_fourier_sin_detail {
public:
    ooura_fourier_sin_detail(const Real relative_error_goal, size_t levels) {
#ifdef BOOST_MATH_INSTRUMENT_OOURA
      std::cout << "ooura_fourier_sin with relative error goal " << relative_error_goal 
        << " & " << levels << " levels." << std::endl;
#endif // BOOST_MATH_INSTRUMENT_OOURA
        if (relative_error_goal < std::numeric_limits<Real>::epsilon() * 2) {
            throw std::domain_error("The relative error goal cannot be smaller than the unit roundoff.");
        }
        using std::abs;
        requested_levels_ = levels;
        starting_level_ = 0;
        rel_err_goal_ = relative_error_goal;
        big_nodes_.reserve(levels);
        bweights_.reserve(levels);
        little_nodes_.reserve(levels);
        lweights_.reserve(levels);

        for (size_t i = 0; i < levels; ++i) {
            if (std::is_same<Real, float>::value) {
                add_level<double>(i);
            }
            else if (std::is_same<Real, double>::value) {
                add_level<long double>(i);
            }
            else {
                add_level<Real>(i);
            }
        }
    }

    std::vector<std::vector<Real>> const & big_nodes() const {
        return big_nodes_;
    }

    std::vector<std::vector<Real>> const & weights_for_big_nodes() const {
        return bweights_;
    }

    std::vector<std::vector<Real>> const & little_nodes() const {
        return little_nodes_;
    }

    std::vector<std::vector<Real>> const & weights_for_little_nodes() const {
        return lweights_;
    }

    template<class F>
    std::pair<Real,Real> integrate(F const & f, Real omega) {
        using std::abs;
        using std::max;
        using boost::math::constants::pi;

        if (omega == 0) {
            return {Real(0), Real(0)};
        }
        if (omega < 0) {
            auto p = this->integrate(f, -omega);
            return {-p.first, p.second};
        }

        Real I1 = std::numeric_limits<Real>::quiet_NaN();
        Real relative_error_estimate = std::numeric_limits<Real>::quiet_NaN();
        // As we compute integrals, we learn about their structure.
        // Assuming we compute f(t)sin(wt) for many different omega, this gives some
        // a posteriori ability to choose a refinement level that is roughly appropriate.
        size_t i = starting_level_;
        do {
            Real I0 = estimate_integral(f, omega, i);
#ifdef BOOST_MATH_INSTRUMENT_OOURA
            print_ooura_estimate(i, I0, I1, omega);
#endif
            Real absolute_error_estimate = abs(I0-I1);
            Real scale = (max)(abs(I0), abs(I1));
            if (!isnan(I1) && absolute_error_estimate <= rel_err_goal_*scale) {
                starting_level_ = (max)(long(i) - 1, long(0));
                return {I0/omega, absolute_error_estimate/scale};
            }
            I1 = I0;
        } while(++i < big_nodes_.size());

        // We've used up all our precomputed levels.
        // Now we need to add more.
        // It might seems reasonable to just keep adding levels indefinitely, if that's what the user wants.
        // But in fact the nodes and weights just merge into each other and the error gets worse after a certain number.
        // This value for max_additional_levels was chosen by observation of a slowly converging oscillatory integral:
        // f(x) := cos(7cos(x))sin(x)/x
        size_t max_additional_levels = 4;
        while (big_nodes_.size() < requested_levels_ + max_additional_levels) {
            size_t ii = big_nodes_.size();
            if (std::is_same<Real, float>::value) {
                add_level<double>(ii);
            }
            else if (std::is_same<Real, double>::value) {
                add_level<long double>(ii);
            }
            else {
                add_level<Real>(ii);
            }
            Real I0 = estimate_integral(f, omega, ii);
            Real absolute_error_estimate = abs(I0-I1);
            Real scale = (max)(abs(I0), abs(I1));
#ifdef BOOST_MATH_INSTRUMENT_OOURA
            print_ooura_estimate(ii, I0, I1, omega);
#endif
            if (absolute_error_estimate <= rel_err_goal_*scale) {
                starting_level_ = (max)(long(ii) - 1, long(0));
                return {I0/omega, absolute_error_estimate/scale};
            }
            I1 = I0;
            ++ii;
        }

        starting_level_ = static_cast<long>(big_nodes_.size() - 2);
        return {I1/omega, relative_error_estimate};
    }

private:

    template<class PreciseReal>
    void add_level(size_t i) {
        using std::abs;
        size_t current_num_levels = big_nodes_.size();
        Real unit_roundoff = std::numeric_limits<Real>::epsilon()/2;
        // h0 = 1. Then all further levels have h_i = 1/2^i.
        // Since the nodes don't nest, we could conceivably divide h by (say) 1.5, or 3.
        // It's not clear how much benefit (or loss) would be obtained from this.
        PreciseReal h = PreciseReal(1)/PreciseReal(1<<i);

        std::vector<Real> bnode_row;
        std::vector<Real> bweight_row;

        // This is a pretty good estimate for how many elements will be placed in the vector:
        bnode_row.reserve((static_cast<size_t>(1)<<i)*sizeof(Real));
        bweight_row.reserve((static_cast<size_t>(1)<<i)*sizeof(Real));

        std::vector<Real> lnode_row;
        std::vector<Real> lweight_row;

        lnode_row.reserve((static_cast<size_t>(1)<<i)*sizeof(Real));
        lweight_row.reserve((static_cast<size_t>(1)<<i)*sizeof(Real));

        Real max_weight = 1;
        auto alpha = calculate_ooura_alpha(h);
        long n = 0;
        Real w;
        do {
            auto precise_nw = ooura_sin_node_and_weight(n, h, alpha);
            Real node = static_cast<Real>(precise_nw.first);
            Real weight = static_cast<Real>(precise_nw.second);
            w = weight;
            if (bnode_row.size() == bnode_row.capacity()) {
                bnode_row.reserve(2*bnode_row.size());
                bweight_row.reserve(2*bnode_row.size());
            }

            bnode_row.push_back(node);
            bweight_row.push_back(weight);
            if (abs(weight) > max_weight) {
                max_weight = abs(weight);
            }
            ++n;
            // f(t)->0 as t->infty, which is why the weights are computed up to the unit roundoff.
        } while(abs(w) > unit_roundoff*max_weight);

        // This class tends to consume a lot of memory; shrink the vectors back down to size:
        bnode_row.shrink_to_fit();
        bweight_row.shrink_to_fit();
        // Why we are splitting the nodes into regimes where t_n >> 1 and t_n << 1?
        // It will create the opportunity to sensibly truncate the quadrature sum to significant terms.
        n = -1;
        do {
            auto precise_nw = ooura_sin_node_and_weight(n, h, alpha);
            Real node = static_cast<Real>(precise_nw.first);
            if (node <= 0) {
                break;
            }
            Real weight = static_cast<Real>(precise_nw.second);
            w = weight;
            using std::isnan;
            if (isnan(node)) {
                // This occurs at n = -11 in quad precision:
                break;
            }
            if (lnode_row.size() > 0) {
                if (lnode_row[lnode_row.size()-1] == node) {
                    // The nodes have fused into each other:
                    break;
                }
            }
            if (lnode_row.size() == lnode_row.capacity()) {
                lnode_row.reserve(2*lnode_row.size());
                lweight_row.reserve(2*lnode_row.size());
            }
            lnode_row.push_back(node);
            lweight_row.push_back(weight);
            if (abs(weight) > max_weight) {
                max_weight = abs(weight);
            }
            --n;
            // f(t)->infty is possible as t->0, hence compute up to the min.
        } while(abs(w) > (std::numeric_limits<Real>::min)()*max_weight);

        lnode_row.shrink_to_fit();
        lweight_row.shrink_to_fit();

        #ifdef BOOST_HAS_THREADS
        // std::scoped_lock once C++17 is more common?
        std::lock_guard<std::mutex> lock(node_weight_mutex_);
        #endif 
        // Another thread might have already finished this calculation and appended it to the nodes/weights:
        if (current_num_levels == big_nodes_.size()) {
            big_nodes_.push_back(bnode_row);
            bweights_.push_back(bweight_row);

            little_nodes_.push_back(lnode_row);
            lweights_.push_back(lweight_row);
        }
    }

    template<class F>
    Real estimate_integral(F const & f, Real omega, size_t i) {
        // Because so few function evaluations are required to get high accuracy on the integrals in the tests,
        // Kahan summation doesn't really help.
        //auto cond = boost::math::tools::summation_condition_number<Real, true>(0);
        Real I0 = 0;
        auto const & b_nodes = big_nodes_[i];
        auto const & b_weights = bweights_[i];
        // Will benchmark if this is helpful:
        Real inv_omega = 1/omega;
        for(size_t j = 0 ; j < b_nodes.size(); ++j) {
            I0 += f(b_nodes[j]*inv_omega)*b_weights[j];
        }

        auto const & l_nodes = little_nodes_[i];
        auto const & l_weights = lweights_[i];
        // If f decays rapidly as |t|->infty, not all of these calls are necessary.
        for (size_t j = 0; j < l_nodes.size(); ++j) {
            I0 += f(l_nodes[j]*inv_omega)*l_weights[j];
        }
        return I0;
    }

    #ifdef BOOST_HAS_THREADS
    std::mutex node_weight_mutex_;
    #endif
    // Nodes for n >= 0, giving t_n = pi*phi(nh)/h. Generally t_n >> 1.
    std::vector<std::vector<Real>> big_nodes_;
    // The term bweights_ will indicate that these are weights corresponding
    // to the big nodes:
    std::vector<std::vector<Real>> bweights_;

    // Nodes for n < 0: Generally t_n << 1, and an invariant is that t_n > 0.
    std::vector<std::vector<Real>> little_nodes_;
    std::vector<std::vector<Real>> lweights_;
    Real rel_err_goal_;

    #ifdef BOOST_HAS_THREADS
    std::atomic<long> starting_level_;
    #else
    long starting_level_;
    #endif
    size_t requested_levels_;
};

template<class Real>
class ooura_fourier_cos_detail {
public:
    ooura_fourier_cos_detail(const Real relative_error_goal, size_t levels) {
#ifdef BOOST_MATH_INSTRUMENT_OOURA
      std::cout << "ooura_fourier_cos with relative error goal " << relative_error_goal
        << " & " << levels << " levels." << std::endl;
      std::cout << "epsilon for type = " << std::numeric_limits<Real>::epsilon() << std::endl;
#endif // BOOST_MATH_INSTRUMENT_OOURA
        if (relative_error_goal < std::numeric_limits<Real>::epsilon() * 2) {
            throw std::domain_error("The relative error goal cannot be smaller than the unit roundoff!");
        }

        using std::abs;
        requested_levels_ = levels;
        starting_level_ = 0;
        rel_err_goal_ = relative_error_goal;
        big_nodes_.reserve(levels);
        bweights_.reserve(levels);
        little_nodes_.reserve(levels);
        lweights_.reserve(levels);

        for (size_t i = 0; i < levels; ++i) {
            if (std::is_same<Real, float>::value) {
                add_level<double>(i);
            }
            else if (std::is_same<Real, double>::value) {
                add_level<long double>(i);
            }
            else {
                add_level<Real>(i);
            }
        }

    }

    template<class F>
    std::pair<Real,Real> integrate(F const & f, Real omega) {
        using std::abs;
        using std::max;
        using boost::math::constants::pi;

        if (omega == 0) {
            throw std::domain_error("At omega = 0, the integral is not oscillatory. The user must choose an appropriate method for this case.\n");
        }

        if (omega < 0) {
            return this->integrate(f, -omega);
        }

        Real I1 = std::numeric_limits<Real>::quiet_NaN();
        Real absolute_error_estimate = std::numeric_limits<Real>::quiet_NaN();
        Real scale = std::numeric_limits<Real>::quiet_NaN();
        size_t i = starting_level_;
        do {
            Real I0 = estimate_integral(f, omega, i);
#ifdef BOOST_MATH_INSTRUMENT_OOURA
            print_ooura_estimate(i, I0, I1, omega);
#endif
            absolute_error_estimate = abs(I0-I1);
            scale = (max)(abs(I0), abs(I1));
            if (!isnan(I1) && absolute_error_estimate <= rel_err_goal_*scale) {
                starting_level_ = (max)(long(i) - 1, long(0));
                return {I0/omega, absolute_error_estimate/scale};
            }
            I1 = I0;
        } while(++i < big_nodes_.size());

        size_t max_additional_levels = 4;
        while (big_nodes_.size() < requested_levels_ + max_additional_levels) {
            size_t ii = big_nodes_.size();
            if (std::is_same<Real, float>::value) {
                add_level<double>(ii);
            }
            else if (std::is_same<Real, double>::value) {
                add_level<long double>(ii);
            }
            else {
                add_level<Real>(ii);
            }
            Real I0 = estimate_integral(f, omega, ii);
#ifdef BOOST_MATH_INSTRUMENT_OOURA
            print_ooura_estimate(ii, I0, I1, omega);
#endif
            absolute_error_estimate = abs(I0-I1);
            scale = (max)(abs(I0), abs(I1));
            if (absolute_error_estimate <= rel_err_goal_*scale) {
                starting_level_ = (max)(long(ii) - 1, long(0));
                return {I0/omega, absolute_error_estimate/scale};
            }
            I1 = I0;
            ++ii;
        }

        starting_level_ = static_cast<long>(big_nodes_.size() - 2);
        return {I1/omega, absolute_error_estimate/scale};
    }

private:

    template<class PreciseReal>
    void add_level(size_t i) {
        using std::abs;
        size_t current_num_levels = big_nodes_.size();
        Real unit_roundoff = std::numeric_limits<Real>::epsilon()/2;
        PreciseReal h = PreciseReal(1)/PreciseReal(1<<i);

        std::vector<Real> bnode_row;
        std::vector<Real> bweight_row;
        bnode_row.reserve((static_cast<size_t>(1)<<i)*sizeof(Real));
        bweight_row.reserve((static_cast<size_t>(1)<<i)*sizeof(Real));

        std::vector<Real> lnode_row;
        std::vector<Real> lweight_row;

        lnode_row.reserve((static_cast<size_t>(1)<<i)*sizeof(Real));
        lweight_row.reserve((static_cast<size_t>(1)<<i)*sizeof(Real));

        Real max_weight = 1;
        auto alpha = calculate_ooura_alpha(h);
        long n = 0;
        Real w;
        do {
            auto precise_nw = ooura_cos_node_and_weight(n, h, alpha);
            Real node = static_cast<Real>(precise_nw.first);
            Real weight = static_cast<Real>(precise_nw.second);
            w = weight;
            if (bnode_row.size() == bnode_row.capacity()) {
                bnode_row.reserve(2*bnode_row.size());
                bweight_row.reserve(2*bnode_row.size());
            }

            bnode_row.push_back(node);
            bweight_row.push_back(weight);
            if (abs(weight) > max_weight) {
                max_weight = abs(weight);
            }
            ++n;
            // f(t)->0 as t->infty, which is why the weights are computed up to the unit roundoff.
        } while(abs(w) > unit_roundoff*max_weight);

        bnode_row.shrink_to_fit();
        bweight_row.shrink_to_fit();
        n = -1;
        do {
            auto precise_nw = ooura_cos_node_and_weight(n, h, alpha);
            Real node = static_cast<Real>(precise_nw.first);
            // The function cannot be singular at zero,
            // so zero is not a unreasonable node,
            // unlike in the case of the Fourier Sine.
            // Hence only break if the node is negative.
            if (node < 0) {
                break;
            }
            Real weight = static_cast<Real>(precise_nw.second);
            w = weight;
            if (lnode_row.size() > 0) {
                if (lnode_row.back() == node) {
                    // The nodes have fused into each other:
                    break;
                }
            }
            if (lnode_row.size() == lnode_row.capacity()) {
                lnode_row.reserve(2*lnode_row.size());
                lweight_row.reserve(2*lnode_row.size());
            }

            lnode_row.push_back(node);
            lweight_row.push_back(weight);
            if (abs(weight) > max_weight) {
                max_weight = abs(weight);
            }
            --n;
        } while(abs(w) > (std::numeric_limits<Real>::min)()*max_weight);

        lnode_row.shrink_to_fit();
        lweight_row.shrink_to_fit();

        #ifdef BOOST_HAS_THREADS
        std::lock_guard<std::mutex> lock(node_weight_mutex_);
        #endif

        // Another thread might have already finished this calculation and appended it to the nodes/weights:
        if (current_num_levels == big_nodes_.size()) {
            big_nodes_.push_back(bnode_row);
            bweights_.push_back(bweight_row);

            little_nodes_.push_back(lnode_row);
            lweights_.push_back(lweight_row);
        }
    }

    template<class F>
    Real estimate_integral(F const & f, Real omega, size_t i) {
        Real I0 = 0;
        auto const & b_nodes = big_nodes_[i];
        auto const & b_weights = bweights_[i];
        Real inv_omega = 1/omega;
        for(size_t j = 0 ; j < b_nodes.size(); ++j) {
            I0 += f(b_nodes[j]*inv_omega)*b_weights[j];
        }

        auto const & l_nodes = little_nodes_[i];
        auto const & l_weights = lweights_[i];
        for (size_t j = 0; j < l_nodes.size(); ++j) {
            I0 += f(l_nodes[j]*inv_omega)*l_weights[j];
        }
        return I0;
    }

    #ifdef BOOST_HAS_THREADS
    std::mutex node_weight_mutex_;
    #endif 

    std::vector<std::vector<Real>> big_nodes_;
    std::vector<std::vector<Real>> bweights_;

    std::vector<std::vector<Real>> little_nodes_;
    std::vector<std::vector<Real>> lweights_;
    Real rel_err_goal_;

    #ifdef BOOST_HAS_THREADS
    std::atomic<long> starting_level_;
    #else
    long starting_level_;
    #endif

    size_t requested_levels_;
};


}}}}
#endif

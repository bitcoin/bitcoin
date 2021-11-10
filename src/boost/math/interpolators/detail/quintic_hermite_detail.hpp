/*
 * Copyright Nick Thompson, 2020
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef BOOST_MATH_INTERPOLATORS_DETAIL_QUINTIC_HERMITE_DETAIL_HPP
#define BOOST_MATH_INTERPOLATORS_DETAIL_QUINTIC_HERMITE_DETAIL_HPP
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <limits>
#include <cmath>

namespace boost {
namespace math {
namespace interpolators {
namespace detail {

template<class RandomAccessContainer>
class quintic_hermite_detail {
public:
    using Real = typename RandomAccessContainer::value_type;
    quintic_hermite_detail(RandomAccessContainer && x, RandomAccessContainer && y, RandomAccessContainer && dydx, RandomAccessContainer && d2ydx2) : x_{std::move(x)}, y_{std::move(y)}, dydx_{std::move(dydx)}, d2ydx2_{std::move(d2ydx2)}
    {
        if (x_.size() != y_.size())
        {
            throw std::domain_error("Number of abscissas must = number of ordinates.");
        }
        if (x_.size() != dydx_.size())
        {
            throw std::domain_error("Numbers of derivatives must = number of abscissas.");
        }
        if (x_.size() != d2ydx2_.size())
        {
            throw std::domain_error("Number of second derivatives must equal number of abscissas.");
        }
        if (x_.size() < 2)
        {
            throw std::domain_error("At least 2 abscissas are required.");
        }
        Real x0 = x_[0];
        for (decltype(x_.size()) i = 1; i < x_.size(); ++i)
        {
            Real x1 = x_[i];
            if (x1 <= x0)
            {
                throw std::domain_error("Abscissas must be sorted in strictly increasing order x0 < x1 < ... < x_{n-1}");
            }
            x0 = x1;
        }
    }

    void push_back(Real x, Real y, Real dydx, Real d2ydx2)
    {
        using std::abs;
        using std::isnan;
        if (x <= x_.back())
        {
             throw std::domain_error("Calling push_back must preserve the monotonicity of the x's");
        }
        x_.push_back(x);
        y_.push_back(y);
        dydx_.push_back(dydx);
        d2ydx2_.push_back(d2ydx2);
    }

    inline Real operator()(Real x) const
    {
        if  (x < x_[0] || x > x_.back())
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x_[0] << ", " << x_.back() << "]";
            throw std::domain_error(oss.str());
        }
        // We need t := (x-x_k)/(x_{k+1}-x_k) \in [0,1) for this to work.
        // Sadly this neccessitates this loathesome check, otherwise we get t = 1 at x = xf.
        if (x == x_.back())
        {
            return y_.back();
        }

        auto it = std::upper_bound(x_.begin(), x_.end(), x);
        auto i = std::distance(x_.begin(), it) -1;
        Real x0 = *(it-1);
        Real x1 = *it;
        Real y0 = y_[i];
        Real y1 = y_[i+1];
        Real v0 = dydx_[i];
        Real v1 = dydx_[i+1];
        Real a0 = d2ydx2_[i];
        Real a1 = d2ydx2_[i+1];

        Real dx = (x1-x0);
        Real t = (x-x0)/dx;
        Real t2 = t*t;
        Real t3 = t2*t;

        // See the 'Basis functions' section of:
        // https://www.rose-hulman.edu/~finn/CCLI/Notes/day09.pdf
        // Also: https://github.com/MrHexxx/QuinticHermiteSpline/blob/master/HermiteSpline.cs
        Real y = (1- t3*(10 + t*(-15 + 6*t)))*y0;
        y += t*(1+ t2*(-6 + t*(8 -3*t)))*v0*dx;
        y += t2*(1 + t*(-3 + t*(3-t)))*a0*dx*dx/2;
        y += t3*((1 + t*(-2 + t))*a1*dx*dx/2 + (-4 + t*(7 - 3*t))*v1*dx + (10 + t*(-15 + 6*t))*y1);
        return y;
    }

    inline Real prime(Real x) const
    {
        if  (x < x_[0] || x > x_.back())
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x_[0] << ", " << x_.back() << "]";
            throw std::domain_error(oss.str());
        }
        if (x == x_.back())
        {
            return dydx_.back();
        }

        auto it = std::upper_bound(x_.begin(), x_.end(), x);
        auto i = std::distance(x_.begin(), it) -1;
        Real x0 = *(it-1);
        Real x1 = *it;
        Real dx = x1 - x0;

        Real y0 = y_[i];
        Real y1 = y_[i+1];
        Real v0 = dydx_[i];
        Real v1 = dydx_[i+1];
        Real a0 = d2ydx2_[i];
        Real a1 = d2ydx2_[i+1];
        Real t= (x-x0)/dx;
        Real t2 = t*t;

        Real dydx = 30*t2*(1 - 2*t + t*t)*(y1-y0)/dx;
        dydx += (1-18*t*t + 32*t*t*t - 15*t*t*t*t)*v0 - t*t*(12 - 28*t + 15*t*t)*v1;
        dydx += (t*dx/2)*((2 - 9*t + 12*t*t - 5*t*t*t)*a0 + t*(3 - 8*t + 5*t*t)*a1);
        return dydx;
    }

    inline Real double_prime(Real x) const
    {
        if  (x < x_[0] || x > x_.back())
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x_[0] << ", " << x_.back() << "]";
            throw std::domain_error(oss.str());
        }
        if (x == x_.back())
        {
            return d2ydx2_.back();
        }

        auto it = std::upper_bound(x_.begin(), x_.end(), x);
        auto i = std::distance(x_.begin(), it) -1;
        Real x0 = *(it-1);
        Real x1 = *it;
        Real dx = x1 - x0;

        Real y0 = y_[i];
        Real y1 = y_[i+1];
        Real v0 = dydx_[i];
        Real v1 = dydx_[i+1];
        Real a0 = d2ydx2_[i];
        Real a1 = d2ydx2_[i+1];
        Real t = (x-x0)/dx;

        Real d2ydx2 = 60*t*(1 + t*(-3 + 2*t))*(y1-y0)/(dx*dx);
        d2ydx2 += 12*t*(-3 + t*(8 - 5*t))*v0/dx;
        d2ydx2 -= 12*t*(2 + t*(-7 + 5*t))*v1/dx;
        d2ydx2 += (1 + t*(-9 + t*(18 - 10*t)))*a0;
        d2ydx2 += t*(3 + t*(-12 + 10*t))*a1;

        return d2ydx2;
    }

    friend std::ostream& operator<<(std::ostream & os, const quintic_hermite_detail & m)
    {
        os << "(x,y,y') = {";
        for (size_t i = 0; i < m.x_.size() - 1; ++i) {
            os << "(" << m.x_[i] << ", " << m.y_[i] << ", " << m.dydx_[i] << ", " << m.d2ydx2_[i] << "),  ";
        }
        auto n = m.x_.size()-1;
        os << "(" << m.x_[n] << ", " << m.y_[n] << ", " << m.dydx_[n] << ", " << m.d2ydx2_[n] << ")}";
        return os;
    }

    int64_t bytes() const
    {
        return 4*x_.size()*sizeof(x_);
    }

    std::pair<Real, Real> domain() const
    {
        return {x_.front(), x_.back()};
    }

private:
    RandomAccessContainer x_;
    RandomAccessContainer y_;
    RandomAccessContainer dydx_;
    RandomAccessContainer d2ydx2_;
};


template<class RandomAccessContainer>
class cardinal_quintic_hermite_detail {
public:
    using Real = typename RandomAccessContainer::value_type;
    cardinal_quintic_hermite_detail(RandomAccessContainer && y, RandomAccessContainer && dydx, RandomAccessContainer && d2ydx2, Real x0, Real dx)
    : y_{std::move(y)}, dy_{std::move(dydx)}, d2y_{std::move(d2ydx2)}, x0_{x0}, inv_dx_{1/dx}
    {
        if (y_.size() != dy_.size())
        {
            throw std::domain_error("Numbers of derivatives must = number of abscissas.");
        }
        if (y_.size() != d2y_.size())
        {
            throw std::domain_error("Number of second derivatives must equal number of abscissas.");
        }
        if (y_.size() < 2)
        {
            throw std::domain_error("At least 2 abscissas are required.");
        }
        if (dx <= 0)
        {
            throw std::domain_error("dx > 0 is required.");
        }

        for (auto & dy : dy_)
        {
            dy *= dx;
        }

        for (auto & d2y : d2y_)
        {
            d2y *= (dx*dx)/2;
        }
    }


    inline Real operator()(Real x) const
    {
        const Real xf = x0_ + (y_.size()-1)/inv_dx_;
        if  (x < x0_ || x > xf)
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x0_ << ", " << xf << "]";
            throw std::domain_error(oss.str());
        }
        if (x == xf)
        {
            return y_.back();
        }
        return this->unchecked_evaluation(x);
    }

    inline Real unchecked_evaluation(Real x) const
    {
        using std::floor;
        Real s = (x-x0_)*inv_dx_;
        Real ii = floor(s);
        auto i = static_cast<decltype(y_.size())>(ii);
        Real t = s - ii;
        if (t == 0)
        {
            return y_[i];
        }
        Real y0 = y_[i];
        Real y1 = y_[i+1];
        Real dy0 = dy_[i];
        Real dy1 = dy_[i+1];
        Real d2y0 = d2y_[i];
        Real d2y1 = d2y_[i+1];

        // See the 'Basis functions' section of:
        // https://www.rose-hulman.edu/~finn/CCLI/Notes/day09.pdf
        // Also: https://github.com/MrHexxx/QuinticHermiteSpline/blob/master/HermiteSpline.cs
        Real y = (1- t*t*t*(10 + t*(-15 + 6*t)))*y0;
        y += t*(1+ t*t*(-6 + t*(8 -3*t)))*dy0;
        y += t*t*(1 + t*(-3 + t*(3-t)))*d2y0;
        y += t*t*t*((1 + t*(-2 + t))*d2y1 + (-4 + t*(7 -3*t))*dy1 + (10 + t*(-15 + 6*t))*y1);
        return y;
    }

    inline Real prime(Real x) const
    {
        const Real xf = x0_ + (y_.size()-1)/inv_dx_;
        if  (x < x0_ || x > xf)
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x0_ << ", " << xf << "]";
            throw std::domain_error(oss.str());
        }
        if (x == xf)
        {
            return dy_.back()*inv_dx_;
        }

        return this->unchecked_prime(x);
    }

    inline Real unchecked_prime(Real x) const
    {
        using std::floor;
        Real s = (x-x0_)*inv_dx_;
        Real ii = floor(s);
        auto i = static_cast<decltype(y_.size())>(ii);
        Real t = s - ii;
        if (t == 0)
        {
            return dy_[i]*inv_dx_;
        }
        Real y0 = y_[i];
        Real y1 = y_[i+1];
        Real dy0 = dy_[i];
        Real dy1 = dy_[i+1];
        Real d2y0 = d2y_[i];
        Real d2y1 = d2y_[i+1];

        Real dydx = 30*t*t*(1 - 2*t + t*t)*(y1-y0);
        dydx += (1-18*t*t + 32*t*t*t - 15*t*t*t*t)*dy0 - t*t*(12 - 28*t + 15*t*t)*dy1;
        dydx += t*((2 - 9*t + 12*t*t - 5*t*t*t)*d2y0 + t*(3 - 8*t + 5*t*t)*d2y1);
        dydx *= inv_dx_;
        return dydx;
    }

    inline Real double_prime(Real x) const
    {
        const Real xf = x0_ + (y_.size()-1)/inv_dx_;
        if  (x < x0_ || x > xf) {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x0_ << ", " << xf << "]";
            throw std::domain_error(oss.str());
        }
        if (x == xf)
        {
            return d2y_.back()*2*inv_dx_*inv_dx_;
        }

        return this->unchecked_double_prime(x);
    }

    inline Real unchecked_double_prime(Real x) const
    {
        using std::floor;
        Real s = (x-x0_)*inv_dx_;
        Real ii = floor(s);
        auto i = static_cast<decltype(y_.size())>(ii);
        Real t = s - ii;
        if (t==0)
        {
            return d2y_[i]*2*inv_dx_*inv_dx_;
        }

        Real y0 = y_[i];
        Real y1 = y_[i+1];
        Real dy0 = dy_[i];
        Real dy1 = dy_[i+1];
        Real d2y0 = d2y_[i];
        Real d2y1 = d2y_[i+1];

        Real d2ydx2 = 60*t*(1 - 3*t + 2*t*t)*(y1 - y0)*inv_dx_*inv_dx_;
        d2ydx2 += (12*t)*((-3 + 8*t - 5*t*t)*dy0 - (2 - 7*t + 5*t*t)*dy1);
        d2ydx2 += (1 - 9*t + 18*t*t - 10*t*t*t)*d2y0*(2*inv_dx_*inv_dx_) + t*(3 - 12*t + 10*t*t)*d2y1*(2*inv_dx_*inv_dx_);
        return d2ydx2;
    }

    int64_t bytes() const
    {
        return 3*y_.size()*sizeof(Real) + 2*sizeof(Real);
    }

    std::pair<Real, Real> domain() const
    {
        Real xf = x0_ + (y_.size()-1)/inv_dx_;
        return {x0_, xf};
    }

private:
    RandomAccessContainer y_;
    RandomAccessContainer dy_;
    RandomAccessContainer d2y_;
    Real x0_;
    Real inv_dx_;
};


template<class RandomAccessContainer>
class cardinal_quintic_hermite_detail_aos {
public:
    using Point = typename RandomAccessContainer::value_type;
    using Real = typename Point::value_type;
    cardinal_quintic_hermite_detail_aos(RandomAccessContainer && data, Real x0, Real dx)
    : data_{std::move(data)} , x0_{x0}, inv_dx_{1/dx}
    {
        if (data_.size() < 2)
        {
            throw std::domain_error("At least two points are required to interpolate using cardinal_quintic_hermite_aos");
        }

        if (data_[0].size() != 3)
        {
            throw std::domain_error("Each datum passed to the cardinal_quintic_hermite_aos must have three elements: {y, y', y''}");
        }
        if (dx <= 0)
        {
            throw std::domain_error("dx > 0 is required.");
        }

        for (auto & datum : data_)
        {
            datum[1] *= dx;
            datum[2] *= (dx*dx/2);
        }
    }


    inline Real operator()(Real x) const
    {
        const Real xf = x0_ + (data_.size()-1)/inv_dx_;
        if  (x < x0_ || x > xf)
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x0_ << ", " << xf << "]";
            throw std::domain_error(oss.str());
        }
        if (x == xf)
        {
            return data_.back()[0];
        }
        return this->unchecked_evaluation(x);
    }

    inline Real unchecked_evaluation(Real x) const
    {
        using std::floor;
        Real s = (x-x0_)*inv_dx_;
        Real ii = floor(s);
        auto i = static_cast<decltype(data_.size())>(ii);
        Real t = s - ii;
        if (t == 0)
        {
            return data_[i][0];
        }

        Real y0 = data_[i][0];
        Real dy0 = data_[i][1];
        Real d2y0 = data_[i][2];
        Real y1 = data_[i+1][0];
        Real dy1 = data_[i+1][1];
        Real d2y1 = data_[i+1][2];

        Real y = (1 - t*t*t*(10 + t*(-15 + 6*t)))*y0;
        y += t*(1 + t*t*(-6 + t*(8 - 3*t)))*dy0;
        y += t*t*(1 + t*(-3 + t*(3 - t)))*d2y0;
        y += t*t*t*((1 + t*(-2 + t))*d2y1 + (-4 + t*(7 - 3*t))*dy1 + (10 + t*(-15 + 6*t))*y1);
        return y;
    }

    inline Real prime(Real x) const
    {
        const Real xf = x0_ + (data_.size()-1)/inv_dx_;
        if  (x < x0_ || x > xf)
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x0_ << ", " << xf << "]";
            throw std::domain_error(oss.str());
        }
        if (x == xf)
        {
            return data_.back()[1]*inv_dx_;
        }

        return this->unchecked_prime(x);
    }

    inline Real unchecked_prime(Real x) const
    {
        using std::floor;
        Real s = (x-x0_)*inv_dx_;
        Real ii = floor(s);
        auto i = static_cast<decltype(data_.size())>(ii);
        Real t = s - ii;
        if (t == 0)
        {
            return data_[i][1]*inv_dx_;
        }


        Real y0 = data_[i][0];
        Real y1 = data_[i+1][0];
        Real v0 = data_[i][1];
        Real v1 = data_[i+1][1];
        Real a0 = data_[i][2];
        Real a1 = data_[i+1][2];

        Real dy = 30*t*t*(1 - 2*t + t*t)*(y1-y0);
        dy += (1-18*t*t + 32*t*t*t - 15*t*t*t*t)*v0 - t*t*(12 - 28*t + 15*t*t)*v1;
        dy += t*((2 - 9*t + 12*t*t - 5*t*t*t)*a0 + t*(3 - 8*t + 5*t*t)*a1);
        return dy*inv_dx_;
    }

    inline Real double_prime(Real x) const
    {
        const Real xf = x0_ + (data_.size()-1)/inv_dx_;
        if  (x < x0_ || x > xf)
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa x = " << x << ", which is outside of allowed range ["
                << x0_ << ", " << xf << "]";
            throw std::domain_error(oss.str());
        }
        if (x == xf)
        {
            return data_.back()[2]*2*inv_dx_*inv_dx_;
        }

        return this->unchecked_double_prime(x);
    }

    inline Real unchecked_double_prime(Real x) const
    {
        using std::floor;
        Real s = (x-x0_)*inv_dx_;
        Real ii = floor(s);
        auto i = static_cast<decltype(data_.size())>(ii);
        Real t = s - ii;
        if (t == 0) {
            return data_[i][2]*2*inv_dx_*inv_dx_;
        }
        Real y0 = data_[i][0];
        Real dy0 = data_[i][1];
        Real d2y0 = data_[i][2];
        Real y1 = data_[i+1][0];
        Real dy1 = data_[i+1][1];
        Real d2y1 = data_[i+1][2];

        Real d2ydx2 = 60*t*(1 - 3*t + 2*t*t)*(y1 - y0)*inv_dx_*inv_dx_;
        d2ydx2 += (12*t)*((-3 + 8*t - 5*t*t)*dy0 - (2 - 7*t + 5*t*t)*dy1);
        d2ydx2 += (1 - 9*t + 18*t*t - 10*t*t*t)*d2y0*(2*inv_dx_*inv_dx_) + t*(3 - 12*t + 10*t*t)*d2y1*(2*inv_dx_*inv_dx_);
        return d2ydx2;
    }

    int64_t bytes() const
    {
        return data_.size()*data_[0].size()*sizeof(Real) + 2*sizeof(Real);
    }

    std::pair<Real, Real> domain() const
    {
        Real xf = x0_ + (data_.size()-1)/inv_dx_;
        return {x0_, xf};
    }

private:
    RandomAccessContainer data_;
    Real x0_;
    Real inv_dx_;
};

}
}
}
}
#endif

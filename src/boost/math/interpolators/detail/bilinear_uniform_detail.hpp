// Copyright Nick Thompson, 2021
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_INTERPOLATORS_BILINEAR_UNIFORM_DETAIL_HPP
#define BOOST_MATH_INTERPOLATORS_BILINEAR_UNIFORM_DETAIL_HPP

#include <stdexcept>
#include <iostream>
#include <string>
#include <limits>
#include <cmath>
#include <utility>

namespace boost::math::interpolators::detail {

template <class RandomAccessContainer>
class bilinear_uniform_imp
{
public:
    using Real = typename RandomAccessContainer::value_type;
    using Z = typename RandomAccessContainer::size_type;

    bilinear_uniform_imp(RandomAccessContainer && fieldData, Z rows, Z cols, Real dx = 1, Real dy = 1, Real x0 = 0, Real y0 = 0)
    {
        using std::to_string;
        if(fieldData.size() != rows*cols)
        {
            std::string err = std::string(__FILE__) + ":" + to_string(__LINE__)
               + " The field data must have rows*cols elements. There are " + to_string(rows)  + " rows and " + to_string(cols) + " columns but " + to_string(fieldData.size()) + " elements in the field data.";
            throw std::logic_error(err);
        }
        if (rows < 2) {
            throw std::logic_error("There must be at least two rows of data for bilinear interpolation to be well-defined.");
        }
        if (cols < 2) {
            throw std::logic_error("There must be at least two columns of data for bilinear interpolation to be well-defined.");
        }

        fieldData_ = std::move(fieldData);
        rows_ = rows;
        cols_ = cols;
        x0_ = x0;
        y0_ = y0;
        dx_ = dx;
        dy_ = dy;

        if (dx_ <= 0) {
            std::string err = std::string(__FILE__) + ":" + to_string(__LINE__) + " dx = " + to_string(dx) + ", but dx > 0 is required. Are the arguments out of order?";
            throw std::logic_error(err);
        }
        if (dy_ <= 0) {
            std::string err = std::string(__FILE__) + ":" + to_string(__LINE__) + " dy = " + to_string(dy) + ", but dy > 0 is required. Are the arguments out of order?";
            throw std::logic_error(err);
        }
    }

    Real operator()(Real x, Real y) const
    {
        using std::floor;
        if (x > x0_ + (cols_ - 1)*dx_ || x < x0_) {
            std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\n";
            std::cerr << "Querying the bilinear_uniform interpolator at (x,y) = (" << x << ", " << y << ") is not allowed.\n";
            std::cerr << "x must lie in the interval [" << x0_ << ", " << x0_ + (cols_ -1)*dx_ << "]\n";
            return std::numeric_limits<Real>::quiet_NaN();
        }
        if (y > y0_ + (rows_ - 1)*dy_ || y < y0_) {
            std::cerr << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\n";
            std::cerr << "Querying the bilinear_uniform interpolator at (x,y) = (" << x << ", " << y << ") is not allowed.\n";
            std::cerr << "y must lie in the interval [" << y0_ << ", " << y0_ + (rows_ -1)*dy_ << "]\n";
            return std::numeric_limits<Real>::quiet_NaN(); 
        }

        Real s = (x - x0_)/dx_;
        Real s0 = floor(s);
        Real t = (y - y0_)/dy_;
        Real t0 = floor(t);
        Z xidx = s0;
        Z yidx = t0;
        Z idx = yidx*cols_  + xidx;
        Real alpha = s - s0;
        Real beta = t - t0;

        Real fhi;
        // If alpha = 0, then we can segfault by reading fieldData_[idx+1]:
        if (alpha <= 2*s0*std::numeric_limits<Real>::epsilon())  {
            fhi = fieldData_[idx];
        } else {
            fhi = (1 - alpha)*fieldData_[idx] + alpha*fieldData_[idx + 1];
        }

        // Again, we can get OOB access without this check.
        // This corresponds to interpolation over a line segment aligned with the axes.
        if (beta <= 2*t0*std::numeric_limits<Real>::epsilon()) {
            return fhi;
        }

        auto bottom_left = fieldData_[idx + cols_];
        Real flo;
        if (alpha <= 2*s0*std::numeric_limits<Real>::epsilon()) {
            flo = bottom_left;
        }
        else {
            flo = (1 - alpha)*bottom_left + alpha*fieldData_[idx + cols_ + 1];
        }
        // Convex combination over vertical to get the value:
        return (1 - beta)*fhi + beta*flo;
    }

    friend std::ostream& operator<<(std::ostream& out, bilinear_uniform_imp<RandomAccessContainer> const & bu) {
        out << "(x0, y0) = (" << bu.x0_ << ", " << bu.y0_ << "), (dx, dy) = (" << bu.dx_ << ", " << bu.dy_ << "), ";
        out << "(xf, yf) = (" << bu.x0_ + (bu.cols_ - 1)*bu.dx_ << ", " << bu.y0_ + (bu.rows_ - 1)*bu.dy_ << ")\n";
        for (Z j = 0; j < bu.rows_; ++j) {
            out << "{";
            for (Z i = 0; i < bu.cols_ - 1; ++i) {
                out << bu.fieldData_[j*bu.cols_ + i] << ", ";
            }
            out << bu.fieldData_[j*bu.cols_ + bu.cols_ - 1] << "}\n";
        }
        return out;
    }

private:
    RandomAccessContainer fieldData_;
    Z rows_;
    Z cols_;
    Real x0_;
    Real y0_;
    Real dx_;
    Real dy_;
};


}
#endif

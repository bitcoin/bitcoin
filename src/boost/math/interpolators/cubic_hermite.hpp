// Copyright Nick Thompson, 2020
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_INTERPOLATORS_CUBIC_HERMITE_HPP
#define BOOST_MATH_INTERPOLATORS_CUBIC_HERMITE_HPP
#include <memory>
#include <boost/math/interpolators/detail/cubic_hermite_detail.hpp>

namespace boost {
namespace math {
namespace interpolators {

template<class RandomAccessContainer>
class cubic_hermite {
public:
    using Real = typename RandomAccessContainer::value_type;

    cubic_hermite(RandomAccessContainer && x, RandomAccessContainer && y, RandomAccessContainer && dydx) 
    : impl_(std::make_shared<detail::cubic_hermite_detail<RandomAccessContainer>>(std::move(x), std::move(y), std::move(dydx)))
    {}

    inline Real operator()(Real x) const {
        return impl_->operator()(x);
    }

    inline Real prime(Real x) const {
        return impl_->prime(x);
    }

    friend std::ostream& operator<<(std::ostream & os, const cubic_hermite & m)
    {
        os << *m.impl_;
        return os;
    }

    void push_back(Real x, Real y, Real dydx)
    {
        impl_->push_back(x, y, dydx);
    }

    int64_t bytes() const
    {
        return impl_->bytes() + sizeof(impl_);
    }

    std::pair<Real, Real> domain() const
    {
        return impl_->domain();
    }

private:
    std::shared_ptr<detail::cubic_hermite_detail<RandomAccessContainer>> impl_;
};

template<class RandomAccessContainer>
class cardinal_cubic_hermite {
public:
    using Real = typename RandomAccessContainer::value_type;

    cardinal_cubic_hermite(RandomAccessContainer && y, RandomAccessContainer && dydx, Real x0, Real dx) 
    : impl_(std::make_shared<detail::cardinal_cubic_hermite_detail<RandomAccessContainer>>(std::move(y), std::move(dydx), x0, dx))
    {}

    inline Real operator()(Real x) const
    {
        return impl_->operator()(x);
    }

    inline Real prime(Real x) const
    {
        return impl_->prime(x);
    }

    friend std::ostream& operator<<(std::ostream & os, const cardinal_cubic_hermite & m)
    {
        os << *m.impl_;
        return os;
    }

    int64_t bytes() const
    {
        return impl_->bytes() + sizeof(impl_);
    }

    std::pair<Real, Real> domain() const
    {
        return impl_->domain();
    }

private:
    std::shared_ptr<detail::cardinal_cubic_hermite_detail<RandomAccessContainer>> impl_;
};


template<class RandomAccessContainer>
class cardinal_cubic_hermite_aos {
public:
    using Point = typename RandomAccessContainer::value_type;
    using Real = typename Point::value_type;

    cardinal_cubic_hermite_aos(RandomAccessContainer && data, Real x0, Real dx) 
    : impl_(std::make_shared<detail::cardinal_cubic_hermite_detail_aos<RandomAccessContainer>>(std::move(data), x0, dx))
    {}

    inline Real operator()(Real x) const
    {
        return impl_->operator()(x);
    }

    inline Real prime(Real x) const
    {
        return impl_->prime(x);
    }

    friend std::ostream& operator<<(std::ostream & os, const cardinal_cubic_hermite_aos & m)
    {
        os << *m.impl_;
        return os;
    }

    int64_t bytes() const
    {
        return impl_->bytes() + sizeof(impl_);
    }

    std::pair<Real, Real> domain() const
    {
        return impl_->domain();
    }

private:
    std::shared_ptr<detail::cardinal_cubic_hermite_detail_aos<RandomAccessContainer>> impl_;
};

}
}
}
#endif

// Copyright Nick Thompson, 2017
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// This computes the Catmull-Rom spline from a list of points.

#ifndef BOOST_MATH_INTERPOLATORS_CATMULL_ROM
#define BOOST_MATH_INTERPOLATORS_CATMULL_ROM

#include <cmath>
#include <vector>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <limits>

namespace std_workaround {

#if defined(__cpp_lib_nonmember_container_access) || (defined(_MSC_VER) && (_MSC_VER >= 1900))
   using std::size;
#else
   template <class C>
   inline constexpr std::size_t size(const C& c)
   {
      return c.size();
   }
   template <class T, std::size_t N>
   inline constexpr std::size_t size(const T(&array)[N]) noexcept
   {
      return N;
   }
#endif
}

namespace boost{ namespace math{

    namespace detail
    {
        template<class Point>
        typename Point::value_type alpha_distance(Point const & p1, Point const & p2, typename Point::value_type alpha)
        {
            using std::pow;
            using std_workaround::size;
            typename Point::value_type dsq = 0;
            for (size_t i = 0; i < size(p1); ++i)
            {
                typename Point::value_type dx = p1[i] - p2[i];
                dsq += dx*dx;
            }
            return pow(dsq, alpha/2);
        }
    }

template <class Point, class RandomAccessContainer = std::vector<Point> >
class catmull_rom
{
   typedef typename Point::value_type value_type;
public:

    catmull_rom(RandomAccessContainer&& points, bool closed = false, value_type alpha = (value_type) 1/ (value_type) 2);

    catmull_rom(std::initializer_list<Point> l, bool closed = false, value_type alpha = (value_type) 1/ (value_type) 2) : catmull_rom<Point, RandomAccessContainer>(RandomAccessContainer(l), closed, alpha) {}

    value_type max_parameter() const
    {
        return m_max_s;
    }

    value_type parameter_at_point(size_t i) const
    {
        return m_s[i+1];
    }

    Point operator()(const value_type s) const;

    Point prime(const value_type s) const;

    RandomAccessContainer&& get_points()
    {
        return std::move(m_pnts);
    }

private:
    RandomAccessContainer m_pnts;
    std::vector<value_type> m_s;
    value_type m_max_s;
};

template<class Point, class RandomAccessContainer >
catmull_rom<Point, RandomAccessContainer>::catmull_rom(RandomAccessContainer&& points, bool closed, typename Point::value_type alpha) : m_pnts(std::move(points))
{
    std::size_t num_pnts = m_pnts.size();
    //std::cout << "Number of points = " << num_pnts << "\n";
    if (num_pnts < 4)
    {
        throw std::domain_error("The Catmull-Rom curve requires at least 4 points.");
    }
    if (alpha < 0 || alpha > 1)
    {
        throw std::domain_error("The parametrization alpha must be in the range [0,1].");
    }

    using std::abs;
    m_s.resize(num_pnts+3);
    m_pnts.resize(num_pnts+3);
    //std::cout << "Number of points now = " << m_pnts.size() << "\n";

    m_pnts[num_pnts+1] = m_pnts[0];
    m_pnts[num_pnts+2] = m_pnts[1];

    auto tmp = m_pnts[num_pnts-1];
    for (std::ptrdiff_t i = num_pnts-1; i >= 0; --i)
    {
        m_pnts[i+1] = m_pnts[i];
    }
    m_pnts[0] = tmp;

    m_s[0] = -detail::alpha_distance<Point>(m_pnts[0], m_pnts[1], alpha);
    if (abs(m_s[0]) < std::numeric_limits<typename Point::value_type>::epsilon())
    {
        throw std::domain_error("The first and last point should not be the same.\n");
    }
    m_s[1] = 0;
    for (size_t i = 2; i < m_s.size(); ++i)
    {
        typename Point::value_type d = detail::alpha_distance<Point>(m_pnts[i], m_pnts[i-1], alpha);
        if (abs(d) < std::numeric_limits<typename Point::value_type>::epsilon())
        {
            throw std::domain_error("The control points of the Catmull-Rom curve are too close together; this will lead to ill-conditioning.\n");
        }
        m_s[i] = m_s[i-1] + d;
    }
    if(closed)
    {
        m_max_s = m_s[num_pnts+1];
    }
    else
    {
        m_max_s = m_s[num_pnts];
    }
}


template<class Point, class RandomAccessContainer >
Point catmull_rom<Point, RandomAccessContainer>::operator()(const typename Point::value_type s) const
{
    using std_workaround::size;
    if (s < 0 || s > m_max_s)
    {
        throw std::domain_error("Parameter outside bounds.");
    }
    auto it = std::upper_bound(m_s.begin(), m_s.end(), s);
    //Now *it >= s. We want the index such that m_s[i] <= s < m_s[i+1]:
    size_t i = std::distance(m_s.begin(), it - 1);

    // Only denom21 is used twice:
    typename Point::value_type denom21 = 1/(m_s[i+1] - m_s[i]);
    typename Point::value_type s0s = m_s[i-1] - s;
    typename Point::value_type s1s = m_s[i] - s;
    typename Point::value_type s2s = m_s[i+1] - s;
    size_t ip2 = i + 2;
    // When the curve is closed and we evaluate at the end, the endpoint is in fact the startpoint.
    if (ip2 == m_s.size()) {
        ip2 = 0;
    }
    typename Point::value_type s3s = m_s[ip2] - s;

    Point A1_or_A3;
    typename Point::value_type denom = 1/(m_s[i] - m_s[i-1]);
    for(size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A1_or_A3[j] = denom*(s1s*m_pnts[i-1][j] - s0s*m_pnts[i][j]);
    }

    Point A2_or_B2;
    for(size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A2_or_B2[j] = denom21*(s2s*m_pnts[i][j] - s1s*m_pnts[i+1][j]);
    }

    Point B1_or_C;
    denom = 1/(m_s[i+1] - m_s[i-1]);
    for(size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        B1_or_C[j] = denom*(s2s*A1_or_A3[j] - s0s*A2_or_B2[j]);
    }

    denom = 1/(m_s[ip2] - m_s[i+1]);
    for(size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A1_or_A3[j] = denom*(s3s*m_pnts[i+1][j] - s2s*m_pnts[ip2][j]);
    }

    Point B2;
    denom = 1/(m_s[ip2] - m_s[i]);
    for(size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        B2[j] = denom*(s3s*A2_or_B2[j] - s1s*A1_or_A3[j]);
    }

    for(size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        B1_or_C[j] = denom21*(s2s*B1_or_C[j] - s1s*B2[j]);
    }

    return B1_or_C;
}

template<class Point, class RandomAccessContainer >
Point catmull_rom<Point, RandomAccessContainer>::prime(const typename Point::value_type s) const
{
    using std_workaround::size;
    // https://math.stackexchange.com/questions/843595/how-can-i-calculate-the-derivative-of-a-catmull-rom-spline-with-nonuniform-param
    // http://denkovacs.com/2016/02/catmull-rom-spline-derivatives/
    if (s < 0 || s > m_max_s)
    {
        throw std::domain_error("Parameter outside bounds.\n");
    }
    auto it = std::upper_bound(m_s.begin(), m_s.end(), s);
    //Now *it >= s. We want the index such that m_s[i] <= s < m_s[i+1]:
    size_t i = std::distance(m_s.begin(), it - 1);
    Point A1;
    typename Point::value_type denom = 1/(m_s[i] - m_s[i-1]);
    typename Point::value_type k1 = (m_s[i]-s)*denom;
    typename Point::value_type k2 = (s - m_s[i-1])*denom;
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A1[j] = k1*m_pnts[i-1][j] + k2*m_pnts[i][j];
    }

    Point A1p;
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A1p[j] = denom*(m_pnts[i][j] - m_pnts[i-1][j]);
    }

    Point A2;
    denom = 1/(m_s[i+1] - m_s[i]);
    k1 = (m_s[i+1]-s)*denom;
    k2 = (s - m_s[i])*denom;
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A2[j] = k1*m_pnts[i][j] + k2*m_pnts[i+1][j];
    }

    Point A2p;
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A2p[j] = denom*(m_pnts[i+1][j] - m_pnts[i][j]);
    }


    Point B1;
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        B1[j] = k1*A1[j] + k2*A2[j];
    }

    Point A3;
    denom = 1/(m_s[i+2] - m_s[i+1]);
    k1 = (m_s[i+2]-s)*denom;
    k2 = (s - m_s[i+1])*denom;
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A3[j] = k1*m_pnts[i+1][j] + k2*m_pnts[i+2][j];
    }

    Point A3p;
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        A3p[j] = denom*(m_pnts[i+2][j] - m_pnts[i+1][j]);
    }

    Point B2;
    denom = 1/(m_s[i+2] - m_s[i]);
    k1 = (m_s[i+2]-s)*denom;
    k2 = (s - m_s[i])*denom;
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        B2[j] = k1*A2[j] + k2*A3[j];
    }

    Point B1p;
    denom = 1/(m_s[i+1] - m_s[i-1]);
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        B1p[j] = denom*(A2[j] - A1[j] + (m_s[i+1]- s)*A1p[j] + (s-m_s[i-1])*A2p[j]);
    }

    Point B2p;
    denom = 1/(m_s[i+2] - m_s[i]);
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        B2p[j] = denom*(A3[j] - A2[j] + (m_s[i+2] - s)*A2p[j] + (s - m_s[i])*A3p[j]);
    }

    Point Cp;
    denom = 1/(m_s[i+1] - m_s[i]);
    for (size_t j = 0; j < size(m_pnts[0]); ++j)
    {
        Cp[j] = denom*(B2[j] - B1[j] + (m_s[i+1] - s)*B1p[j] + (s - m_s[i])*B2p[j]);
    }
    return Cp;
}


}}
#endif

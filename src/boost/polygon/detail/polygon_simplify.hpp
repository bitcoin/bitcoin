// Copyright 2011, Andrew Ross
//
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt).
#ifndef BOOST_POLYGON_DETAIL_SIMPLIFY_HPP
#define BOOST_POLYGON_DETAIL_SIMPLIFY_HPP
#include <vector>

namespace boost { namespace polygon { namespace detail { namespace simplify_detail {

  // Does a simplification/optimization pass on the polygon.  If a given
  // vertex lies within "len" of the line segment joining its neighbor
  // vertices, it is removed.
  template <typename T> //T is a model of point concept
  std::size_t simplify(std::vector<T>& dst, const std::vector<T>& src,
                       typename coordinate_traits<
                       typename point_traits<T>::coordinate_type
                       >::coordinate_distance len)
  {
    using namespace boost::polygon;
    typedef typename point_traits<T>::coordinate_type coordinate_type;
    typedef typename coordinate_traits<coordinate_type>::area_type ftype;
    typedef typename std::vector<T>::const_iterator iter;

    std::vector<T> out;
    out.reserve(src.size());
    dst = src;
    std::size_t final_result = 0;
    std::size_t orig_size = src.size();

    //I can't use == if T doesn't provide it, so use generic point concept compare
    bool closed = equivalence(src.front(), src.back());

    //we need to keep smoothing until we don't find points to remove
    //because removing points in the first iteration through the
    //polygon may leave it in a state where more removal is possible
    bool not_done = true;
    while(not_done) {
      if(dst.size() < 3) {
        dst.clear();
        return orig_size;
      }

      // Start with the second, test for the last point
      // explicitly, and exit after looping back around to the first.
      ftype len2 = ftype(len) * ftype(len);
      for(iter prev=dst.begin(), i=prev+1, next; /**/; i = next) {
        next = i+1;
        if(next == dst.end())
          next = dst.begin();

        // points A, B, C
        ftype ax = x(*prev), ay = y(*prev);
        ftype bx = x(*i), by = y(*i);
        ftype cx = x(*next), cy = y(*next);

        // vectors AB, BC and AC:
        ftype abx = bx-ax, aby = by-ay;
        ftype bcx = cx-bx, bcy = cy-by;
        ftype acx = cx-ax, acy = cy-ay;

        // dot products
        ftype ab_ab = abx*abx + aby*aby;
        ftype bc_bc = bcx*bcx + bcy*bcy;
        ftype ac_ac = acx*acx + acy*acy;
        ftype ab_ac = abx*acx + aby*acy;

        // projection of AB along AC
        ftype projf = ab_ac / ac_ac;
        ftype projx = acx * projf, projy = acy * projf;

        // perpendicular vector from the line AC to point B (i.e. AB - proj)
        ftype perpx = abx - projx, perpy = aby - projy;

        // Squared fractional distance of projection. FIXME: can
        // remove this division, the decisions below can be made with
        // just the sign of the quotient and a check to see if
        // abs(numerator) is greater than abs(divisor).
        ftype f2 = (projx*acx + projy*acx) / ac_ac;

        // Square of the relevant distance from point B:
        ftype dist2;
        if     (f2 < 0) dist2 = ab_ab;
        else if(f2 > 1) dist2 = bc_bc;
        else            dist2 = perpx*perpx + perpy*perpy;

        if(dist2 > len2) {
          prev = i; // bump prev, we didn't remove the segment
          out.push_back(*i);
        }

        if(i == dst.begin())
          break;
      }
      std::size_t result = dst.size() - out.size();
      if(result == 0) {
        not_done = false;
      } else {
        final_result += result;
        dst = out;
        out.clear();
      }
    } //end of while loop
    if(closed) {
      //if the input was closed we want the output to be closed
      --final_result;
      dst.push_back(dst.front());
    }
    return final_result;
  }


}}}}

#endif

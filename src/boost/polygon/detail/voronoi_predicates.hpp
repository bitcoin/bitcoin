// Boost.Polygon library detail/voronoi_predicates.hpp header file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_POLYGON_DETAIL_VORONOI_PREDICATES
#define BOOST_POLYGON_DETAIL_VORONOI_PREDICATES

#include <utility>

#include "voronoi_robust_fpt.hpp"

namespace boost {
namespace polygon {
namespace detail {

// Predicate utilities. Operates with the coordinate types that could
// be converted to the 32-bit signed integer without precision loss.
template <typename CTYPE_TRAITS>
class voronoi_predicates {
 public:
  typedef typename CTYPE_TRAITS::int_type int_type;
  typedef typename CTYPE_TRAITS::int_x2_type int_x2_type;
  typedef typename CTYPE_TRAITS::uint_x2_type uint_x2_type;
  typedef typename CTYPE_TRAITS::big_int_type big_int_type;
  typedef typename CTYPE_TRAITS::fpt_type fpt_type;
  typedef typename CTYPE_TRAITS::efpt_type efpt_type;
  typedef typename CTYPE_TRAITS::ulp_cmp_type ulp_cmp_type;
  typedef typename CTYPE_TRAITS::to_fpt_converter_type to_fpt_converter;
  typedef typename CTYPE_TRAITS::to_efpt_converter_type to_efpt_converter;

  enum {
    ULPS = 64,
    ULPSx2 = 128
  };

  template <typename Point>
  static bool is_vertical(const Point& point1, const Point& point2) {
    return point1.x() == point2.x();
  }

  template <typename Site>
  static bool is_vertical(const Site& site) {
    return is_vertical(site.point0(), site.point1());
  }

  // Compute robust cross_product: a1 * b2 - b1 * a2.
  // It was mathematically proven that the result is correct
  // with epsilon relative error equal to 1EPS.
  static fpt_type robust_cross_product(int_x2_type a1_,
                                       int_x2_type b1_,
                                       int_x2_type a2_,
                                       int_x2_type b2_) {
    static to_fpt_converter to_fpt;
    uint_x2_type a1 = static_cast<uint_x2_type>(is_neg(a1_) ? -a1_ : a1_);
    uint_x2_type b1 = static_cast<uint_x2_type>(is_neg(b1_) ? -b1_ : b1_);
    uint_x2_type a2 = static_cast<uint_x2_type>(is_neg(a2_) ? -a2_ : a2_);
    uint_x2_type b2 = static_cast<uint_x2_type>(is_neg(b2_) ? -b2_ : b2_);

    uint_x2_type l = a1 * b2;
    uint_x2_type r = b1 * a2;

    if (is_neg(a1_) ^ is_neg(b2_)) {
      if (is_neg(a2_) ^ is_neg(b1_))
        return (l > r) ? -to_fpt(l - r) : to_fpt(r - l);
      else
        return -to_fpt(l + r);
    } else {
      if (is_neg(a2_) ^ is_neg(b1_))
        return to_fpt(l + r);
      else
        return (l < r) ? -to_fpt(r - l) : to_fpt(l - r);
    }
  }

  struct orientation_test {
   public:
    // Represents orientation test result.
    enum Orientation {
      RIGHT = -1,
      COLLINEAR = 0,
      LEFT = 1
    };

    // Value is a determinant of two vectors (e.g. x1 * y2 - x2 * y1).
    // Return orientation based on the sign of the determinant.
    template <typename T>
    static Orientation eval(T value) {
      if (is_zero(value)) return COLLINEAR;
      return (is_neg(value)) ? RIGHT : LEFT;
    }

    static Orientation eval(int_x2_type dif_x1_,
                            int_x2_type dif_y1_,
                            int_x2_type dif_x2_,
                            int_x2_type dif_y2_) {
      return eval(robust_cross_product(dif_x1_, dif_y1_, dif_x2_, dif_y2_));
    }

    template <typename Point>
    static Orientation eval(const Point& point1,
                            const Point& point2,
                            const Point& point3) {
      int_x2_type dx1 = static_cast<int_x2_type>(point1.x()) -
                        static_cast<int_x2_type>(point2.x());
      int_x2_type dx2 = static_cast<int_x2_type>(point2.x()) -
                        static_cast<int_x2_type>(point3.x());
      int_x2_type dy1 = static_cast<int_x2_type>(point1.y()) -
                        static_cast<int_x2_type>(point2.y());
      int_x2_type dy2 = static_cast<int_x2_type>(point2.y()) -
                        static_cast<int_x2_type>(point3.y());
      return eval(robust_cross_product(dx1, dy1, dx2, dy2));
    }
  };
  typedef orientation_test ot;

  template <typename Point>
  class point_comparison_predicate {
   public:
    typedef Point point_type;

    bool operator()(const point_type& lhs, const point_type& rhs) const {
      if (lhs.x() == rhs.x())
        return lhs.y() < rhs.y();
      return lhs.x() < rhs.x();
    }
  };

  template <typename Site, typename Circle>
  class event_comparison_predicate {
   public:
    typedef Site site_type;
    typedef Circle circle_type;

    bool operator()(const site_type& lhs, const site_type& rhs) const {
      if (lhs.x0() != rhs.x0())
        return lhs.x0() < rhs.x0();
      if (!lhs.is_segment()) {
        if (!rhs.is_segment())
          return lhs.y0() < rhs.y0();
        if (is_vertical(rhs))
          return lhs.y0() <= rhs.y0();
        return true;
      } else {
        if (is_vertical(rhs)) {
          if (is_vertical(lhs))
            return lhs.y0() < rhs.y0();
          return false;
        }
        if (is_vertical(lhs))
          return true;
        if (lhs.y0() != rhs.y0())
          return lhs.y0() < rhs.y0();
        return ot::eval(lhs.point1(), lhs.point0(), rhs.point1()) == ot::LEFT;
      }
    }

    bool operator()(const site_type& lhs, const circle_type& rhs) const {
      typename ulp_cmp_type::Result xCmp =
          ulp_cmp(to_fpt(lhs.x0()), to_fpt(rhs.lower_x()), ULPS);
      return xCmp == ulp_cmp_type::LESS;
    }

    bool operator()(const circle_type& lhs, const site_type& rhs) const {
      typename ulp_cmp_type::Result xCmp =
          ulp_cmp(to_fpt(lhs.lower_x()), to_fpt(rhs.x0()), ULPS);
      return xCmp == ulp_cmp_type::LESS;
    }

    bool operator()(const circle_type& lhs, const circle_type& rhs) const {
      if (lhs.lower_x() != rhs.lower_x()) {
        return lhs.lower_x() < rhs.lower_x();
      }
      return lhs.y() < rhs.y();
    }

   private:
    ulp_cmp_type ulp_cmp;
    to_fpt_converter to_fpt;
  };

  template <typename Site>
  class distance_predicate {
   public:
    typedef Site site_type;
    typedef typename site_type::point_type point_type;

    // Returns true if a horizontal line going through a new site intersects
    // right arc at first, else returns false. If horizontal line goes
    // through intersection point of the given two arcs returns false also.
    bool operator()(const site_type& left_site,
                    const site_type& right_site,
                    const point_type& new_point) const {
      if (!left_site.is_segment()) {
        if (!right_site.is_segment()) {
          return pp(left_site, right_site, new_point);
        } else {
          return ps(left_site, right_site, new_point, false);
        }
      } else {
        if (!right_site.is_segment()) {
          return ps(right_site, left_site, new_point, true);
        } else {
          return ss(left_site, right_site, new_point);
        }
      }
    }

   private:
    // Represents the result of the epsilon robust predicate. If the
    // result is undefined some further processing is usually required.
    enum kPredicateResult {
      LESS = -1,
      UNDEFINED = 0,
      MORE = 1
    };

    // Robust predicate, avoids using high-precision libraries.
    // Returns true if a horizontal line going through the new point site
    // intersects right arc at first, else returns false. If horizontal line
    // goes through intersection point of the given two arcs returns false.
    bool pp(const site_type& left_site,
            const site_type& right_site,
            const point_type& new_point) const {
      const point_type& left_point = left_site.point0();
      const point_type& right_point = right_site.point0();
      if (left_point.x() > right_point.x()) {
        if (new_point.y() <= left_point.y())
          return false;
      } else if (left_point.x() < right_point.x()) {
        if (new_point.y() >= right_point.y())
          return true;
      } else {
        return static_cast<int_x2_type>(left_point.y()) +
               static_cast<int_x2_type>(right_point.y()) <
               static_cast<int_x2_type>(new_point.y()) * 2;
      }

      fpt_type dist1 = find_distance_to_point_arc(left_site, new_point);
      fpt_type dist2 = find_distance_to_point_arc(right_site, new_point);

      // The undefined ulp range is equal to 3EPS + 3EPS <= 6ULP.
      return dist1 < dist2;
    }

    bool ps(const site_type& left_site, const site_type& right_site,
            const point_type& new_point, bool reverse_order) const {
      kPredicateResult fast_res = fast_ps(
        left_site, right_site, new_point, reverse_order);
      if (fast_res != UNDEFINED) {
        return fast_res == LESS;
      }

      fpt_type dist1 = find_distance_to_point_arc(left_site, new_point);
      fpt_type dist2 = find_distance_to_segment_arc(right_site, new_point);

      // The undefined ulp range is equal to 3EPS + 7EPS <= 10ULP.
      return reverse_order ^ (dist1 < dist2);
    }

    bool ss(const site_type& left_site,
            const site_type& right_site,
            const point_type& new_point) const {
      // Handle temporary segment sites.
      if (left_site.sorted_index() == right_site.sorted_index()) {
        return ot::eval(
            left_site.point0(), left_site.point1(), new_point) == ot::LEFT;
      }

      fpt_type dist1 = find_distance_to_segment_arc(left_site, new_point);
      fpt_type dist2 = find_distance_to_segment_arc(right_site, new_point);

      // The undefined ulp range is equal to 7EPS + 7EPS <= 14ULP.
      return dist1 < dist2;
    }

    fpt_type find_distance_to_point_arc(
        const site_type& site, const point_type& point) const {
      fpt_type dx = to_fpt(site.x()) - to_fpt(point.x());
      fpt_type dy = to_fpt(site.y()) - to_fpt(point.y());
      // The relative error is at most 3EPS.
      return (dx * dx + dy * dy) / (to_fpt(2.0) * dx);
    }

    fpt_type find_distance_to_segment_arc(
        const site_type& site, const point_type& point) const {
      if (is_vertical(site)) {
        return (to_fpt(site.x()) - to_fpt(point.x())) * to_fpt(0.5);
      } else {
        const point_type& segment0 = site.point0();
        const point_type& segment1 = site.point1();
        fpt_type a1 = to_fpt(segment1.x()) - to_fpt(segment0.x());
        fpt_type b1 = to_fpt(segment1.y()) - to_fpt(segment0.y());
        fpt_type k = get_sqrt(a1 * a1 + b1 * b1);
        // Avoid subtraction while computing k.
        if (!is_neg(b1)) {
          k = to_fpt(1.0) / (b1 + k);
        } else {
          k = (k - b1) / (a1 * a1);
        }
        // The relative error is at most 7EPS.
        return k * robust_cross_product(
            static_cast<int_x2_type>(segment1.x()) -
            static_cast<int_x2_type>(segment0.x()),
            static_cast<int_x2_type>(segment1.y()) -
            static_cast<int_x2_type>(segment0.y()),
            static_cast<int_x2_type>(point.x()) -
            static_cast<int_x2_type>(segment0.x()),
            static_cast<int_x2_type>(point.y()) -
            static_cast<int_x2_type>(segment0.y()));
      }
    }

    kPredicateResult fast_ps(
        const site_type& left_site, const site_type& right_site,
        const point_type& new_point, bool reverse_order) const {
      const point_type& site_point = left_site.point0();
      const point_type& segment_start = right_site.point0();
      const point_type& segment_end = right_site.point1();

      if (ot::eval(segment_start, segment_end, new_point) != ot::RIGHT)
        return (!right_site.is_inverse()) ? LESS : MORE;

      fpt_type dif_x = to_fpt(new_point.x()) - to_fpt(site_point.x());
      fpt_type dif_y = to_fpt(new_point.y()) - to_fpt(site_point.y());
      fpt_type a = to_fpt(segment_end.x()) - to_fpt(segment_start.x());
      fpt_type b = to_fpt(segment_end.y()) - to_fpt(segment_start.y());

      if (is_vertical(right_site)) {
        if (new_point.y() < site_point.y() && !reverse_order)
          return MORE;
        else if (new_point.y() > site_point.y() && reverse_order)
          return LESS;
        return UNDEFINED;
      } else {
        typename ot::Orientation orientation = ot::eval(
            static_cast<int_x2_type>(segment_end.x()) -
            static_cast<int_x2_type>(segment_start.x()),
            static_cast<int_x2_type>(segment_end.y()) -
            static_cast<int_x2_type>(segment_start.y()),
            static_cast<int_x2_type>(new_point.x()) -
            static_cast<int_x2_type>(site_point.x()),
            static_cast<int_x2_type>(new_point.y()) -
            static_cast<int_x2_type>(site_point.y()));
        if (orientation == ot::LEFT) {
          if (!right_site.is_inverse())
            return reverse_order ? LESS : UNDEFINED;
          return reverse_order ? UNDEFINED : MORE;
        }
      }

      fpt_type fast_left_expr = a * (dif_y + dif_x) * (dif_y - dif_x);
      fpt_type fast_right_expr = (to_fpt(2.0) * b) * dif_x * dif_y;
      typename ulp_cmp_type::Result expr_cmp =
          ulp_cmp(fast_left_expr, fast_right_expr, 4);
      if (expr_cmp != ulp_cmp_type::EQUAL) {
        if ((expr_cmp == ulp_cmp_type::MORE) ^ reverse_order)
          return reverse_order ? LESS : MORE;
        return UNDEFINED;
      }
      return UNDEFINED;
    }

   private:
    ulp_cmp_type ulp_cmp;
    to_fpt_converter to_fpt;
  };

  template <typename Node>
  class node_comparison_predicate {
   public:
    typedef Node node_type;
    typedef typename Node::site_type site_type;
    typedef typename site_type::point_type point_type;
    typedef typename point_type::coordinate_type coordinate_type;
    typedef point_comparison_predicate<point_type> point_comparison_type;
    typedef distance_predicate<site_type> distance_predicate_type;

    // Compares nodes in the balanced binary search tree. Nodes are
    // compared based on the y coordinates of the arcs intersection points.
    // Nodes with less y coordinate of the intersection point go first.
    // Comparison is only called during the new site events processing.
    // That's why one of the nodes will always lie on the sweepline and may
    // be represented as a straight horizontal line.
    bool operator() (const node_type& node1,
                     const node_type& node2) const {
      // Get x coordinate of the rightmost site from both nodes.
      const site_type& site1 = get_comparison_site(node1);
      const site_type& site2 = get_comparison_site(node2);
      const point_type& point1 = get_comparison_point(site1);
      const point_type& point2 = get_comparison_point(site2);

      if (point1.x() < point2.x()) {
        // The second node contains a new site.
        return distance_predicate_(
            node1.left_site(), node1.right_site(), point2);
      } else if (point1.x() > point2.x()) {
        // The first node contains a new site.
        return !distance_predicate_(
            node2.left_site(), node2.right_site(), point1);
      } else {
        // This checks were evaluated experimentally.
        if (site1.sorted_index() == site2.sorted_index()) {
          // Both nodes are new (inserted during same site event processing).
          return get_comparison_y(node1) < get_comparison_y(node2);
        } else if (site1.sorted_index() < site2.sorted_index()) {
          std::pair<coordinate_type, int> y1 = get_comparison_y(node1, false);
          std::pair<coordinate_type, int> y2 = get_comparison_y(node2, true);
          if (y1.first != y2.first) return y1.first < y2.first;
          return (!site1.is_segment()) ? (y1.second < 0) : false;
        } else {
          std::pair<coordinate_type, int> y1 = get_comparison_y(node1, true);
          std::pair<coordinate_type, int> y2 = get_comparison_y(node2, false);
          if (y1.first != y2.first) return y1.first < y2.first;
          return (!site2.is_segment()) ? (y2.second > 0) : true;
        }
      }
    }

   private:
    // Get the newer site.
    const site_type& get_comparison_site(const node_type& node) const {
      if (node.left_site().sorted_index() > node.right_site().sorted_index()) {
        return node.left_site();
      }
      return node.right_site();
    }

    const point_type& get_comparison_point(const site_type& site) const {
      return point_comparison_(site.point0(), site.point1()) ?
          site.point0() : site.point1();
    }

    // Get comparison pair: y coordinate and direction of the newer site.
    std::pair<coordinate_type, int> get_comparison_y(
      const node_type& node, bool is_new_node = true) const {
      if (node.left_site().sorted_index() ==
          node.right_site().sorted_index()) {
        return std::make_pair(node.left_site().y0(), 0);
      }
      if (node.left_site().sorted_index() > node.right_site().sorted_index()) {
        if (!is_new_node &&
            node.left_site().is_segment() &&
            is_vertical(node.left_site())) {
          return std::make_pair(node.left_site().y0(), 1);
        }
        return std::make_pair(node.left_site().y1(), 1);
      }
      return std::make_pair(node.right_site().y0(), -1);
    }

    point_comparison_type point_comparison_;
    distance_predicate_type distance_predicate_;
  };

  template <typename Site>
  class circle_existence_predicate {
   public:
    typedef typename Site::point_type point_type;
    typedef Site site_type;

    bool ppp(const site_type& site1,
             const site_type& site2,
             const site_type& site3) const {
      return ot::eval(site1.point0(),
                      site2.point0(),
                      site3.point0()) == ot::RIGHT;
    }

    bool pps(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             int segment_index) const {
      if (segment_index != 2) {
        typename ot::Orientation orient1 = ot::eval(
            site1.point0(), site2.point0(), site3.point0());
        typename ot::Orientation orient2 = ot::eval(
            site1.point0(), site2.point0(), site3.point1());
        if (segment_index == 1 && site1.x0() >= site2.x0()) {
          if (orient1 != ot::RIGHT)
            return false;
        } else if (segment_index == 3 && site2.x0() >= site1.x0()) {
          if (orient2 != ot::RIGHT)
            return false;
        } else if (orient1 != ot::RIGHT && orient2 != ot::RIGHT) {
          return false;
        }
      } else {
        return (site3.point0() != site1.point0()) ||
               (site3.point1() != site2.point0());
      }
      return true;
    }

    bool pss(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             int point_index) const {
      if (site2.sorted_index() == site3.sorted_index()) {
        return false;
      }
      if (point_index == 2) {
        if (!site2.is_inverse() && site3.is_inverse())
          return false;
        if (site2.is_inverse() == site3.is_inverse() &&
            ot::eval(site2.point0(),
                     site1.point0(),
                     site3.point1()) != ot::RIGHT)
          return false;
      }
      return true;
    }

    bool sss(const site_type& site1,
             const site_type& site2,
             const site_type& site3) const {
      return (site1.sorted_index() != site2.sorted_index()) &&
             (site2.sorted_index() != site3.sorted_index());
    }
  };

  template <typename Site, typename Circle>
  class mp_circle_formation_functor {
   public:
    typedef typename Site::point_type point_type;
    typedef Site site_type;
    typedef Circle circle_type;
    typedef robust_sqrt_expr<big_int_type, efpt_type, to_efpt_converter>
        robust_sqrt_expr_type;

    void ppp(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             circle_type& circle,
             bool recompute_c_x = true,
             bool recompute_c_y = true,
             bool recompute_lower_x = true) {
      big_int_type dif_x[3], dif_y[3], sum_x[2], sum_y[2];
      dif_x[0] = static_cast<int_x2_type>(site1.x()) -
                 static_cast<int_x2_type>(site2.x());
      dif_x[1] = static_cast<int_x2_type>(site2.x()) -
                 static_cast<int_x2_type>(site3.x());
      dif_x[2] = static_cast<int_x2_type>(site1.x()) -
                 static_cast<int_x2_type>(site3.x());
      dif_y[0] = static_cast<int_x2_type>(site1.y()) -
                 static_cast<int_x2_type>(site2.y());
      dif_y[1] = static_cast<int_x2_type>(site2.y()) -
                 static_cast<int_x2_type>(site3.y());
      dif_y[2] = static_cast<int_x2_type>(site1.y()) -
                 static_cast<int_x2_type>(site3.y());
      sum_x[0] = static_cast<int_x2_type>(site1.x()) +
                 static_cast<int_x2_type>(site2.x());
      sum_x[1] = static_cast<int_x2_type>(site2.x()) +
                 static_cast<int_x2_type>(site3.x());
      sum_y[0] = static_cast<int_x2_type>(site1.y()) +
                 static_cast<int_x2_type>(site2.y());
      sum_y[1] = static_cast<int_x2_type>(site2.y()) +
                 static_cast<int_x2_type>(site3.y());
      fpt_type inv_denom = to_fpt(0.5) / to_fpt(static_cast<big_int_type>(
          dif_x[0] * dif_y[1] - dif_x[1] * dif_y[0]));
      big_int_type numer1 = dif_x[0] * sum_x[0] + dif_y[0] * sum_y[0];
      big_int_type numer2 = dif_x[1] * sum_x[1] + dif_y[1] * sum_y[1];

      if (recompute_c_x || recompute_lower_x) {
        big_int_type c_x = numer1 * dif_y[1] - numer2 * dif_y[0];
        circle.x(to_fpt(c_x) * inv_denom);

        if (recompute_lower_x) {
          // Evaluate radius of the circle.
          big_int_type sqr_r = (dif_x[0] * dif_x[0] + dif_y[0] * dif_y[0]) *
                               (dif_x[1] * dif_x[1] + dif_y[1] * dif_y[1]) *
                               (dif_x[2] * dif_x[2] + dif_y[2] * dif_y[2]);
          fpt_type r = get_sqrt(to_fpt(sqr_r));

          // If c_x >= 0 then lower_x = c_x + r,
          // else lower_x = (c_x * c_x - r * r) / (c_x - r).
          // To guarantee epsilon relative error.
          if (!is_neg(circle.x())) {
            if (!is_neg(inv_denom)) {
              circle.lower_x(circle.x() + r * inv_denom);
            } else {
              circle.lower_x(circle.x() - r * inv_denom);
            }
          } else {
            big_int_type numer = c_x * c_x - sqr_r;
            fpt_type lower_x = to_fpt(numer) * inv_denom / (to_fpt(c_x) + r);
            circle.lower_x(lower_x);
          }
        }
      }

      if (recompute_c_y) {
        big_int_type c_y = numer2 * dif_x[0] - numer1 * dif_x[1];
        circle.y(to_fpt(c_y) * inv_denom);
      }
    }

    // Recompute parameters of the circle event using high-precision library.
    void pps(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             int segment_index,
             circle_type& c_event,
             bool recompute_c_x = true,
             bool recompute_c_y = true,
             bool recompute_lower_x = true) {
      big_int_type cA[4], cB[4];
      big_int_type line_a = static_cast<int_x2_type>(site3.y1()) -
                            static_cast<int_x2_type>(site3.y0());
      big_int_type line_b = static_cast<int_x2_type>(site3.x0()) -
                            static_cast<int_x2_type>(site3.x1());
      big_int_type segm_len = line_a * line_a + line_b * line_b;
      big_int_type vec_x = static_cast<int_x2_type>(site2.y()) -
                           static_cast<int_x2_type>(site1.y());
      big_int_type vec_y = static_cast<int_x2_type>(site1.x()) -
                           static_cast<int_x2_type>(site2.x());
      big_int_type sum_x = static_cast<int_x2_type>(site1.x()) +
                           static_cast<int_x2_type>(site2.x());
      big_int_type sum_y = static_cast<int_x2_type>(site1.y()) +
                           static_cast<int_x2_type>(site2.y());
      big_int_type teta = line_a * vec_x + line_b * vec_y;
      big_int_type denom = vec_x * line_b - vec_y * line_a;

      big_int_type dif0 = static_cast<int_x2_type>(site3.y1()) -
                          static_cast<int_x2_type>(site1.y());
      big_int_type dif1 = static_cast<int_x2_type>(site1.x()) -
                          static_cast<int_x2_type>(site3.x1());
      big_int_type A = line_a * dif1 - line_b * dif0;
      dif0 = static_cast<int_x2_type>(site3.y1()) -
             static_cast<int_x2_type>(site2.y());
      dif1 = static_cast<int_x2_type>(site2.x()) -
             static_cast<int_x2_type>(site3.x1());
      big_int_type B = line_a * dif1 - line_b * dif0;
      big_int_type sum_AB = A + B;

      if (is_zero(denom)) {
        big_int_type numer = teta * teta - sum_AB * sum_AB;
        denom = teta * sum_AB;
        cA[0] = denom * sum_x * 2 + numer * vec_x;
        cB[0] = segm_len;
        cA[1] = denom * sum_AB * 2 + numer * teta;
        cB[1] = 1;
        cA[2] = denom * sum_y * 2 + numer * vec_y;
        fpt_type inv_denom = to_fpt(1.0) / to_fpt(denom);
        if (recompute_c_x)
          c_event.x(to_fpt(0.25) * to_fpt(cA[0]) * inv_denom);
        if (recompute_c_y)
          c_event.y(to_fpt(0.25) * to_fpt(cA[2]) * inv_denom);
        if (recompute_lower_x) {
          c_event.lower_x(to_fpt(0.25) * to_fpt(sqrt_expr_.eval2(cA, cB)) *
              inv_denom / get_sqrt(to_fpt(segm_len)));
        }
        return;
      }

      big_int_type det = (teta * teta + denom * denom) * A * B * 4;
      fpt_type inv_denom_sqr = to_fpt(1.0) / to_fpt(denom);
      inv_denom_sqr *= inv_denom_sqr;

      if (recompute_c_x || recompute_lower_x) {
        cA[0] = sum_x * denom * denom + teta * sum_AB * vec_x;
        cB[0] = 1;
        cA[1] = (segment_index == 2) ? -vec_x : vec_x;
        cB[1] = det;
        if (recompute_c_x) {
          c_event.x(to_fpt(0.5) * to_fpt(sqrt_expr_.eval2(cA, cB)) *
              inv_denom_sqr);
        }
      }

      if (recompute_c_y || recompute_lower_x) {
        cA[2] = sum_y * denom * denom + teta * sum_AB * vec_y;
        cB[2] = 1;
        cA[3] = (segment_index == 2) ? -vec_y : vec_y;
        cB[3] = det;
        if (recompute_c_y) {
          c_event.y(to_fpt(0.5) * to_fpt(sqrt_expr_.eval2(&cA[2], &cB[2])) *
                    inv_denom_sqr);
        }
      }

      if (recompute_lower_x) {
        cB[0] = cB[0] * segm_len;
        cB[1] = cB[1] * segm_len;
        cA[2] = sum_AB * (denom * denom + teta * teta);
        cB[2] = 1;
        cA[3] = (segment_index == 2) ? -teta : teta;
        cB[3] = det;
        c_event.lower_x(to_fpt(0.5) * to_fpt(sqrt_expr_.eval4(cA, cB)) *
            inv_denom_sqr / get_sqrt(to_fpt(segm_len)));
      }
    }

    // Recompute parameters of the circle event using high-precision library.
    void pss(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             int point_index,
             circle_type& c_event,
             bool recompute_c_x = true,
             bool recompute_c_y = true,
             bool recompute_lower_x = true) {
      big_int_type a[2], b[2], c[2], cA[4], cB[4];
      const point_type& segm_start1 = site2.point1();
      const point_type& segm_end1 = site2.point0();
      const point_type& segm_start2 = site3.point0();
      const point_type& segm_end2 = site3.point1();
      a[0] = static_cast<int_x2_type>(segm_end1.x()) -
             static_cast<int_x2_type>(segm_start1.x());
      b[0] = static_cast<int_x2_type>(segm_end1.y()) -
             static_cast<int_x2_type>(segm_start1.y());
      a[1] = static_cast<int_x2_type>(segm_end2.x()) -
             static_cast<int_x2_type>(segm_start2.x());
      b[1] = static_cast<int_x2_type>(segm_end2.y()) -
             static_cast<int_x2_type>(segm_start2.y());
      big_int_type orientation = a[1] * b[0] - a[0] * b[1];
      if (is_zero(orientation)) {
        fpt_type denom = to_fpt(2.0) * to_fpt(
            static_cast<big_int_type>(a[0] * a[0] + b[0] * b[0]));
        c[0] = b[0] * (static_cast<int_x2_type>(segm_start2.x()) -
                       static_cast<int_x2_type>(segm_start1.x())) -
               a[0] * (static_cast<int_x2_type>(segm_start2.y()) -
                       static_cast<int_x2_type>(segm_start1.y()));
        big_int_type dx = a[0] * (static_cast<int_x2_type>(site1.y()) -
                                  static_cast<int_x2_type>(segm_start1.y())) -
                          b[0] * (static_cast<int_x2_type>(site1.x()) -
                                  static_cast<int_x2_type>(segm_start1.x()));
        big_int_type dy = b[0] * (static_cast<int_x2_type>(site1.x()) -
                                  static_cast<int_x2_type>(segm_start2.x())) -
                          a[0] * (static_cast<int_x2_type>(site1.y()) -
                                  static_cast<int_x2_type>(segm_start2.y()));
        cB[0] = dx * dy;
        cB[1] = 1;

        if (recompute_c_y) {
          cA[0] = b[0] * ((point_index == 2) ? 2 : -2);
          cA[1] = a[0] * a[0] * (static_cast<int_x2_type>(segm_start1.y()) +
                                 static_cast<int_x2_type>(segm_start2.y())) -
                  a[0] * b[0] * (static_cast<int_x2_type>(segm_start1.x()) +
                                 static_cast<int_x2_type>(segm_start2.x()) -
                                 static_cast<int_x2_type>(site1.x()) * 2) +
                  b[0] * b[0] * (static_cast<int_x2_type>(site1.y()) * 2);
          fpt_type c_y = to_fpt(sqrt_expr_.eval2(cA, cB));
          c_event.y(c_y / denom);
        }

        if (recompute_c_x || recompute_lower_x) {
          cA[0] = a[0] * ((point_index == 2) ? 2 : -2);
          cA[1] = b[0] * b[0] * (static_cast<int_x2_type>(segm_start1.x()) +
                                 static_cast<int_x2_type>(segm_start2.x())) -
                  a[0] * b[0] * (static_cast<int_x2_type>(segm_start1.y()) +
                                 static_cast<int_x2_type>(segm_start2.y()) -
                                 static_cast<int_x2_type>(site1.y()) * 2) +
                  a[0] * a[0] * (static_cast<int_x2_type>(site1.x()) * 2);

          if (recompute_c_x) {
            fpt_type c_x = to_fpt(sqrt_expr_.eval2(cA, cB));
            c_event.x(c_x / denom);
          }

          if (recompute_lower_x) {
            cA[2] = is_neg(c[0]) ? -c[0] : c[0];
            cB[2] = a[0] * a[0] + b[0] * b[0];
            fpt_type lower_x = to_fpt(sqrt_expr_.eval3(cA, cB));
            c_event.lower_x(lower_x / denom);
          }
        }
        return;
      }
      c[0] = b[0] * segm_end1.x() - a[0] * segm_end1.y();
      c[1] = a[1] * segm_end2.y() - b[1] * segm_end2.x();
      big_int_type ix = a[0] * c[1] + a[1] * c[0];
      big_int_type iy = b[0] * c[1] + b[1] * c[0];
      big_int_type dx = ix - orientation * site1.x();
      big_int_type dy = iy - orientation * site1.y();
      if (is_zero(dx) && is_zero(dy)) {
        fpt_type denom = to_fpt(orientation);
        fpt_type c_x = to_fpt(ix) / denom;
        fpt_type c_y = to_fpt(iy) / denom;
        c_event = circle_type(c_x, c_y, c_x);
        return;
      }

      big_int_type sign = ((point_index == 2) ? 1 : -1) *
                          (is_neg(orientation) ? 1 : -1);
      cA[0] = a[1] * -dx + b[1] * -dy;
      cA[1] = a[0] * -dx + b[0] * -dy;
      cA[2] = sign;
      cA[3] = 0;
      cB[0] = a[0] * a[0] + b[0] * b[0];
      cB[1] = a[1] * a[1] + b[1] * b[1];
      cB[2] = a[0] * a[1] + b[0] * b[1];
      cB[3] = (a[0] * dy - b[0] * dx) * (a[1] * dy - b[1] * dx) * -2;
      fpt_type temp = to_fpt(
          sqrt_expr_evaluator_pss4<big_int_type, efpt_type>(cA, cB));
      fpt_type denom = temp * to_fpt(orientation);

      if (recompute_c_y) {
        cA[0] = b[1] * (dx * dx + dy * dy) - iy * (dx * a[1] + dy * b[1]);
        cA[1] = b[0] * (dx * dx + dy * dy) - iy * (dx * a[0] + dy * b[0]);
        cA[2] = iy * sign;
        fpt_type cy = to_fpt(
            sqrt_expr_evaluator_pss4<big_int_type, efpt_type>(cA, cB));
        c_event.y(cy / denom);
      }

      if (recompute_c_x || recompute_lower_x) {
        cA[0] = a[1] * (dx * dx + dy * dy) - ix * (dx * a[1] + dy * b[1]);
        cA[1] = a[0] * (dx * dx + dy * dy) - ix * (dx * a[0] + dy * b[0]);
        cA[2] = ix * sign;

        if (recompute_c_x) {
          fpt_type cx = to_fpt(
              sqrt_expr_evaluator_pss4<big_int_type, efpt_type>(cA, cB));
          c_event.x(cx / denom);
        }

        if (recompute_lower_x) {
          cA[3] = orientation * (dx * dx + dy * dy) * (is_neg(temp) ? -1 : 1);
          fpt_type lower_x = to_fpt(
              sqrt_expr_evaluator_pss4<big_int_type, efpt_type>(cA, cB));
          c_event.lower_x(lower_x / denom);
        }
      }
    }

    // Recompute parameters of the circle event using high-precision library.
    void sss(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             circle_type& c_event,
             bool recompute_c_x = true,
             bool recompute_c_y = true,
             bool recompute_lower_x = true) {
      big_int_type a[3], b[3], c[3], cA[4], cB[4];
      // cA - corresponds to the cross product.
      // cB - corresponds to the squared length.
      a[0] = static_cast<int_x2_type>(site1.x1()) -
             static_cast<int_x2_type>(site1.x0());
      a[1] = static_cast<int_x2_type>(site2.x1()) -
             static_cast<int_x2_type>(site2.x0());
      a[2] = static_cast<int_x2_type>(site3.x1()) -
             static_cast<int_x2_type>(site3.x0());

      b[0] = static_cast<int_x2_type>(site1.y1()) -
             static_cast<int_x2_type>(site1.y0());
      b[1] = static_cast<int_x2_type>(site2.y1()) -
             static_cast<int_x2_type>(site2.y0());
      b[2] = static_cast<int_x2_type>(site3.y1()) -
             static_cast<int_x2_type>(site3.y0());

      c[0] = static_cast<int_x2_type>(site1.x0()) *
             static_cast<int_x2_type>(site1.y1()) -
             static_cast<int_x2_type>(site1.y0()) *
             static_cast<int_x2_type>(site1.x1());
      c[1] = static_cast<int_x2_type>(site2.x0()) *
             static_cast<int_x2_type>(site2.y1()) -
             static_cast<int_x2_type>(site2.y0()) *
             static_cast<int_x2_type>(site2.x1());
      c[2] = static_cast<int_x2_type>(site3.x0()) *
             static_cast<int_x2_type>(site3.y1()) -
             static_cast<int_x2_type>(site3.y0()) *
             static_cast<int_x2_type>(site3.x1());

      for (int i = 0; i < 3; ++i)
        cB[i] = a[i] * a[i] + b[i] * b[i];

      for (int i = 0; i < 3; ++i) {
        int j = (i+1) % 3;
        int k = (i+2) % 3;
        cA[i] = a[j] * b[k] - a[k] * b[j];
      }
      fpt_type denom = to_fpt(sqrt_expr_.eval3(cA, cB));

      if (recompute_c_y) {
        for (int i = 0; i < 3; ++i) {
          int j = (i+1) % 3;
          int k = (i+2) % 3;
          cA[i] = b[j] * c[k] - b[k] * c[j];
        }
        fpt_type c_y = to_fpt(sqrt_expr_.eval3(cA, cB));
        c_event.y(c_y / denom);
      }

      if (recompute_c_x || recompute_lower_x) {
        cA[3] = 0;
        for (int i = 0; i < 3; ++i) {
          int j = (i+1) % 3;
          int k = (i+2) % 3;
          cA[i] = a[j] * c[k] - a[k] * c[j];
          if (recompute_lower_x) {
            cA[3] = cA[3] + cA[i] * b[i];
          }
        }

        if (recompute_c_x) {
          fpt_type c_x = to_fpt(sqrt_expr_.eval3(cA, cB));
          c_event.x(c_x / denom);
        }

        if (recompute_lower_x) {
          cB[3] = 1;
          fpt_type lower_x = to_fpt(sqrt_expr_.eval4(cA, cB));
          c_event.lower_x(lower_x / denom);
        }
      }
    }

   private:
    // Evaluates A[3] + A[0] * sqrt(B[0]) + A[1] * sqrt(B[1]) +
    //           A[2] * sqrt(B[3] * (sqrt(B[0] * B[1]) + B[2])).
    template <typename _int, typename _fpt>
    _fpt sqrt_expr_evaluator_pss4(_int *A, _int *B) {
      _int cA[4], cB[4];
      if (is_zero(A[3])) {
        _fpt lh = sqrt_expr_.eval2(A, B);
        cA[0] = 1;
        cB[0] = B[0] * B[1];
        cA[1] = B[2];
        cB[1] = 1;
        _fpt rh = sqrt_expr_.eval1(A+2, B+3) *
            get_sqrt(sqrt_expr_.eval2(cA, cB));
        if ((!is_neg(lh) && !is_neg(rh)) || (!is_pos(lh) && !is_pos(rh)))
          return lh + rh;
        cA[0] = A[0] * A[0] * B[0] + A[1] * A[1] * B[1] -
                A[2] * A[2] * B[3] * B[2];
        cB[0] = 1;
        cA[1] = A[0] * A[1] * 2 - A[2] * A[2] * B[3];
        cB[1] = B[0] * B[1];
        _fpt numer = sqrt_expr_.eval2(cA, cB);
        return numer / (lh - rh);
      }
      cA[0] = 1;
      cB[0] = B[0] * B[1];
      cA[1] = B[2];
      cB[1] = 1;
      _fpt rh = sqrt_expr_.eval1(A+2, B+3) * get_sqrt(sqrt_expr_.eval2(cA, cB));
      cA[0] = A[0];
      cB[0] = B[0];
      cA[1] = A[1];
      cB[1] = B[1];
      cA[2] = A[3];
      cB[2] = 1;
      _fpt lh = sqrt_expr_.eval3(cA, cB);
      if ((!is_neg(lh) && !is_neg(rh)) || (!is_pos(lh) && !is_pos(rh)))
        return lh + rh;
      cA[0] = A[3] * A[0] * 2;
      cA[1] = A[3] * A[1] * 2;
      cA[2] = A[0] * A[0] * B[0] + A[1] * A[1] * B[1] +
              A[3] * A[3] - A[2] * A[2] * B[2] * B[3];
      cA[3] = A[0] * A[1] * 2 - A[2] * A[2] * B[3];
      cB[3] = B[0] * B[1];
      _fpt numer = sqrt_expr_evaluator_pss3<_int, _fpt>(cA, cB);
      return numer / (lh - rh);
    }

    template <typename _int, typename _fpt>
    // Evaluates A[0] * sqrt(B[0]) + A[1] * sqrt(B[1]) +
    //           A[2] + A[3] * sqrt(B[0] * B[1]).
    // B[3] = B[0] * B[1].
    _fpt sqrt_expr_evaluator_pss3(_int *A, _int *B) {
      _int cA[2], cB[2];
      _fpt lh = sqrt_expr_.eval2(A, B);
      _fpt rh = sqrt_expr_.eval2(A+2, B+2);
      if ((!is_neg(lh) && !is_neg(rh)) || (!is_pos(lh) && !is_pos(rh)))
        return lh + rh;
      cA[0] = A[0] * A[0] * B[0] + A[1] * A[1] * B[1] -
              A[2] * A[2] - A[3] * A[3] * B[0] * B[1];
      cB[0] = 1;
      cA[1] = (A[0] * A[1] - A[2] * A[3]) * 2;
      cB[1] = B[3];
      _fpt numer = sqrt_expr_.eval2(cA, cB);
      return numer / (lh - rh);
    }

    robust_sqrt_expr_type sqrt_expr_;
    to_fpt_converter to_fpt;
  };

  template <typename Site, typename Circle>
  class lazy_circle_formation_functor {
   public:
    typedef robust_fpt<fpt_type> robust_fpt_type;
    typedef robust_dif<robust_fpt_type> robust_dif_type;
    typedef typename Site::point_type point_type;
    typedef Site site_type;
    typedef Circle circle_type;
    typedef mp_circle_formation_functor<site_type, circle_type>
        exact_circle_formation_functor_type;

    void ppp(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             circle_type& c_event) {
      fpt_type dif_x1 = to_fpt(site1.x()) - to_fpt(site2.x());
      fpt_type dif_x2 = to_fpt(site2.x()) - to_fpt(site3.x());
      fpt_type dif_y1 = to_fpt(site1.y()) - to_fpt(site2.y());
      fpt_type dif_y2 = to_fpt(site2.y()) - to_fpt(site3.y());
      fpt_type orientation = robust_cross_product(
          static_cast<int_x2_type>(site1.x()) -
          static_cast<int_x2_type>(site2.x()),
          static_cast<int_x2_type>(site2.x()) -
          static_cast<int_x2_type>(site3.x()),
          static_cast<int_x2_type>(site1.y()) -
          static_cast<int_x2_type>(site2.y()),
          static_cast<int_x2_type>(site2.y()) -
          static_cast<int_x2_type>(site3.y()));
      robust_fpt_type inv_orientation(to_fpt(0.5) / orientation, to_fpt(2.0));
      fpt_type sum_x1 = to_fpt(site1.x()) + to_fpt(site2.x());
      fpt_type sum_x2 = to_fpt(site2.x()) + to_fpt(site3.x());
      fpt_type sum_y1 = to_fpt(site1.y()) + to_fpt(site2.y());
      fpt_type sum_y2 = to_fpt(site2.y()) + to_fpt(site3.y());
      fpt_type dif_x3 = to_fpt(site1.x()) - to_fpt(site3.x());
      fpt_type dif_y3 = to_fpt(site1.y()) - to_fpt(site3.y());
      robust_dif_type c_x, c_y;
      c_x += robust_fpt_type(dif_x1 * sum_x1 * dif_y2, to_fpt(2.0));
      c_x += robust_fpt_type(dif_y1 * sum_y1 * dif_y2, to_fpt(2.0));
      c_x -= robust_fpt_type(dif_x2 * sum_x2 * dif_y1, to_fpt(2.0));
      c_x -= robust_fpt_type(dif_y2 * sum_y2 * dif_y1, to_fpt(2.0));
      c_y += robust_fpt_type(dif_x2 * sum_x2 * dif_x1, to_fpt(2.0));
      c_y += robust_fpt_type(dif_y2 * sum_y2 * dif_x1, to_fpt(2.0));
      c_y -= robust_fpt_type(dif_x1 * sum_x1 * dif_x2, to_fpt(2.0));
      c_y -= robust_fpt_type(dif_y1 * sum_y1 * dif_x2, to_fpt(2.0));
      robust_dif_type lower_x(c_x);
      lower_x -= robust_fpt_type(get_sqrt(
          (dif_x1 * dif_x1 + dif_y1 * dif_y1) *
          (dif_x2 * dif_x2 + dif_y2 * dif_y2) *
          (dif_x3 * dif_x3 + dif_y3 * dif_y3)), to_fpt(5.0));
      c_event = circle_type(
          c_x.dif().fpv() * inv_orientation.fpv(),
          c_y.dif().fpv() * inv_orientation.fpv(),
          lower_x.dif().fpv() * inv_orientation.fpv());
      bool recompute_c_x = c_x.dif().ulp() > ULPS;
      bool recompute_c_y = c_y.dif().ulp() > ULPS;
      bool recompute_lower_x = lower_x.dif().ulp() > ULPS;
      if (recompute_c_x || recompute_c_y || recompute_lower_x) {
        exact_circle_formation_functor_.ppp(
            site1, site2, site3, c_event,
            recompute_c_x, recompute_c_y, recompute_lower_x);
      }
    }

    void pps(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             int segment_index,
             circle_type& c_event) {
      fpt_type line_a = to_fpt(site3.y1()) - to_fpt(site3.y0());
      fpt_type line_b = to_fpt(site3.x0()) - to_fpt(site3.x1());
      fpt_type vec_x = to_fpt(site2.y()) - to_fpt(site1.y());
      fpt_type vec_y = to_fpt(site1.x()) - to_fpt(site2.x());
      robust_fpt_type teta(robust_cross_product(
          static_cast<int_x2_type>(site3.y1()) -
          static_cast<int_x2_type>(site3.y0()),
          static_cast<int_x2_type>(site3.x0()) -
          static_cast<int_x2_type>(site3.x1()),
          static_cast<int_x2_type>(site2.x()) -
          static_cast<int_x2_type>(site1.x()),
          static_cast<int_x2_type>(site2.y()) -
          static_cast<int_x2_type>(site1.y())), to_fpt(1.0));
      robust_fpt_type A(robust_cross_product(
          static_cast<int_x2_type>(site3.y0()) -
          static_cast<int_x2_type>(site3.y1()),
          static_cast<int_x2_type>(site3.x0()) -
          static_cast<int_x2_type>(site3.x1()),
          static_cast<int_x2_type>(site3.y1()) -
          static_cast<int_x2_type>(site1.y()),
          static_cast<int_x2_type>(site3.x1()) -
          static_cast<int_x2_type>(site1.x())), to_fpt(1.0));
      robust_fpt_type B(robust_cross_product(
          static_cast<int_x2_type>(site3.y0()) -
          static_cast<int_x2_type>(site3.y1()),
          static_cast<int_x2_type>(site3.x0()) -
          static_cast<int_x2_type>(site3.x1()),
          static_cast<int_x2_type>(site3.y1()) -
          static_cast<int_x2_type>(site2.y()),
          static_cast<int_x2_type>(site3.x1()) -
          static_cast<int_x2_type>(site2.x())), to_fpt(1.0));
      robust_fpt_type denom(robust_cross_product(
          static_cast<int_x2_type>(site1.y()) -
          static_cast<int_x2_type>(site2.y()),
          static_cast<int_x2_type>(site1.x()) -
          static_cast<int_x2_type>(site2.x()),
          static_cast<int_x2_type>(site3.y1()) -
          static_cast<int_x2_type>(site3.y0()),
          static_cast<int_x2_type>(site3.x1()) -
          static_cast<int_x2_type>(site3.x0())), to_fpt(1.0));
      robust_fpt_type inv_segm_len(to_fpt(1.0) /
          get_sqrt(line_a * line_a + line_b * line_b), to_fpt(3.0));
      robust_dif_type t;
      if (ot::eval(denom) == ot::COLLINEAR) {
        t += teta / (robust_fpt_type(to_fpt(8.0)) * A);
        t -= A / (robust_fpt_type(to_fpt(2.0)) * teta);
      } else {
        robust_fpt_type det = ((teta * teta + denom * denom) * A * B).sqrt();
        if (segment_index == 2) {
          t -= det / (denom * denom);
        } else {
          t += det / (denom * denom);
        }
        t += teta * (A + B) / (robust_fpt_type(to_fpt(2.0)) * denom * denom);
      }
      robust_dif_type c_x, c_y;
      c_x += robust_fpt_type(to_fpt(0.5) *
          (to_fpt(site1.x()) + to_fpt(site2.x())));
      c_x += robust_fpt_type(vec_x) * t;
      c_y += robust_fpt_type(to_fpt(0.5) *
          (to_fpt(site1.y()) + to_fpt(site2.y())));
      c_y += robust_fpt_type(vec_y) * t;
      robust_dif_type r, lower_x(c_x);
      r -= robust_fpt_type(line_a) * robust_fpt_type(site3.x0());
      r -= robust_fpt_type(line_b) * robust_fpt_type(site3.y0());
      r += robust_fpt_type(line_a) * c_x;
      r += robust_fpt_type(line_b) * c_y;
      if (r.pos().fpv() < r.neg().fpv())
        r = -r;
      lower_x += r * inv_segm_len;
      c_event = circle_type(
          c_x.dif().fpv(), c_y.dif().fpv(), lower_x.dif().fpv());
      bool recompute_c_x = c_x.dif().ulp() > ULPS;
      bool recompute_c_y = c_y.dif().ulp() > ULPS;
      bool recompute_lower_x = lower_x.dif().ulp() > ULPS;
      if (recompute_c_x || recompute_c_y || recompute_lower_x) {
        exact_circle_formation_functor_.pps(
            site1, site2, site3, segment_index, c_event,
            recompute_c_x, recompute_c_y, recompute_lower_x);
      }
    }

    void pss(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             int point_index,
             circle_type& c_event) {
      const point_type& segm_start1 = site2.point1();
      const point_type& segm_end1 = site2.point0();
      const point_type& segm_start2 = site3.point0();
      const point_type& segm_end2 = site3.point1();
      fpt_type a1 = to_fpt(segm_end1.x()) - to_fpt(segm_start1.x());
      fpt_type b1 = to_fpt(segm_end1.y()) - to_fpt(segm_start1.y());
      fpt_type a2 = to_fpt(segm_end2.x()) - to_fpt(segm_start2.x());
      fpt_type b2 = to_fpt(segm_end2.y()) - to_fpt(segm_start2.y());
      bool recompute_c_x, recompute_c_y, recompute_lower_x;
      robust_fpt_type orientation(robust_cross_product(
        static_cast<int_x2_type>(segm_end1.y()) -
        static_cast<int_x2_type>(segm_start1.y()),
        static_cast<int_x2_type>(segm_end1.x()) -
        static_cast<int_x2_type>(segm_start1.x()),
        static_cast<int_x2_type>(segm_end2.y()) -
        static_cast<int_x2_type>(segm_start2.y()),
        static_cast<int_x2_type>(segm_end2.x()) -
        static_cast<int_x2_type>(segm_start2.x())), to_fpt(1.0));
      if (ot::eval(orientation) == ot::COLLINEAR) {
        robust_fpt_type a(a1 * a1 + b1 * b1, to_fpt(2.0));
        robust_fpt_type c(robust_cross_product(
            static_cast<int_x2_type>(segm_end1.y()) -
            static_cast<int_x2_type>(segm_start1.y()),
            static_cast<int_x2_type>(segm_end1.x()) -
            static_cast<int_x2_type>(segm_start1.x()),
            static_cast<int_x2_type>(segm_start2.y()) -
            static_cast<int_x2_type>(segm_start1.y()),
            static_cast<int_x2_type>(segm_start2.x()) -
            static_cast<int_x2_type>(segm_start1.x())), to_fpt(1.0));
        robust_fpt_type det(
            robust_cross_product(
                static_cast<int_x2_type>(segm_end1.x()) -
                static_cast<int_x2_type>(segm_start1.x()),
                static_cast<int_x2_type>(segm_end1.y()) -
                static_cast<int_x2_type>(segm_start1.y()),
                static_cast<int_x2_type>(site1.x()) -
                static_cast<int_x2_type>(segm_start1.x()),
                static_cast<int_x2_type>(site1.y()) -
                static_cast<int_x2_type>(segm_start1.y())) *
            robust_cross_product(
                static_cast<int_x2_type>(segm_end1.y()) -
                static_cast<int_x2_type>(segm_start1.y()),
                static_cast<int_x2_type>(segm_end1.x()) -
                static_cast<int_x2_type>(segm_start1.x()),
                static_cast<int_x2_type>(site1.y()) -
                static_cast<int_x2_type>(segm_start2.y()),
                static_cast<int_x2_type>(site1.x()) -
                static_cast<int_x2_type>(segm_start2.x())),
            to_fpt(3.0));
        robust_dif_type t;
        t -= robust_fpt_type(a1) * robust_fpt_type((
             to_fpt(segm_start1.x()) + to_fpt(segm_start2.x())) * to_fpt(0.5) -
             to_fpt(site1.x()));
        t -= robust_fpt_type(b1) * robust_fpt_type((
             to_fpt(segm_start1.y()) + to_fpt(segm_start2.y())) * to_fpt(0.5) -
             to_fpt(site1.y()));
        if (point_index == 2) {
          t += det.sqrt();
        } else {
          t -= det.sqrt();
        }
        t /= a;
        robust_dif_type c_x, c_y;
        c_x += robust_fpt_type(to_fpt(0.5) * (
            to_fpt(segm_start1.x()) + to_fpt(segm_start2.x())));
        c_x += robust_fpt_type(a1) * t;
        c_y += robust_fpt_type(to_fpt(0.5) * (
            to_fpt(segm_start1.y()) + to_fpt(segm_start2.y())));
        c_y += robust_fpt_type(b1) * t;
        robust_dif_type lower_x(c_x);
        if (is_neg(c)) {
          lower_x -= robust_fpt_type(to_fpt(0.5)) * c / a.sqrt();
        } else {
          lower_x += robust_fpt_type(to_fpt(0.5)) * c / a.sqrt();
        }
        recompute_c_x = c_x.dif().ulp() > ULPS;
        recompute_c_y = c_y.dif().ulp() > ULPS;
        recompute_lower_x = lower_x.dif().ulp() > ULPS;
        c_event =
            circle_type(c_x.dif().fpv(), c_y.dif().fpv(), lower_x.dif().fpv());
      } else {
        robust_fpt_type sqr_sum1(get_sqrt(a1 * a1 + b1 * b1), to_fpt(2.0));
        robust_fpt_type sqr_sum2(get_sqrt(a2 * a2 + b2 * b2), to_fpt(2.0));
        robust_fpt_type a(robust_cross_product(
          static_cast<int_x2_type>(segm_end1.x()) -
          static_cast<int_x2_type>(segm_start1.x()),
          static_cast<int_x2_type>(segm_end1.y()) -
          static_cast<int_x2_type>(segm_start1.y()),
          static_cast<int_x2_type>(segm_start2.y()) -
          static_cast<int_x2_type>(segm_end2.y()),
          static_cast<int_x2_type>(segm_end2.x()) -
          static_cast<int_x2_type>(segm_start2.x())), to_fpt(1.0));
        if (!is_neg(a)) {
          a += sqr_sum1 * sqr_sum2;
        } else {
          a = (orientation * orientation) / (sqr_sum1 * sqr_sum2 - a);
        }
        robust_fpt_type or1(robust_cross_product(
            static_cast<int_x2_type>(segm_end1.y()) -
            static_cast<int_x2_type>(segm_start1.y()),
            static_cast<int_x2_type>(segm_end1.x()) -
            static_cast<int_x2_type>(segm_start1.x()),
            static_cast<int_x2_type>(segm_end1.y()) -
            static_cast<int_x2_type>(site1.y()),
            static_cast<int_x2_type>(segm_end1.x()) -
            static_cast<int_x2_type>(site1.x())), to_fpt(1.0));
        robust_fpt_type or2(robust_cross_product(
            static_cast<int_x2_type>(segm_end2.x()) -
            static_cast<int_x2_type>(segm_start2.x()),
            static_cast<int_x2_type>(segm_end2.y()) -
            static_cast<int_x2_type>(segm_start2.y()),
            static_cast<int_x2_type>(segm_end2.x()) -
            static_cast<int_x2_type>(site1.x()),
            static_cast<int_x2_type>(segm_end2.y()) -
            static_cast<int_x2_type>(site1.y())), to_fpt(1.0));
        robust_fpt_type det = robust_fpt_type(to_fpt(2.0)) * a * or1 * or2;
        robust_fpt_type c1(robust_cross_product(
            static_cast<int_x2_type>(segm_end1.y()) -
            static_cast<int_x2_type>(segm_start1.y()),
            static_cast<int_x2_type>(segm_end1.x()) -
            static_cast<int_x2_type>(segm_start1.x()),
            static_cast<int_x2_type>(segm_end1.y()),
            static_cast<int_x2_type>(segm_end1.x())), to_fpt(1.0));
        robust_fpt_type c2(robust_cross_product(
            static_cast<int_x2_type>(segm_end2.x()) -
            static_cast<int_x2_type>(segm_start2.x()),
            static_cast<int_x2_type>(segm_end2.y()) -
            static_cast<int_x2_type>(segm_start2.y()),
            static_cast<int_x2_type>(segm_end2.x()),
            static_cast<int_x2_type>(segm_end2.y())), to_fpt(1.0));
        robust_fpt_type inv_orientation =
            robust_fpt_type(to_fpt(1.0)) / orientation;
        robust_dif_type t, b, ix, iy;
        ix += robust_fpt_type(a2) * c1 * inv_orientation;
        ix += robust_fpt_type(a1) * c2 * inv_orientation;
        iy += robust_fpt_type(b1) * c2 * inv_orientation;
        iy += robust_fpt_type(b2) * c1 * inv_orientation;

        b += ix * (robust_fpt_type(a1) * sqr_sum2);
        b += ix * (robust_fpt_type(a2) * sqr_sum1);
        b += iy * (robust_fpt_type(b1) * sqr_sum2);
        b += iy * (robust_fpt_type(b2) * sqr_sum1);
        b -= sqr_sum1 * robust_fpt_type(robust_cross_product(
            static_cast<int_x2_type>(segm_end2.x()) -
            static_cast<int_x2_type>(segm_start2.x()),
            static_cast<int_x2_type>(segm_end2.y()) -
            static_cast<int_x2_type>(segm_start2.y()),
            static_cast<int_x2_type>(-site1.y()),
            static_cast<int_x2_type>(site1.x())), to_fpt(1.0));
        b -= sqr_sum2 * robust_fpt_type(robust_cross_product(
            static_cast<int_x2_type>(segm_end1.x()) -
            static_cast<int_x2_type>(segm_start1.x()),
            static_cast<int_x2_type>(segm_end1.y()) -
            static_cast<int_x2_type>(segm_start1.y()),
            static_cast<int_x2_type>(-site1.y()),
            static_cast<int_x2_type>(site1.x())), to_fpt(1.0));
        t -= b;
        if (point_index == 2) {
          t += det.sqrt();
        } else {
          t -= det.sqrt();
        }
        t /= (a * a);
        robust_dif_type c_x(ix), c_y(iy);
        c_x += t * (robust_fpt_type(a1) * sqr_sum2);
        c_x += t * (robust_fpt_type(a2) * sqr_sum1);
        c_y += t * (robust_fpt_type(b1) * sqr_sum2);
        c_y += t * (robust_fpt_type(b2) * sqr_sum1);
        if (t.pos().fpv() < t.neg().fpv()) {
          t = -t;
        }
        robust_dif_type lower_x(c_x);
        if (is_neg(orientation)) {
          lower_x -= t * orientation;
        } else {
          lower_x += t * orientation;
        }
        recompute_c_x = c_x.dif().ulp() > ULPS;
        recompute_c_y = c_y.dif().ulp() > ULPS;
        recompute_lower_x = lower_x.dif().ulp() > ULPS;
        c_event = circle_type(
            c_x.dif().fpv(), c_y.dif().fpv(), lower_x.dif().fpv());
      }
      if (recompute_c_x || recompute_c_y || recompute_lower_x) {
          exact_circle_formation_functor_.pss(
              site1, site2, site3, point_index, c_event,
        recompute_c_x, recompute_c_y, recompute_lower_x);
      }
    }

    void sss(const site_type& site1,
             const site_type& site2,
             const site_type& site3,
             circle_type& c_event) {
      robust_fpt_type a1(to_fpt(site1.x1()) - to_fpt(site1.x0()));
      robust_fpt_type b1(to_fpt(site1.y1()) - to_fpt(site1.y0()));
      robust_fpt_type c1(robust_cross_product(
          site1.x0(), site1.y0(),
          site1.x1(), site1.y1()), to_fpt(1.0));

      robust_fpt_type a2(to_fpt(site2.x1()) - to_fpt(site2.x0()));
      robust_fpt_type b2(to_fpt(site2.y1()) - to_fpt(site2.y0()));
      robust_fpt_type c2(robust_cross_product(
          site2.x0(), site2.y0(),
          site2.x1(), site2.y1()), to_fpt(1.0));

      robust_fpt_type a3(to_fpt(site3.x1()) - to_fpt(site3.x0()));
      robust_fpt_type b3(to_fpt(site3.y1()) - to_fpt(site3.y0()));
      robust_fpt_type c3(robust_cross_product(
          site3.x0(), site3.y0(),
          site3.x1(), site3.y1()), to_fpt(1.0));

      robust_fpt_type len1 = (a1 * a1 + b1 * b1).sqrt();
      robust_fpt_type len2 = (a2 * a2 + b2 * b2).sqrt();
      robust_fpt_type len3 = (a3 * a3 + b3 * b3).sqrt();
      robust_fpt_type cross_12(robust_cross_product(
          static_cast<int_x2_type>(site1.x1()) -
          static_cast<int_x2_type>(site1.x0()),
          static_cast<int_x2_type>(site1.y1()) -
          static_cast<int_x2_type>(site1.y0()),
          static_cast<int_x2_type>(site2.x1()) -
          static_cast<int_x2_type>(site2.x0()),
          static_cast<int_x2_type>(site2.y1()) -
          static_cast<int_x2_type>(site2.y0())), to_fpt(1.0));
      robust_fpt_type cross_23(robust_cross_product(
          static_cast<int_x2_type>(site2.x1()) -
          static_cast<int_x2_type>(site2.x0()),
          static_cast<int_x2_type>(site2.y1()) -
          static_cast<int_x2_type>(site2.y0()),
          static_cast<int_x2_type>(site3.x1()) -
          static_cast<int_x2_type>(site3.x0()),
          static_cast<int_x2_type>(site3.y1()) -
          static_cast<int_x2_type>(site3.y0())), to_fpt(1.0));
      robust_fpt_type cross_31(robust_cross_product(
          static_cast<int_x2_type>(site3.x1()) -
          static_cast<int_x2_type>(site3.x0()),
          static_cast<int_x2_type>(site3.y1()) -
          static_cast<int_x2_type>(site3.y0()),
          static_cast<int_x2_type>(site1.x1()) -
          static_cast<int_x2_type>(site1.x0()),
          static_cast<int_x2_type>(site1.y1()) -
          static_cast<int_x2_type>(site1.y0())), to_fpt(1.0));

      // denom = cross_12 * len3 + cross_23 * len1 + cross_31 * len2.
      robust_dif_type denom;
      denom += cross_12 * len3;
      denom += cross_23 * len1;
      denom += cross_31 * len2;

      // denom * r = (b2 * c_x - a2 * c_y - c2 * denom) / len2.
      robust_dif_type r;
      r -= cross_12 * c3;
      r -= cross_23 * c1;
      r -= cross_31 * c2;

      robust_dif_type c_x;
      c_x += a1 * c2 * len3;
      c_x -= a2 * c1 * len3;
      c_x += a2 * c3 * len1;
      c_x -= a3 * c2 * len1;
      c_x += a3 * c1 * len2;
      c_x -= a1 * c3 * len2;

      robust_dif_type c_y;
      c_y += b1 * c2 * len3;
      c_y -= b2 * c1 * len3;
      c_y += b2 * c3 * len1;
      c_y -= b3 * c2 * len1;
      c_y += b3 * c1 * len2;
      c_y -= b1 * c3 * len2;

      robust_dif_type lower_x = c_x + r;

      robust_fpt_type denom_dif = denom.dif();
      robust_fpt_type c_x_dif = c_x.dif() / denom_dif;
      robust_fpt_type c_y_dif = c_y.dif() / denom_dif;
      robust_fpt_type lower_x_dif = lower_x.dif() / denom_dif;

      bool recompute_c_x = c_x_dif.ulp() > ULPS;
      bool recompute_c_y = c_y_dif.ulp() > ULPS;
      bool recompute_lower_x = lower_x_dif.ulp() > ULPS;
      c_event = circle_type(c_x_dif.fpv(), c_y_dif.fpv(), lower_x_dif.fpv());
      if (recompute_c_x || recompute_c_y || recompute_lower_x) {
        exact_circle_formation_functor_.sss(
            site1, site2, site3, c_event,
            recompute_c_x, recompute_c_y, recompute_lower_x);
      }
    }

   private:
    exact_circle_formation_functor_type exact_circle_formation_functor_;
    to_fpt_converter to_fpt;
  };

  template <typename Site,
            typename Circle,
            typename CEP = circle_existence_predicate<Site>,
            typename CFF = lazy_circle_formation_functor<Site, Circle> >
  class circle_formation_predicate {
   public:
    typedef Site site_type;
    typedef Circle circle_type;
    typedef CEP circle_existence_predicate_type;
    typedef CFF circle_formation_functor_type;

    // Create a circle event from the given three sites.
    // Returns true if the circle event exists, else false.
    // If exists circle event is saved into the c_event variable.
    bool operator()(const site_type& site1, const site_type& site2,
                    const site_type& site3, circle_type& circle) {
      if (!site1.is_segment()) {
        if (!site2.is_segment()) {
          if (!site3.is_segment()) {
            // (point, point, point) sites.
            if (!circle_existence_predicate_.ppp(site1, site2, site3))
              return false;
            circle_formation_functor_.ppp(site1, site2, site3, circle);
          } else {
            // (point, point, segment) sites.
            if (!circle_existence_predicate_.pps(site1, site2, site3, 3))
              return false;
            circle_formation_functor_.pps(site1, site2, site3, 3, circle);
          }
        } else {
          if (!site3.is_segment()) {
            // (point, segment, point) sites.
            if (!circle_existence_predicate_.pps(site1, site3, site2, 2))
              return false;
            circle_formation_functor_.pps(site1, site3, site2, 2, circle);
          } else {
            // (point, segment, segment) sites.
            if (!circle_existence_predicate_.pss(site1, site2, site3, 1))
              return false;
            circle_formation_functor_.pss(site1, site2, site3, 1, circle);
          }
        }
      } else {
        if (!site2.is_segment()) {
          if (!site3.is_segment()) {
            // (segment, point, point) sites.
            if (!circle_existence_predicate_.pps(site2, site3, site1, 1))
              return false;
            circle_formation_functor_.pps(site2, site3, site1, 1, circle);
          } else {
            // (segment, point, segment) sites.
            if (!circle_existence_predicate_.pss(site2, site1, site3, 2))
              return false;
            circle_formation_functor_.pss(site2, site1, site3, 2, circle);
          }
        } else {
          if (!site3.is_segment()) {
            // (segment, segment, point) sites.
            if (!circle_existence_predicate_.pss(site3, site1, site2, 3))
              return false;
            circle_formation_functor_.pss(site3, site1, site2, 3, circle);
          } else {
            // (segment, segment, segment) sites.
            if (!circle_existence_predicate_.sss(site1, site2, site3))
              return false;
            circle_formation_functor_.sss(site1, site2, site3, circle);
          }
        }
      }
      if (lies_outside_vertical_segment(circle, site1) ||
          lies_outside_vertical_segment(circle, site2) ||
          lies_outside_vertical_segment(circle, site3)) {
        return false;
      }
      return true;
    }

   private:
    bool lies_outside_vertical_segment(
        const circle_type& c, const site_type& s) {
      if (!s.is_segment() || !is_vertical(s)) {
        return false;
      }
      fpt_type y0 = to_fpt(s.is_inverse() ? s.y1() : s.y0());
      fpt_type y1 = to_fpt(s.is_inverse() ? s.y0() : s.y1());
      return ulp_cmp(c.y(), y0, ULPS) == ulp_cmp_type::LESS ||
             ulp_cmp(c.y(), y1, ULPS) == ulp_cmp_type::MORE;
    }

   private:
    to_fpt_converter to_fpt;
    ulp_cmp_type ulp_cmp;
    circle_existence_predicate_type circle_existence_predicate_;
    circle_formation_functor_type circle_formation_functor_;
  };
};
}  // detail
}  // polygon
}  // boost

#endif  // BOOST_POLYGON_DETAIL_VORONOI_PREDICATES

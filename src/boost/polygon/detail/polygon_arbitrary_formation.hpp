/*
    Copyright 2008 Intel Corporation

    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_ARBITRARY_FORMATION_HPP
#define BOOST_POLYGON_POLYGON_ARBITRARY_FORMATION_HPP
namespace boost { namespace polygon{
  template <typename T, typename T2>
  struct PolyLineArbitraryByConcept {};

  template <typename T>
  class poly_line_arbitrary_polygon_data;
  template <typename T>
  class poly_line_arbitrary_hole_data;

  template <typename Unit>
  struct scanline_base {

    typedef point_data<Unit> Point;
    typedef std::pair<Point, Point> half_edge;

    class less_point {
    public:
      typedef Point first_argument_type;
      typedef Point second_argument_type;
      typedef bool result_type;
      inline less_point() {}
      inline bool operator () (const Point& pt1, const Point& pt2) const {
        if(pt1.get(HORIZONTAL) < pt2.get(HORIZONTAL)) return true;
        if(pt1.get(HORIZONTAL) == pt2.get(HORIZONTAL)) {
          if(pt1.get(VERTICAL) < pt2.get(VERTICAL)) return true;
        }
        return false;
      }
    };

    static inline bool between(Point pt, Point pt1, Point pt2) {
      less_point lp;
      if(lp(pt1, pt2))
        return lp(pt, pt2) && lp(pt1, pt);
      return lp(pt, pt1) && lp(pt2, pt);
    }

    template <typename area_type>
    static inline Unit compute_intercept(const area_type& dy2,
                                         const area_type& dx1,
                                         const area_type& dx2) {
      //intercept = dy2 * dx1 / dx2
      //return (Unit)(((area_type)dy2 * (area_type)dx1) / (area_type)dx2);
      area_type dx1_q = dx1 / dx2;
      area_type dx1_r = dx1 % dx2;
      return dx1_q * dy2 + (dy2 * dx1_r)/dx2;
    }

    template <typename area_type>
    static inline bool equal_slope(area_type dx1, area_type dy1, area_type dx2, area_type dy2) {
      typedef typename coordinate_traits<Unit>::unsigned_area_type unsigned_product_type;
      unsigned_product_type cross_1 = (unsigned_product_type)(dx2 < 0 ? -dx2 :dx2) * (unsigned_product_type)(dy1 < 0 ? -dy1 : dy1);
      unsigned_product_type cross_2 = (unsigned_product_type)(dx1 < 0 ? -dx1 :dx1) * (unsigned_product_type)(dy2 < 0 ? -dy2 : dy2);
      int dx1_sign = dx1 < 0 ? -1 : 1;
      int dx2_sign = dx2 < 0 ? -1 : 1;
      int dy1_sign = dy1 < 0 ? -1 : 1;
      int dy2_sign = dy2 < 0 ? -1 : 1;
      int cross_1_sign = dx2_sign * dy1_sign;
      int cross_2_sign = dx1_sign * dy2_sign;
      return cross_1 == cross_2 && (cross_1_sign == cross_2_sign || cross_1 == 0);
    }

    template <typename T>
    static inline bool equal_slope_hp(const T& dx1, const T& dy1, const T& dx2, const T& dy2) {
      return dx1 * dy2 == dx2 * dy1;
    }

    static inline bool equal_slope(const Unit& x, const Unit& y,
                                   const Point& pt1, const Point& pt2) {
      const Point* pts[2] = {&pt1, &pt2};
      typedef typename coordinate_traits<Unit>::manhattan_area_type at;
      at dy2 = (at)pts[1]->get(VERTICAL) - (at)y;
      at dy1 = (at)pts[0]->get(VERTICAL) - (at)y;
      at dx2 = (at)pts[1]->get(HORIZONTAL) - (at)x;
      at dx1 = (at)pts[0]->get(HORIZONTAL) - (at)x;
      return equal_slope(dx1, dy1, dx2, dy2);
    }

    template <typename area_type>
    static inline bool less_slope(area_type dx1, area_type dy1, area_type dx2, area_type dy2) {
      //reflext x and y slopes to right hand side half plane
      if(dx1 < 0) {
        dy1 *= -1;
        dx1 *= -1;
      } else if(dx1 == 0) {
        //if the first slope is vertical the first cannot be less
        return false;
      }
      if(dx2 < 0) {
        dy2 *= -1;
        dx2 *= -1;
      } else if(dx2 == 0) {
        //if the second slope is vertical the first is always less unless it is also vertical, in which case they are equal
        return dx1 != 0;
      }
      typedef typename coordinate_traits<Unit>::unsigned_area_type unsigned_product_type;
      unsigned_product_type cross_1 = (unsigned_product_type)(dx2 < 0 ? -dx2 :dx2) * (unsigned_product_type)(dy1 < 0 ? -dy1 : dy1);
      unsigned_product_type cross_2 = (unsigned_product_type)(dx1 < 0 ? -dx1 :dx1) * (unsigned_product_type)(dy2 < 0 ? -dy2 : dy2);
      int dx1_sign = dx1 < 0 ? -1 : 1;
      int dx2_sign = dx2 < 0 ? -1 : 1;
      int dy1_sign = dy1 < 0 ? -1 : 1;
      int dy2_sign = dy2 < 0 ? -1 : 1;
      int cross_1_sign = dx2_sign * dy1_sign;
      int cross_2_sign = dx1_sign * dy2_sign;
      if(cross_1_sign < cross_2_sign) return true;
      if(cross_2_sign < cross_1_sign) return false;
      if(cross_1_sign == -1) return cross_2 < cross_1;
      return cross_1 < cross_2;
    }

    static inline bool less_slope(const Unit& x, const Unit& y,
                                  const Point& pt1, const Point& pt2) {
      const Point* pts[2] = {&pt1, &pt2};
      //compute y value on edge from pt_ to pts[1] at the x value of pts[0]
      typedef typename coordinate_traits<Unit>::manhattan_area_type at;
      at dy2 = (at)pts[1]->get(VERTICAL) - (at)y;
      at dy1 = (at)pts[0]->get(VERTICAL) - (at)y;
      at dx2 = (at)pts[1]->get(HORIZONTAL) - (at)x;
      at dx1 = (at)pts[0]->get(HORIZONTAL) - (at)x;
      return less_slope(dx1, dy1, dx2, dy2);
    }

    //return -1 below, 0 on and 1 above line
    static inline int on_above_or_below(Point pt, const half_edge& he) {
      if(pt == he.first || pt == he.second) return 0;
      if(equal_slope(pt.get(HORIZONTAL), pt.get(VERTICAL), he.first, he.second)) return 0;
      bool less_result = less_slope(pt.get(HORIZONTAL), pt.get(VERTICAL), he.first, he.second);
      int retval = less_result ? -1 : 1;
      less_point lp;
      if(lp(he.second, he.first)) retval *= -1;
      if(!between(pt, he.first, he.second)) retval *= -1;
      return retval;
    }

    //returns true is the segment intersects the integer grid square with lower
    //left corner at point
    static inline bool intersects_grid(Point pt, const half_edge& he) {
      if(pt == he.second) return true;
      if(pt == he.first) return true;
      rectangle_data<Unit> rect1;
      set_points(rect1, he.first, he.second);
      if(contains(rect1, pt, true)) {
        if(is_vertical(he) || is_horizontal(he)) return true;
      } else {
        return false; //can't intersect a grid not within bounding box
      }
      Unit x = pt.get(HORIZONTAL);
      Unit y = pt.get(VERTICAL);
      if(equal_slope(x, y, he.first, he.second) &&
         between(pt, he.first, he.second)) return true;
      Point pt01(pt.get(HORIZONTAL), pt.get(VERTICAL) + 1);
      Point pt10(pt.get(HORIZONTAL) + 1, pt.get(VERTICAL));
      Point pt11(pt.get(HORIZONTAL) + 1, pt.get(VERTICAL) + 1);
//       if(pt01 == he.first) return true;
//       if(pt10 == he.first) return true;
//       if(pt11 == he.first) return true;
//       if(pt01 == he.second) return true;
//       if(pt10 == he.second) return true;
//       if(pt11 == he.second) return true;
      //check non-integer intersections
      half_edge widget1(pt, pt11);
      //intersects but not just at pt11
      if(intersects(widget1, he) && on_above_or_below(pt11, he)) return true;
      half_edge widget2(pt01, pt10);
      //intersects but not just at pt01 or 10
      if(intersects(widget2, he) && on_above_or_below(pt01, he) && on_above_or_below(pt10, he)) return true;
      return false;
    }

    static inline Unit evalAtXforYlazy(Unit xIn, Point pt, Point other_pt) {
      long double
        evalAtXforYret, evalAtXforYxIn, evalAtXforYx1, evalAtXforYy1, evalAtXforYdx1, evalAtXforYdx,
        evalAtXforYdy, evalAtXforYx2, evalAtXforYy2, evalAtXforY0;
      //y = (x - x1)dy/dx + y1
      //y = (xIn - pt.x)*(other_pt.y-pt.y)/(other_pt.x-pt.x) + pt.y
      //assert pt.x != other_pt.x
      if(pt.y() == other_pt.y())
        return pt.y();
      evalAtXforYxIn = xIn;
      evalAtXforYx1 = pt.get(HORIZONTAL);
      evalAtXforYy1 = pt.get(VERTICAL);
      evalAtXforYdx1 = evalAtXforYxIn - evalAtXforYx1;
      evalAtXforY0 = 0;
      if(evalAtXforYdx1 == evalAtXforY0) return (Unit)evalAtXforYy1;
      evalAtXforYx2 = other_pt.get(HORIZONTAL);
      evalAtXforYy2 = other_pt.get(VERTICAL);

      evalAtXforYdx = evalAtXforYx2 - evalAtXforYx1;
      evalAtXforYdy = evalAtXforYy2 - evalAtXforYy1;
      evalAtXforYret = ((evalAtXforYdx1) * evalAtXforYdy / evalAtXforYdx + evalAtXforYy1);
      return (Unit)evalAtXforYret;
    }

    static inline typename high_precision_type<Unit>::type evalAtXforY(Unit xIn, Point pt, Point other_pt) {
      typename high_precision_type<Unit>::type
        evalAtXforYret, evalAtXforYxIn, evalAtXforYx1, evalAtXforYy1, evalAtXforYdx1, evalAtXforYdx,
        evalAtXforYdy, evalAtXforYx2, evalAtXforYy2, evalAtXforY0;
      //y = (x - x1)dy/dx + y1
      //y = (xIn - pt.x)*(other_pt.y-pt.y)/(other_pt.x-pt.x) + pt.y
      //assert pt.x != other_pt.x
      typedef typename high_precision_type<Unit>::type high_precision;
      if(pt.y() == other_pt.y())
        return (high_precision)pt.y();
      evalAtXforYxIn = (high_precision)xIn;
      evalAtXforYx1 = pt.get(HORIZONTAL);
      evalAtXforYy1 = pt.get(VERTICAL);
      evalAtXforYdx1 = evalAtXforYxIn - evalAtXforYx1;
      evalAtXforY0 = high_precision(0);
      if(evalAtXforYdx1 == evalAtXforY0) return evalAtXforYret = evalAtXforYy1;
      evalAtXforYx2 = (high_precision)other_pt.get(HORIZONTAL);
      evalAtXforYy2 = (high_precision)other_pt.get(VERTICAL);

      evalAtXforYdx = evalAtXforYx2 - evalAtXforYx1;
      evalAtXforYdy = evalAtXforYy2 - evalAtXforYy1;
      evalAtXforYret = ((evalAtXforYdx1) * evalAtXforYdy / evalAtXforYdx + evalAtXforYy1);
      return evalAtXforYret;
    }

    struct evalAtXforYPack {
    typename high_precision_type<Unit>::type
    evalAtXforYret, evalAtXforYxIn, evalAtXforYx1, evalAtXforYy1, evalAtXforYdx1, evalAtXforYdx,
                           evalAtXforYdy, evalAtXforYx2, evalAtXforYy2, evalAtXforY0;
      inline const typename high_precision_type<Unit>::type& evalAtXforY(Unit xIn, Point pt, Point other_pt) {
        //y = (x - x1)dy/dx + y1
        //y = (xIn - pt.x)*(other_pt.y-pt.y)/(other_pt.x-pt.x) + pt.y
        //assert pt.x != other_pt.x
        typedef typename high_precision_type<Unit>::type high_precision;
        if(pt.y() == other_pt.y()) {
          evalAtXforYret = (high_precision)pt.y();
          return evalAtXforYret;
        }
        evalAtXforYxIn = (high_precision)xIn;
        evalAtXforYx1 = pt.get(HORIZONTAL);
        evalAtXforYy1 = pt.get(VERTICAL);
        evalAtXforYdx1 = evalAtXforYxIn - evalAtXforYx1;
        evalAtXforY0 = high_precision(0);
        if(evalAtXforYdx1 == evalAtXforY0) return evalAtXforYret = evalAtXforYy1;
        evalAtXforYx2 = (high_precision)other_pt.get(HORIZONTAL);
        evalAtXforYy2 = (high_precision)other_pt.get(VERTICAL);

        evalAtXforYdx = evalAtXforYx2 - evalAtXforYx1;
        evalAtXforYdy = evalAtXforYy2 - evalAtXforYy1;
        evalAtXforYret = ((evalAtXforYdx1) * evalAtXforYdy / evalAtXforYdx + evalAtXforYy1);
        return evalAtXforYret;
      }
    };

    static inline bool is_vertical(const half_edge& he) {
      return he.first.get(HORIZONTAL) == he.second.get(HORIZONTAL);
    }

    static inline bool is_horizontal(const half_edge& he) {
      return he.first.get(VERTICAL) == he.second.get(VERTICAL);
    }

    static inline bool is_45_degree(const half_edge& he) {
      return euclidean_distance(he.first, he.second, HORIZONTAL) == euclidean_distance(he.first, he.second, VERTICAL);
    }

    //scanline comparator functor
    class less_half_edge {
    private:
      Unit *x_; //x value at which to apply comparison
      int *justBefore_;
      evalAtXforYPack * pack_;
    public:
      typedef half_edge first_argument_type;
      typedef half_edge second_argument_type;
      typedef bool result_type;
      inline less_half_edge() : x_(0), justBefore_(0), pack_(0) {}
      inline less_half_edge(Unit *x, int *justBefore, evalAtXforYPack * packIn) : x_(x), justBefore_(justBefore), pack_(packIn) {}
      inline less_half_edge(const less_half_edge& that) : x_(that.x_), justBefore_(that.justBefore_),
                                                          pack_(that.pack_){}
      inline less_half_edge& operator=(const less_half_edge& that) {
        x_ = that.x_;
        justBefore_ = that.justBefore_;
        pack_ = that.pack_;
        return *this; }
      inline bool operator () (const half_edge& elm1, const half_edge& elm2) const {
        if((std::max)(elm1.first.y(), elm1.second.y()) < (std::min)(elm2.first.y(), elm2.second.y()))
          return true;
        if((std::min)(elm1.first.y(), elm1.second.y()) > (std::max)(elm2.first.y(), elm2.second.y()))
          return false;

        //check if either x of elem1 is equal to x_
        Unit localx = *x_;
        Unit elm1y = 0;
        bool elm1_at_x = false;
        if(localx == elm1.first.get(HORIZONTAL)) {
          elm1_at_x = true;
          elm1y = elm1.first.get(VERTICAL);
        } else if(localx == elm1.second.get(HORIZONTAL)) {
          elm1_at_x = true;
          elm1y = elm1.second.get(VERTICAL);
        }
        Unit elm2y = 0;
        bool elm2_at_x = false;
        if(localx == elm2.first.get(HORIZONTAL)) {
          elm2_at_x = true;
          elm2y = elm2.first.get(VERTICAL);
        } else if(localx == elm2.second.get(HORIZONTAL)) {
          elm2_at_x = true;
          elm2y = elm2.second.get(VERTICAL);
        }
        bool retval = false;
        if(!(elm1_at_x && elm2_at_x)) {
          //at least one of the segments doesn't have an end point a the current x
          //-1 below, 1 above
          int pt1_oab = on_above_or_below(elm1.first, half_edge(elm2.first, elm2.second));
          int pt2_oab = on_above_or_below(elm1.second, half_edge(elm2.first, elm2.second));
          if(pt1_oab == pt2_oab) {
            if(pt1_oab == -1)
              retval = true; //pt1 is below elm2 so elm1 is below elm2
          } else {
            //the segments can't cross so elm2 is on whatever side of elm1 that one of its ends is
            int pt3_oab = on_above_or_below(elm2.first, half_edge(elm1.first, elm1.second));
            if(pt3_oab == 1)
              retval = true; //elm1's point is above elm1
          }
        } else {
          if(elm1y < elm2y) {
            retval = true;
          } else if(elm1y == elm2y) {
            if(elm1 == elm2)
              return false;
            retval = less_slope(elm1.second.get(HORIZONTAL) - elm1.first.get(HORIZONTAL),
                                     elm1.second.get(VERTICAL) - elm1.first.get(VERTICAL),
                                     elm2.second.get(HORIZONTAL) - elm2.first.get(HORIZONTAL),
                                     elm2.second.get(VERTICAL) - elm2.first.get(VERTICAL));
            retval = ((*justBefore_) != 0) ^ retval;
          }
        }
        return retval;
      }
    };

    template <typename unsigned_product_type>
    static inline void unsigned_add(unsigned_product_type& result, int& result_sign, unsigned_product_type a, int a_sign, unsigned_product_type b, int b_sign) {
      int switcher = 0;
      if(a_sign < 0) switcher += 1;
      if(b_sign < 0) switcher += 2;
      if(a < b) switcher += 4;
      switch (switcher) {
      case 0: //both positive
        result = a + b;
        result_sign = 1;
        break;
      case 1: //a is negative
        result = a - b;
        result_sign = -1;
        break;
      case 2: //b is negative
        result = a - b;
        result_sign = 1;
        break;
      case 3: //both negative
        result = a + b;
        result_sign = -1;
        break;
      case 4: //both positive
        result = a + b;
        result_sign = 1;
        break;
      case 5: //a is negative
        result = b - a;
        result_sign = 1;
        break;
      case 6: //b is negative
        result = b - a;
        result_sign = -1;
        break;
      case 7: //both negative
        result = b + a;
        result_sign = -1;
        break;
      };
    }

    struct compute_intersection_pack {
      typedef typename high_precision_type<Unit>::type high_precision;
      high_precision y_high, dx1, dy1, dx2, dy2, x11, x21, y11, y21, x_num, y_num, x_den, y_den, x, y;
      static inline bool compute_lazy_intersection(Point& intersection, const half_edge& he1, const half_edge& he2,
                                                   bool projected = false, bool round_closest = false) {
        long double y_high, dx1, dy1, dx2, dy2, x11, x21, y11, y21, x_num, y_num, x_den, y_den, x, y;
        typedef rectangle_data<Unit> Rectangle;
        Rectangle rect1, rect2;
        set_points(rect1, he1.first, he1.second);
        set_points(rect2, he2.first, he2.second);
        if(!projected && !::boost::polygon::intersects(rect1, rect2, true)) return false;
        if(is_vertical(he1)) {
          if(is_vertical(he2)) return false;
          y_high = evalAtXforYlazy(he1.first.get(HORIZONTAL), he2.first, he2.second);
          Unit y_local = (Unit)y_high;
          if(y_high < y_local) --y_local;
          if(projected || contains(rect1.get(VERTICAL), y_local, true)) {
            intersection = Point(he1.first.get(HORIZONTAL), y_local);
            return true;
          } else {
            return false;
          }
        } else if(is_vertical(he2)) {
          y_high = evalAtXforYlazy(he2.first.get(HORIZONTAL), he1.first, he1.second);
          Unit y_local = (Unit)y_high;
          if(y_high < y_local) --y_local;
          if(projected || contains(rect2.get(VERTICAL), y_local, true)) {
            intersection = Point(he2.first.get(HORIZONTAL), y_local);
            return true;
          } else {
            return false;
          }
        }
        //the bounding boxes of the two line segments intersect, so we check closer to find the intersection point
        dy2 = (he2.second.get(VERTICAL)) -
          (he2.first.get(VERTICAL));
        dy1 = (he1.second.get(VERTICAL)) -
          (he1.first.get(VERTICAL));
        dx2 = (he2.second.get(HORIZONTAL)) -
          (he2.first.get(HORIZONTAL));
        dx1 = (he1.second.get(HORIZONTAL)) -
          (he1.first.get(HORIZONTAL));
        if(equal_slope_hp(dx1, dy1, dx2, dy2)) return false;
        //the line segments have different slopes
        //we can assume that the line segments are not vertical because such an intersection is handled elsewhere
        x11 = (he1.first.get(HORIZONTAL));
        x21 = (he2.first.get(HORIZONTAL));
        y11 = (he1.first.get(VERTICAL));
        y21 = (he2.first.get(VERTICAL));
        //Unit exp_x = ((at)x11 * (at)dy1 * (at)dx2 - (at)x21 * (at)dy2 * (at)dx1 + (at)y21 * (at)dx1 * (at)dx2 - (at)y11 * (at)dx1 * (at)dx2) / ((at)dy1 * (at)dx2 - (at)dy2 * (at)dx1);
        //Unit exp_y = ((at)y11 * (at)dx1 * (at)dy2 - (at)y21 * (at)dx2 * (at)dy1 + (at)x21 * (at)dy1 * (at)dy2 - (at)x11 * (at)dy1 * (at)dy2) / ((at)dx1 * (at)dy2 - (at)dx2 * (at)dy1);
        x_num = (x11 * dy1 * dx2 - x21 * dy2 * dx1 + y21 * dx1 * dx2 - y11 * dx1 * dx2);
        x_den = (dy1 * dx2 - dy2 * dx1);
        y_num = (y11 * dx1 * dy2 - y21 * dx2 * dy1 + x21 * dy1 * dy2 - x11 * dy1 * dy2);
        y_den = (dx1 * dy2 - dx2 * dy1);
        x = x_num / x_den;
        y = y_num / y_den;
        //std::cout << "cross1 " << dy1 << " " << dx2 << " " << dy1 * dx2 << "\n";
        //std::cout << "cross2 " << dy2 << " " << dx1 << " " << dy2 * dx1 << "\n";
        //Unit exp_x = compute_x_intercept<at>(x11, x21, y11, y21, dy1, dy2, dx1, dx2);
        //Unit exp_y = compute_x_intercept<at>(y11, y21, x11, x21, dx1, dx2, dy1, dy2);
        if(round_closest) {
          x = x + 0.5;
          y = y + 0.5;
        }
        Unit x_unit = (Unit)(x);
        Unit y_unit = (Unit)(y);
        //truncate downward if it went up due to negative number
        if(x < x_unit) --x_unit;
        if(y < y_unit) --y_unit;
        if(is_horizontal(he1))
          y_unit = he1.first.y();
        if(is_horizontal(he2))
          y_unit = he2.first.y();
        //if(x != exp_x || y != exp_y)
        //  std::cout << exp_x << " " << exp_y << " " << x << " " << y << "\n";
        //Unit y1 = evalAtXforY(exp_x, he1.first, he1.second);
        //Unit y2 = evalAtXforY(exp_x, he2.first, he2.second);
        //std::cout << exp_x << " " << exp_y << " " << y1 << " " << y2 << "\n";
        Point result(x_unit, y_unit);
        if(!projected && !contains(rect1, result, true)) return false;
        if(!projected && !contains(rect2, result, true)) return false;
        if(projected) {
          rectangle_data<long double> inf_rect(-(long double)(std::numeric_limits<Unit>::max)(),
                                               -(long double) (std::numeric_limits<Unit>::max)(),
                                               (long double)(std::numeric_limits<Unit>::max)(),
                                               (long double) (std::numeric_limits<Unit>::max)() );
          if(contains(inf_rect, point_data<long double>(x, y), true)) {
            intersection = result;
            return true;
          } else
            return false;
        }
        intersection = result;
        return true;
      }

      inline bool compute_intersection(Point& intersection, const half_edge& he1, const half_edge& he2,
                                       bool projected = false, bool round_closest = false) {
        if(!projected && !intersects(he1, he2))
           return false;
        bool lazy_success = compute_lazy_intersection(intersection, he1, he2, projected);
        if(!projected) {
          if(lazy_success) {
            if(intersects_grid(intersection, he1) &&
               intersects_grid(intersection, he2))
              return true;
          }
        } else {
          return lazy_success;
        }
        return compute_exact_intersection(intersection, he1, he2, projected, round_closest);
      }

      inline bool compute_exact_intersection(Point& intersection, const half_edge& he1, const half_edge& he2,
                                             bool projected = false, bool round_closest = false) {
        if(!projected && !intersects(he1, he2))
           return false;
        typedef rectangle_data<Unit> Rectangle;
        Rectangle rect1, rect2;
        set_points(rect1, he1.first, he1.second);
        set_points(rect2, he2.first, he2.second);
        if(!::boost::polygon::intersects(rect1, rect2, true)) return false;
        if(is_vertical(he1)) {
          if(is_vertical(he2)) return false;
          y_high = evalAtXforY(he1.first.get(HORIZONTAL), he2.first, he2.second);
          Unit y = convert_high_precision_type<Unit>(y_high);
          if(y_high < (high_precision)y) --y;
          if(contains(rect1.get(VERTICAL), y, true)) {
            intersection = Point(he1.first.get(HORIZONTAL), y);
            return true;
          } else {
            return false;
          }
        } else if(is_vertical(he2)) {
          y_high = evalAtXforY(he2.first.get(HORIZONTAL), he1.first, he1.second);
          Unit y = convert_high_precision_type<Unit>(y_high);
          if(y_high < (high_precision)y) --y;
          if(contains(rect2.get(VERTICAL), y, true)) {
            intersection = Point(he2.first.get(HORIZONTAL), y);
            return true;
          } else {
            return false;
          }
        }
        //the bounding boxes of the two line segments intersect, so we check closer to find the intersection point
        dy2 = (high_precision)(he2.second.get(VERTICAL)) -
          (high_precision)(he2.first.get(VERTICAL));
        dy1 = (high_precision)(he1.second.get(VERTICAL)) -
          (high_precision)(he1.first.get(VERTICAL));
        dx2 = (high_precision)(he2.second.get(HORIZONTAL)) -
          (high_precision)(he2.first.get(HORIZONTAL));
        dx1 = (high_precision)(he1.second.get(HORIZONTAL)) -
          (high_precision)(he1.first.get(HORIZONTAL));
        if(equal_slope_hp(dx1, dy1, dx2, dy2)) return false;
        //the line segments have different slopes
        //we can assume that the line segments are not vertical because such an intersection is handled elsewhere
        x11 = (high_precision)(he1.first.get(HORIZONTAL));
        x21 = (high_precision)(he2.first.get(HORIZONTAL));
        y11 = (high_precision)(he1.first.get(VERTICAL));
        y21 = (high_precision)(he2.first.get(VERTICAL));
        //Unit exp_x = ((at)x11 * (at)dy1 * (at)dx2 - (at)x21 * (at)dy2 * (at)dx1 + (at)y21 * (at)dx1 * (at)dx2 - (at)y11 * (at)dx1 * (at)dx2) / ((at)dy1 * (at)dx2 - (at)dy2 * (at)dx1);
        //Unit exp_y = ((at)y11 * (at)dx1 * (at)dy2 - (at)y21 * (at)dx2 * (at)dy1 + (at)x21 * (at)dy1 * (at)dy2 - (at)x11 * (at)dy1 * (at)dy2) / ((at)dx1 * (at)dy2 - (at)dx2 * (at)dy1);
        x_num = (x11 * dy1 * dx2 - x21 * dy2 * dx1 + y21 * dx1 * dx2 - y11 * dx1 * dx2);
        x_den = (dy1 * dx2 - dy2 * dx1);
        y_num = (y11 * dx1 * dy2 - y21 * dx2 * dy1 + x21 * dy1 * dy2 - x11 * dy1 * dy2);
        y_den = (dx1 * dy2 - dx2 * dy1);
        x = x_num / x_den;
        y = y_num / y_den;
        //std::cout << x << " " << y << "\n";
        //std::cout << "cross1 " << dy1 << " " << dx2 << " " << dy1 * dx2 << "\n";
        //std::cout << "cross2 " << dy2 << " " << dx1 << " " << dy2 * dx1 << "\n";
        //Unit exp_x = compute_x_intercept<at>(x11, x21, y11, y21, dy1, dy2, dx1, dx2);
        //Unit exp_y = compute_x_intercept<at>(y11, y21, x11, x21, dx1, dx2, dy1, dy2);
        if(round_closest) {
          x = x + (high_precision)0.5;
          y = y + (high_precision)0.5;
        }
        Unit x_unit = convert_high_precision_type<Unit>(x);
        Unit y_unit = convert_high_precision_type<Unit>(y);
        //truncate downward if it went up due to negative number
        if(x < (high_precision)x_unit) --x_unit;
        if(y < (high_precision)y_unit) --y_unit;
        if(is_horizontal(he1))
          y_unit = he1.first.y();
        if(is_horizontal(he2))
          y_unit = he2.first.y();
        //if(x != exp_x || y != exp_y)
        //  std::cout << exp_x << " " << exp_y << " " << x << " " << y << "\n";
        //Unit y1 = evalAtXforY(exp_x, he1.first, he1.second);
        //Unit y2 = evalAtXforY(exp_x, he2.first, he2.second);
        //std::cout << exp_x << " " << exp_y << " " << y1 << " " << y2 << "\n";
        Point result(x_unit, y_unit);
        if(!contains(rect1, result, true)) return false;
        if(!contains(rect2, result, true)) return false;
        if(projected) {
          high_precision b1 = (high_precision) (std::numeric_limits<Unit>::min)();
          high_precision b2 = (high_precision) (std::numeric_limits<Unit>::max)();
          if(x > b2 || y > b2 || x < b1 || y < b1)
            return false;
        }
        intersection = result;
        return true;
      }
    };

    static inline bool compute_intersection(Point& intersection, const half_edge& he1, const half_edge& he2) {
      typedef typename high_precision_type<Unit>::type high_precision;
      typedef rectangle_data<Unit> Rectangle;
      Rectangle rect1, rect2;
      set_points(rect1, he1.first, he1.second);
      set_points(rect2, he2.first, he2.second);
      if(!::boost::polygon::intersects(rect1, rect2, true)) return false;
      if(is_vertical(he1)) {
        if(is_vertical(he2)) return false;
        high_precision y_high = evalAtXforY(he1.first.get(HORIZONTAL), he2.first, he2.second);
        Unit y = convert_high_precision_type<Unit>(y_high);
        if(y_high < (high_precision)y) --y;
        if(contains(rect1.get(VERTICAL), y, true)) {
          intersection = Point(he1.first.get(HORIZONTAL), y);
          return true;
        } else {
          return false;
        }
      } else if(is_vertical(he2)) {
        high_precision y_high = evalAtXforY(he2.first.get(HORIZONTAL), he1.first, he1.second);
        Unit y = convert_high_precision_type<Unit>(y_high);
        if(y_high < (high_precision)y) --y;
        if(contains(rect2.get(VERTICAL), y, true)) {
          intersection = Point(he2.first.get(HORIZONTAL), y);
          return true;
        } else {
          return false;
        }
      }
      //the bounding boxes of the two line segments intersect, so we check closer to find the intersection point
      high_precision dy2 = (high_precision)(he2.second.get(VERTICAL)) -
        (high_precision)(he2.first.get(VERTICAL));
      high_precision dy1 = (high_precision)(he1.second.get(VERTICAL)) -
        (high_precision)(he1.first.get(VERTICAL));
      high_precision dx2 = (high_precision)(he2.second.get(HORIZONTAL)) -
        (high_precision)(he2.first.get(HORIZONTAL));
      high_precision dx1 = (high_precision)(he1.second.get(HORIZONTAL)) -
        (high_precision)(he1.first.get(HORIZONTAL));
      if(equal_slope_hp(dx1, dy1, dx2, dy2)) return false;
      //the line segments have different slopes
      //we can assume that the line segments are not vertical because such an intersection is handled elsewhere
      high_precision x11 = (high_precision)(he1.first.get(HORIZONTAL));
      high_precision x21 = (high_precision)(he2.first.get(HORIZONTAL));
      high_precision y11 = (high_precision)(he1.first.get(VERTICAL));
      high_precision y21 = (high_precision)(he2.first.get(VERTICAL));
      //Unit exp_x = ((at)x11 * (at)dy1 * (at)dx2 - (at)x21 * (at)dy2 * (at)dx1 + (at)y21 * (at)dx1 * (at)dx2 - (at)y11 * (at)dx1 * (at)dx2) / ((at)dy1 * (at)dx2 - (at)dy2 * (at)dx1);
      //Unit exp_y = ((at)y11 * (at)dx1 * (at)dy2 - (at)y21 * (at)dx2 * (at)dy1 + (at)x21 * (at)dy1 * (at)dy2 - (at)x11 * (at)dy1 * (at)dy2) / ((at)dx1 * (at)dy2 - (at)dx2 * (at)dy1);
      high_precision x_num = (x11 * dy1 * dx2 - x21 * dy2 * dx1 + y21 * dx1 * dx2 - y11 * dx1 * dx2);
      high_precision x_den = (dy1 * dx2 - dy2 * dx1);
      high_precision y_num = (y11 * dx1 * dy2 - y21 * dx2 * dy1 + x21 * dy1 * dy2 - x11 * dy1 * dy2);
      high_precision y_den = (dx1 * dy2 - dx2 * dy1);
      high_precision x = x_num / x_den;
      high_precision y = y_num / y_den;
      //std::cout << "cross1 " << dy1 << " " << dx2 << " " << dy1 * dx2 << "\n";
      //std::cout << "cross2 " << dy2 << " " << dx1 << " " << dy2 * dx1 << "\n";
      //Unit exp_x = compute_x_intercept<at>(x11, x21, y11, y21, dy1, dy2, dx1, dx2);
      //Unit exp_y = compute_x_intercept<at>(y11, y21, x11, x21, dx1, dx2, dy1, dy2);
      Unit x_unit = convert_high_precision_type<Unit>(x);
      Unit y_unit = convert_high_precision_type<Unit>(y);
      //truncate downward if it went up due to negative number
      if(x < (high_precision)x_unit) --x_unit;
      if(y < (high_precision)y_unit) --y_unit;
      if(is_horizontal(he1))
        y_unit = he1.first.y();
      if(is_horizontal(he2))
        y_unit = he2.first.y();
      //if(x != exp_x || y != exp_y)
      //  std::cout << exp_x << " " << exp_y << " " << x << " " << y << "\n";
      //Unit y1 = evalAtXforY(exp_x, he1.first, he1.second);
      //Unit y2 = evalAtXforY(exp_x, he2.first, he2.second);
      //std::cout << exp_x << " " << exp_y << " " << y1 << " " << y2 << "\n";
      Point result(x_unit, y_unit);
      if(!contains(rect1, result, true)) return false;
      if(!contains(rect2, result, true)) return false;
      intersection = result;
      return true;
    }

    static inline bool intersects(const half_edge& he1, const half_edge& he2) {
      typedef rectangle_data<Unit> Rectangle;
      Rectangle rect1, rect2;
      set_points(rect1, he1.first, he1.second);
      set_points(rect2, he2.first, he2.second);
      if(::boost::polygon::intersects(rect1, rect2, false)) {
        if(he1.first == he2.first) {
          if(he1.second != he2.second && equal_slope(he1.first.get(HORIZONTAL), he1.first.get(VERTICAL),
                                                     he1.second, he2.second)) {
            return true;
          } else {
            return false;
          }
        }
        if(he1.first == he2.second) {
          if(he1.second != he2.first && equal_slope(he1.first.get(HORIZONTAL), he1.first.get(VERTICAL),
                                                    he1.second, he2.first)) {
            return true;
          } else {
            return false;
          }
        }
        if(he1.second == he2.first) {
          if(he1.first != he2.second && equal_slope(he1.second.get(HORIZONTAL), he1.second.get(VERTICAL),
                                                    he1.first, he2.second)) {
            return true;
          } else {
            return false;
          }
        }
        if(he1.second == he2.second) {
          if(he1.first != he2.first && equal_slope(he1.second.get(HORIZONTAL), he1.second.get(VERTICAL),
                                                   he1.first, he2.first)) {
            return true;
          } else {
            return false;
          }
        }
        int oab1 = on_above_or_below(he1.first, he2);
        if(oab1 == 0 && between(he1.first, he2.first, he2.second)) return true;
        int oab2 = on_above_or_below(he1.second, he2);
        if(oab2 == 0 && between(he1.second, he2.first, he2.second)) return true;
        if(oab1 == oab2 && oab1 != 0) return false; //both points of he1 are on same side of he2
        int oab3 = on_above_or_below(he2.first, he1);
        if(oab3 == 0 && between(he2.first, he1.first, he1.second)) return true;
        int oab4 = on_above_or_below(he2.second, he1);
        if(oab4 == 0 && between(he2.second, he1.first, he1.second)) return true;
        if(oab3 == oab4) return false; //both points of he2 are on same side of he1
        return true; //they must cross
      }
      if(is_vertical(he1) && is_vertical(he2) && he1.first.get(HORIZONTAL) == he2.first.get(HORIZONTAL))
        return ::boost::polygon::intersects(rect1.get(VERTICAL), rect2.get(VERTICAL), false) &&
          rect1.get(VERTICAL) != rect2.get(VERTICAL);
      if(is_horizontal(he1) && is_horizontal(he2) && he1.first.get(VERTICAL) == he2.first.get(VERTICAL))
        return ::boost::polygon::intersects(rect1.get(HORIZONTAL), rect2.get(HORIZONTAL), false) &&
          rect1.get(HORIZONTAL) != rect2.get(HORIZONTAL);
      return false;
    }

    class vertex_half_edge {
    public:
      typedef typename high_precision_type<Unit>::type high_precision;
      Point pt;
      Point other_pt; // 1, 0 or -1
      int count; //dxdydTheta
      inline vertex_half_edge() : pt(), other_pt(), count() {}
      inline vertex_half_edge(const Point& point, const Point& other_point, int countIn) : pt(point), other_pt(other_point), count(countIn) {}
      inline vertex_half_edge(const vertex_half_edge& vertex) : pt(vertex.pt), other_pt(vertex.other_pt), count(vertex.count) {}
      inline vertex_half_edge& operator=(const vertex_half_edge& vertex){
        pt = vertex.pt; other_pt = vertex.other_pt; count = vertex.count; return *this; }
      inline bool operator==(const vertex_half_edge& vertex) const {
        return pt == vertex.pt && other_pt == vertex.other_pt && count == vertex.count; }
      inline bool operator!=(const vertex_half_edge& vertex) const { return !((*this) == vertex); }
      inline bool operator<(const vertex_half_edge& vertex) const {
        if(pt.get(HORIZONTAL) < vertex.pt.get(HORIZONTAL)) return true;
        if(pt.get(HORIZONTAL) == vertex.pt.get(HORIZONTAL)) {
          if(pt.get(VERTICAL) < vertex.pt.get(VERTICAL)) return true;
          if(pt.get(VERTICAL) == vertex.pt.get(VERTICAL)) { return less_slope(pt.get(HORIZONTAL), pt.get(VERTICAL),
                                                                              other_pt, vertex.other_pt);
          }
        }
        return false;
      }
      inline bool operator>(const vertex_half_edge& vertex) const { return vertex < (*this); }
      inline bool operator<=(const vertex_half_edge& vertex) const { return !((*this) > vertex); }
      inline bool operator>=(const vertex_half_edge& vertex) const { return !((*this) < vertex); }
      inline high_precision evalAtX(Unit xIn) const { return evalAtXforYlazy(xIn, pt, other_pt); }
      inline bool is_vertical() const {
        return pt.get(HORIZONTAL) == other_pt.get(HORIZONTAL);
      }
      inline bool is_begin() const {
        return pt.get(HORIZONTAL) < other_pt.get(HORIZONTAL) ||
          (pt.get(HORIZONTAL) == other_pt.get(HORIZONTAL) &&
           (pt.get(VERTICAL) < other_pt.get(VERTICAL)));
      }
    };

    //when scanning Vertex45 for polygon formation we need a scanline comparator functor
    class less_vertex_half_edge {
    private:
      Unit *x_; //x value at which to apply comparison
      int *justBefore_;
    public:
      typedef vertex_half_edge first_argument_type;
      typedef vertex_half_edge second_argument_type;
      typedef bool result_type;
      inline less_vertex_half_edge() : x_(0), justBefore_(0) {}
      inline less_vertex_half_edge(Unit *x, int *justBefore) : x_(x), justBefore_(justBefore) {}
      inline less_vertex_half_edge(const less_vertex_half_edge& that) : x_(that.x_), justBefore_(that.justBefore_) {}
      inline less_vertex_half_edge& operator=(const less_vertex_half_edge& that) { x_ = that.x_; justBefore_ = that.justBefore_; return *this; }
      inline bool operator () (const vertex_half_edge& elm1, const vertex_half_edge& elm2) const {
        if((std::max)(elm1.pt.y(), elm1.other_pt.y()) < (std::min)(elm2.pt.y(), elm2.other_pt.y()))
          return true;
        if((std::min)(elm1.pt.y(), elm1.other_pt.y()) > (std::max)(elm2.pt.y(), elm2.other_pt.y()))
          return false;
        //check if either x of elem1 is equal to x_
        Unit localx = *x_;
        Unit elm1y = 0;
        bool elm1_at_x = false;
        if(localx == elm1.pt.get(HORIZONTAL)) {
          elm1_at_x = true;
          elm1y = elm1.pt.get(VERTICAL);
        } else if(localx == elm1.other_pt.get(HORIZONTAL)) {
          elm1_at_x = true;
          elm1y = elm1.other_pt.get(VERTICAL);
        }
        Unit elm2y = 0;
        bool elm2_at_x = false;
        if(localx == elm2.pt.get(HORIZONTAL)) {
          elm2_at_x = true;
          elm2y = elm2.pt.get(VERTICAL);
        } else if(localx == elm2.other_pt.get(HORIZONTAL)) {
          elm2_at_x = true;
          elm2y = elm2.other_pt.get(VERTICAL);
        }
        bool retval = false;
        if(!(elm1_at_x && elm2_at_x)) {
          //at least one of the segments doesn't have an end point a the current x
          //-1 below, 1 above
          int pt1_oab = on_above_or_below(elm1.pt, half_edge(elm2.pt, elm2.other_pt));
          int pt2_oab = on_above_or_below(elm1.other_pt, half_edge(elm2.pt, elm2.other_pt));
          if(pt1_oab == pt2_oab) {
            if(pt1_oab == -1)
              retval = true; //pt1 is below elm2 so elm1 is below elm2
          } else {
            //the segments can't cross so elm2 is on whatever side of elm1 that one of its ends is
            int pt3_oab = on_above_or_below(elm2.pt, half_edge(elm1.pt, elm1.other_pt));
            if(pt3_oab == 1)
              retval = true; //elm1's point is above elm1
          }
        } else {
          if(elm1y < elm2y) {
            retval = true;
          } else if(elm1y == elm2y) {
            if(elm1.pt == elm2.pt && elm1.other_pt == elm2.other_pt)
              return false;
            retval = less_slope(elm1.other_pt.get(HORIZONTAL) - elm1.pt.get(HORIZONTAL),
                                     elm1.other_pt.get(VERTICAL) - elm1.pt.get(VERTICAL),
                                     elm2.other_pt.get(HORIZONTAL) - elm2.pt.get(HORIZONTAL),
                                     elm2.other_pt.get(VERTICAL) - elm2.pt.get(VERTICAL));
            retval = ((*justBefore_) != 0) ^ retval;
          }
        }
        return retval;
      }
    };

  };

  template <typename Unit>
  class polygon_arbitrary_formation : public scanline_base<Unit> {
  public:
    typedef typename scanline_base<Unit>::Point Point;
    typedef typename scanline_base<Unit>::half_edge half_edge;
    typedef typename scanline_base<Unit>::vertex_half_edge vertex_half_edge;
    typedef typename scanline_base<Unit>::less_vertex_half_edge less_vertex_half_edge;

    class poly_line_arbitrary {
    public:
      typedef typename std::list<Point>::const_iterator iterator;

      // default constructor of point does not initialize x and y
      inline poly_line_arbitrary() : points() {} //do nothing default constructor

      // initialize a polygon from x,y values, it is assumed that the first is an x
      // and that the input is a well behaved polygon
      template<class iT>
      inline poly_line_arbitrary& set(iT inputBegin, iT inputEnd) {
        points.clear();  //just in case there was some old data there
        while(inputBegin != inputEnd) {
          points.insert(points.end(), *inputBegin);
          ++inputBegin;
        }
        return *this;
      }

      // copy constructor (since we have dynamic memory)
      inline poly_line_arbitrary(const poly_line_arbitrary& that) : points(that.points) {}

      // assignment operator (since we have dynamic memory do a deep copy)
      inline poly_line_arbitrary& operator=(const poly_line_arbitrary& that) {
        points = that.points;
        return *this;
      }

      // get begin iterator, returns a pointer to a const Unit
      inline iterator begin() const { return points.begin(); }

      // get end iterator, returns a pointer to a const Unit
      inline iterator end() const { return points.end(); }

      inline std::size_t size() const { return points.size(); }

      //public data member
      std::list<Point> points;
    };

    class active_tail_arbitrary {
    protected:
      //data
      poly_line_arbitrary* tailp_;
      active_tail_arbitrary *otherTailp_;
      std::list<active_tail_arbitrary*> holesList_;
      bool head_;
    public:

      /**
       * @brief iterator over coordinates of the figure
       */
      typedef typename poly_line_arbitrary::iterator iterator;

      /**
       * @brief iterator over holes contained within the figure
       */
      typedef typename std::list<active_tail_arbitrary*>::const_iterator iteratorHoles;

      //default constructor
      inline active_tail_arbitrary() : tailp_(), otherTailp_(), holesList_(), head_() {}

      //constructor
      inline active_tail_arbitrary(const vertex_half_edge& vertex, active_tail_arbitrary* otherTailp = 0) : tailp_(), otherTailp_(), holesList_(), head_() {
        tailp_ = new poly_line_arbitrary;
        tailp_->points.push_back(vertex.pt);
        //bool headArray[4] = {false, true, true, true};
        bool inverted = vertex.count == -1;
        head_ = (!vertex.is_vertical) ^ inverted;
        otherTailp_ = otherTailp;
      }

      inline active_tail_arbitrary(Point point, active_tail_arbitrary* otherTailp, bool head = true) :
        tailp_(), otherTailp_(), holesList_(), head_() {
        tailp_ = new poly_line_arbitrary;
        tailp_->points.push_back(point);
        head_ = head;
        otherTailp_ = otherTailp;

      }
      inline active_tail_arbitrary(active_tail_arbitrary* otherTailp) :
        tailp_(), otherTailp_(), holesList_(), head_() {
        tailp_ = otherTailp->tailp_;
        otherTailp_ = otherTailp;
      }

      //copy constructor
      inline active_tail_arbitrary(const active_tail_arbitrary& that) :
        tailp_(), otherTailp_(), holesList_(), head_() { (*this) = that; }

      //destructor
      inline ~active_tail_arbitrary() {
        destroyContents();
      }

      //assignment operator
      inline active_tail_arbitrary& operator=(const active_tail_arbitrary& that) {
        tailp_ = new poly_line_arbitrary(*(that.tailp_));
        head_ = that.head_;
        otherTailp_ = that.otherTailp_;
        holesList_ = that.holesList_;
        return *this;
      }

      //equivalence operator
      inline bool operator==(const active_tail_arbitrary& b) const {
        return tailp_ == b.tailp_ && head_ == b.head_;
      }

      /**
       * @brief get the pointer to the polyline that this is an active tail of
       */
      inline poly_line_arbitrary* getTail() const { return tailp_; }

      /**
       * @brief get the pointer to the polyline at the other end of the chain
       */
      inline poly_line_arbitrary* getOtherTail() const { return otherTailp_->tailp_; }

      /**
       * @brief get the pointer to the activetail at the other end of the chain
       */
      inline active_tail_arbitrary* getOtherActiveTail() const { return otherTailp_; }

      /**
       * @brief test if another active tail is the other end of the chain
       */
      inline bool isOtherTail(const active_tail_arbitrary& b) const { return &b == otherTailp_; }

      /**
       * @brief update this end of chain pointer to new polyline
       */
      inline active_tail_arbitrary& updateTail(poly_line_arbitrary* newTail) { tailp_ = newTail; return *this; }

      inline bool join(active_tail_arbitrary* tail) {
        if(tail == otherTailp_) {
          //std::cout << "joining to other tail!\n";
          return false;
        }
        if(tail->head_ == head_) {
          //std::cout << "joining head to head!\n";
          return false;
        }
        if(!tailp_) {
          //std::cout << "joining empty tail!\n";
          return false;
        }
        if(!(otherTailp_->head_)) {
          otherTailp_->copyHoles(*tail);
          otherTailp_->copyHoles(*this);
        } else {
          tail->otherTailp_->copyHoles(*this);
          tail->otherTailp_->copyHoles(*tail);
        }
        poly_line_arbitrary* tail1 = tailp_;
        poly_line_arbitrary* tail2 = tail->tailp_;
        if(head_) std::swap(tail1, tail2);
        typename std::list<point_data<Unit> >::reverse_iterator riter = tail1->points.rbegin();
        typename std::list<point_data<Unit> >::iterator iter = tail2->points.begin();
        if(*riter == *iter) {
          tail1->points.pop_back(); //remove duplicate point
        }
        tail1->points.splice(tail1->points.end(), tail2->points);
        delete tail2;
        otherTailp_->tailp_ = tail1;
        tail->otherTailp_->tailp_ = tail1;
        otherTailp_->otherTailp_ = tail->otherTailp_;
        tail->otherTailp_->otherTailp_ = otherTailp_;
        tailp_ = 0;
        tail->tailp_ = 0;
        tail->otherTailp_ = 0;
        otherTailp_ = 0;
        return true;
      }

      /**
       * @brief associate a hole to this active tail by the specified policy
       */
      inline active_tail_arbitrary* addHole(active_tail_arbitrary* hole) {
        holesList_.push_back(hole);
        copyHoles(*hole);
        copyHoles(*(hole->otherTailp_));
        return this;
      }

      /**
       * @brief get the list of holes
       */
      inline const std::list<active_tail_arbitrary*>& getHoles() const { return holesList_; }

      /**
       * @brief copy holes from that to this
       */
      inline void copyHoles(active_tail_arbitrary& that) { holesList_.splice(holesList_.end(), that.holesList_); }

      /**
       * @brief find out if solid to right
       */
      inline bool solidToRight() const { return !head_; }
      inline bool solidToLeft() const { return head_; }

      /**
       * @brief get vertex
       */
      inline Point getPoint() const {
        if(head_) return tailp_->points.front();
        return tailp_->points.back();
      }

      /**
       * @brief add a coordinate to the polygon at this active tail end, properly handle degenerate edges by removing redundant coordinate
       */
      inline void pushPoint(Point point) {
        if(head_) {
          //if(tailp_->points.size() < 2) {
          //   tailp_->points.push_front(point);
          //   return;
          //}
          typename std::list<Point>::iterator iter = tailp_->points.begin();
          if(iter == tailp_->points.end()) {
            tailp_->points.push_front(point);
            return;
          }
          ++iter;
          if(iter == tailp_->points.end()) {
            tailp_->points.push_front(point);
            return;
          }
          --iter;
          if(*iter != point) {
            tailp_->points.push_front(point);
          }
          return;
        }
        //if(tailp_->points.size() < 2) {
        //   tailp_->points.push_back(point);
        //   return;
        //}
        typename std::list<Point>::reverse_iterator iter = tailp_->points.rbegin();
        if(iter == tailp_->points.rend()) {
          tailp_->points.push_back(point);
          return;
        }
        ++iter;
        if(iter == tailp_->points.rend()) {
          tailp_->points.push_back(point);
          return;
        }
        --iter;
        if(*iter != point) {
          tailp_->points.push_back(point);
        }
      }

      /**
       * @brief joins the two chains that the two active tail tails are ends of
       * checks for closure of figure and writes out polygons appropriately
       * returns a handle to a hole if one is closed
       */
      template <class cT>
      static inline active_tail_arbitrary* joinChains(Point point, active_tail_arbitrary* at1, active_tail_arbitrary* at2, bool solid,
                                                      cT& output) {
        if(at1->otherTailp_ == at2) {
          //if(at2->otherTailp_ != at1) std::cout << "half closed error\n";
          //we are closing a figure
          at1->pushPoint(point);
          at2->pushPoint(point);
          if(solid) {
            //we are closing a solid figure, write to output
            //std::cout << "test1\n";
            at1->copyHoles(*(at1->otherTailp_));
            typename PolyLineArbitraryByConcept<Unit, typename geometry_concept<typename cT::value_type>::type>::type polyData(at1);
            //poly_line_arbitrary_polygon_data polyData(at1);
            //std::cout << "test2\n";
            //std::cout << poly << "\n";
            //std::cout << "test3\n";
            typedef typename cT::value_type result_type;
            output.push_back(result_type());
            assign(output.back(), polyData);
            //std::cout << "test4\n";
            //std::cout << "delete " << at1->otherTailp_ << "\n";
            //at1->print();
            //at1->otherTailp_->print();
            delete at1->otherTailp_;
            //at1->print();
            //at1->otherTailp_->print();
            //std::cout << "test5\n";
            //std::cout << "delete " << at1 << "\n";
            delete at1;
            //std::cout << "test6\n";
            return 0;
          } else {
            //we are closing a hole, return the tail end active tail of the figure
            return at1;
          }
        }
        //we are not closing a figure
        at1->pushPoint(point);
        at1->join(at2);
        delete at1;
        delete at2;
        return 0;
      }

      inline void destroyContents() {
        if(otherTailp_) {
          //std::cout << "delete p " << tailp_ << "\n";
          if(tailp_) delete tailp_;
          tailp_ = 0;
          otherTailp_->otherTailp_ = 0;
          otherTailp_->tailp_ = 0;
          otherTailp_ = 0;
        }
        for(typename std::list<active_tail_arbitrary*>::iterator itr = holesList_.begin(); itr != holesList_.end(); ++itr) {
          //std::cout << "delete p " << (*itr) << "\n";
          if(*itr) {
            if((*itr)->otherTailp_) {
              delete (*itr)->otherTailp_;
              (*itr)->otherTailp_ = 0;
            }
            delete (*itr);
          }
          (*itr) = 0;
        }
        holesList_.clear();
      }

      inline void print() {
        //std::cout << this << " " << tailp_ << " " << otherTailp_ << " " << holesList_.size() << " " << head_ << "\n";
      }

      static inline std::pair<active_tail_arbitrary*, active_tail_arbitrary*> createActiveTailsAsPair(Point point, bool solid,
                                                                                                      active_tail_arbitrary* phole, bool fractureHoles) {
        active_tail_arbitrary* at1 = 0;
        active_tail_arbitrary* at2 = 0;
        if(phole && fractureHoles) {
          //std::cout << "adding hole\n";
          at1 = phole;
          //assert solid == false, we should be creating a corner with solid below and to the left if there was a hole
          at2 = at1->getOtherActiveTail();
          at2->pushPoint(point);
          at1->pushPoint(point);
        } else {
          at1 = new active_tail_arbitrary(point, at2, solid);
          at2 = new active_tail_arbitrary(at1);
          at1->otherTailp_ = at2;
          at2->head_ = !solid;
          if(phole)
            at2->addHole(phole); //assert fractureHoles == false
        }
        return std::pair<active_tail_arbitrary*, active_tail_arbitrary*>(at1, at2);
      }

    };


    typedef std::vector<std::pair<Point, int> > vertex_arbitrary_count;

    class less_half_edge_count {
    private:
      Point pt_;
    public:
      typedef vertex_half_edge first_argument_type;
      typedef vertex_half_edge second_argument_type;
      typedef bool result_type;
      inline less_half_edge_count() : pt_() {}
      inline less_half_edge_count(Point point) : pt_(point) {}
      inline bool operator () (const std::pair<Point, int>& elm1, const std::pair<Point, int>& elm2) const {
        return scanline_base<Unit>::less_slope(pt_.get(HORIZONTAL), pt_.get(VERTICAL), elm1.first, elm2.first);
      }
    };

    static inline void sort_vertex_arbitrary_count(vertex_arbitrary_count& count, const Point& pt) {
      less_half_edge_count lfec(pt);
      polygon_sort(count.begin(), count.end(), lfec);
    }

    typedef std::vector<std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*> > incoming_count;

    class less_incoming_count {
    private:
      Point pt_;
    public:
      typedef std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*> first_argument_type;
      typedef std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*> second_argument_type;
      typedef bool result_type;
      inline less_incoming_count() : pt_() {}
      inline less_incoming_count(Point point) : pt_(point) {}
      inline bool operator () (const std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*>& elm1,
                               const std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*>& elm2) const {
        Unit dx1 = elm1.first.first.first.get(HORIZONTAL) - elm1.first.first.second.get(HORIZONTAL);
        Unit dx2 = elm2.first.first.first.get(HORIZONTAL) - elm2.first.first.second.get(HORIZONTAL);
        Unit dy1 = elm1.first.first.first.get(VERTICAL) - elm1.first.first.second.get(VERTICAL);
        Unit dy2 = elm2.first.first.first.get(VERTICAL) - elm2.first.first.second.get(VERTICAL);
        return scanline_base<Unit>::less_slope(dx1, dy1, dx2, dy2);
      }
    };

    static inline void sort_incoming_count(incoming_count& count, const Point& pt) {
      less_incoming_count lfec(pt);
      polygon_sort(count.begin(), count.end(), lfec);
    }

    static inline void compact_vertex_arbitrary_count(const Point& pt, vertex_arbitrary_count &count) {
      if(count.empty()) return;
      vertex_arbitrary_count tmp;
      tmp.reserve(count.size());
      tmp.push_back(count[0]);
      //merge duplicates
      for(std::size_t i = 1; i < count.size(); ++i) {
        if(!equal_slope(pt.get(HORIZONTAL), pt.get(VERTICAL), tmp[i-1].first, count[i].first)) {
          tmp.push_back(count[i]);
        } else {
          tmp.back().second += count[i].second;
        }
      }
      count.clear();
      count.swap(tmp);
    }

    // inline std::ostream& operator<< (std::ostream& o, const vertex_arbitrary_count& c) {
//       for(unsinged int i = 0; i < c.size(); ++i) {
//         o << c[i].first << " " << c[i].second << " ";
//       }
//       return o;
//     }

    class vertex_arbitrary_compact {
    public:
      Point pt;
      vertex_arbitrary_count count;
      inline vertex_arbitrary_compact() : pt(), count() {}
      inline vertex_arbitrary_compact(const Point& point, const Point& other_point, int countIn) : pt(point), count() {
        count.push_back(std::pair<Point, int>(other_point, countIn));
      }
      inline vertex_arbitrary_compact(const vertex_half_edge& vertex) : pt(vertex.pt), count() {
        count.push_back(std::pair<Point, int>(vertex.other_pt, vertex.count));
      }
      inline vertex_arbitrary_compact(const vertex_arbitrary_compact& vertex) : pt(vertex.pt), count(vertex.count) {}
      inline vertex_arbitrary_compact& operator=(const vertex_arbitrary_compact& vertex){
        pt = vertex.pt; count = vertex.count; return *this; }
      inline bool operator==(const vertex_arbitrary_compact& vertex) const {
        return pt == vertex.pt && count == vertex.count; }
      inline bool operator!=(const vertex_arbitrary_compact& vertex) const { return !((*this) == vertex); }
      inline bool operator<(const vertex_arbitrary_compact& vertex) const {
        if(pt.get(HORIZONTAL) < vertex.pt.get(HORIZONTAL)) return true;
        if(pt.get(HORIZONTAL) == vertex.pt.get(HORIZONTAL)) {
          return pt.get(VERTICAL) < vertex.pt.get(VERTICAL);
        }
        return false;
      }
      inline bool operator>(const vertex_arbitrary_compact& vertex) const { return vertex < (*this); }
      inline bool operator<=(const vertex_arbitrary_compact& vertex) const { return !((*this) > vertex); }
      inline bool operator>=(const vertex_arbitrary_compact& vertex) const { return !((*this) < vertex); }
      inline bool have_vertex_half_edge(int index) const { return count[index]; }
      inline vertex_half_edge operator[](int index) const { return vertex_half_edge(pt, count[index]); }
      };

//     inline std::ostream& operator<< (std::ostream& o, const vertex_arbitrary_compact& c) {
//       o << c.pt << ", " << c.count;
//       return o;
//     }

  protected:
    //definitions
    typedef std::map<vertex_half_edge, active_tail_arbitrary*, less_vertex_half_edge> scanline_data;
    typedef typename scanline_data::iterator iterator;
    typedef typename scanline_data::const_iterator const_iterator;

    //data
    scanline_data scanData_;
    Unit x_;
    int justBefore_;
    int fractureHoles_;
  public:
    inline polygon_arbitrary_formation() :
      scanData_(), x_((std::numeric_limits<Unit>::min)()), justBefore_(false), fractureHoles_(0) {
      less_vertex_half_edge lessElm(&x_, &justBefore_);
      scanData_ = scanline_data(lessElm);
    }
    inline polygon_arbitrary_formation(bool fractureHoles) :
      scanData_(), x_((std::numeric_limits<Unit>::min)()), justBefore_(false), fractureHoles_(fractureHoles) {
      less_vertex_half_edge lessElm(&x_, &justBefore_);
      scanData_ = scanline_data(lessElm);
    }
    inline polygon_arbitrary_formation(const polygon_arbitrary_formation& that) :
      scanData_(), x_((std::numeric_limits<Unit>::min)()), justBefore_(false), fractureHoles_(0) { (*this) = that; }
    inline polygon_arbitrary_formation& operator=(const polygon_arbitrary_formation& that) {
      x_ = that.x_;
      justBefore_ = that.justBefore_;
      fractureHoles_ = that.fractureHoles_;
      less_vertex_half_edge lessElm(&x_, &justBefore_);
      scanData_ = scanline_data(lessElm);
      for(const_iterator itr = that.scanData_.begin(); itr != that.scanData_.end(); ++itr){
        scanData_.insert(scanData_.end(), *itr);
      }
      return *this;
    }

    //cT is an output container of Polygon45 or Polygon45WithHoles
    //iT is an iterator over vertex_half_edge elements
    //inputBegin - inputEnd is a range of sorted iT that represents
    //one or more scanline stops worth of data
    template <class cT, class iT>
    void scan(cT& output, iT inputBegin, iT inputEnd) {
      //std::cout << "1\n";
      while(inputBegin != inputEnd) {
        //std::cout << "2\n";
        x_ = (*inputBegin).pt.get(HORIZONTAL);
        //std::cout << "SCAN FORMATION " << x_ << "\n";
        //std::cout << "x_ = " << x_ << "\n";
        //std::cout << "scan line size: " << scanData_.size() << "\n";
        inputBegin = processEvent_(output, inputBegin, inputEnd);
      }
      //std::cout << "scan line size: " << scanData_.size() << "\n";
    }

  protected:
    //functions
    template <class cT, class cT2>
    inline std::pair<std::pair<Point, int>, active_tail_arbitrary*> processPoint_(cT& output, cT2& elements, Point point,
                                                                                  incoming_count& counts_from_scanline, vertex_arbitrary_count& incoming_count) {
      //std::cout << "\nAT POINT: " <<  point << "\n";
      //join any closing solid corners
      std::vector<int> counts;
      std::vector<int> incoming;
      std::vector<active_tail_arbitrary*> tails;
      counts.reserve(counts_from_scanline.size());
      tails.reserve(counts_from_scanline.size());
      incoming.reserve(incoming_count.size());
      for(std::size_t i = 0; i < counts_from_scanline.size(); ++i) {
        counts.push_back(counts_from_scanline[i].first.second);
        tails.push_back(counts_from_scanline[i].second);
      }
      for(std::size_t i = 0; i < incoming_count.size(); ++i) {
        incoming.push_back(incoming_count[i].second);
        if(incoming_count[i].first < point) {
          incoming.back() = 0;
        }
      }

      active_tail_arbitrary* returnValue = 0;
      std::pair<Point, int> returnCount(Point(0, 0), 0);
      int i_size_less_1 = (int)(incoming.size()) -1;
      int c_size_less_1 = (int)(counts.size()) -1;
      int i_size = incoming.size();
      int c_size = counts.size();

      bool have_vertical_tail_from_below = false;
      if(c_size &&
         scanline_base<Unit>::is_vertical(counts_from_scanline.back().first.first)) {
        have_vertical_tail_from_below = true;
      }
      //assert size = size_less_1 + 1
      //std::cout << tails.size() << " " << incoming.size() << " " << counts_from_scanline.size() << " " << incoming_count.size() << "\n";
      //         for(std::size_t i = 0; i < counts.size(); ++i) {
      //           std::cout << counts_from_scanline[i].first.first.first.get(HORIZONTAL) << ",";
      //           std::cout << counts_from_scanline[i].first.first.first.get(VERTICAL) << " ";
      //           std::cout << counts_from_scanline[i].first.first.second.get(HORIZONTAL) << ",";
      //           std::cout << counts_from_scanline[i].first.first.second.get(VERTICAL) << ":";
      //           std::cout << counts_from_scanline[i].first.second << " ";
      //         } std::cout << "\n";
      //         print(incoming_count);
      {
        for(int i = 0; i < c_size_less_1; ++i) {
          //std::cout << i << "\n";
          if(counts[i] == -1) {
            //std::cout << "fixed i\n";
            for(int j = i + 1; j < c_size; ++j) {
              //std::cout << j << "\n";
              if(counts[j]) {
                if(counts[j] == 1) {
                  //std::cout << "case1: " << i << " " << j << "\n";
                  //if a figure is closed it will be written out by this function to output
                  active_tail_arbitrary::joinChains(point, tails[i], tails[j], true, output);
                  counts[i] = 0;
                  counts[j] = 0;
                  tails[i] = 0;
                  tails[j] = 0;
                }
                break;
              }
            }
          }
        }
      }
      //find any pairs of incoming edges that need to create pair for leading solid
      //std::cout << "checking case2\n";
      {
        for(int i = 0; i < i_size_less_1; ++i) {
          //std::cout << i << "\n";
          if(incoming[i] == 1) {
            //std::cout << "fixed i\n";
            for(int j = i + 1; j < i_size; ++j) {
              //std::cout << j << "\n";
              if(incoming[j]) {
                //std::cout << incoming[j] << "\n";
                if(incoming[j] == -1) {
                  //std::cout << "case2: " << i << " " << j << "\n";
                  //std::cout << "creating active tail pair\n";
                  std::pair<active_tail_arbitrary*, active_tail_arbitrary*> tailPair =
                    active_tail_arbitrary::createActiveTailsAsPair(point, true, 0, fractureHoles_ != 0);
                  //tailPair.first->print();
                  //tailPair.second->print();
                  if(j == i_size_less_1 && incoming_count[j].first.get(HORIZONTAL) == point.get(HORIZONTAL)) {
                    //vertical active tail becomes return value
                    returnValue = tailPair.first;
                    returnCount.first = point;
                    returnCount.second = 1;
                  } else {
                    //std::cout << "new element " << j-1 << " " << -1 << "\n";
                    //std::cout << point << " " <<  incoming_count[j].first << "\n";
                    elements.push_back(std::pair<vertex_half_edge,
                                       active_tail_arbitrary*>(vertex_half_edge(point,
                                                                                incoming_count[j].first, -1), tailPair.first));
                  }
                  //std::cout << "new element " << i-1 << " " << 1 << "\n";
                  //std::cout << point << " " <<  incoming_count[i].first << "\n";
                  elements.push_back(std::pair<vertex_half_edge,
                                     active_tail_arbitrary*>(vertex_half_edge(point,
                                                                              incoming_count[i].first, 1), tailPair.second));
                  incoming[i] = 0;
                  incoming[j] = 0;
                }
                break;
              }
            }
          }
        }
      }
      //find any active tail that needs to pass through to an incoming edge
      //we expect to find no more than two pass through

      //find pass through with solid on top
      {
        //std::cout << "checking case 3\n";
        for(int i = 0; i < c_size; ++i) {
          //std::cout << i << "\n";
          if(counts[i] != 0) {
            if(counts[i] == 1) {
              //std::cout << "fixed i\n";
              for(int j = i_size_less_1; j >= 0; --j) {
                if(incoming[j] != 0) {
                  if(incoming[j] == 1) {
                    //std::cout << "case3: " << i << " " << j << "\n";
                    //tails[i]->print();
                    //pass through solid on top
                    tails[i]->pushPoint(point);
                    //std::cout << "after push\n";
                    if(j == i_size_less_1 && incoming_count[j].first.get(HORIZONTAL) == point.get(HORIZONTAL)) {
                      returnValue = tails[i];
                      returnCount.first = point;
                      returnCount.second = -1;
                    } else {
                      elements.push_back(std::pair<vertex_half_edge,
                                         active_tail_arbitrary*>(vertex_half_edge(point,
                                                                                  incoming_count[j].first, incoming[j]), tails[i]));
                    }
                    tails[i] = 0;
                    counts[i] = 0;
                    incoming[j] = 0;
                  }
                  break;
                }
              }
            }
            break;
          }
        }
      }
      //std::cout << "checking case 4\n";
      //find pass through with solid on bottom
      {
        for(int i = c_size_less_1; i >= 0; --i) {
          //std::cout << "i = " << i << " with count " << counts[i] << "\n";
          if(counts[i] != 0) {
            if(counts[i] == -1) {
              for(int j = 0; j < i_size; ++j) {
                if(incoming[j] != 0) {
                  if(incoming[j] == -1) {
                    //std::cout << "case4: " << i << " " << j << "\n";
                    //pass through solid on bottom
                    tails[i]->pushPoint(point);
                    if(j == i_size_less_1 && incoming_count[j].first.get(HORIZONTAL) == point.get(HORIZONTAL)) {
                      returnValue = tails[i];
                      returnCount.first = point;
                      returnCount.second = 1;
                    } else {
                      //std::cout << "new element " << j-1 << " " << incoming[j] << "\n";
                      //std::cout << point << " " <<  incoming_count[j].first << "\n";
                      elements.push_back(std::pair<vertex_half_edge,
                                         active_tail_arbitrary*>(vertex_half_edge(point,
                                                                                  incoming_count[j].first, incoming[j]), tails[i]));
                    }
                    tails[i] = 0;
                    counts[i] = 0;
                    incoming[j] = 0;
                  }
                  break;
                }
              }
            }
            break;
          }
        }
      }
      //find the end of a hole or the beginning of a hole

      //find end of a hole
      {
        for(int i = 0; i < c_size_less_1; ++i) {
          if(counts[i] != 0) {
            for(int j = i+1; j < c_size; ++j) {
              if(counts[j] != 0) {
                //std::cout << "case5: " << i << " " << j << "\n";
                //we are ending a hole and may potentially close a figure and have to handle the hole
                returnValue = active_tail_arbitrary::joinChains(point, tails[i], tails[j], false, output);
                if(returnValue) returnCount.first = point;
                //std::cout << returnValue << "\n";
                tails[i] = 0;
                tails[j] = 0;
                counts[i] = 0;
                counts[j] = 0;
                break;
              }
            }
            break;
          }
        }
      }
      //find beginning of a hole
      {
        for(int i = 0; i < i_size_less_1; ++i) {
          if(incoming[i] != 0) {
            for(int j = i+1; j < i_size; ++j) {
              if(incoming[j] != 0) {
                //std::cout << "case6: " << i << " " << j << "\n";
                //we are beginning a empty space
                active_tail_arbitrary* holep = 0;
                //if(c_size && counts[c_size_less_1] == 0 &&
                //   counts_from_scanline[c_size_less_1].first.first.first.get(HORIZONTAL) == point.get(HORIZONTAL))
                if(have_vertical_tail_from_below) {
                  holep = tails[c_size_less_1];
                  tails[c_size_less_1] = 0;
                  have_vertical_tail_from_below = false;
                }
                std::pair<active_tail_arbitrary*, active_tail_arbitrary*> tailPair =
                  active_tail_arbitrary::createActiveTailsAsPair(point, false, holep, fractureHoles_ != 0);
                if(j == i_size_less_1 && incoming_count[j].first.get(HORIZONTAL) == point.get(HORIZONTAL)) {
                  //std::cout << "vertical element " << point << "\n";
                  returnValue = tailPair.first;
                  returnCount.first = point;
                  //returnCount = incoming_count[j];
                  returnCount.second = -1;
                } else {
                  //std::cout << "new element " << j-1 << " " << incoming[j] << "\n";
                  //std::cout << point << " " <<  incoming_count[j].first << "\n";
                  elements.push_back(std::pair<vertex_half_edge,
                                     active_tail_arbitrary*>(vertex_half_edge(point,
                                                                              incoming_count[j].first, incoming[j]), tailPair.first));
                }
                //std::cout << "new element " << i-1 << " " << incoming[i] << "\n";
                //std::cout << point << " " <<  incoming_count[i].first << "\n";
                elements.push_back(std::pair<vertex_half_edge,
                                   active_tail_arbitrary*>(vertex_half_edge(point,
                                                                            incoming_count[i].first, incoming[i]), tailPair.second));
                incoming[i] = 0;
                incoming[j] = 0;
                break;
              }
            }
            break;
          }
        }
      }
      if(have_vertical_tail_from_below) {
        if(tails.back()) {
          tails.back()->pushPoint(point);
          returnValue = tails.back();
          returnCount.first = point;
          returnCount.second = counts.back();
        }
      }
      //assert that tails, counts and incoming are all null
      return std::pair<std::pair<Point, int>, active_tail_arbitrary*>(returnCount, returnValue);
    }

    static inline void print(const vertex_arbitrary_count& count) {
      for(unsigned i = 0; i < count.size(); ++i) {
        //std::cout << count[i].first.get(HORIZONTAL) << ",";
        //std::cout << count[i].first.get(VERTICAL) << ":";
        //std::cout << count[i].second << " ";
      } //std::cout << "\n";
    }

    static inline void print(const scanline_data& data) {
      for(typename scanline_data::const_iterator itr = data.begin(); itr != data.end(); ++itr){
        //std::cout << itr->first.pt << ", " << itr->first.other_pt << "; ";
      } //std::cout << "\n";
    }

    template <class cT, class iT>
    inline iT processEvent_(cT& output, iT inputBegin, iT inputEnd) {
      typedef typename high_precision_type<Unit>::type high_precision;
      //std::cout << "processEvent_\n";
      justBefore_ = true;
      //collect up all elements from the tree that are at the y
      //values of events in the input queue
      //create vector of new elements to add into tree
      active_tail_arbitrary* verticalTail = 0;
      std::pair<Point, int> verticalCount(Point(0, 0), 0);
      iT currentIter = inputBegin;
      std::vector<iterator> elementIters;
      std::vector<std::pair<vertex_half_edge, active_tail_arbitrary*> > elements;
      while(currentIter != inputEnd && currentIter->pt.get(HORIZONTAL) == x_) {
        //std::cout << "loop\n";
        Unit currentY = (*currentIter).pt.get(VERTICAL);
        //std::cout << "current Y " << currentY << "\n";
        //std::cout << "scanline size " << scanData_.size() << "\n";
        //print(scanData_);
        iterator iter = lookUp_(currentY);
        //std::cout << "found element in scanline " << (iter != scanData_.end()) << "\n";
        //int counts[4] = {0, 0, 0, 0};
        incoming_count counts_from_scanline;
        //std::cout << "finding elements in tree\n";
        //if(iter != scanData_.end())
        //  std::cout << "first iter y is " << iter->first.evalAtX(x_) << "\n";
        while(iter != scanData_.end() &&
              ((iter->first.pt.x() == x_ && iter->first.pt.y() == currentY) ||
               (iter->first.other_pt.x() == x_ && iter->first.other_pt.y() == currentY))) {
                //iter->first.evalAtX(x_) == (high_precision)currentY) {
          //std::cout << "loop2\n";
          elementIters.push_back(iter);
          counts_from_scanline.push_back(std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*>
                                         (std::pair<std::pair<Point, Point>, int>(std::pair<Point, Point>(iter->first.pt,
                                                                                                          iter->first.other_pt),
                                                                                  iter->first.count),
                                          iter->second));
          ++iter;
        }
        Point currentPoint(x_, currentY);
        //std::cout << "counts_from_scanline size " << counts_from_scanline.size() << "\n";
        sort_incoming_count(counts_from_scanline, currentPoint);

        vertex_arbitrary_count incoming;
        //std::cout << "aggregating\n";
        do {
          //std::cout << "loop3\n";
          const vertex_half_edge& elem = *currentIter;
          incoming.push_back(std::pair<Point, int>(elem.other_pt, elem.count));
          ++currentIter;
        } while(currentIter != inputEnd && currentIter->pt.get(VERTICAL) == currentY &&
                currentIter->pt.get(HORIZONTAL) == x_);
        //print(incoming);
        sort_vertex_arbitrary_count(incoming, currentPoint);
        //std::cout << currentPoint.get(HORIZONTAL) << "," << currentPoint.get(VERTICAL) << "\n";
        //print(incoming);
        //std::cout << "incoming counts from input size " << incoming.size() << "\n";
        //compact_vertex_arbitrary_count(currentPoint, incoming);
        vertex_arbitrary_count tmp;
        tmp.reserve(incoming.size());
        for(std::size_t i = 0; i < incoming.size(); ++i) {
          if(currentPoint < incoming[i].first) {
            tmp.push_back(incoming[i]);
          }
        }
        incoming.swap(tmp);
        //std::cout << "incoming counts from input size " << incoming.size() << "\n";
        //now counts_from_scanline has the data from the left and
        //incoming has the data from the right at this point
        //cancel out any end points
        if(verticalTail) {
          //std::cout << "adding vertical tail to counts from scanline\n";
          //std::cout << -verticalCount.second << "\n";
          counts_from_scanline.push_back(std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*>
                                         (std::pair<std::pair<Point, Point>, int>(std::pair<Point, Point>(verticalCount.first,
                                                                                                          currentPoint),
                                                                                  -verticalCount.second),
                                          verticalTail));
        }
        if(!incoming.empty() && incoming.back().first.get(HORIZONTAL) == x_) {
          //std::cout << "inverted vertical event\n";
          incoming.back().second *= -1;
        }
        //std::cout << "calling processPoint_\n";
        std::pair<std::pair<Point, int>, active_tail_arbitrary*> result = processPoint_(output, elements, Point(x_, currentY), counts_from_scanline, incoming);
        verticalCount = result.first;
        verticalTail = result.second;
        //if(verticalTail) {
        //  std::cout << "have vertical tail\n";
        //  std::cout << verticalCount.second << "\n";
        //}
        if(verticalTail && !(verticalCount.second)) {
          //we got a hole out of the point we just processed
          //iter is still at the next y element above the current y value in the tree
          //std::cout << "checking whether ot handle hole\n";
          if(currentIter == inputEnd ||
             currentIter->pt.get(HORIZONTAL) != x_ ||
             scanline_base<Unit>::on_above_or_below(currentIter->pt, half_edge(iter->first.pt, iter->first.other_pt)) != -1) {
            //(high_precision)(currentIter->pt.get(VERTICAL)) >= iter->first.evalAtX(x_)) {

            //std::cout << "handle hole here\n";
            if(fractureHoles_) {
              //std::cout << "fracture hole here\n";
              //we need to handle the hole now and not at the next input vertex
              active_tail_arbitrary* at = iter->second;
              high_precision precise_y = iter->first.evalAtX(x_);
              Unit fracture_y = convert_high_precision_type<Unit>(precise_y);
              if(precise_y < fracture_y) --fracture_y;
              Point point(x_, fracture_y);
              verticalTail->getOtherActiveTail()->pushPoint(point);
              iter->second = verticalTail->getOtherActiveTail();
              at->pushPoint(point);
              verticalTail->join(at);
              delete at;
              delete verticalTail;
              verticalTail = 0;
            } else {
              //std::cout << "push hole onto list\n";
              iter->second->addHole(verticalTail);
              verticalTail = 0;
            }
          }
        }
      }
      //std::cout << "erasing\n";
      //erase all elements from the tree
      for(typename std::vector<iterator>::iterator iter = elementIters.begin();
          iter != elementIters.end(); ++iter) {
        //std::cout << "erasing loop\n";
        scanData_.erase(*iter);
      }
      //switch comparison tie breaking policy
      justBefore_ = false;
      //add new elements into tree
      //std::cout << "inserting\n";
      for(typename std::vector<std::pair<vertex_half_edge, active_tail_arbitrary*> >::iterator iter = elements.begin();
          iter != elements.end(); ++iter) {
        //std::cout << "inserting loop\n";
        scanData_.insert(scanData_.end(), *iter);
      }
      //std::cout << "end processEvent\n";
      return currentIter;
    }

    inline iterator lookUp_(Unit y){
      //if just before then we need to look from 1 not -1
      //std::cout << "just before " << justBefore_ << "\n";
      return scanData_.lower_bound(vertex_half_edge(Point(x_, y), Point(x_, y+1), 0));
    }

  public: //test functions

    template <typename stream_type>
    static inline bool testPolygonArbitraryFormationRect(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      polygon_arbitrary_formation pf(true);
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(0, 10), 1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(10, 10), -1));
      data.push_back(vertex_half_edge(Point(10, 0), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(10, 0), Point(10, 10), -1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(0, 10), 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygonArbitraryFormationP1(stream_type& stdcout) {
      stdcout << "testing polygon formation P1\n";
      polygon_arbitrary_formation pf(true);
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(10, 10), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(0, 10), 1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(10, 20), -1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(10, 20), -1));
      data.push_back(vertex_half_edge(Point(10, 20), Point(10, 10), 1));
      data.push_back(vertex_half_edge(Point(10, 20), Point(0, 10), 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygonArbitraryFormationP2(stream_type& stdcout) {
      stdcout << "testing polygon formation P2\n";
      polygon_arbitrary_formation pf(true);
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(-3, 1), Point(2, -4), 1));
      data.push_back(vertex_half_edge(Point(-3, 1), Point(-2, 2), -1));
      data.push_back(vertex_half_edge(Point(-2, 2), Point(2, 4), -1));
      data.push_back(vertex_half_edge(Point(-2, 2), Point(-3, 1), 1));
      data.push_back(vertex_half_edge(Point(2, -4), Point(-3, 1), -1));
      data.push_back(vertex_half_edge(Point(2, -4), Point(2, 4), -1));
      data.push_back(vertex_half_edge(Point(2, 4), Point(-2, 2), 1));
      data.push_back(vertex_half_edge(Point(2, 4), Point(2, -4), 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }


    template <typename stream_type>
    static inline bool testPolygonArbitraryFormationPolys(stream_type& stdcout) {
      stdcout << "testing polygon formation polys\n";
      polygon_arbitrary_formation pf(false);
      std::vector<polygon_with_holes_data<Unit> > polys;
      polygon_arbitrary_formation pf2(true);
      std::vector<polygon_with_holes_data<Unit> > polys2;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(100, 1), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(1, 100), -1));
      data.push_back(vertex_half_edge(Point(1, 100), Point(0, 0), 1));
      data.push_back(vertex_half_edge(Point(1, 100), Point(101, 101), -1));
      data.push_back(vertex_half_edge(Point(100, 1), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(100, 1), Point(101, 101), 1));
      data.push_back(vertex_half_edge(Point(101, 101), Point(100, 1), -1));
      data.push_back(vertex_half_edge(Point(101, 101), Point(1, 100), 1));

      data.push_back(vertex_half_edge(Point(2, 2), Point(10, 2), -1));
      data.push_back(vertex_half_edge(Point(2, 2), Point(2, 10), -1));
      data.push_back(vertex_half_edge(Point(2, 10), Point(2, 2), 1));
      data.push_back(vertex_half_edge(Point(2, 10), Point(10, 10), 1));
      data.push_back(vertex_half_edge(Point(10, 2), Point(2, 2), 1));
      data.push_back(vertex_half_edge(Point(10, 2), Point(10, 10), 1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(10, 2), -1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(2, 10), -1));

      data.push_back(vertex_half_edge(Point(2, 12), Point(10, 12), -1));
      data.push_back(vertex_half_edge(Point(2, 12), Point(2, 22), -1));
      data.push_back(vertex_half_edge(Point(2, 22), Point(2, 12), 1));
      data.push_back(vertex_half_edge(Point(2, 22), Point(10, 22), 1));
      data.push_back(vertex_half_edge(Point(10, 12), Point(2, 12), 1));
      data.push_back(vertex_half_edge(Point(10, 12), Point(10, 22), 1));
      data.push_back(vertex_half_edge(Point(10, 22), Point(10, 12), -1));
      data.push_back(vertex_half_edge(Point(10, 22), Point(2, 22), -1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      pf2.scan(polys2, data.begin(), data.end());
      stdcout << "result size: " << polys2.size() << "\n";
      for(std::size_t i = 0; i < polys2.size(); ++i) {
        stdcout << polys2[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygonArbitraryFormationSelfTouch1(stream_type& stdcout) {
      stdcout << "testing polygon formation self touch 1\n";
      polygon_arbitrary_formation pf(true);
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(0, 10), 1));

      data.push_back(vertex_half_edge(Point(0, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(5, 10), -1));

      data.push_back(vertex_half_edge(Point(10, 0), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(10, 0), Point(10, 5), -1));

      data.push_back(vertex_half_edge(Point(10, 5), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(10, 5), Point(5, 5), 1));

      data.push_back(vertex_half_edge(Point(5, 10), Point(5, 5), 1));
      data.push_back(vertex_half_edge(Point(5, 10), Point(0, 10), 1));

      data.push_back(vertex_half_edge(Point(5, 2), Point(5, 5), -1));
      data.push_back(vertex_half_edge(Point(5, 2), Point(7, 2), -1));

      data.push_back(vertex_half_edge(Point(5, 5), Point(5, 10), -1));
      data.push_back(vertex_half_edge(Point(5, 5), Point(5, 2), 1));
      data.push_back(vertex_half_edge(Point(5, 5), Point(10, 5), -1));
      data.push_back(vertex_half_edge(Point(5, 5), Point(7, 2), 1));

      data.push_back(vertex_half_edge(Point(7, 2), Point(5, 5), -1));
      data.push_back(vertex_half_edge(Point(7, 2), Point(5, 2), 1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygonArbitraryFormationSelfTouch2(stream_type& stdcout) {
      stdcout << "testing polygon formation self touch 2\n";
      polygon_arbitrary_formation pf(true);
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(0, 10), 1));

      data.push_back(vertex_half_edge(Point(0, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(5, 10), -1));

      data.push_back(vertex_half_edge(Point(10, 0), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(10, 0), Point(10, 5), -1));

      data.push_back(vertex_half_edge(Point(10, 5), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(10, 5), Point(5, 5), 1));

      data.push_back(vertex_half_edge(Point(5, 10), Point(4, 1), -1));
      data.push_back(vertex_half_edge(Point(5, 10), Point(0, 10), 1));

      data.push_back(vertex_half_edge(Point(4, 1), Point(5, 10), 1));
      data.push_back(vertex_half_edge(Point(4, 1), Point(7, 2), -1));

      data.push_back(vertex_half_edge(Point(5, 5), Point(10, 5), -1));
      data.push_back(vertex_half_edge(Point(5, 5), Point(7, 2), 1));

      data.push_back(vertex_half_edge(Point(7, 2), Point(5, 5), -1));
      data.push_back(vertex_half_edge(Point(7, 2), Point(4, 1), 1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygonArbitraryFormationSelfTouch3(stream_type& stdcout) {
      stdcout << "testing polygon formation self touch 3\n";
      polygon_arbitrary_formation pf(true);
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(0, 10), 1));

      data.push_back(vertex_half_edge(Point(0, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(6, 10), -1));

      data.push_back(vertex_half_edge(Point(10, 0), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(10, 0), Point(10, 5), -1));

      data.push_back(vertex_half_edge(Point(10, 5), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(10, 5), Point(5, 5), 1));

      data.push_back(vertex_half_edge(Point(6, 10), Point(4, 1), -1));
      data.push_back(vertex_half_edge(Point(6, 10), Point(0, 10), 1));

      data.push_back(vertex_half_edge(Point(4, 1), Point(6, 10), 1));
      data.push_back(vertex_half_edge(Point(4, 1), Point(7, 2), -1));

      data.push_back(vertex_half_edge(Point(5, 5), Point(10, 5), -1));
      data.push_back(vertex_half_edge(Point(5, 5), Point(7, 2), 1));

      data.push_back(vertex_half_edge(Point(7, 2), Point(5, 5), -1));
      data.push_back(vertex_half_edge(Point(7, 2), Point(4, 1), 1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygonArbitraryFormationColinear(stream_type& stdcout) {
      stdcout << "testing polygon formation colinear 3\n";
      stdcout << "Polygon Set Data { <-3 2, -2 2>:1 <-3 2, -1 4>:-1 <-2 2, 0 2>:1 <-1 4, 0 2>:-1 } \n";
      polygon_arbitrary_formation pf(true);
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(-3, 2), Point(-2, 2), 1));
      data.push_back(vertex_half_edge(Point(-2, 2), Point(-3, 2), -1));

      data.push_back(vertex_half_edge(Point(-3, 2), Point(-1, 4), -1));
      data.push_back(vertex_half_edge(Point(-1, 4), Point(-3, 2), 1));

      data.push_back(vertex_half_edge(Point(-2, 2), Point(0, 2), 1));
      data.push_back(vertex_half_edge(Point(0, 2), Point(-2, 2), -1));

      data.push_back(vertex_half_edge(Point(-1, 4), Point(0, 2), -1));
      data.push_back(vertex_half_edge(Point(0, 2), Point(-1, 4), 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testSegmentIntersection(stream_type& stdcout) {
      stdcout << "testing segment intersection\n";
      half_edge he1, he2;
      he1.first = Point(0, 0);
      he1.second = Point(10, 10);
      he2.first = Point(0, 0);
      he2.second = Point(10, 20);
      Point result;
      bool b = scanline_base<Unit>::compute_intersection(result, he1, he2);
      if(!b || result != Point(0, 0)) return false;
      he1.first = Point(0, 10);
      b = scanline_base<Unit>::compute_intersection(result, he1, he2);
      if(!b || result != Point(5, 10)) return false;
      he1.first = Point(0, 11);
      b = scanline_base<Unit>::compute_intersection(result, he1, he2);
      if(!b || result != Point(5, 10)) return false;
      he1.first = Point(0, 0);
      he1.second = Point(1, 9);
      he2.first = Point(0, 9);
      he2.second = Point(1, 0);
      b = scanline_base<Unit>::compute_intersection(result, he1, he2);
      if(!b || result != Point(0, 4)) return false;

      he1.first = Point(0, -10);
      he1.second = Point(1, -1);
      he2.first = Point(0, -1);
      he2.second = Point(1, -10);
      b = scanline_base<Unit>::compute_intersection(result, he1, he2);
      if(!b || result != Point(0, -5)) return false;
      he1.first = Point((std::numeric_limits<int>::max)(), (std::numeric_limits<int>::max)()-1);
      he1.second = Point((std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)());
      //he1.second = Point(0, (std::numeric_limits<int>::max)());
      he2.first = Point((std::numeric_limits<int>::max)()-1, (std::numeric_limits<int>::max)());
      he2.second = Point((std::numeric_limits<int>::max)(), (std::numeric_limits<int>::min)());
      //he2.second = Point((std::numeric_limits<int>::max)(), 0);
      b = scanline_base<Unit>::compute_intersection(result, he1, he2);
      //b is false because of overflow error
      he1.first = Point(1000, 2000);
      he1.second = Point(1010, 2010);
      he2.first = Point(1000, 2000);
      he2.second = Point(1010, 2020);
      b = scanline_base<Unit>::compute_intersection(result, he1, he2);
      if(!b || result != Point(1000, 2000)) return false;

      return b;
    }

  };

  template <typename Unit>
  class poly_line_arbitrary_hole_data {
  private:
    typedef typename polygon_arbitrary_formation<Unit>::active_tail_arbitrary active_tail_arbitrary;
    active_tail_arbitrary* p_;
  public:
    typedef point_data<Unit> Point;
    typedef Point point_type;
    typedef Unit coordinate_type;
    typedef typename active_tail_arbitrary::iterator iterator_type;
    //typedef iterator_points_to_compact<iterator_type, Point> compact_iterator_type;

    typedef iterator_type iterator;
    inline poly_line_arbitrary_hole_data() : p_(0) {}
    inline poly_line_arbitrary_hole_data(active_tail_arbitrary* p) : p_(p) {}
    //use default copy and assign
    inline iterator begin() const { return p_->getTail()->begin(); }
    inline iterator end() const { return p_->getTail()->end(); }
    inline std::size_t size() const { return 0; }
  };

  template <typename Unit>
  class poly_line_arbitrary_polygon_data {
  private:
    typedef typename polygon_arbitrary_formation<Unit>::active_tail_arbitrary active_tail_arbitrary;
    active_tail_arbitrary* p_;
  public:
    typedef point_data<Unit> Point;
    typedef Point point_type;
    typedef Unit coordinate_type;
    typedef typename active_tail_arbitrary::iterator iterator_type;
    //typedef iterator_points_to_compact<iterator_type, Point> compact_iterator_type;
    typedef typename coordinate_traits<Unit>::coordinate_distance area_type;

    class iterator_holes_type {
    private:
      typedef poly_line_arbitrary_hole_data<Unit> holeType;
      mutable holeType hole_;
      typename active_tail_arbitrary::iteratorHoles itr_;

    public:
      typedef std::forward_iterator_tag iterator_category;
      typedef holeType value_type;
      typedef std::ptrdiff_t difference_type;
      typedef const holeType* pointer; //immutable
      typedef const holeType& reference; //immutable
      inline iterator_holes_type() : hole_(), itr_() {}
      inline iterator_holes_type(typename active_tail_arbitrary::iteratorHoles itr) : hole_(), itr_(itr) {}
      inline iterator_holes_type(const iterator_holes_type& that) : hole_(that.hole_), itr_(that.itr_) {}
      inline iterator_holes_type& operator=(const iterator_holes_type& that) {
        itr_ = that.itr_;
        return *this;
      }
      inline bool operator==(const iterator_holes_type& that) { return itr_ == that.itr_; }
      inline bool operator!=(const iterator_holes_type& that) { return itr_ != that.itr_; }
      inline iterator_holes_type& operator++() {
        ++itr_;
        return *this;
      }
      inline const iterator_holes_type operator++(int) {
        iterator_holes_type tmp = *this;
        ++(*this);
        return tmp;
      }
      inline reference operator*() {
        hole_ = holeType(*itr_);
        return hole_;
      }
    };

    typedef poly_line_arbitrary_hole_data<Unit> hole_type;

    inline poly_line_arbitrary_polygon_data() : p_(0) {}
    inline poly_line_arbitrary_polygon_data(active_tail_arbitrary* p) : p_(p) {}
    //use default copy and assign
    inline iterator_type begin() const { return p_->getTail()->begin(); }
    inline iterator_type end() const { return p_->getTail()->end(); }
    //inline compact_iterator_type begin_compact() const { return p_->getTail()->begin(); }
    //inline compact_iterator_type end_compact() const { return p_->getTail()->end(); }
    inline iterator_holes_type begin_holes() const { return iterator_holes_type(p_->getHoles().begin()); }
    inline iterator_holes_type end_holes() const { return iterator_holes_type(p_->getHoles().end()); }
    inline active_tail_arbitrary* yield() { return p_; }
    //stub out these four required functions that will not be used but are needed for the interface
    inline std::size_t size_holes() const { return 0; }
    inline std::size_t size() const { return 0; }
  };

  template <typename Unit>
  class trapezoid_arbitrary_formation : public polygon_arbitrary_formation<Unit> {
  private:
    typedef typename scanline_base<Unit>::Point Point;
    typedef typename scanline_base<Unit>::half_edge half_edge;
    typedef typename scanline_base<Unit>::vertex_half_edge vertex_half_edge;
    typedef typename scanline_base<Unit>::less_vertex_half_edge less_vertex_half_edge;

    typedef typename polygon_arbitrary_formation<Unit>::poly_line_arbitrary poly_line_arbitrary;

    typedef typename polygon_arbitrary_formation<Unit>::active_tail_arbitrary active_tail_arbitrary;

    typedef std::vector<std::pair<Point, int> > vertex_arbitrary_count;

    typedef typename polygon_arbitrary_formation<Unit>::less_half_edge_count less_half_edge_count;

    typedef std::vector<std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*> > incoming_count;

    typedef typename polygon_arbitrary_formation<Unit>::less_incoming_count less_incoming_count;

    typedef typename polygon_arbitrary_formation<Unit>::vertex_arbitrary_compact vertex_arbitrary_compact;

  private:
    //definitions
    typedef std::map<vertex_half_edge, active_tail_arbitrary*, less_vertex_half_edge> scanline_data;
    typedef typename scanline_data::iterator iterator;
    typedef typename scanline_data::const_iterator const_iterator;

    //data
  public:
    inline trapezoid_arbitrary_formation() : polygon_arbitrary_formation<Unit>() {}
    inline trapezoid_arbitrary_formation(const trapezoid_arbitrary_formation& that) : polygon_arbitrary_formation<Unit>(that) {}
    inline trapezoid_arbitrary_formation& operator=(const trapezoid_arbitrary_formation& that) {
      * static_cast<polygon_arbitrary_formation<Unit>*>(this) = * static_cast<polygon_arbitrary_formation<Unit>*>(&that);
      return *this;
    }

    //cT is an output container of Polygon45 or Polygon45WithHoles
    //iT is an iterator over vertex_half_edge elements
    //inputBegin - inputEnd is a range of sorted iT that represents
    //one or more scanline stops worth of data
    template <class cT, class iT>
    void scan(cT& output, iT inputBegin, iT inputEnd) {
      //std::cout << "1\n";
      while(inputBegin != inputEnd) {
        //std::cout << "2\n";
        polygon_arbitrary_formation<Unit>::x_ = (*inputBegin).pt.get(HORIZONTAL);
        //std::cout << "SCAN FORMATION " << x_ << "\n";
        //std::cout << "x_ = " << x_ << "\n";
        //std::cout << "scan line size: " << scanData_.size() << "\n";
        inputBegin = processEvent_(output, inputBegin, inputEnd);
      }
      //std::cout << "scan line size: " << scanData_.size() << "\n";
    }

  private:
    //functions
    inline void getVerticalPair_(std::pair<active_tail_arbitrary*,
                                 active_tail_arbitrary*>& verticalPair,
                                 iterator previter) {
      active_tail_arbitrary* iterTail = (*previter).second;
      Point prevPoint(polygon_arbitrary_formation<Unit>::x_,
                      convert_high_precision_type<Unit>(previter->first.evalAtX(polygon_arbitrary_formation<Unit>::x_)));
      iterTail->pushPoint(prevPoint);
      std::pair<active_tail_arbitrary*, active_tail_arbitrary*> tailPair =
        active_tail_arbitrary::createActiveTailsAsPair(prevPoint, true, 0, false);
      verticalPair.first = iterTail;
      verticalPair.second = tailPair.first;
      (*previter).second = tailPair.second;
    }

    template <class cT, class cT2>
    inline std::pair<std::pair<Point, int>, active_tail_arbitrary*>
    processPoint_(cT& output, cT2& elements,
                  std::pair<active_tail_arbitrary*, active_tail_arbitrary*>& verticalPair,
                  iterator previter, Point point, incoming_count& counts_from_scanline,
                  vertex_arbitrary_count& incoming_count) {
      //std::cout << "\nAT POINT: " <<  point << "\n";
      //join any closing solid corners
      std::vector<int> counts;
      std::vector<int> incoming;
      std::vector<active_tail_arbitrary*> tails;
      counts.reserve(counts_from_scanline.size());
      tails.reserve(counts_from_scanline.size());
      incoming.reserve(incoming_count.size());
      for(std::size_t i = 0; i < counts_from_scanline.size(); ++i) {
        counts.push_back(counts_from_scanline[i].first.second);
        tails.push_back(counts_from_scanline[i].second);
      }
      for(std::size_t i = 0; i < incoming_count.size(); ++i) {
        incoming.push_back(incoming_count[i].second);
        if(incoming_count[i].first < point) {
          incoming.back() = 0;
        }
      }

      active_tail_arbitrary* returnValue = 0;
      std::pair<active_tail_arbitrary*, active_tail_arbitrary*> verticalPairOut;
      verticalPairOut.first = 0;
      verticalPairOut.second = 0;
      std::pair<Point, int> returnCount(Point(0, 0), 0);
      int i_size_less_1 = (int)(incoming.size()) -1;
      int c_size_less_1 = (int)(counts.size()) -1;
      int i_size = incoming.size();
      int c_size = counts.size();

      bool have_vertical_tail_from_below = false;
      if(c_size &&
         scanline_base<Unit>::is_vertical(counts_from_scanline.back().first.first)) {
        have_vertical_tail_from_below = true;
      }
      //assert size = size_less_1 + 1
      //std::cout << tails.size() << " " << incoming.size() << " " << counts_from_scanline.size() << " " << incoming_count.size() << "\n";
      //         for(std::size_t i = 0; i < counts.size(); ++i) {
      //           std::cout << counts_from_scanline[i].first.first.first.get(HORIZONTAL) << ",";
      //           std::cout << counts_from_scanline[i].first.first.first.get(VERTICAL) << " ";
      //           std::cout << counts_from_scanline[i].first.first.second.get(HORIZONTAL) << ",";
      //           std::cout << counts_from_scanline[i].first.first.second.get(VERTICAL) << ":";
      //           std::cout << counts_from_scanline[i].first.second << " ";
      //         } std::cout << "\n";
      //         print(incoming_count);
      {
        for(int i = 0; i < c_size_less_1; ++i) {
          //std::cout << i << "\n";
          if(counts[i] == -1) {
            //std::cout << "fixed i\n";
            for(int j = i + 1; j < c_size; ++j) {
              //std::cout << j << "\n";
              if(counts[j]) {
                if(counts[j] == 1) {
                  //std::cout << "case1: " << i << " " << j << "\n";
                  //if a figure is closed it will be written out by this function to output
                  active_tail_arbitrary::joinChains(point, tails[i], tails[j], true, output);
                  counts[i] = 0;
                  counts[j] = 0;
                  tails[i] = 0;
                  tails[j] = 0;
                }
                break;
              }
            }
          }
        }
      }
      //find any pairs of incoming edges that need to create pair for leading solid
      //std::cout << "checking case2\n";
      {
        for(int i = 0; i < i_size_less_1; ++i) {
          //std::cout << i << "\n";
          if(incoming[i] == 1) {
            //std::cout << "fixed i\n";
            for(int j = i + 1; j < i_size; ++j) {
              //std::cout << j << "\n";
              if(incoming[j]) {
                //std::cout << incoming[j] << "\n";
                if(incoming[j] == -1) {
                  //std::cout << "case2: " << i << " " << j << "\n";
                  //std::cout << "creating active tail pair\n";
                  std::pair<active_tail_arbitrary*, active_tail_arbitrary*> tailPair =
                    active_tail_arbitrary::createActiveTailsAsPair(point, true, 0, polygon_arbitrary_formation<Unit>::fractureHoles_ != 0);
                  //tailPair.first->print();
                  //tailPair.second->print();
                  if(j == i_size_less_1 && incoming_count[j].first.get(HORIZONTAL) == point.get(HORIZONTAL)) {
                    //vertical active tail becomes return value
                    returnValue = tailPair.first;
                    returnCount.first = point;
                    returnCount.second = 1;
                  } else {
                    //std::cout << "new element " << j-1 << " " << -1 << "\n";
                    //std::cout << point << " " <<  incoming_count[j].first << "\n";
                    elements.push_back(std::pair<vertex_half_edge,
                                       active_tail_arbitrary*>(vertex_half_edge(point,
                                                                                incoming_count[j].first, -1), tailPair.first));
                  }
                  //std::cout << "new element " << i-1 << " " << 1 << "\n";
                  //std::cout << point << " " <<  incoming_count[i].first << "\n";
                  elements.push_back(std::pair<vertex_half_edge,
                                     active_tail_arbitrary*>(vertex_half_edge(point,
                                                                              incoming_count[i].first, 1), tailPair.second));
                  incoming[i] = 0;
                  incoming[j] = 0;
                }
                break;
              }
            }
          }
        }
      }
      //find any active tail that needs to pass through to an incoming edge
      //we expect to find no more than two pass through

      //find pass through with solid on top
      {
        //std::cout << "checking case 3\n";
        for(int i = 0; i < c_size; ++i) {
          //std::cout << i << "\n";
          if(counts[i] != 0) {
            if(counts[i] == 1) {
              //std::cout << "fixed i\n";
              for(int j = i_size_less_1; j >= 0; --j) {
                if(incoming[j] != 0) {
                  if(incoming[j] == 1) {
                    //std::cout << "case3: " << i << " " << j << "\n";
                    //tails[i]->print();
                    //pass through solid on top
                    tails[i]->pushPoint(point);
                    //std::cout << "after push\n";
                    if(j == i_size_less_1 && incoming_count[j].first.get(HORIZONTAL) == point.get(HORIZONTAL)) {
                      returnValue = tails[i];
                      returnCount.first = point;
                      returnCount.second = -1;
                    } else {
                      std::pair<active_tail_arbitrary*, active_tail_arbitrary*> tailPair =
                        active_tail_arbitrary::createActiveTailsAsPair(point, true, 0, false);
                      verticalPairOut.first = tails[i];
                      verticalPairOut.second = tailPair.first;
                      elements.push_back(std::pair<vertex_half_edge,
                                         active_tail_arbitrary*>(vertex_half_edge(point,
                                                                                  incoming_count[j].first, incoming[j]), tailPair.second));
                    }
                    tails[i] = 0;
                    counts[i] = 0;
                    incoming[j] = 0;
                  }
                  break;
                }
              }
            }
            break;
          }
        }
      }
      //std::cout << "checking case 4\n";
      //find pass through with solid on bottom
      {
        for(int i = c_size_less_1; i >= 0; --i) {
          //std::cout << "i = " << i << " with count " << counts[i] << "\n";
          if(counts[i] != 0) {
            if(counts[i] == -1) {
              for(int j = 0; j < i_size; ++j) {
                if(incoming[j] != 0) {
                  if(incoming[j] == -1) {
                    //std::cout << "case4: " << i << " " << j << "\n";
                    //pass through solid on bottom

                    //if count from scanline is vertical
                    if(i == c_size_less_1 &&
                       counts_from_scanline[i].first.first.first.get(HORIZONTAL) ==
                       point.get(HORIZONTAL)) {
                       //if incoming count is vertical
                       if(j == i_size_less_1 &&
                          incoming_count[j].first.get(HORIZONTAL) == point.get(HORIZONTAL)) {
                         returnValue = tails[i];
                         returnCount.first = point;
                         returnCount.second = 1;
                       } else {
                         tails[i]->pushPoint(point);
                         elements.push_back(std::pair<vertex_half_edge,
                                         active_tail_arbitrary*>(vertex_half_edge(point,
                                                                                  incoming_count[j].first, incoming[j]), tails[i]));
                       }
                    } else if(j == i_size_less_1 &&
                              incoming_count[j].first.get(HORIZONTAL) ==
                              point.get(HORIZONTAL)) {
                      if(verticalPair.first == 0) {
                        getVerticalPair_(verticalPair, previter);
                      }
                      active_tail_arbitrary::joinChains(point, tails[i], verticalPair.first, true, output);
                      returnValue = verticalPair.second;
                      returnCount.first = point;
                      returnCount.second = 1;
                    } else {
                      //neither is vertical
                      if(verticalPair.first == 0) {
                        getVerticalPair_(verticalPair, previter);
                      }
                      active_tail_arbitrary::joinChains(point, tails[i], verticalPair.first, true, output);
                      verticalPair.second->pushPoint(point);
                      elements.push_back(std::pair<vertex_half_edge,
                                         active_tail_arbitrary*>(vertex_half_edge(point,
                                                                                  incoming_count[j].first, incoming[j]), verticalPair.second));
                    }
                    tails[i] = 0;
                    counts[i] = 0;
                    incoming[j] = 0;
                  }
                  break;
                }
              }
            }
            break;
          }
        }
      }
      //find the end of a hole or the beginning of a hole

      //find end of a hole
      {
        for(int i = 0; i < c_size_less_1; ++i) {
          if(counts[i] != 0) {
            for(int j = i+1; j < c_size; ++j) {
              if(counts[j] != 0) {
                //std::cout << "case5: " << i << " " << j << "\n";
                //we are ending a hole and may potentially close a figure and have to handle the hole
                tails[i]->pushPoint(point);
                verticalPairOut.first = tails[i];
                if(j == c_size_less_1 &&
                   counts_from_scanline[j].first.first.first.get(HORIZONTAL) ==
                   point.get(HORIZONTAL)) {
                  verticalPairOut.second = tails[j];
                } else {
                  //need to close a trapezoid below
                  if(verticalPair.first == 0) {
                    getVerticalPair_(verticalPair, previter);
                  }
                  active_tail_arbitrary::joinChains(point, tails[j], verticalPair.first, true, output);
                  verticalPairOut.second = verticalPair.second;
                }
                tails[i] = 0;
                tails[j] = 0;
                counts[i] = 0;
                counts[j] = 0;
                break;
              }
            }
            break;
          }
        }
      }
      //find beginning of a hole
      {
        for(int i = 0; i < i_size_less_1; ++i) {
          if(incoming[i] != 0) {
            for(int j = i+1; j < i_size; ++j) {
              if(incoming[j] != 0) {
                //std::cout << "case6: " << i << " " << j << "\n";
                //we are beginning a empty space
                if(verticalPair.first == 0) {
                  getVerticalPair_(verticalPair, previter);
                }
                verticalPair.second->pushPoint(point);
                if(j == i_size_less_1 &&
                   incoming_count[j].first.get(HORIZONTAL) == point.get(HORIZONTAL)) {
                  returnValue = verticalPair.first;
                  returnCount.first = point;
                  returnCount.second = -1;
                } else {
                  std::pair<active_tail_arbitrary*, active_tail_arbitrary*> tailPair =
                  active_tail_arbitrary::createActiveTailsAsPair(point, false, 0, false);
                  elements.push_back(std::pair<vertex_half_edge,
                                     active_tail_arbitrary*>(vertex_half_edge(point,
                                                                              incoming_count[j].first, incoming[j]), tailPair.second));
                  verticalPairOut.second = tailPair.first;
                  verticalPairOut.first = verticalPair.first;
                }
                elements.push_back(std::pair<vertex_half_edge,
                                   active_tail_arbitrary*>(vertex_half_edge(point,
                                                                            incoming_count[i].first, incoming[i]), verticalPair.second));
                incoming[i] = 0;
                incoming[j] = 0;
                break;
              }
            }
            break;
          }
        }
      }
      if(have_vertical_tail_from_below) {
        if(tails.back()) {
          tails.back()->pushPoint(point);
          returnValue = tails.back();
          returnCount.first = point;
          returnCount.second = counts.back();
        }
      }
      verticalPair = verticalPairOut;
      //assert that tails, counts and incoming are all null
      return std::pair<std::pair<Point, int>, active_tail_arbitrary*>(returnCount, returnValue);
    }

    static inline void print(const vertex_arbitrary_count& count) {
      for(unsigned i = 0; i < count.size(); ++i) {
        //std::cout << count[i].first.get(HORIZONTAL) << ",";
        //std::cout << count[i].first.get(VERTICAL) << ":";
        //std::cout << count[i].second << " ";
      } //std::cout << "\n";
    }

    static inline void print(const scanline_data& data) {
      for(typename scanline_data::const_iterator itr = data.begin(); itr != data.end(); ++itr){
        //std::cout << itr->first.pt << ", " << itr->first.other_pt << "; ";
      } //std::cout << "\n";
    }

    template <class cT, class iT>
    inline iT processEvent_(cT& output, iT inputBegin, iT inputEnd) {
      //typedef typename high_precision_type<Unit>::type high_precision;
      //std::cout << "processEvent_\n";
      polygon_arbitrary_formation<Unit>::justBefore_ = true;
      //collect up all elements from the tree that are at the y
      //values of events in the input queue
      //create vector of new elements to add into tree
      active_tail_arbitrary* verticalTail = 0;
      std::pair<active_tail_arbitrary*, active_tail_arbitrary*> verticalPair;
      std::pair<Point, int> verticalCount(Point(0, 0), 0);
      iT currentIter = inputBegin;
      std::vector<iterator> elementIters;
      std::vector<std::pair<vertex_half_edge, active_tail_arbitrary*> > elements;
      while(currentIter != inputEnd && currentIter->pt.get(HORIZONTAL) == polygon_arbitrary_formation<Unit>::x_) {
        //std::cout << "loop\n";
        Unit currentY = (*currentIter).pt.get(VERTICAL);
        //std::cout << "current Y " << currentY << "\n";
        //std::cout << "scanline size " << scanData_.size() << "\n";
        //print(scanData_);
        iterator iter = this->lookUp_(currentY);
        //std::cout << "found element in scanline " << (iter != scanData_.end()) << "\n";
        //int counts[4] = {0, 0, 0, 0};
        incoming_count counts_from_scanline;
        //std::cout << "finding elements in tree\n";
        //if(iter != scanData_.end())
        //  std::cout << "first iter y is " << iter->first.evalAtX(x_) << "\n";
        iterator previter = iter;
        if(previter != polygon_arbitrary_formation<Unit>::scanData_.end() &&
             previter->first.evalAtX(polygon_arbitrary_formation<Unit>::x_) >= currentY &&
             previter != polygon_arbitrary_formation<Unit>::scanData_.begin())
           --previter;
        while(iter != polygon_arbitrary_formation<Unit>::scanData_.end() &&
              ((iter->first.pt.x() == polygon_arbitrary_formation<Unit>::x_ && iter->first.pt.y() == currentY) ||
               (iter->first.other_pt.x() == polygon_arbitrary_formation<Unit>::x_ && iter->first.other_pt.y() == currentY))) {
               //iter->first.evalAtX(polygon_arbitrary_formation<Unit>::x_) == (high_precision)currentY) {
          //std::cout << "loop2\n";
          elementIters.push_back(iter);
          counts_from_scanline.push_back(std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*>
                                         (std::pair<std::pair<Point, Point>, int>(std::pair<Point, Point>(iter->first.pt,
                                                                                                          iter->first.other_pt),
                                                                                  iter->first.count),
                                          iter->second));
          ++iter;
        }
        Point currentPoint(polygon_arbitrary_formation<Unit>::x_, currentY);
        //std::cout << "counts_from_scanline size " << counts_from_scanline.size() << "\n";
        this->sort_incoming_count(counts_from_scanline, currentPoint);

        vertex_arbitrary_count incoming;
        //std::cout << "aggregating\n";
        do {
          //std::cout << "loop3\n";
          const vertex_half_edge& elem = *currentIter;
          incoming.push_back(std::pair<Point, int>(elem.other_pt, elem.count));
          ++currentIter;
        } while(currentIter != inputEnd && currentIter->pt.get(VERTICAL) == currentY &&
                currentIter->pt.get(HORIZONTAL) == polygon_arbitrary_formation<Unit>::x_);
        //print(incoming);
        this->sort_vertex_arbitrary_count(incoming, currentPoint);
        //std::cout << currentPoint.get(HORIZONTAL) << "," << currentPoint.get(VERTICAL) << "\n";
        //print(incoming);
        //std::cout << "incoming counts from input size " << incoming.size() << "\n";
        //compact_vertex_arbitrary_count(currentPoint, incoming);
        vertex_arbitrary_count tmp;
        tmp.reserve(incoming.size());
        for(std::size_t i = 0; i < incoming.size(); ++i) {
          if(currentPoint < incoming[i].first) {
            tmp.push_back(incoming[i]);
          }
        }
        incoming.swap(tmp);
        //std::cout << "incoming counts from input size " << incoming.size() << "\n";
        //now counts_from_scanline has the data from the left and
        //incoming has the data from the right at this point
        //cancel out any end points
        if(verticalTail) {
          //std::cout << "adding vertical tail to counts from scanline\n";
          //std::cout << -verticalCount.second << "\n";
          counts_from_scanline.push_back(std::pair<std::pair<std::pair<Point, Point>, int>, active_tail_arbitrary*>
                                         (std::pair<std::pair<Point, Point>, int>(std::pair<Point, Point>(verticalCount.first,
                                                                                                          currentPoint),
                                                                                  -verticalCount.second),
                                          verticalTail));
        }
        if(!incoming.empty() && incoming.back().first.get(HORIZONTAL) == polygon_arbitrary_formation<Unit>::x_) {
          //std::cout << "inverted vertical event\n";
          incoming.back().second *= -1;
        }
        //std::cout << "calling processPoint_\n";
           std::pair<std::pair<Point, int>, active_tail_arbitrary*> result = processPoint_(output, elements, verticalPair, previter, Point(polygon_arbitrary_formation<Unit>::x_, currentY), counts_from_scanline, incoming);
        verticalCount = result.first;
        verticalTail = result.second;
        if(verticalPair.first != 0 && iter != polygon_arbitrary_formation<Unit>::scanData_.end() &&
           (currentIter == inputEnd || currentIter->pt.x() != polygon_arbitrary_formation<Unit>::x_ ||
            currentIter->pt.y() > (*iter).first.evalAtX(polygon_arbitrary_formation<Unit>::x_))) {
          //splice vertical pair into edge above
          active_tail_arbitrary* tailabove = (*iter).second;
          Point point(polygon_arbitrary_formation<Unit>::x_,
                      convert_high_precision_type<Unit>((*iter).first.evalAtX(polygon_arbitrary_formation<Unit>::x_)));
          verticalPair.second->pushPoint(point);
          active_tail_arbitrary::joinChains(point, tailabove, verticalPair.first, true, output);
          (*iter).second = verticalPair.second;
          verticalPair.first = 0;
          verticalPair.second = 0;
        }
      }
      //std::cout << "erasing\n";
      //erase all elements from the tree
      for(typename std::vector<iterator>::iterator iter = elementIters.begin();
          iter != elementIters.end(); ++iter) {
        //std::cout << "erasing loop\n";
        polygon_arbitrary_formation<Unit>::scanData_.erase(*iter);
      }
      //switch comparison tie breaking policy
      polygon_arbitrary_formation<Unit>::justBefore_ = false;
      //add new elements into tree
      //std::cout << "inserting\n";
      for(typename std::vector<std::pair<vertex_half_edge, active_tail_arbitrary*> >::iterator iter = elements.begin();
          iter != elements.end(); ++iter) {
        //std::cout << "inserting loop\n";
        polygon_arbitrary_formation<Unit>::scanData_.insert(polygon_arbitrary_formation<Unit>::scanData_.end(), *iter);
      }
      //std::cout << "end processEvent\n";
      return currentIter;
    }
  public:
    template <typename stream_type>
    static inline bool testTrapezoidArbitraryFormationRect(stream_type& stdcout) {
      stdcout << "testing trapezoid formation\n";
      trapezoid_arbitrary_formation pf;
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(0, 10), 1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(10, 10), -1));
      data.push_back(vertex_half_edge(Point(10, 0), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(10, 0), Point(10, 10), -1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(0, 10), 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing trapezoid formation\n";
      return true;
    }
    template <typename stream_type>
    static inline bool testTrapezoidArbitraryFormationP1(stream_type& stdcout) {
      stdcout << "testing trapezoid formation P1\n";
      trapezoid_arbitrary_formation pf;
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(10, 10), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(0, 10), 1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(10, 20), -1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(10, 20), -1));
      data.push_back(vertex_half_edge(Point(10, 20), Point(10, 10), 1));
      data.push_back(vertex_half_edge(Point(10, 20), Point(0, 10), 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing trapezoid formation\n";
      return true;
    }
    template <typename stream_type>
    static inline bool testTrapezoidArbitraryFormationP2(stream_type& stdcout) {
      stdcout << "testing trapezoid formation P2\n";
      trapezoid_arbitrary_formation pf;
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(-3, 1), Point(2, -4), 1));
      data.push_back(vertex_half_edge(Point(-3, 1), Point(-2, 2), -1));
      data.push_back(vertex_half_edge(Point(-2, 2), Point(2, 4), -1));
      data.push_back(vertex_half_edge(Point(-2, 2), Point(-3, 1), 1));
      data.push_back(vertex_half_edge(Point(2, -4), Point(-3, 1), -1));
      data.push_back(vertex_half_edge(Point(2, -4), Point(2, 4), -1));
      data.push_back(vertex_half_edge(Point(2, 4), Point(-2, 2), 1));
      data.push_back(vertex_half_edge(Point(2, 4), Point(2, -4), 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing trapezoid formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testTrapezoidArbitraryFormationPolys(stream_type& stdcout) {
      stdcout << "testing trapezoid formation polys\n";
      trapezoid_arbitrary_formation pf;
      std::vector<polygon_with_holes_data<Unit> > polys;
      //trapezoid_arbitrary_formation pf2(true);
      //std::vector<polygon_with_holes_data<Unit> > polys2;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(100, 1), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(1, 100), -1));
      data.push_back(vertex_half_edge(Point(1, 100), Point(0, 0), 1));
      data.push_back(vertex_half_edge(Point(1, 100), Point(101, 101), -1));
      data.push_back(vertex_half_edge(Point(100, 1), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(100, 1), Point(101, 101), 1));
      data.push_back(vertex_half_edge(Point(101, 101), Point(100, 1), -1));
      data.push_back(vertex_half_edge(Point(101, 101), Point(1, 100), 1));

      data.push_back(vertex_half_edge(Point(2, 2), Point(10, 2), -1));
      data.push_back(vertex_half_edge(Point(2, 2), Point(2, 10), -1));
      data.push_back(vertex_half_edge(Point(2, 10), Point(2, 2), 1));
      data.push_back(vertex_half_edge(Point(2, 10), Point(10, 10), 1));
      data.push_back(vertex_half_edge(Point(10, 2), Point(2, 2), 1));
      data.push_back(vertex_half_edge(Point(10, 2), Point(10, 10), 1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(10, 2), -1));
      data.push_back(vertex_half_edge(Point(10, 10), Point(2, 10), -1));

      data.push_back(vertex_half_edge(Point(2, 12), Point(10, 12), -1));
      data.push_back(vertex_half_edge(Point(2, 12), Point(2, 22), -1));
      data.push_back(vertex_half_edge(Point(2, 22), Point(2, 12), 1));
      data.push_back(vertex_half_edge(Point(2, 22), Point(10, 22), 1));
      data.push_back(vertex_half_edge(Point(10, 12), Point(2, 12), 1));
      data.push_back(vertex_half_edge(Point(10, 12), Point(10, 22), 1));
      data.push_back(vertex_half_edge(Point(10, 22), Point(10, 12), -1));
      data.push_back(vertex_half_edge(Point(10, 22), Point(2, 22), -1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      //pf2.scan(polys2, data.begin(), data.end());
      //stdcout << "result size: " << polys2.size() << "\n";
      //for(std::size_t i = 0; i < polys2.size(); ++i) {
      //  stdcout << polys2[i] << "\n";
      //}
      stdcout << "done testing trapezoid formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testTrapezoidArbitraryFormationSelfTouch1(stream_type& stdcout) {
      stdcout << "testing trapezoid formation self touch 1\n";
      trapezoid_arbitrary_formation pf;
      std::vector<polygon_data<Unit> > polys;
      std::vector<vertex_half_edge> data;
      data.push_back(vertex_half_edge(Point(0, 0), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(0, 0), Point(0, 10), 1));

      data.push_back(vertex_half_edge(Point(0, 10), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(0, 10), Point(5, 10), -1));

      data.push_back(vertex_half_edge(Point(10, 0), Point(0, 0), -1));
      data.push_back(vertex_half_edge(Point(10, 0), Point(10, 5), -1));

      data.push_back(vertex_half_edge(Point(10, 5), Point(10, 0), 1));
      data.push_back(vertex_half_edge(Point(10, 5), Point(5, 5), 1));

      data.push_back(vertex_half_edge(Point(5, 10), Point(5, 5), 1));
      data.push_back(vertex_half_edge(Point(5, 10), Point(0, 10), 1));

      data.push_back(vertex_half_edge(Point(5, 2), Point(5, 5), -1));
      data.push_back(vertex_half_edge(Point(5, 2), Point(7, 2), -1));

      data.push_back(vertex_half_edge(Point(5, 5), Point(5, 10), -1));
      data.push_back(vertex_half_edge(Point(5, 5), Point(5, 2), 1));
      data.push_back(vertex_half_edge(Point(5, 5), Point(10, 5), -1));
      data.push_back(vertex_half_edge(Point(5, 5), Point(7, 2), 1));

      data.push_back(vertex_half_edge(Point(7, 2), Point(5, 5), -1));
      data.push_back(vertex_half_edge(Point(7, 2), Point(5, 2), 1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing trapezoid formation\n";
      return true;
    }
  };

  template <typename T>
  struct PolyLineArbitraryByConcept<T, polygon_with_holes_concept> { typedef poly_line_arbitrary_polygon_data<T> type; };
  template <typename T>
  struct PolyLineArbitraryByConcept<T, polygon_concept> { typedef poly_line_arbitrary_hole_data<T> type; };

  template <typename T>
  struct geometry_concept<poly_line_arbitrary_polygon_data<T> > { typedef polygon_45_with_holes_concept type; };
  template <typename T>
  struct geometry_concept<poly_line_arbitrary_hole_data<T> > { typedef polygon_45_concept type; };
}
}
#endif

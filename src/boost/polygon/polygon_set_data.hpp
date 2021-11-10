/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_SET_DATA_HPP
#define BOOST_POLYGON_POLYGON_SET_DATA_HPP
#include "polygon_45_set_data.hpp"
#include "polygon_45_set_concept.hpp"
#include "polygon_traits.hpp"
#include "detail/polygon_arbitrary_formation.hpp"

namespace boost { namespace polygon {


  // utility function to round coordinate types down
  // rounds down for both negative and positive numbers
  // intended really for integer type T (does not make sense for float)
  template <typename T>
  static inline T round_down(double val) {
     T rounded_val = (T)(val);
     if(val < (double)rounded_val)
        --rounded_val;
     return rounded_val;
  }
  template <typename T>
  static inline point_data<T> round_down(point_data<double> v) {
     return point_data<T>(round_down<T>(v.x()),round_down<T>(v.y()));
  }



  //foward declare view
  template <typename ltype, typename rtype, int op_type> class polygon_set_view;

  template <typename T>
  class polygon_set_data {
  public:
    typedef T coordinate_type;
    typedef point_data<T> point_type;
    typedef std::pair<point_type, point_type> edge_type;
    typedef std::pair<edge_type, int> element_type;
    typedef std::vector<element_type> value_type;
    typedef typename value_type::const_iterator iterator_type;
    typedef polygon_set_data operator_arg_type;

    // default constructor
    inline polygon_set_data() : data_(), dirty_(false), unsorted_(false), is_45_(true) {}

    // constructor from an iterator pair over edge data
    template <typename iT>
    inline polygon_set_data(iT input_begin, iT input_end) : data_(), dirty_(false), unsorted_(false), is_45_(true) {
      for( ; input_begin != input_end; ++input_begin) { insert(*input_begin); }
    }

    // copy constructor
    inline polygon_set_data(const polygon_set_data& that) :
      data_(that.data_), dirty_(that.dirty_), unsorted_(that.unsorted_), is_45_(that.is_45_) {}

    // copy constructor
    template <typename ltype, typename rtype, int op_type>
    inline polygon_set_data(const polygon_set_view<ltype, rtype, op_type>& that);

    // destructor
    inline ~polygon_set_data() {}

    // assignement operator
    inline polygon_set_data& operator=(const polygon_set_data& that) {
      if(this == &that) return *this;
      data_ = that.data_;
      dirty_ = that.dirty_;
      unsorted_ = that.unsorted_;
      is_45_ = that.is_45_;
      return *this;
    }

    template <typename ltype, typename rtype, int op_type>
    inline polygon_set_data& operator=(const polygon_set_view<ltype, rtype, op_type>& geometry) {
      (*this) = geometry.value();
      dirty_ = false;
      unsorted_ = false;
      return *this;
    }

    template <typename geometry_object>
    inline polygon_set_data& operator=(const geometry_object& geometry) {
      data_.clear();
      insert(geometry);
      return *this;
    }


    // insert iterator range
    inline void insert(iterator_type input_begin, iterator_type input_end, bool is_hole = false) {
      if(input_begin == input_end || (!data_.empty() && &(*input_begin) == &(*(data_.begin())))) return;
      dirty_ = true;
      unsorted_ = true;
      while(input_begin != input_end) {
        insert(*input_begin, is_hole);
        ++input_begin;
      }
    }

    // insert iterator range
    template <typename iT>
    inline void insert(iT input_begin, iT input_end, bool is_hole = false) {
      if(input_begin == input_end) return;
      for(; input_begin != input_end; ++input_begin) {
        insert(*input_begin, is_hole);
      }
    }

    template <typename geometry_type>
    inline void insert(const geometry_type& geometry_object, bool is_hole = false) {
      insert(geometry_object, is_hole, typename geometry_concept<geometry_type>::type());
    }

    template <typename polygon_type>
    inline void insert(const polygon_type& polygon_object, bool is_hole, polygon_concept ) {
      insert_vertex_sequence(begin_points(polygon_object), end_points(polygon_object), winding(polygon_object), is_hole);
    }

    inline void insert(const polygon_set_data& ps, bool is_hole = false) {
      insert(ps.data_.begin(), ps.data_.end(), is_hole);
    }

    template <typename polygon_45_set_type>
    inline void insert(const polygon_45_set_type& ps, bool is_hole, polygon_45_set_concept) {
      std::vector<polygon_45_with_holes_data<typename polygon_45_set_traits<polygon_45_set_type>::coordinate_type> > polys;
      assign(polys, ps);
      insert(polys.begin(), polys.end(), is_hole);
    }

    template <typename polygon_90_set_type>
    inline void insert(const polygon_90_set_type& ps, bool is_hole, polygon_90_set_concept) {
      std::vector<polygon_90_with_holes_data<typename polygon_90_set_traits<polygon_90_set_type>::coordinate_type> > polys;
      assign(polys, ps);
      insert(polys.begin(), polys.end(), is_hole);
    }

    template <typename polygon_type>
    inline void insert(const polygon_type& polygon_object, bool is_hole, polygon_45_concept ) {
      insert(polygon_object, is_hole, polygon_concept()); }

    template <typename polygon_type>
    inline void insert(const polygon_type& polygon_object, bool is_hole, polygon_90_concept ) {
      insert(polygon_object, is_hole, polygon_concept()); }

    template <typename polygon_with_holes_type>
    inline void insert(const polygon_with_holes_type& polygon_with_holes_object, bool is_hole,
                       polygon_with_holes_concept ) {
      insert(polygon_with_holes_object, is_hole, polygon_concept());
      for(typename polygon_with_holes_traits<polygon_with_holes_type>::iterator_holes_type itr =
            begin_holes(polygon_with_holes_object);
          itr != end_holes(polygon_with_holes_object); ++itr) {
        insert(*itr, !is_hole, polygon_concept());
      }
    }

    template <typename polygon_with_holes_type>
    inline void insert(const polygon_with_holes_type& polygon_with_holes_object, bool is_hole,
                       polygon_45_with_holes_concept ) {
      insert(polygon_with_holes_object, is_hole, polygon_with_holes_concept()); }

    template <typename polygon_with_holes_type>
    inline void insert(const polygon_with_holes_type& polygon_with_holes_object, bool is_hole,
                       polygon_90_with_holes_concept ) {
      insert(polygon_with_holes_object, is_hole, polygon_with_holes_concept()); }

    template <typename rectangle_type>
    inline void insert(const rectangle_type& rectangle_object, bool is_hole, rectangle_concept ) {
      polygon_90_data<coordinate_type> poly;
      assign(poly, rectangle_object);
      insert(poly, is_hole, polygon_concept());
    }

    inline void insert_clean(const element_type& edge, bool is_hole = false) {
      if( ! scanline_base<coordinate_type>::is_45_degree(edge.first) &&
          ! scanline_base<coordinate_type>::is_horizontal(edge.first) &&
          ! scanline_base<coordinate_type>::is_vertical(edge.first) ) is_45_ = false;
      data_.push_back(edge);
      if(data_.back().first.second < data_.back().first.first) {
        std::swap(data_.back().first.second, data_.back().first.first);
        data_.back().second *= -1;
      }
      if(is_hole)
        data_.back().second *= -1;
    }

    inline void insert(const element_type& edge, bool is_hole = false) {
      insert_clean(edge, is_hole);
      dirty_ = true;
      unsorted_ = true;
    }

    template <class iT>
    inline void insert_vertex_sequence(iT begin_vertex, iT end_vertex, direction_1d winding, bool is_hole) {
      if (begin_vertex == end_vertex) {
        // No edges to insert.
        return;
      }
      // Current edge endpoints.
      iT vertex0 = begin_vertex;
      iT vertex1 = begin_vertex;
      if (++vertex1 == end_vertex) {
        // No edges to insert.
        return;
      }
      int wmultiplier = (winding == COUNTERCLOCKWISE) ? 1 : -1;
      if (is_hole) {
        wmultiplier = -wmultiplier;
      }
      dirty_ = true;
      unsorted_ = true;
      while (vertex0 != end_vertex) {
        point_type p0, p1;
        assign(p0, *vertex0);
        assign(p1, *vertex1);
        if (p0 != p1) {
          int hmultiplier = (p0.get(HORIZONTAL) == p1.get(HORIZONTAL)) ? -1 : 1;
          element_type elem(edge_type(p0, p1), hmultiplier * wmultiplier);
          insert_clean(elem);
        }
        ++vertex0;
        ++vertex1;
        if (vertex1 == end_vertex) {
          vertex1 = begin_vertex;
        }
      }
    }

    template <typename output_container>
    inline void get(output_container& output) const {
      get_dispatch(output, typename geometry_concept<typename output_container::value_type>::type());
    }

    // append to the container cT with polygons of three or four verticies
    // slicing orientation is vertical
    template <class cT>
    void get_trapezoids(cT& container) const {
      clean();
      trapezoid_arbitrary_formation<coordinate_type> pf;
      typedef typename polygon_arbitrary_formation<coordinate_type>::vertex_half_edge vertex_half_edge;
      std::vector<vertex_half_edge> data;
      for(iterator_type itr = data_.begin(); itr != data_.end(); ++itr){
        data.push_back(vertex_half_edge((*itr).first.first, (*itr).first.second, (*itr).second));
        data.push_back(vertex_half_edge((*itr).first.second, (*itr).first.first, -1 * (*itr).second));
      }
      polygon_sort(data.begin(), data.end());
      pf.scan(container, data.begin(), data.end());
      //std::cout << "DONE FORMING POLYGONS\n";
    }

    // append to the container cT with polygons of three or four verticies
    template <class cT>
    void get_trapezoids(cT& container, orientation_2d slicing_orientation) const {
      if(slicing_orientation == VERTICAL) {
        get_trapezoids(container);
      } else {
        polygon_set_data<T> ps(*this);
        ps.transform(axis_transformation(axis_transformation::SWAP_XY));
        cT result;
        ps.get_trapezoids(result);
        for(typename cT::iterator itr = result.begin(); itr != result.end(); ++itr) {
          ::boost::polygon::transform(*itr, axis_transformation(axis_transformation::SWAP_XY));
        }
        container.insert(container.end(), result.begin(), result.end());
      }
    }

    // equivalence operator
    inline bool operator==(const polygon_set_data& p) const;

    // inequivalence operator
    inline bool operator!=(const polygon_set_data& p) const {
      return !((*this) == p);
    }

    // get iterator to begin vertex data
    inline iterator_type begin() const {
      return data_.begin();
    }

    // get iterator to end vertex data
    inline iterator_type end() const {
      return data_.end();
    }

    const value_type& value() const {
      return data_;
    }

    // clear the contents of the polygon_set_data
    inline void clear() { data_.clear(); dirty_ = unsorted_ = false; }

    // find out if Polygon set is empty
    inline bool empty() const { return data_.empty(); }

    // get the Polygon set size in vertices
    inline std::size_t size() const { clean(); return data_.size(); }

    // get the current Polygon set capacity in vertices
    inline std::size_t capacity() const { return data_.capacity(); }

    // reserve size of polygon set in vertices
    inline void reserve(std::size_t size) { return data_.reserve(size); }

    // find out if Polygon set is sorted
    inline bool sorted() const { return !unsorted_; }

    // find out if Polygon set is clean
    inline bool dirty() const { return dirty_; }

    void clean() const;

    void sort() const{
      if(unsorted_) {
        polygon_sort(data_.begin(), data_.end());
        unsorted_ = false;
      }
    }

    template <typename input_iterator_type>
    void set(input_iterator_type input_begin, input_iterator_type input_end) {
      clear();
      reserve(std::distance(input_begin,input_end));
      insert(input_begin, input_end);
      dirty_ = true;
      unsorted_ = true;
    }

    void set(const value_type& value) {
      data_ = value;
      dirty_ = true;
      unsorted_ = true;
    }

    template <typename rectangle_type>
    bool extents(rectangle_type& rect) {
      clean();
      if(empty()) return false;
      bool first_iteration = true;
      for(iterator_type itr = begin();
          itr != end(); ++itr) {
        rectangle_type edge_box;
        set_points(edge_box, (*itr).first.first, (*itr).first.second);
        if(first_iteration)
          rect = edge_box;
        else
          encompass(rect, edge_box);
        first_iteration = false;
      }
      return true;
    }

    inline polygon_set_data&
    resize(coordinate_type resizing, bool corner_fill_arc = false, unsigned int num_circle_segments=0);

    template <typename transform_type>
    inline polygon_set_data&
    transform(const transform_type& tr) {
      std::vector<polygon_with_holes_data<T> > polys;
      get(polys);
      clear();
      for(std::size_t i = 0 ; i < polys.size(); ++i) {
        ::boost::polygon::transform(polys[i], tr);
        insert(polys[i]);
      }
      unsorted_ = true;
      dirty_ = true;
      return *this;
    }

    inline polygon_set_data&
    scale_up(typename coordinate_traits<coordinate_type>::unsigned_area_type factor) {
      for(typename value_type::iterator itr = data_.begin(); itr != data_.end(); ++itr) {
        ::boost::polygon::scale_up((*itr).first.first, factor);
        ::boost::polygon::scale_up((*itr).first.second, factor);
      }
      return *this;
    }

    inline polygon_set_data&
    scale_down(typename coordinate_traits<coordinate_type>::unsigned_area_type factor) {
      for(typename value_type::iterator itr = data_.begin(); itr != data_.end(); ++itr) {
        bool vb = (*itr).first.first.x() == (*itr).first.second.x();
        ::boost::polygon::scale_down((*itr).first.first, factor);
        ::boost::polygon::scale_down((*itr).first.second, factor);
        bool va = (*itr).first.first.x() == (*itr).first.second.x();
        if(!vb && va) {
          (*itr).second *= -1;
        }
      }
      unsorted_ = true;
      dirty_ = true;
      return *this;
    }

    template <typename scaling_type>
    inline polygon_set_data& scale(polygon_set_data&,
                                   const scaling_type& scaling) {
      for(typename value_type::iterator itr = begin(); itr != end(); ++itr) {
        bool vb = (*itr).first.first.x() == (*itr).first.second.x();
        ::boost::polygon::scale((*itr).first.first, scaling);
        ::boost::polygon::scale((*itr).first.second, scaling);
        bool va = (*itr).first.first.x() == (*itr).first.second.x();
        if(!vb && va) {
          (*itr).second *= -1;
        }
      }
      unsorted_ = true;
      dirty_ = true;
      return *this;
    }

    static inline void compute_offset_edge(point_data<long double>& pt1, point_data<long double>& pt2,
                                           const point_data<long double>&  prev_pt,
                                           const point_data<long double>&  current_pt,
                                           long double distance, int multiplier) {
      long double dx = current_pt.x() - prev_pt.x();
      long double dy = current_pt.y() - prev_pt.y();
      long double edge_length = std::sqrt(dx*dx + dy*dy);
      long double dnx = dy;
      long double dny = -dx;
      dnx = dnx * (long double)distance / edge_length;
      dny = dny * (long double)distance / edge_length;
      pt1.x(prev_pt.x() + (long double)dnx * (long double)multiplier);
      pt2.x(current_pt.x() + (long double)dnx * (long double)multiplier);
      pt1.y(prev_pt.y() + (long double)dny * (long double)multiplier);
      pt2.y(current_pt.y() + (long double)dny * (long double)multiplier);
    }

    static inline void modify_pt(point_data<coordinate_type>& pt, const point_data<coordinate_type>&  prev_pt,
                                 const point_data<coordinate_type>&  current_pt,  const point_data<coordinate_type>&  next_pt,
                                 coordinate_type distance, coordinate_type multiplier) {
      std::pair<point_data<long double>, point_data<long double> > he1, he2;
      he1.first.x((long double)(prev_pt.x()));
      he1.first.y((long double)(prev_pt.y()));
      he1.second.x((long double)(current_pt.x()));
      he1.second.y((long double)(current_pt.y()));
      he2.first.x((long double)(current_pt.x()));
      he2.first.y((long double)(current_pt.y()));
      he2.second.x((long double)(next_pt.x()));
      he2.second.y((long double)(next_pt.y()));
      compute_offset_edge(he1.first, he1.second, prev_pt, current_pt, distance, multiplier);
      compute_offset_edge(he2.first, he2.second, current_pt, next_pt, distance, multiplier);
      typedef scanline_base<long double>::compute_intersection_pack pack;
      point_data<long double> rpt;
      point_data<long double> bisectorpt((he1.second.x()+he2.first.x())/2,
                                         (he1.second.y()+he2.first.y())/2);
      point_data<long double> orig_pt((long double)pt.x(), (long double)pt.y());
      if(euclidean_distance(bisectorpt, orig_pt) < distance/2) {
        if(!pack::compute_lazy_intersection(rpt, he1, he2, true, false)) {
          rpt = he1.second; //colinear offset edges use shared point
        }
      } else {
        if(!pack::compute_lazy_intersection(rpt, he1, std::pair<point_data<long double>, point_data<long double> >(orig_pt, bisectorpt), true, false)) {
          rpt = he1.second; //colinear offset edges use shared point
        }
      }
      pt.x((coordinate_type)(std::floor(rpt.x()+0.5)));
      pt.y((coordinate_type)(std::floor(rpt.y()+0.5)));
    }

    static void resize_poly_up(std::vector<point_data<coordinate_type> >& poly, coordinate_type distance, coordinate_type multiplier) {
      point_data<coordinate_type> first_pt = poly[0];
      point_data<coordinate_type> second_pt = poly[1];
      point_data<coordinate_type> prev_pt = poly[0];
      point_data<coordinate_type> current_pt = poly[1];
      for(std::size_t i = 2; i < poly.size()-1; ++i) {
        point_data<coordinate_type> next_pt = poly[i];
        modify_pt(poly[i-1], prev_pt, current_pt, next_pt, distance, multiplier);
        prev_pt = current_pt;
        current_pt = next_pt;
      }
      point_data<coordinate_type> next_pt = first_pt;
      modify_pt(poly[poly.size()-2], prev_pt, current_pt, next_pt, distance, multiplier);
      prev_pt = current_pt;
      current_pt = next_pt;
      next_pt = second_pt;
      modify_pt(poly[0], prev_pt, current_pt, next_pt, distance, multiplier);
      poly.back() = poly.front();
    }
    static bool resize_poly_down(std::vector<point_data<coordinate_type> >& poly, coordinate_type distance, coordinate_type multiplier) {
      std::vector<point_data<coordinate_type> > orig_poly(poly);
      rectangle_data<coordinate_type> extents_rectangle;
      set_points(extents_rectangle, poly[0], poly[0]);
      point_data<coordinate_type> first_pt = poly[0];
      point_data<coordinate_type> second_pt = poly[1];
      point_data<coordinate_type> prev_pt = poly[0];
      point_data<coordinate_type> current_pt = poly[1];
      encompass(extents_rectangle, current_pt);
      for(std::size_t i = 2; i < poly.size()-1; ++i) {
        point_data<coordinate_type> next_pt = poly[i];
        encompass(extents_rectangle, next_pt);
        modify_pt(poly[i-1], prev_pt, current_pt, next_pt, distance, multiplier);
        prev_pt = current_pt;
        current_pt = next_pt;
      }
      if(delta(extents_rectangle, HORIZONTAL) <= std::abs(2*distance))
        return false;
      if(delta(extents_rectangle, VERTICAL) <= std::abs(2*distance))
        return false;
      point_data<coordinate_type> next_pt = first_pt;
      modify_pt(poly[poly.size()-2], prev_pt, current_pt, next_pt, distance, multiplier);
      prev_pt = current_pt;
      current_pt = next_pt;
      next_pt = second_pt;
      modify_pt(poly[0], prev_pt, current_pt, next_pt, distance, multiplier);
      poly.back() = poly.front();
      //if the line segments formed between orignial and new points cross for an edge that edge inverts
      //if all edges invert the polygon should be discarded
      //if even one edge does not invert return true because the polygon is valid
      bool non_inverting_edge = false;
      for(std::size_t i = 1; i < poly.size(); ++i) {
        std::pair<point_data<coordinate_type>, point_data<coordinate_type> >
          he1(poly[i], orig_poly[i]),
          he2(poly[i-1], orig_poly[i-1]);
        if(!scanline_base<coordinate_type>::intersects(he1, he2)) {
          non_inverting_edge = true;
          break;
        }
      }
      return non_inverting_edge;
    }

    polygon_set_data&
    bloat(typename coordinate_traits<coordinate_type>::unsigned_area_type distance) {
      std::list<polygon_with_holes_data<coordinate_type> > polys;
      get(polys);
      clear();
      for(typename std::list<polygon_with_holes_data<coordinate_type> >::iterator itr = polys.begin();
          itr != polys.end(); ++itr) {
        resize_poly_up((*itr).self_.coords_, (coordinate_type)distance, (coordinate_type)1);
        insert_vertex_sequence((*itr).self_.begin(), (*itr).self_.end(), COUNTERCLOCKWISE, false); //inserts without holes
        for(typename std::list<polygon_data<coordinate_type> >::iterator itrh = (*itr).holes_.begin();
            itrh != (*itr).holes_.end(); ++itrh) {
          if(resize_poly_down((*itrh).coords_, (coordinate_type)distance, (coordinate_type)1)) {
            insert_vertex_sequence((*itrh).coords_.begin(), (*itrh).coords_.end(), CLOCKWISE, true);
          }
        }
      }
      return *this;
    }

    polygon_set_data&
    shrink(typename coordinate_traits<coordinate_type>::unsigned_area_type distance) {
      std::list<polygon_with_holes_data<coordinate_type> > polys;
      get(polys);
      clear();
      for(typename std::list<polygon_with_holes_data<coordinate_type> >::iterator itr = polys.begin();
          itr != polys.end(); ++itr) {
        if(resize_poly_down((*itr).self_.coords_, (coordinate_type)distance, (coordinate_type)-1)) {
          insert_vertex_sequence((*itr).self_.begin(), (*itr).self_.end(), COUNTERCLOCKWISE, false); //inserts without holes
          for(typename std::list<polygon_data<coordinate_type> >::iterator itrh = (*itr).holes_.begin();
              itrh != (*itr).holes_.end(); ++itrh) {
            resize_poly_up((*itrh).coords_, (coordinate_type)distance, (coordinate_type)-1);
            insert_vertex_sequence((*itrh).coords_.begin(), (*itrh).coords_.end(), CLOCKWISE, true);
          }
        }
      }
      return *this;
    }

    // TODO:: should be private
    template <typename geometry_type>
    inline polygon_set_data&
    insert_with_resize(const geometry_type& poly, coordinate_type resizing, bool corner_fill_arc=false, unsigned int num_circle_segments=0, bool hole = false) {
      return insert_with_resize_dispatch(poly, resizing,  corner_fill_arc, num_circle_segments, hole, typename geometry_concept<geometry_type>::type());
    }

    template <typename geometry_type>
    inline polygon_set_data&
    insert_with_resize_dispatch(const geometry_type& poly, coordinate_type resizing, bool corner_fill_arc, unsigned int num_circle_segments, bool hole,
                               polygon_with_holes_concept) {
      insert_with_resize_dispatch(poly, resizing, corner_fill_arc, num_circle_segments, hole, polygon_concept());
      for(typename polygon_with_holes_traits<geometry_type>::iterator_holes_type itr =
            begin_holes(poly); itr != end_holes(poly);
          ++itr) {
        insert_with_resize_dispatch(*itr, resizing,  corner_fill_arc, num_circle_segments, !hole, polygon_concept());
      }
      return *this;
    }

    template <typename geometry_type>
    inline polygon_set_data&
    insert_with_resize_dispatch(const geometry_type& poly, coordinate_type resizing, bool corner_fill_arc, unsigned int num_circle_segments, bool hole,
                          polygon_concept) {

      if (resizing==0)
         return *this;


      // one dimensional used to store CCW/CW flag
      //direction_1d wdir = winding(poly);
      // LOW==CLOCKWISE just faster to type
      // so > 0 is CCW
      //int multiplier = wdir == LOW ? -1 : 1;
      //std::cout<<" multiplier : "<<multiplier<<std::endl;
      //if(hole) resizing *= -1;
      direction_1d resize_wdir = resizing>0?COUNTERCLOCKWISE:CLOCKWISE;

      typedef typename polygon_data<T>::iterator_type piterator;
      piterator first, second, third, end, real_end;
      real_end = end_points(poly);
      third = begin_points(poly);
      first = third;
      if(first == real_end) return *this;
      ++third;
      if(third == real_end) return *this;
      second = end = third;
      ++third;
      if(third == real_end) return *this;

        // for 1st corner
      std::vector<point_data<T> > first_pts;
      std::vector<point_data<T> > all_pts;
      direction_1d first_wdir = CLOCKWISE;

      // for all corners
      polygon_set_data<T> sizingSet;
      bool sizing_sign = resizing<0;
      bool prev_concave = true;
      point_data<T> prev_point;
      //int iCtr=0;


      //insert minkofski shapes on edges and corners
      do { // REAL WORK IS HERE


        //first, second and third point to points in correct CCW order
        // check if convex or concave case
        point_data<coordinate_type> normal1( second->y()-first->y(), first->x()-second->x());
        point_data<coordinate_type> normal2( third->y()-second->y(), second->x()-third->x());
        double direction = normal1.x()*normal2.y()- normal2.x()*normal1.y();
        bool convex = direction>0;

        bool treat_as_concave = !convex;
        if(sizing_sign)
          treat_as_concave = convex;
        point_data<double> v;
        assign(v, normal1);
        double s2 = (v.x()*v.x()+v.y()*v.y());
        double s = std::sqrt(s2)/resizing;
        v = point_data<double>(v.x()/s,v.y()/s);
        point_data<T> curr_prev;
        if (prev_concave)
          //TODO missing round_down()
          curr_prev = point_data<T>(first->x()+v.x(),first->y()+v.y());
        else
          curr_prev = prev_point;

           // around concave corners - insert rectangle
           // if previous corner is concave it's point info may be ignored
        if ( treat_as_concave) {
           std::vector<point_data<T> > pts;

           pts.push_back(point_data<T>(second->x()+v.x(),second->y()+v.y()));
           pts.push_back(*second);
           pts.push_back(*first);
           pts.push_back(point_data<T>(curr_prev));
           if (first_pts.size()){
              sizingSet.insert_vertex_sequence(pts.begin(),pts.end(), resize_wdir,false);
           }else {
               first_pts=pts;
               first_wdir = resize_wdir;
           }
        } else {

            // add either intersection_quad or pie_shape, based on corner_fill_arc option
           // for convex corner (convexity depends on sign of resizing, whether we shrink or grow)
           std::vector< std::vector<point_data<T> > > pts;
           direction_1d winding;
           winding = convex?COUNTERCLOCKWISE:CLOCKWISE;
           if (make_resizing_vertex_list(pts, curr_prev, prev_concave, *first, *second, *third, resizing
                                         , num_circle_segments, corner_fill_arc))
           {
               if (first_pts.size()) {
                  for (int i=0; i<pts.size(); i++) {
                    sizingSet.insert_vertex_sequence(pts[i].begin(),pts[i].end(),winding,false);
                  }

               } else {
                  first_pts = pts[0];
                  first_wdir = resize_wdir;
                  for (int i=1; i<pts.size(); i++) {
                    sizingSet.insert_vertex_sequence(pts[i].begin(),pts[i].end(),winding,false);
                  }
               }
               prev_point = curr_prev;

           } else {
              treat_as_concave = true;
           }
        }

        prev_concave = treat_as_concave;
        first = second;
        second = third;
        ++third;
        if(third == real_end) {
          third = begin_points(poly);
          if(*second == *third) {
            ++third; //skip first point if it is duplicate of last point
          }
        }
      } while(second != end);

      // handle insertion of first point
      if (!prev_concave) {
          first_pts[first_pts.size()-1]=prev_point;
      }
      sizingSet.insert_vertex_sequence(first_pts.begin(),first_pts.end(),first_wdir,false);

      polygon_set_data<coordinate_type> tmp;

      //insert original shape
      tmp.insert(poly, false, polygon_concept());
      if((resizing < 0) ^ hole) tmp -= sizingSet;
      else tmp += sizingSet;
      //tmp.clean();
      insert(tmp, hole);
      return (*this);
    }


    inline polygon_set_data&
    interact(const polygon_set_data& that);

    inline bool downcast(polygon_45_set_data<coordinate_type>& result) const {
      if(!is_45_) return false;
      for(iterator_type itr = begin(); itr != end(); ++itr) {
        const element_type& elem = *itr;
        int count = elem.second;
        int rise = 1; //up sloping 45
        if(scanline_base<coordinate_type>::is_horizontal(elem.first)) rise = 0;
        else if(scanline_base<coordinate_type>::is_vertical(elem.first)) rise = 2;
        else {
          if(!scanline_base<coordinate_type>::is_45_degree(elem.first)) {
            is_45_ = false;
            return false; //consider throwing because is_45_ has be be wrong
          }
          if(elem.first.first.y() > elem.first.second.y()) rise = -1; //down sloping 45
        }
        typename polygon_45_set_data<coordinate_type>::Vertex45Compact vertex(elem.first.first, rise, count);
        result.insert(vertex);
        typename polygon_45_set_data<coordinate_type>::Vertex45Compact vertex2(elem.first.second, rise, -count);
        result.insert(vertex2);
      }
      return true;
    }

    inline GEOMETRY_CONCEPT_ID concept_downcast() const {
      typedef typename coordinate_traits<coordinate_type>::coordinate_difference delta_type;
      bool is_45 = false;
      for(iterator_type itr = begin(); itr != end(); ++itr) {
        const element_type& elem = *itr;
        delta_type h_delta = euclidean_distance(elem.first.first, elem.first.second, HORIZONTAL);
        delta_type v_delta = euclidean_distance(elem.first.first, elem.first.second, VERTICAL);
        if(h_delta != 0 || v_delta != 0) {
          //neither delta is zero and the edge is not MANHATTAN
          if(v_delta != h_delta && v_delta != -h_delta) return POLYGON_SET_CONCEPT;
          else is_45 = true;
        }
      }
      if(is_45) return POLYGON_45_SET_CONCEPT;
      return POLYGON_90_SET_CONCEPT;
    }

  private:
    mutable value_type data_;
    mutable bool dirty_;
    mutable bool unsorted_;
    mutable bool is_45_;

  private:
    //functions

    template <typename output_container>
    void get_dispatch(output_container& output, polygon_concept tag) const {
      get_fracture(output, true, tag);
    }
    template <typename output_container>
    void get_dispatch(output_container& output, polygon_with_holes_concept tag) const {
      get_fracture(output, false, tag);
    }
    template <typename output_container, typename concept_type>
    void get_fracture(output_container& container, bool fracture_holes, concept_type ) const {
      clean();
      polygon_arbitrary_formation<coordinate_type> pf(fracture_holes);
      typedef typename polygon_arbitrary_formation<coordinate_type>::vertex_half_edge vertex_half_edge;
      std::vector<vertex_half_edge> data;
      for(iterator_type itr = data_.begin(); itr != data_.end(); ++itr){
        data.push_back(vertex_half_edge((*itr).first.first, (*itr).first.second, (*itr).second));
        data.push_back(vertex_half_edge((*itr).first.second, (*itr).first.first, -1 * (*itr).second));
      }
      polygon_sort(data.begin(), data.end());
      pf.scan(container, data.begin(), data.end());
    }
  };

  struct polygon_set_concept;
  template <typename T>
  struct geometry_concept<polygon_set_data<T> > {
    typedef polygon_set_concept type;
  };

//   template <typename  T>
//   inline double compute_area(point_data<T>& a, point_data<T>& b, point_data<T>& c) {

//      return (double)(b.x()-a.x())*(double)(c.y()-a.y())- (double)(c.x()-a.x())*(double)(b.y()-a.y());


//   }

  template <typename  T>
  inline int make_resizing_vertex_list(std::vector<std::vector<point_data< T> > >& return_points,
                       point_data<T>& curr_prev, bool ignore_prev_point,
                       point_data< T> start, point_data<T> middle, point_data< T>  end,
                       double sizing_distance, unsigned int num_circle_segments, bool corner_fill_arc) {

      // handle the case of adding an intersection point
      point_data<double> dn1( middle.y()-start.y(), start.x()-middle.x());
      double size = sizing_distance/std::sqrt( dn1.x()*dn1.x()+dn1.y()*dn1.y());
      dn1 = point_data<double>( dn1.x()*size, dn1.y()* size);
      point_data<double> dn2( end.y()-middle.y(), middle.x()-end.x());
      size = sizing_distance/std::sqrt( dn2.x()*dn2.x()+dn2.y()*dn2.y());
      dn2 = point_data<double>( dn2.x()*size, dn2.y()* size);
      point_data<double> start_offset((start.x()+dn1.x()),(start.y()+dn1.y()));
      point_data<double> mid1_offset((middle.x()+dn1.x()),(middle.y()+dn1.y()));
      point_data<double> end_offset((end.x()+dn2.x()),(end.y()+dn2.y()));
      point_data<double> mid2_offset((middle.x()+dn2.x()),(middle.y()+dn2.y()));
      if (ignore_prev_point)
            curr_prev = round_down<T>(start_offset);


      if (corner_fill_arc) {
         std::vector<point_data< T> > return_points1;
         return_points.push_back(return_points1);
         std::vector<point_data< T> >& return_points_back = return_points[return_points.size()-1];
         return_points_back.push_back(round_down<T>(mid1_offset));
         return_points_back.push_back(middle);
         return_points_back.push_back(start);
         return_points_back.push_back(curr_prev);
         point_data<double> dmid(middle.x(),middle.y());
         return_points.push_back(return_points1);
         int num = make_arc(return_points[return_points.size()-1],mid1_offset,mid2_offset,dmid,sizing_distance,num_circle_segments);
         curr_prev = round_down<T>(mid2_offset);
         return num;

      }

      std::pair<point_data<double>,point_data<double> > he1(start_offset,mid1_offset);
      std::pair<point_data<double>,point_data<double> > he2(mid2_offset ,end_offset);
      //typedef typename high_precision_type<double>::type high_precision;

      point_data<T> intersect;
      typename scanline_base<T>::compute_intersection_pack pack;
      bool res = pack.compute_intersection(intersect,he1,he2,true);
      if( res ) {
         std::vector<point_data< T> > return_points1;
         return_points.push_back(return_points1);
         std::vector<point_data< T> >& return_points_back = return_points[return_points.size()-1];
         return_points_back.push_back(intersect);
         return_points_back.push_back(middle);
         return_points_back.push_back(start);
         return_points_back.push_back(curr_prev);

         //double d1= compute_area(intersect,middle,start);
         //double d2= compute_area(start,curr_prev,intersect);

         curr_prev = intersect;


         return return_points.size();
      }
      return 0;

  }

  // this routine should take in start and end point s.t. end point is CCW from start
  // it sould make a pie slice polygon  that is an intersection of that arc
  // with an ngon segments approximation of the circle centered at center with radius r
  // point start is gauaranteed to be on the segmentation
  // returnPoints will start with the first point after start
  // returnPoints vector  may be empty
  template <typename  T>
  inline int  make_arc(std::vector<point_data< T> >& return_points,
                       point_data< double> start, point_data< double>  end,
                       point_data< double> center,  double r, unsigned int num_circle_segments) {
      const double our_pi=3.1415926535897932384626433832795028841971;

      // derive start and end angles
      double ps = atan2(start.y()-center.y(), start.x()-center.x());
      double pe = atan2(end.y()-center.y(), end.x()-center.x());
      if (ps <  0.0)
         ps += 2.0 * our_pi;
      if (pe <= 0.0)
         pe += 2.0 * our_pi;
      if (ps >= 2.0 * our_pi)
         ps -= 2.0 * our_pi;
      while (pe <= ps)
         pe += 2.0 * our_pi;
      double delta_angle = (2.0 * our_pi) / (double)num_circle_segments;
      if ( start==end) // full circle?
      {
          ps = delta_angle*0.5;
          pe = ps + our_pi * 2.0;
          double x,y;
          x =  center.x() + r * cos(ps);
          y = center.y() + r * sin(ps);
          start = point_data<double>(x,y);
          end = start;
      }
      return_points.push_back(round_down<T>(center));
      return_points.push_back(round_down<T>(start));
      unsigned int i=0;
      double curr_angle = ps+delta_angle;
      while( curr_angle < pe - 0.01 && i < 2 * num_circle_segments) {
         i++;
         double x = center.x() + r * cos( curr_angle);
         double y = center.y() + r * sin( curr_angle);
         return_points.push_back( round_down<T>((point_data<double>(x,y))));
         curr_angle+=delta_angle;
      }
      return_points.push_back(round_down<T>(end));
      return return_points.size();
  }

}// close namespace
}// close name space

#include "detail/scan_arbitrary.hpp"

namespace boost { namespace polygon {
  //ConnectivityExtraction computes the graph of connectivity between rectangle, polygon and
  //polygon set graph nodes where an edge is created whenever the geometry in two nodes overlap
  template <typename coordinate_type>
  class connectivity_extraction{
  private:
    typedef arbitrary_connectivity_extraction<coordinate_type, int> ce;
    ce ce_;
    unsigned int nodeCount_;
  public:
    inline connectivity_extraction() : ce_(), nodeCount_(0) {}
    inline connectivity_extraction(const connectivity_extraction& that) : ce_(that.ce_),
                                                                          nodeCount_(that.nodeCount_) {}
    inline connectivity_extraction& operator=(const connectivity_extraction& that) {
      ce_ = that.ce_;
      nodeCount_ = that.nodeCount_; {}
      return *this;
    }

    //insert a polygon set graph node, the value returned is the id of the graph node
    inline unsigned int insert(const polygon_set_data<coordinate_type>& ps) {
      ps.clean();
      ce_.populateTouchSetData(ps.begin(), ps.end(), nodeCount_);
      return nodeCount_++;
    }
    template <class GeoObjT>
    inline unsigned int insert(const GeoObjT& geoObj) {
      polygon_set_data<coordinate_type> ps;
      ps.insert(geoObj);
      return insert(ps);
    }

    //extract connectivity and store the edges in the graph
    //graph must be indexable by graph node id and the indexed value must be a std::set of
    //graph node id
    template <class GraphT>
    inline void extract(GraphT& graph) {
      ce_.execute(graph);
    }
  };

  template <typename T>
  polygon_set_data<T>&
  polygon_set_data<T>::interact(const polygon_set_data<T>& that) {
    connectivity_extraction<coordinate_type> ce;
    std::vector<polygon_with_holes_data<T> > polys;
    get(polys);
    clear();
    for(std::size_t i = 0; i < polys.size(); ++i) {
      ce.insert(polys[i]);
    }
    int id = ce.insert(that);
    std::vector<std::set<int> > graph(id+1);
    ce.extract(graph);
    for(std::set<int>::iterator itr = graph[id].begin();
        itr != graph[id].end(); ++itr) {
      insert(polys[*itr]);
    }
    return *this;
  }
}
}

#include "polygon_set_traits.hpp"
#include "detail/polygon_set_view.hpp"

#include "polygon_set_concept.hpp"
#include "detail/minkowski.hpp"
#endif

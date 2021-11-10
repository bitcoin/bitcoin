/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_45_SET_DATA_HPP
#define BOOST_POLYGON_POLYGON_45_SET_DATA_HPP
#include "polygon_90_set_data.hpp"
#include "detail/boolean_op_45.hpp"
#include "detail/polygon_45_formation.hpp"
#include "detail/polygon_45_touch.hpp"
#include "detail/property_merge_45.hpp"
namespace boost { namespace polygon{

  enum RoundingOption { CLOSEST = 0, OVERSIZE = 1, UNDERSIZE = 2, SQRT2 = 3, SQRT1OVER2 = 4 };
  enum CornerOption { INTERSECTION = 0, ORTHOGONAL = 1, UNFILLED = 2 };

  template <typename ltype, typename rtype, int op_type>
  class polygon_45_set_view;

  struct polygon_45_set_concept {};

  template <typename Unit>
  class polygon_45_set_data {
  public:
    typedef typename polygon_45_formation<Unit>::Vertex45Compact Vertex45Compact;
    typedef std::vector<Vertex45Compact> Polygon45VertexData;

    typedef Unit coordinate_type;
    typedef Polygon45VertexData value_type;
    typedef typename value_type::const_iterator iterator_type;
    typedef polygon_45_set_data operator_arg_type;

    // default constructor
    inline polygon_45_set_data() : error_data_(), data_(), dirty_(false), unsorted_(false), is_manhattan_(true) {}

    // constructor from a geometry object
    template <typename geometry_type>
    inline polygon_45_set_data(const geometry_type& that) : error_data_(), data_(), dirty_(false), unsorted_(false), is_manhattan_(true) {
      insert(that);
    }

    // copy constructor
    inline polygon_45_set_data(const polygon_45_set_data& that) :
      error_data_(that.error_data_), data_(that.data_), dirty_(that.dirty_),
      unsorted_(that.unsorted_), is_manhattan_(that.is_manhattan_) {}

    template <typename ltype, typename rtype, int op_type>
    inline polygon_45_set_data(const polygon_45_set_view<ltype, rtype, op_type>& that) :
      error_data_(), data_(), dirty_(false), unsorted_(false), is_manhattan_(true) {
      (*this) = that.value();
    }

    // destructor
    inline ~polygon_45_set_data() {}

    // assignement operator
    inline polygon_45_set_data& operator=(const polygon_45_set_data& that) {
      if(this == &that) return *this;
      error_data_ = that.error_data_;
      data_ = that.data_;
      dirty_ = that.dirty_;
      unsorted_ = that.unsorted_;
      is_manhattan_ = that.is_manhattan_;
      return *this;
    }

    template <typename ltype, typename rtype, int op_type>
    inline polygon_45_set_data& operator=(const polygon_45_set_view<ltype, rtype, op_type>& that) {
      (*this) = that.value();
      return *this;
    }

    template <typename geometry_object>
    inline polygon_45_set_data& operator=(const geometry_object& geometry) {
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
      dirty_ = true;
      unsorted_ = true;
      while(input_begin != input_end) {
        insert(*input_begin, is_hole);
        ++input_begin;
      }
    }

    inline void insert(const polygon_45_set_data& polygon_set, bool is_hole = false);
    template <typename coord_type>
    inline void insert(const polygon_45_set_data<coord_type>& polygon_set, bool is_hole = false);

    template <typename geometry_type>
    inline void insert(const geometry_type& geometry_object, bool is_hole = false) {
      insert_dispatch(geometry_object, is_hole, typename geometry_concept<geometry_type>::type());
    }

    inline void insert_clean(const Vertex45Compact& vertex_45, bool is_hole = false) {
      if(vertex_45.count.is_45()) is_manhattan_ = false;
      data_.push_back(vertex_45);
      if(is_hole) data_.back().count.invert();
    }

    inline void insert(const Vertex45Compact& vertex_45, bool is_hole = false) {
      dirty_ = true;
      unsorted_ = true;
      insert_clean(vertex_45, is_hole);
    }

    template <typename coordinate_type_2>
    inline void insert(const polygon_90_set_data<coordinate_type_2>& polygon_set, bool is_hole = false) {
      if(polygon_set.orient() == VERTICAL) {
        for(typename polygon_90_set_data<coordinate_type_2>::iterator_type itr = polygon_set.begin();
            itr != polygon_set.end(); ++itr) {
          Vertex45Compact vertex_45(point_data<Unit>((*itr).first, (*itr).second.first), 2, (*itr).second.second);
          vertex_45.count[1] = (*itr).second.second;
          if(is_hole) vertex_45.count[1] *= - 1;
          insert_clean(vertex_45, is_hole);
        }
      } else {
        for(typename polygon_90_set_data<coordinate_type_2>::iterator_type itr = polygon_set.begin();
            itr != polygon_set.end(); ++itr) {
          Vertex45Compact vertex_45(point_data<Unit>((*itr).second.first, (*itr).first), 2, (*itr).second.second);
          vertex_45.count[1] = (*itr).second.second;
          if(is_hole) vertex_45.count[1] *= - 1;
          insert_clean(vertex_45, is_hole);
        }
      }
      dirty_ = true;
      unsorted_ = true;
    }

    template <typename output_container>
    inline void get(output_container& output) const {
      get_dispatch(output, typename geometry_concept<typename output_container::value_type>::type());
    }

    inline bool has_error_data() const { return !error_data_.empty(); }
    inline std::size_t error_count() const { return error_data_.size() / 4; }
    inline void get_error_data(polygon_45_set_data& p) const {
      p.data_.insert(p.data_.end(), error_data_.begin(), error_data_.end());
    }

    // equivalence operator
    inline bool operator==(const polygon_45_set_data& p) const {
      clean();
      p.clean();
      return data_ == p.data_;
    }

    // inequivalence operator
    inline bool operator!=(const polygon_45_set_data& p) const {
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

    // clear the contents of the polygon_45_set_data
    inline void clear() { data_.clear(); error_data_.clear(); dirty_ = unsorted_ = false; is_manhattan_ = true; }

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

    // find out if Polygon set is clean
    inline bool is_manhattan() const { return is_manhattan_; }

    bool clean() const;

    void sort() const{
      if(unsorted_) {
        polygon_sort(data_.begin(), data_.end());
        unsorted_ = false;
      }
    }

    template <typename input_iterator_type>
    void set(input_iterator_type input_begin, input_iterator_type input_end) {
      data_.clear();
      reserve(std::distance(input_begin, input_end));
      insert(input_begin, input_end);
      dirty_ = true;
      unsorted_ = true;
    }

    void set_clean(const value_type& value) {
      data_ = value;
      dirty_ = false;
      unsorted_ = false;
    }

    void set(const value_type& value) {
      data_ = value;
      dirty_ = true;
      unsorted_ = true;
    }

    // append to the container cT with polygons (holes will be fractured vertically)
    template <class cT>
    void get_polygons(cT& container) const {
      get_dispatch(container, polygon_45_concept());
    }

    // append to the container cT with PolygonWithHoles objects
    template <class cT>
    void get_polygons_with_holes(cT& container) const {
      get_dispatch(container, polygon_45_with_holes_concept());
    }

    // append to the container cT with polygons of three or four verticies
    // slicing orientation is vertical
    template <class cT>
    void get_trapezoids(cT& container) const {
      clean();
      typename polygon_45_formation<Unit>::Polygon45Tiling pf;
      //std::cout << "FORMING POLYGONS\n";
      pf.scan(container, data_.begin(), data_.end());
      //std::cout << "DONE FORMING POLYGONS\n";
    }

    // append to the container cT with polygons of three or four verticies
    template <class cT>
    void get_trapezoids(cT& container, orientation_2d slicing_orientation) const {
      if(slicing_orientation == VERTICAL) {
        get_trapezoids(container);
      } else {
        polygon_45_set_data<Unit> ps(*this);
        ps.transform(axis_transformation(axis_transformation::SWAP_XY));
        cT result;
        ps.get_trapezoids(result);
        for(typename cT::iterator itr = result.begin(); itr != result.end(); ++itr) {
          ::boost::polygon::transform(*itr, axis_transformation(axis_transformation::SWAP_XY));
        }
        container.insert(container.end(), result.begin(), result.end());
      }
    }

    // insert vertex sequence
    template <class iT>
    void insert_vertex_sequence(iT begin_vertex, iT end_vertex,
                                direction_1d winding, bool is_hole = false);

    // get the external boundary rectangle
    template <typename rectangle_type>
    bool extents(rectangle_type& rect) const;

    // snap verticies of set to even,even or odd,odd coordinates
    void snap() const;

    // |= &= += *= -= ^= binary operators
    polygon_45_set_data& operator|=(const polygon_45_set_data& b);
    polygon_45_set_data& operator&=(const polygon_45_set_data& b);
    polygon_45_set_data& operator+=(const polygon_45_set_data& b);
    polygon_45_set_data& operator*=(const polygon_45_set_data& b);
    polygon_45_set_data& operator-=(const polygon_45_set_data& b);
    polygon_45_set_data& operator^=(const polygon_45_set_data& b);

    // resizing operations
    polygon_45_set_data& operator+=(Unit delta);
    polygon_45_set_data& operator-=(Unit delta);

    // shrink the Polygon45Set by shrinking
    polygon_45_set_data& resize(coordinate_type resizing, RoundingOption rounding = CLOSEST,
                                CornerOption corner = INTERSECTION);

    // transform set
    template <typename transformation_type>
    polygon_45_set_data& transform(const transformation_type& tr);

    // scale set
    polygon_45_set_data& scale_up(typename coordinate_traits<Unit>::unsigned_area_type factor);
    polygon_45_set_data& scale_down(typename coordinate_traits<Unit>::unsigned_area_type factor);
    polygon_45_set_data& scale(double scaling);

    // self_intersect
    polygon_45_set_data& self_intersect() {
      sort();
      applyAdaptiveUnary_<1>(); //1 = AND
      dirty_ = false;
      return *this;
    }

    // self_xor
    polygon_45_set_data& self_xor() {
      sort();
      applyAdaptiveUnary_<3>(); //3 = XOR
      dirty_ = false;
      return *this;
    }

    // accumulate the bloated polygon
    template <typename geometry_type>
    polygon_45_set_data& insert_with_resize(const geometry_type& poly,
                                            coordinate_type resizing, RoundingOption rounding = CLOSEST,
                                            CornerOption corner = INTERSECTION,
                                            bool hole = false) {
      return insert_with_resize_dispatch(poly, resizing, rounding, corner, hole, typename geometry_concept<geometry_type>::type());
    }

  private:
    mutable value_type error_data_;
    mutable value_type data_;
    mutable bool dirty_;
    mutable bool unsorted_;
    mutable bool is_manhattan_;

  private:
    //functions
    template <typename output_container>
    void get_dispatch(output_container& output, polygon_45_concept tag) const {
      get_fracture(output, true, tag);
    }
    template <typename output_container>
    void get_dispatch(output_container& output, polygon_45_with_holes_concept tag) const {
      get_fracture(output, false, tag);
    }
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
      typename polygon_45_formation<Unit>::Polygon45Formation pf(fracture_holes);
      //std::cout << "FORMING POLYGONS\n";
      pf.scan(container, data_.begin(), data_.end());
    }

    template <typename geometry_type>
    void insert_dispatch(const geometry_type& geometry_object, bool is_hole, undefined_concept) {
      insert(geometry_object.begin(), geometry_object.end(), is_hole);
    }
    template <typename geometry_type>
    void insert_dispatch(const geometry_type& geometry_object, bool is_hole, rectangle_concept tag);
    template <typename geometry_type>
    void insert_dispatch(const geometry_type& geometry_object, bool is_hole, polygon_90_concept ) {
      insert_vertex_sequence(begin_points(geometry_object), end_points(geometry_object), winding(geometry_object), is_hole);
    }
    template <typename geometry_type>
    void insert_dispatch(const geometry_type& geometry_object, bool is_hole, polygon_90_with_holes_concept ) {
      insert_vertex_sequence(begin_points(geometry_object), end_points(geometry_object), winding(geometry_object), is_hole);
      for(typename polygon_with_holes_traits<geometry_type>::iterator_holes_type itr =
            begin_holes(geometry_object); itr != end_holes(geometry_object);
          ++itr) {
        insert_vertex_sequence(begin_points(*itr), end_points(*itr), winding(*itr), !is_hole);
      }
    }
    template <typename geometry_type>
    void insert_dispatch(const geometry_type& geometry_object, bool is_hole, polygon_45_concept ) {
      insert_vertex_sequence(begin_points(geometry_object), end_points(geometry_object), winding(geometry_object), is_hole);
    }
    template <typename geometry_type>
    void insert_dispatch(const geometry_type& geometry_object, bool is_hole, polygon_45_with_holes_concept ) {
      insert_vertex_sequence(begin_points(geometry_object), end_points(geometry_object), winding(geometry_object), is_hole);
      for(typename polygon_with_holes_traits<geometry_type>::iterator_holes_type itr =
            begin_holes(geometry_object); itr != end_holes(geometry_object);
          ++itr) {
        insert_vertex_sequence(begin_points(*itr), end_points(*itr), winding(*itr), !is_hole);
      }
    }
    template <typename geometry_type>
    void insert_dispatch(const geometry_type& geometry_object, bool is_hole, polygon_45_set_concept ) {
      polygon_45_set_data ps;
      assign(ps, geometry_object);
      insert(ps, is_hole);
    }
    template <typename geometry_type>
    void insert_dispatch(const geometry_type& geometry_object, bool is_hole, polygon_90_set_concept ) {
      std::list<polygon_90_data<coordinate_type> > pl;
      assign(pl, geometry_object);
      insert(pl.begin(), pl.end(), is_hole);
    }

    void insert_vertex_half_edge_45_pair(const point_data<Unit>& pt1, point_data<Unit>& pt2,
                                         const point_data<Unit>& pt3, direction_1d wdir);

    template <typename geometry_type>
    polygon_45_set_data& insert_with_resize_dispatch(const geometry_type& poly,
                                                     coordinate_type resizing, RoundingOption rounding,
                                                     CornerOption corner, bool hole, polygon_45_concept tag);

    // accumulate the bloated polygon with holes
    template <typename geometry_type>
    polygon_45_set_data& insert_with_resize_dispatch(const geometry_type& poly,
                                                     coordinate_type resizing, RoundingOption rounding,
                                                     CornerOption corner, bool hole, polygon_45_with_holes_concept tag);

    static void snap_vertex_45(Vertex45Compact& vertex);

  public:
    template <int op>
    void applyAdaptiveBoolean_(const polygon_45_set_data& rvalue) const;
    template <int op>
    void applyAdaptiveBoolean_(polygon_45_set_data& result, const polygon_45_set_data& rvalue) const;
    template <int op>
    void applyAdaptiveUnary_() const;
  };

  template <typename T>
  struct geometry_concept<polygon_45_set_data<T> > {
    typedef polygon_45_set_concept type;
  };

  template <typename iT, typename T>
  void scale_up_vertex_45_compact_range(iT beginr, iT endr, T factor) {
    for( ; beginr != endr; ++beginr) {
      scale_up((*beginr).pt, factor);
    }
  }
  template <typename iT, typename T>
  void scale_down_vertex_45_compact_range_blindly(iT beginr, iT endr, T factor) {
    for( ; beginr != endr; ++beginr) {
      scale_down((*beginr).pt, factor);
    }
  }

  template <typename Unit>
  inline std::pair<int, int> characterizeEdge45(const point_data<Unit>& pt1, const point_data<Unit>& pt2) {
    std::pair<int, int> retval(0, 1);
    if(pt1.x() == pt2.x()) {
      retval.first = 3;
      retval.second = -1;
      return retval;
    }
    //retval.second = pt1.x() < pt2.x() ? -1 : 1;
    retval.second = 1;
    if(pt1.y() == pt2.y()) {
      retval.first = 1;
    } else if(pt1.x() < pt2.x()) {
      if(pt1.y() < pt2.y()) {
        retval.first = 2;
      } else {
        retval.first = 0;
      }
    } else {
      if(pt1.y() < pt2.y()) {
        retval.first = 0;
      } else {
        retval.first = 2;
      }
    }
    return retval;
  }

  template <typename cT, typename pT>
  bool insert_vertex_half_edge_45_pair_into_vector(cT& output,
                                       const pT& pt1, pT& pt2,
                                       const pT& pt3,
                                       direction_1d wdir) {
    int multiplier = wdir == LOW ? -1 : 1;
    typename cT::value_type vertex(pt2, 0, 0);
    //std::cout << pt1 << " " << pt2 << " " << pt3 << std::endl;
    std::pair<int, int> check;
    check = characterizeEdge45(pt1, pt2);
    //std::cout << "index " << check.first << " " << check.second * -multiplier << std::endl;
    vertex.count[check.first] += check.second * -multiplier;
    check = characterizeEdge45(pt2, pt3);
    //std::cout << "index " << check.first << " " << check.second * multiplier << std::endl;
    vertex.count[check.first] += check.second * multiplier;
    output.push_back(vertex);
    return vertex.count.is_45();
  }

  template <typename Unit>
  inline void polygon_45_set_data<Unit>::insert_vertex_half_edge_45_pair(const point_data<Unit>& pt1, point_data<Unit>& pt2,
                                                                         const point_data<Unit>& pt3,
                                                                         direction_1d wdir) {
    if(insert_vertex_half_edge_45_pair_into_vector(data_, pt1, pt2, pt3, wdir)) is_manhattan_ = false;
  }

  template <typename Unit>
  template <class iT>
  inline void polygon_45_set_data<Unit>::insert_vertex_sequence(iT begin_vertex, iT end_vertex,
                                                                direction_1d winding, bool is_hole) {
    if(begin_vertex == end_vertex) return;
    if(is_hole) winding = winding.backward();
    iT itr = begin_vertex;
    if(itr == end_vertex) return;
    point_data<Unit> firstPt = *itr;
    ++itr;
    point_data<Unit> secondPt(firstPt);
    //skip any duplicate points
    do {
      if(itr == end_vertex) return;
      secondPt = *itr;
      ++itr;
    } while(secondPt == firstPt);
    point_data<Unit> prevPt = secondPt;
    point_data<Unit> prevPrevPt = firstPt;
    while(itr != end_vertex) {
      point_data<Unit> pt = *itr;
      //skip any duplicate points
      if(pt == prevPt) {
        ++itr;
        continue;
      }
      //operate on the three points
      insert_vertex_half_edge_45_pair(prevPrevPt, prevPt, pt, winding);
      prevPrevPt = prevPt;
      prevPt = pt;
      ++itr;
    }
    if(prevPt != firstPt) {
      insert_vertex_half_edge_45_pair(prevPrevPt, prevPt, firstPt, winding);
      insert_vertex_half_edge_45_pair(prevPt, firstPt, secondPt, winding);
    } else {
      insert_vertex_half_edge_45_pair(prevPrevPt, firstPt, secondPt, winding);
    }
    dirty_ = true;
    unsorted_ = true;
  }

  // insert polygon set
  template <typename Unit>
  inline void polygon_45_set_data<Unit>::insert(const polygon_45_set_data<Unit>& polygon_set, bool is_hole) {
    std::size_t count = data_.size();
    data_.insert(data_.end(), polygon_set.data_.begin(), polygon_set.data_.end());
    error_data_.insert(error_data_.end(), polygon_set.error_data_.begin(),
                       polygon_set.error_data_.end());
    if(is_hole) {
      for(std::size_t i = count; i < data_.size(); ++i) {
        data_[i].count = data_[i].count.invert();
      }
    }
    dirty_ = true;
    unsorted_ = true;
    if(polygon_set.is_manhattan_ == false) is_manhattan_ = false;
    return;
  }
  // insert polygon set
  template <typename Unit>
  template <typename coord_type>
  inline void polygon_45_set_data<Unit>::insert(const polygon_45_set_data<coord_type>& polygon_set, bool is_hole) {
    std::size_t count = data_.size();
    for(typename polygon_45_set_data<coord_type>::iterator_type itr = polygon_set.begin();
        itr != polygon_set.end(); ++itr) {
      const typename polygon_45_set_data<coord_type>::Vertex45Compact& v = *itr;
      typename polygon_45_set_data<Unit>::Vertex45Compact v2;
      v2.pt.x(static_cast<Unit>(v.pt.x()));
      v2.pt.y(static_cast<Unit>(v.pt.y()));
      v2.count = typename polygon_45_formation<Unit>::Vertex45Count(v.count[0], v.count[1], v.count[2], v.count[3]);
      data_.push_back(v2);
    }
    polygon_45_set_data<coord_type> tmp;
    polygon_set.get_error_data(tmp);
    for(typename polygon_45_set_data<coord_type>::iterator_type itr = tmp.begin();
        itr != tmp.end(); ++itr) {
      const typename polygon_45_set_data<coord_type>::Vertex45Compact& v = *itr;
      typename polygon_45_set_data<Unit>::Vertex45Compact v2;
      v2.pt.x(static_cast<Unit>(v.pt.x()));
      v2.pt.y(static_cast<Unit>(v.pt.y()));
      v2.count = typename polygon_45_formation<Unit>::Vertex45Count(v.count[0], v.count[1], v.count[2], v.count[3]);
      error_data_.push_back(v2);
    }
    if(is_hole) {
      for(std::size_t i = count; i < data_.size(); ++i) {
        data_[i].count = data_[i].count.invert();
      }
    }
    dirty_ = true;
    unsorted_ = true;
    if(polygon_set.is_manhattan() == false) is_manhattan_ = false;
    return;
  }

  template <typename cT, typename rT>
  void insert_rectangle_into_vector_45(cT& output, const rT& rect, bool is_hole) {
    point_data<typename rectangle_traits<rT>::coordinate_type>
      llpt = ll(rect), lrpt = lr(rect), ulpt = ul(rect), urpt = ur(rect);
    direction_1d dir = COUNTERCLOCKWISE;
    if(is_hole) dir = CLOCKWISE;
    insert_vertex_half_edge_45_pair_into_vector(output, llpt, lrpt, urpt, dir);
    insert_vertex_half_edge_45_pair_into_vector(output, lrpt, urpt, ulpt, dir);
    insert_vertex_half_edge_45_pair_into_vector(output, urpt, ulpt, llpt, dir);
    insert_vertex_half_edge_45_pair_into_vector(output, ulpt, llpt, lrpt, dir);
  }

  template <typename Unit>
  template <typename geometry_type>
  inline void polygon_45_set_data<Unit>::insert_dispatch(const geometry_type& geometry_object,
                                                         bool is_hole, rectangle_concept ) {
    dirty_ = true;
    unsorted_ = true;
    insert_rectangle_into_vector_45(data_, geometry_object, is_hole);
  }

  // get the external boundary rectangle
  template <typename Unit>
  template <typename rectangle_type>
  inline bool polygon_45_set_data<Unit>::extents(rectangle_type& rect) const{
    clean();
    if(empty()) {
      return false;
    }
    Unit low = (std::numeric_limits<Unit>::max)();
    Unit high = (std::numeric_limits<Unit>::min)();
    interval_data<Unit> xivl(low, high);
    interval_data<Unit> yivl(low, high);
    for(typename value_type::const_iterator itr = data_.begin();
        itr != data_.end(); ++ itr) {
      if((*itr).pt.x() > xivl.get(HIGH))
        xivl.set(HIGH, (*itr).pt.x());
      if((*itr).pt.x() < xivl.get(LOW))
        xivl.set(LOW, (*itr).pt.x());
      if((*itr).pt.y() > yivl.get(HIGH))
        yivl.set(HIGH, (*itr).pt.y());
      if((*itr).pt.y() < yivl.get(LOW))
        yivl.set(LOW, (*itr).pt.y());
    }
    rect = construct<rectangle_type>(xivl, yivl);
    return true;
  }

  //this function snaps the vertex and two half edges
  //to be both even or both odd coordinate values if one of the edges is 45
  //and throws an excpetion if an edge is non-manhattan, non-45.
  template <typename Unit>
  inline void polygon_45_set_data<Unit>::snap_vertex_45(typename polygon_45_set_data<Unit>::Vertex45Compact& vertex) {
    bool plus45 = vertex.count[2] != 0;
    bool minus45 = vertex.count[0] != 0;
    if(plus45 || minus45) {
      if(local_abs(vertex.pt.x()) % 2 != local_abs(vertex.pt.y()) % 2) {
        if(vertex.count[1] != 0 ||
           (plus45 && minus45)) {
          //move right
          vertex.pt.x(vertex.pt.x() + 1);
        } else {
          //assert that vertex.count[3] != 0
          Unit modifier = plus45 ? -1 : 1;
          vertex.pt.y(vertex.pt.y() + modifier);
        }
      }
    }
  }

  template <typename Unit>
  inline void polygon_45_set_data<Unit>::snap() const {
    for(typename value_type::iterator itr = data_.begin();
        itr != data_.end(); ++itr) {
      snap_vertex_45(*itr);
    }
  }

  // |= &= += *= -= ^= binary operators
  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::operator|=(const polygon_45_set_data<Unit>& b) {
    insert(b);
    return *this;
  }
  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::operator&=(const polygon_45_set_data<Unit>& b) {
    //b.sort();
    //sort();
    applyAdaptiveBoolean_<1>(b);
    dirty_ = false;
    unsorted_ = false;
    return *this;
  }
  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::operator+=(const polygon_45_set_data<Unit>& b) {
    return (*this) |= b;
  }
  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::operator*=(const polygon_45_set_data<Unit>& b) {
    return (*this) &= b;
  }
  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::operator-=(const polygon_45_set_data<Unit>& b) {
    //b.sort();
    //sort();
    applyAdaptiveBoolean_<2>(b);
    dirty_ = false;
    unsorted_ = false;
    return *this;
  }
  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::operator^=(const polygon_45_set_data<Unit>& b) {
    //b.sort();
    //sort();
    applyAdaptiveBoolean_<3>(b);
    dirty_ = false;
    unsorted_ = false;
    return *this;
  }

  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::operator+=(Unit delta) {
    return resize(delta);
  }
  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::operator-=(Unit delta) {
    return (*this) += -delta;
  }

  template <typename Unit>
  inline polygon_45_set_data<Unit>&
  polygon_45_set_data<Unit>::resize(Unit resizing, RoundingOption rounding, CornerOption corner) {
    if(resizing == 0) return *this;
    std::list<polygon_45_with_holes_data<Unit> > pl;
    get_polygons_with_holes(pl);
    clear();
    for(typename std::list<polygon_45_with_holes_data<Unit> >::iterator itr = pl.begin(); itr != pl.end(); ++itr) {
      insert_with_resize(*itr, resizing, rounding, corner);
    }
    clean();
    //perterb 45 edges to prevent non-integer intersection errors upon boolean op
    //snap();
    return *this;
  }

  //distance is assumed to be positive
  inline int roundClosest(double distance) {
    int f = (int)distance;
    if(distance - (double)f < 0.5) return f;
    return f+1;
  }

  //distance is assumed to be positive
  template <typename Unit>
  inline Unit roundWithOptions(double distance, RoundingOption rounding) {
    if(rounding == CLOSEST) {
      return roundClosest(distance);
    } else if(rounding == OVERSIZE) {
      return (Unit)distance + 1;
    } else { //UNDERSIZE
      return (Unit)distance;
    }
  }

  // 0 is east, 1 is northeast, 2 is north, 3 is northwest, 4 is west, 5 is southwest, 6 is south
  // 7 is southwest
  template <typename Unit>
  inline point_data<Unit> bloatVertexInDirWithOptions(const point_data<Unit>& point, unsigned int dir,
                                                      Unit bloating, RoundingOption rounding) {
    const double sqrt2 = 1.4142135623730950488016887242097;
    if(dir & 1) {
      Unit unitDistance = (Unit)bloating;
      if(rounding != SQRT2) {
        //45 degree bloating
        double distance = (double)bloating;
        distance /= sqrt2;  // multiply by 1/sqrt2
        unitDistance = roundWithOptions<Unit>(distance, rounding);
      }
      int xMultiplier = 1;
      int yMultiplier = 1;
      if(dir == 3 || dir == 5) xMultiplier = -1;
      if(dir == 5 || dir == 7) yMultiplier = -1;
      return point_data<Unit>(point.x()+xMultiplier*unitDistance,
                   point.y()+yMultiplier*unitDistance);
    } else {
      if(dir == 0)
        return point_data<Unit>(point.x()+bloating, point.y());
      if(dir == 2)
        return point_data<Unit>(point.x(), point.y()+bloating);
      if(dir == 4)
        return point_data<Unit>(point.x()-bloating, point.y());
      if(dir == 6)
        return point_data<Unit>(point.x(), point.y()-bloating);
      return point_data<Unit>();
    }
  }

  template <typename Unit>
  inline unsigned int getEdge45Direction(const point_data<Unit>& pt1, const point_data<Unit>& pt2) {
    if(pt1.x() == pt2.x()) {
      if(pt1.y() < pt2.y()) return 2;
      return 6;
    }
    if(pt1.y() == pt2.y()) {
      if(pt1.x() < pt2.x()) return 0;
      return 4;
    }
    if(pt2.y() > pt1.y()) {
      if(pt2.x() > pt1.x()) return 1;
      return 3;
    }
    if(pt2.x() > pt1.x()) return 7;
    return 5;
  }

  inline unsigned int getEdge45NormalDirection(unsigned int dir, int multiplier) {
    if(multiplier < 0)
      return (dir + 2) % 8;
    return (dir + 4 + 2) % 8;
  }

  template <typename Unit>
  inline point_data<Unit> getIntersectionPoint(const point_data<Unit>& pt1, unsigned int slope1,
                                               const point_data<Unit>& pt2, unsigned int slope2) {
    //the intention here is to use all integer arithmetic without causing overflow
    //turncation error or divide by zero error
    //I don't use floating point arithmetic because its precision may not be high enough
    //at the extremes of the integer range
    typedef typename coordinate_traits<Unit>::area_type LongUnit;
    const Unit rises[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    const Unit runs[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    LongUnit rise1 = rises[slope1];
    LongUnit rise2 = rises[slope2];
    LongUnit run1 = runs[slope1];
    LongUnit run2 = runs[slope2];
    LongUnit x1 = (LongUnit)pt1.x();
    LongUnit x2 = (LongUnit)pt2.x();
    LongUnit y1 = (LongUnit)pt1.y();
    LongUnit y2 = (LongUnit)pt2.y();
    Unit x = 0;
    Unit y = 0;
    if(run1 == 0) {
      x = pt1.x();
      y = (Unit)(((x1 - x2) * rise2) / run2) + pt2.y();
    } else if(run2 == 0) {
      x = pt2.x();
      y = (Unit)(((x2 - x1) * rise1) / run1) + pt1.y();
    } else {
      // y - y1 = (rise1/run1)(x - x1)
      // y - y2 = (rise2/run2)(x - x2)
      // y = (rise1/run1)(x - x1) + y1 = (rise2/run2)(x - x2) + y2
      // (rise1/run1 - rise2/run2)x = y2 - y1 + rise1/run1 x1 - rise2/run2 x2
      // x = (y2 - y1 + rise1/run1 x1 - rise2/run2 x2)/(rise1/run1 - rise2/run2)
      // x = (y2 - y1 + rise1/run1 x1 - rise2/run2 x2)(rise1 run2 - rise2 run1)/(run1 run2)
      x = (Unit)((y2 - y1 + ((rise1 * x1) / run1) - ((rise2 * x2) / run2)) *
                 (run1 * run2) / (rise1 * run2 - rise2 * run1));
      if(rise1 == 0) {
        y = pt1.y();
      } else if(rise2 == 0) {
        y = pt2.y();
      } else {
        // y - y1 = (rise1/run1)(x - x1)
        // (run1/rise1)(y - y1) = x - x1
        // x = (run1/rise1)(y - y1) + x1 = (run2/rise2)(y - y2) + x2
        y = (Unit)((x2 - x1 + ((run1 * y1) / rise1) - ((run2 * y2) / rise2)) *
                   (rise1 * rise2) / (run1 * rise2 - run2 * rise1));
      }
    }
    return point_data<Unit>(x, y);
  }

  template <typename Unit>
  inline
  void handleResizingEdge45_SQRT1OVER2(polygon_45_set_data<Unit>& sizingSet, point_data<Unit> first,
                                       point_data<Unit> second, Unit resizing, CornerOption corner) {
    if(first.x() == second.x()) {
      sizingSet.insert(rectangle_data<Unit>(first.x() - resizing, first.y(), first.x() + resizing, second.y()));
      return;
    }
    if(first.y() == second.y()) {
      sizingSet.insert(rectangle_data<Unit>(first.x(), first.y() - resizing, second.x(), first.y() + resizing));
      return;
    }
    std::vector<point_data<Unit> > pts;
    Unit bloating = resizing < 0 ? -resizing : resizing;
    if(corner == UNFILLED) {
      //we have to round up
      bloating = bloating / 2 + bloating % 2 ; //round up
      if(second.x() < first.x()) std::swap(first, second);
      if(first.y() < second.y()) { //upward sloping
        pts.push_back(point_data<Unit>(first.x() + bloating, first.y() - bloating));
        pts.push_back(point_data<Unit>(first.x() - bloating, first.y() + bloating));
        pts.push_back(point_data<Unit>(second.x() - bloating, second.y() + bloating));
        pts.push_back(point_data<Unit>(second.x() + bloating, second.y() - bloating));
        sizingSet.insert_vertex_sequence(pts.begin(), pts.end(), CLOCKWISE, false);
      } else { //downward sloping
        pts.push_back(point_data<Unit>(first.x() + bloating, first.y() + bloating));
        pts.push_back(point_data<Unit>(first.x() - bloating, first.y() - bloating));
        pts.push_back(point_data<Unit>(second.x() - bloating, second.y() - bloating));
        pts.push_back(point_data<Unit>(second.x() + bloating, second.y() + bloating));
        sizingSet.insert_vertex_sequence(pts.begin(), pts.end(), COUNTERCLOCKWISE, false);
      }
      return;
    }
    if(second.x() < first.x()) std::swap(first, second);
    if(first.y() < second.y()) { //upward sloping
      pts.push_back(point_data<Unit>(first.x(), first.y() - bloating));
      pts.push_back(point_data<Unit>(first.x() - bloating, first.y()));
      pts.push_back(point_data<Unit>(second.x(), second.y() + bloating));
      pts.push_back(point_data<Unit>(second.x() + bloating, second.y()));
      sizingSet.insert_vertex_sequence(pts.begin(), pts.end(), CLOCKWISE, false);
    } else { //downward sloping
      pts.push_back(point_data<Unit>(first.x() - bloating, first.y()));
      pts.push_back(point_data<Unit>(first.x(), first.y() + bloating));
      pts.push_back(point_data<Unit>(second.x() + bloating, second.y()));
      pts.push_back(point_data<Unit>(second.x(), second.y() - bloating));
      sizingSet.insert_vertex_sequence(pts.begin(), pts.end(), CLOCKWISE, false);
    }
  }


  template <typename Unit>
  inline
  void handleResizingEdge45(polygon_45_set_data<Unit>& sizingSet, point_data<Unit> first,
                            point_data<Unit> second, Unit resizing, RoundingOption rounding) {
    if(first.x() == second.x()) {
      sizingSet.insert(rectangle_data<int>(first.x() - resizing, first.y(), first.x() + resizing, second.y()));
      return;
    }
    if(first.y() == second.y()) {
      sizingSet.insert(rectangle_data<int>(first.x(), first.y() - resizing, second.x(), first.y() + resizing));
      return;
    }
    //edge is 45
    std::vector<point_data<Unit> > pts;
    Unit bloating = resizing < 0 ? -resizing : resizing;
    if(second.x() < first.x()) std::swap(first, second);
    if(first.y() < second.y()) {
      pts.push_back(bloatVertexInDirWithOptions(first, 3, bloating, rounding));
      pts.push_back(bloatVertexInDirWithOptions(first, 7, bloating, rounding));
      pts.push_back(bloatVertexInDirWithOptions(second, 7, bloating, rounding));
      pts.push_back(bloatVertexInDirWithOptions(second, 3, bloating, rounding));
      sizingSet.insert_vertex_sequence(pts.begin(), pts.end(), HIGH, false);
    } else {
      pts.push_back(bloatVertexInDirWithOptions(first, 1, bloating, rounding));
      pts.push_back(bloatVertexInDirWithOptions(first, 5, bloating, rounding));
      pts.push_back(bloatVertexInDirWithOptions(second, 5, bloating, rounding));
      pts.push_back(bloatVertexInDirWithOptions(second, 1, bloating, rounding));
      sizingSet.insert_vertex_sequence(pts.begin(), pts.end(), HIGH, false);
    }
  }

  template <typename Unit>
  inline point_data<Unit> bloatVertexInDirWithSQRT1OVER2(int edge1, int normal1, const point_data<Unit>& second, Unit bloating,
                                                         bool first) {
    orientation_2d orient = first ? HORIZONTAL : VERTICAL;
    orientation_2d orientp = orient.get_perpendicular();
    int multiplier = first ? 1 : -1;
    point_data<Unit> pt1(second);
    if(edge1 == 1) {
      if(normal1 == 3) {
        move(pt1, orient, -multiplier * bloating);
      } else {
        move(pt1, orientp, -multiplier * bloating);
      }
    } else if(edge1 == 3) {
      if(normal1 == 1) {
        move(pt1, orient, multiplier * bloating);
      } else {
        move(pt1, orientp, -multiplier * bloating);
      }
    } else if(edge1 == 5) {
      if(normal1 == 3) {
        move(pt1, orientp, multiplier * bloating);
      } else {
        move(pt1, orient, multiplier * bloating);
      }
    } else {
      if(normal1 == 5) {
        move(pt1, orient, -multiplier * bloating);
      } else {
        move(pt1, orientp, multiplier * bloating);
      }
    }
    return pt1;
  }

  template <typename Unit>
  inline
  void handleResizingVertex45(polygon_45_set_data<Unit>& sizingSet, const point_data<Unit>& first,
                              const point_data<Unit>& second, const point_data<Unit>& third, Unit resizing,
                              RoundingOption rounding, CornerOption corner,
                              int multiplier) {
    unsigned int edge1 = getEdge45Direction(first, second);
    unsigned int edge2 = getEdge45Direction(second, third);
    unsigned int diffAngle;
    if(multiplier < 0)
      diffAngle = (edge2 + 8 - edge1) % 8;
    else
      diffAngle = (edge1 + 8 - edge2) % 8;
    if(diffAngle < 4) {
      if(resizing > 0) return; //accute interior corner
      else multiplier *= -1; //make it appear to be an accute exterior angle
    }
    Unit bloating = local_abs(resizing);
    if(rounding == SQRT1OVER2) {
      if(edge1 % 2 && edge2 % 2) return;
      if(corner == ORTHOGONAL && edge1 % 2 == 0 && edge2 % 2 == 0) {
        rectangle_data<Unit> insertion_rect;
        set_points(insertion_rect, second, second);
        bloat(insertion_rect, bloating);
        sizingSet.insert(insertion_rect);
      } else if(corner != ORTHOGONAL) {
        point_data<Unit> pt1(0, 0);
        point_data<Unit> pt2(0, 0);
        unsigned int normal1 = getEdge45NormalDirection(edge1, multiplier);
        unsigned int normal2 = getEdge45NormalDirection(edge2, multiplier);
        if(edge1 % 2) {
          pt1 = bloatVertexInDirWithSQRT1OVER2(edge1, normal1, second, bloating, true);
        } else {
          pt1 = bloatVertexInDirWithOptions(second, normal1, bloating, UNDERSIZE);
        }
        if(edge2 % 2) {
          pt2 = bloatVertexInDirWithSQRT1OVER2(edge2, normal2, second, bloating, false);
        } else {
          pt2 = bloatVertexInDirWithOptions(second, normal2, bloating, UNDERSIZE);
        }
        std::vector<point_data<Unit> > pts;
        pts.push_back(pt1);
        pts.push_back(second);
        pts.push_back(pt2);
        pts.push_back(getIntersectionPoint(pt1, edge1, pt2, edge2));
        polygon_45_data<Unit> poly(pts.begin(), pts.end());
        sizingSet.insert(poly);
      } else {
        //ORTHOGONAL of a 45 degree corner
        int normal = 0;
        if(edge1 % 2) {
          normal = getEdge45NormalDirection(edge2, multiplier);
        } else {
          normal = getEdge45NormalDirection(edge1, multiplier);
        }
        rectangle_data<Unit> insertion_rect;
        point_data<Unit> edgePoint = bloatVertexInDirWithOptions(second, normal, bloating, UNDERSIZE);
        set_points(insertion_rect, second, edgePoint);
        if(normal == 0 || normal == 4)
          bloat(insertion_rect, VERTICAL, bloating);
        else
          bloat(insertion_rect, HORIZONTAL, bloating);
        sizingSet.insert(insertion_rect);
      }
      return;
    }
    unsigned int normal1 = getEdge45NormalDirection(edge1, multiplier);
    unsigned int normal2 = getEdge45NormalDirection(edge2, multiplier);
    point_data<Unit> edgePoint1 = bloatVertexInDirWithOptions(second, normal1, bloating, rounding);
    point_data<Unit> edgePoint2 = bloatVertexInDirWithOptions(second, normal2, bloating, rounding);
    //if the change in angle is 135 degrees it is an accute exterior corner
    if((edge1+ multiplier * 3) % 8 == edge2) {
      if(corner == ORTHOGONAL) {
        rectangle_data<Unit> insertion_rect;
        set_points(insertion_rect, edgePoint1, edgePoint2);
        sizingSet.insert(insertion_rect);
        return;
      }
    }
    std::vector<point_data<Unit> > pts;
    pts.push_back(edgePoint1);
    pts.push_back(second);
    pts.push_back(edgePoint2);
    pts.push_back(getIntersectionPoint(edgePoint1, edge1, edgePoint2, edge2));
    polygon_45_data<Unit> poly(pts.begin(), pts.end());
    sizingSet.insert(poly);
  }

  template <typename Unit>
  template <typename geometry_type>
  inline polygon_45_set_data<Unit>&
  polygon_45_set_data<Unit>::insert_with_resize_dispatch(const geometry_type& poly,
                                                         coordinate_type resizing,
                                                         RoundingOption rounding,
                                                         CornerOption corner,
                                                         bool hole, polygon_45_concept ) {
    direction_1d wdir = winding(poly);
    int multiplier = wdir == LOW ? -1 : 1;
    if(hole) resizing *= -1;
    typedef typename polygon_45_data<Unit>::iterator_type piterator;
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
    polygon_45_set_data<Unit> sizingSet;
    //insert minkofski shapes on edges and corners
    do {
      if(rounding != SQRT1OVER2) {
        handleResizingEdge45(sizingSet, *first, *second, resizing, rounding);
      } else {
        handleResizingEdge45_SQRT1OVER2(sizingSet, *first, *second, resizing, corner);
      }
      if(corner != UNFILLED)
        handleResizingVertex45(sizingSet, *first, *second, *third, resizing, rounding, corner, multiplier);
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
    //sizingSet.snap();
    polygon_45_set_data<Unit> tmp;
    //insert original shape
    tmp.insert_dispatch(poly, false, polygon_45_concept());
    if(resizing < 0) tmp -= sizingSet;
    else tmp += sizingSet;
    tmp.clean();
    insert(tmp, hole);
    dirty_ = true;
    unsorted_ = true;
    return (*this);
  }

  // accumulate the bloated polygon with holes
  template <typename Unit>
  template <typename geometry_type>
  inline polygon_45_set_data<Unit>&
  polygon_45_set_data<Unit>::insert_with_resize_dispatch(const geometry_type& poly,
                                                         coordinate_type resizing,
                                                         RoundingOption rounding,
                                                         CornerOption corner,
                                                         bool hole, polygon_45_with_holes_concept ) {
    insert_with_resize_dispatch(poly, resizing, rounding, corner, hole, polygon_45_concept());
    for(typename polygon_with_holes_traits<geometry_type>::iterator_holes_type itr =
          begin_holes(poly); itr != end_holes(poly);
        ++itr) {
      insert_with_resize_dispatch(*itr, resizing, rounding, corner, !hole, polygon_45_concept());
    }
    return *this;
  }

  // transform set
  template <typename Unit>
  template <typename transformation_type>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::transform(const transformation_type& tr){
    clean();
    std::vector<polygon_45_with_holes_data<Unit> > polys;
    get(polys);
    for(typename std::vector<polygon_45_with_holes_data<Unit> >::iterator itr = polys.begin();
        itr != polys.end(); ++itr) {
      ::boost::polygon::transform(*itr, tr);
    }
    clear();
    insert(polys.begin(), polys.end());
    dirty_ = true;
    unsorted_ = true;
    return *this;
  }

  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::scale_up(typename coordinate_traits<Unit>::unsigned_area_type factor) {
    scale_up_vertex_45_compact_range(data_.begin(), data_.end(), factor);
    return *this;
  }

  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::scale_down(typename coordinate_traits<Unit>::unsigned_area_type factor) {
    clean();
    std::vector<polygon_45_with_holes_data<Unit> > polys;
    get_polygons_with_holes(polys);
    for(typename std::vector<polygon_45_with_holes_data<Unit> >::iterator itr = polys.begin();
        itr != polys.end(); ++itr) {
      ::boost::polygon::scale_down(*itr, factor);
    }
    clear();
    insert(polys.begin(), polys.end());
    dirty_ = true;
    unsorted_ = true;
    return *this;
  }

  template <typename Unit>
  inline polygon_45_set_data<Unit>& polygon_45_set_data<Unit>::scale(double factor) {
    clean();
    std::vector<polygon_45_with_holes_data<Unit> > polys;
    get_polygons_with_holes(polys);
    for(typename std::vector<polygon_45_with_holes_data<Unit> >::iterator itr = polys.begin();
        itr != polys.end(); ++itr) {
      ::boost::polygon::scale(*itr, factor);
    }
    clear();
    insert(polys.begin(), polys.end());
    dirty_ = true;
    unsorted_ = true;
    return *this;
  }

  template <typename Unit>
  inline bool polygon_45_set_data<Unit>::clean() const {
    if(unsorted_) sort();
    if(dirty_) {
      applyAdaptiveUnary_<0>();
      dirty_ = false;
    }
    return true;
  }

  template <typename Unit>
  template <int op>
  inline void polygon_45_set_data<Unit>::applyAdaptiveBoolean_(const polygon_45_set_data<Unit>& rvalue) const {
    polygon_45_set_data<Unit> tmp;
    applyAdaptiveBoolean_<op>(tmp, rvalue);
    data_.swap(tmp.data_); //swapping vectors should be constant time operation
    error_data_.swap(tmp.error_data_);
    is_manhattan_ = tmp.is_manhattan_;
    unsorted_ = false;
    dirty_ = false;
  }

  template <typename Unit2, int op>
  bool applyBoolean45OpOnVectors(std::vector<typename polygon_45_formation<Unit2>::Vertex45Compact>& result_data,
                                 std::vector<typename polygon_45_formation<Unit2>::Vertex45Compact>& lvalue_data,
                                 std::vector<typename polygon_45_formation<Unit2>::Vertex45Compact>& rvalue_data
                                 ) {
    bool result_is_manhattan_ = true;
    typename boolean_op_45<Unit2>::template Scan45<typename boolean_op_45<Unit2>::Count2,
      typename boolean_op_45<Unit2>::template boolean_op_45_output_functor<op> > scan45;
    std::vector<typename boolean_op_45<Unit2>::Vertex45> eventOut;
    typedef std::pair<typename boolean_op_45<Unit2>::Point,
      typename boolean_op_45<Unit2>::template Scan45CountT<typename boolean_op_45<Unit2>::Count2> > Scan45Vertex;
    std::vector<Scan45Vertex> eventIn;
    typedef std::vector<typename polygon_45_formation<Unit2>::Vertex45Compact> value_type;
    typename value_type::const_iterator iter1 = lvalue_data.begin();
    typename value_type::const_iterator iter2 = rvalue_data.begin();
    typename value_type::const_iterator end1 = lvalue_data.end();
    typename value_type::const_iterator end2 = rvalue_data.end();
    const Unit2 UnitMax = (std::numeric_limits<Unit2>::max)();
    Unit2 x = UnitMax;
    while(iter1 != end1 || iter2 != end2) {
      Unit2 currentX = UnitMax;
      if(iter1 != end1) currentX = iter1->pt.x();
      if(iter2 != end2) currentX = (std::min)(currentX, iter2->pt.x());
      if(currentX != x) {
        //std::cout << "SCAN " << currentX << "\n";
        //scan event
        scan45.scan(eventOut, eventIn.begin(), eventIn.end());
        polygon_sort(eventOut.begin(), eventOut.end());
        std::size_t ptCount = 0;
        for(std::size_t i = 0; i < eventOut.size(); ++i) {
          if(!result_data.empty() &&
             result_data.back().pt == eventOut[i].pt) {
            result_data.back().count += eventOut[i];
            ++ptCount;
          } else {
            if(!result_data.empty()) {
              if(result_data.back().count.is_45()) {
                result_is_manhattan_ = false;
              }
              if(ptCount == 2 && result_data.back().count == (typename polygon_45_formation<Unit2>::Vertex45Count(0, 0, 0, 0))) {
                result_data.pop_back();
              }
            }
            result_data.push_back(eventOut[i]);
            ptCount = 1;
          }
        }
        if(ptCount == 2 && result_data.back().count == (typename polygon_45_formation<Unit2>::Vertex45Count(0, 0, 0, 0))) {
          result_data.pop_back();
        }
        eventOut.clear();
        eventIn.clear();
        x = currentX;
      }
      //std::cout << "get next\n";
      if(iter2 != end2 && (iter1 == end1 || iter2->pt.x() < iter1->pt.x() ||
                           (iter2->pt.x() == iter1->pt.x() &&
                            iter2->pt.y() < iter1->pt.y()) )) {
        //std::cout << "case1 next\n";
        eventIn.push_back(Scan45Vertex
                          (iter2->pt,
                           typename polygon_45_formation<Unit2>::
                           Scan45Count(typename polygon_45_formation<Unit2>::Count2(0, iter2->count[0]),
                                       typename polygon_45_formation<Unit2>::Count2(0, iter2->count[1]),
                                       typename polygon_45_formation<Unit2>::Count2(0, iter2->count[2]),
                                       typename polygon_45_formation<Unit2>::Count2(0, iter2->count[3]))));
        ++iter2;
      } else if(iter1 != end1 && (iter2 == end2 || iter1->pt.x() < iter2->pt.x() ||
                                  (iter1->pt.x() == iter2->pt.x() &&
                                   iter1->pt.y() < iter2->pt.y()) )) {
        //std::cout << "case2 next\n";
        eventIn.push_back(Scan45Vertex
                          (iter1->pt,
                           typename polygon_45_formation<Unit2>::
                           Scan45Count(
                                       typename polygon_45_formation<Unit2>::Count2(iter1->count[0], 0),
                                       typename polygon_45_formation<Unit2>::Count2(iter1->count[1], 0),
                                       typename polygon_45_formation<Unit2>::Count2(iter1->count[2], 0),
                                       typename polygon_45_formation<Unit2>::Count2(iter1->count[3], 0))));
        ++iter1;
      } else {
        //std::cout << "case3 next\n";
        eventIn.push_back(Scan45Vertex
                          (iter2->pt,
                           typename polygon_45_formation<Unit2>::
                           Scan45Count(typename polygon_45_formation<Unit2>::Count2(iter1->count[0],
                                                                                    iter2->count[0]),
                                       typename polygon_45_formation<Unit2>::Count2(iter1->count[1],
                                                                                    iter2->count[1]),
                                       typename polygon_45_formation<Unit2>::Count2(iter1->count[2],
                                                                                    iter2->count[2]),
                                       typename polygon_45_formation<Unit2>::Count2(iter1->count[3],
                                                                                    iter2->count[3]))));
        ++iter1;
        ++iter2;
      }
    }
    scan45.scan(eventOut, eventIn.begin(), eventIn.end());
    polygon_sort(eventOut.begin(), eventOut.end());

    std::size_t ptCount = 0;
    for(std::size_t i = 0; i < eventOut.size(); ++i) {
      if(!result_data.empty() &&
         result_data.back().pt == eventOut[i].pt) {
        result_data.back().count += eventOut[i];
        ++ptCount;
      } else {
        if(!result_data.empty()) {
          if(result_data.back().count.is_45()) {
            result_is_manhattan_ = false;
          }
          if(ptCount == 2 && result_data.back().count == (typename polygon_45_formation<Unit2>::Vertex45Count(0, 0, 0, 0))) {
            result_data.pop_back();
          }
        }
        result_data.push_back(eventOut[i]);
        ptCount = 1;
      }
    }
    if(ptCount == 2 && result_data.back().count == (typename polygon_45_formation<Unit2>::Vertex45Count(0, 0, 0, 0))) {
      result_data.pop_back();
    }
    if(!result_data.empty() &&
       result_data.back().count.is_45()) {
      result_is_manhattan_ = false;
    }
    return result_is_manhattan_;
  }

  template <typename Unit2, int op>
  bool applyUnary45OpOnVectors(std::vector<typename polygon_45_formation<Unit2>::Vertex45Compact>& result_data,
                                 std::vector<typename polygon_45_formation<Unit2>::Vertex45Compact>& lvalue_data ) {
    bool result_is_manhattan_ = true;
    typename boolean_op_45<Unit2>::template Scan45<typename boolean_op_45<Unit2>::Count1,
      typename boolean_op_45<Unit2>::template unary_op_45_output_functor<op> > scan45;
    std::vector<typename boolean_op_45<Unit2>::Vertex45> eventOut;
    typedef typename boolean_op_45<Unit2>::template Scan45CountT<typename boolean_op_45<Unit2>::Count1> Scan45Count;
    typedef std::pair<typename boolean_op_45<Unit2>::Point, Scan45Count> Scan45Vertex;
    std::vector<Scan45Vertex> eventIn;
    typedef std::vector<typename polygon_45_formation<Unit2>::Vertex45Compact> value_type;
    typename value_type::const_iterator iter1 = lvalue_data.begin();
    typename value_type::const_iterator end1 = lvalue_data.end();
    const Unit2 UnitMax = (std::numeric_limits<Unit2>::max)();
    Unit2 x = UnitMax;
    while(iter1 != end1) {
      Unit2 currentX = iter1->pt.x();
      if(currentX != x) {
        //std::cout << "SCAN " << currentX << "\n";
        //scan event
        scan45.scan(eventOut, eventIn.begin(), eventIn.end());
        polygon_sort(eventOut.begin(), eventOut.end());
        std::size_t ptCount = 0;
        for(std::size_t i = 0; i < eventOut.size(); ++i) {
          if(!result_data.empty() &&
             result_data.back().pt == eventOut[i].pt) {
            result_data.back().count += eventOut[i];
            ++ptCount;
          } else {
            if(!result_data.empty()) {
              if(result_data.back().count.is_45()) {
                result_is_manhattan_ = false;
              }
              if(ptCount == 2 && result_data.back().count == (typename polygon_45_formation<Unit2>::Vertex45Count(0, 0, 0, 0))) {
                result_data.pop_back();
              }
            }
            result_data.push_back(eventOut[i]);
            ptCount = 1;
          }
        }
        if(ptCount == 2 && result_data.back().count == (typename polygon_45_formation<Unit2>::Vertex45Count(0, 0, 0, 0))) {
          result_data.pop_back();
        }
        eventOut.clear();
        eventIn.clear();
        x = currentX;
      }
      //std::cout << "get next\n";
      eventIn.push_back(Scan45Vertex
                        (iter1->pt,
                         Scan45Count( typename boolean_op_45<Unit2>::Count1(iter1->count[0]),
                                      typename boolean_op_45<Unit2>::Count1(iter1->count[1]),
                                      typename boolean_op_45<Unit2>::Count1(iter1->count[2]),
                                      typename boolean_op_45<Unit2>::Count1(iter1->count[3]))));
      ++iter1;
    }
    scan45.scan(eventOut, eventIn.begin(), eventIn.end());
    polygon_sort(eventOut.begin(), eventOut.end());

    std::size_t ptCount = 0;
    for(std::size_t i = 0; i < eventOut.size(); ++i) {
      if(!result_data.empty() &&
         result_data.back().pt == eventOut[i].pt) {
        result_data.back().count += eventOut[i];
        ++ptCount;
      } else {
        if(!result_data.empty()) {
          if(result_data.back().count.is_45()) {
            result_is_manhattan_ = false;
          }
          if(ptCount == 2 && result_data.back().count == (typename polygon_45_formation<Unit2>::Vertex45Count(0, 0, 0, 0))) {
            result_data.pop_back();
          }
        }
        result_data.push_back(eventOut[i]);
        ptCount = 1;
      }
    }
    if(ptCount == 2 && result_data.back().count == (typename polygon_45_formation<Unit2>::Vertex45Count(0, 0, 0, 0))) {
      result_data.pop_back();
    }
    if(!result_data.empty() &&
       result_data.back().count.is_45()) {
      result_is_manhattan_ = false;
    }
    return result_is_manhattan_;
  }

  template <typename cT, typename iT>
  void get_error_rects_shell(cT& posE, cT& negE, iT beginr, iT endr) {
    typedef typename std::iterator_traits<iT>::value_type Point;
    typedef typename point_traits<Point>::coordinate_type Unit;
    typedef typename coordinate_traits<Unit>::area_type area_type;
    Point pt1, pt2, pt3;
    bool i1 = true;
    bool i2 = true;
    bool not_done = beginr != endr;
    bool next_to_last = false;
    bool last = false;
    Point first, second;
    while(not_done) {
      if(last) {
        last = false;
        not_done = false;
        pt3 = second;
      } else if(next_to_last) {
        next_to_last = false;
        last = true;
        pt3 = first;
      } else if(i1) {
        const Point& pt = *beginr;
        first = pt1 = pt;
        i1 = false;
        i2 = true;
        ++beginr;
        if(beginr == endr) return; //too few points
        continue;
      } else if (i2) {
        const Point& pt = *beginr;
        second = pt2 = pt;
        i2 = false;
        ++beginr;
        if(beginr == endr) return; //too few points
        continue;
      } else {
        const Point& pt = *beginr;
        pt3 = pt;
        ++beginr;
        if(beginr == endr) {
          next_to_last = true;
          //skip last point equal to first
          continue;
        }
      }
      if(local_abs(x(pt2)) % 2) { //y % 2 should also be odd
        //is corner concave or convex?
        Point pts[] = {pt1, pt2, pt3};
        area_type ar = point_sequence_area<Point*, area_type>(pts, pts+3);
        direction_1d dir = ar < 0 ? COUNTERCLOCKWISE : CLOCKWISE;
        //std::cout << pt1 << " " << pt2 << " " << pt3 << " " << ar << std::endl;
        if(dir == CLOCKWISE) {
          posE.push_back(rectangle_data<typename Point::coordinate_type>
                         (x(pt2) - 1, y(pt2) - 1, x(pt2) + 1, y(pt2) + 1));

        } else {
          negE.push_back(rectangle_data<typename Point::coordinate_type>
                         (x(pt2) - 1, y(pt2) - 1, x(pt2) + 1, y(pt2) + 1));
        }
      }
      pt1 = pt2;
      pt2 = pt3;
    }
  }

  template <typename cT, typename pT>
  void get_error_rects(cT& posE, cT& negE, const pT& p) {
    get_error_rects_shell(posE, negE, p.begin(), p.end());
    for(typename pT::iterator_holes_type iHb = p.begin_holes();
        iHb != p.end_holes(); ++iHb) {
      get_error_rects_shell(posE, negE, iHb->begin(), iHb->end());
    }
  }

  template <typename Unit>
  template <int op>
  inline void polygon_45_set_data<Unit>::applyAdaptiveBoolean_(polygon_45_set_data<Unit>& result,
                                                               const polygon_45_set_data<Unit>& rvalue) const {
    result.clear();
    result.error_data_ = error_data_;
    result.error_data_.insert(result.error_data_.end(), rvalue.error_data_.begin(),
                              rvalue.error_data_.end());
    if(is_manhattan() && rvalue.is_manhattan()) {
      //convert each into polygon_90_set data and call boolean operations
      polygon_90_set_data<Unit> l90sd(VERTICAL), r90sd(VERTICAL), output(VERTICAL);
      for(typename value_type::const_iterator itr = data_.begin(); itr != data_.end(); ++itr) {
        if((*itr).count[3] == 0) continue; //skip all non vertical edges
        l90sd.insert(std::make_pair((*itr).pt.x(), std::make_pair<Unit, int>((*itr).pt.y(), (*itr).count[3])), false, VERTICAL);
      }
      for(typename value_type::const_iterator itr = rvalue.data_.begin(); itr != rvalue.data_.end(); ++itr) {
        if((*itr).count[3] == 0) continue; //skip all non vertical edges
        r90sd.insert(std::make_pair((*itr).pt.x(), std::make_pair<Unit, int>((*itr).pt.y(), (*itr).count[3])), false, VERTICAL);
      }
      l90sd.sort();
      r90sd.sort();
#ifdef BOOST_POLYGON_MSVC
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
      if(op == 0) {
        output.applyBooleanBinaryOp(l90sd.begin(), l90sd.end(),
                                    r90sd.begin(), r90sd.end(), boolean_op::BinaryCount<boolean_op::BinaryOr>());
      } else if (op == 1) {
        output.applyBooleanBinaryOp(l90sd.begin(), l90sd.end(),
                                    r90sd.begin(), r90sd.end(), boolean_op::BinaryCount<boolean_op::BinaryAnd>());
      } else if (op == 2) {
        output.applyBooleanBinaryOp(l90sd.begin(), l90sd.end(),
                                    r90sd.begin(), r90sd.end(), boolean_op::BinaryCount<boolean_op::BinaryNot>());
      } else if (op == 3) {
        output.applyBooleanBinaryOp(l90sd.begin(), l90sd.end(),
                                    r90sd.begin(), r90sd.end(), boolean_op::BinaryCount<boolean_op::BinaryXor>());
      }
#ifdef BOOST_POLYGON_MSVC
#pragma warning (pop)
#endif
      result.data_.clear();
      result.insert(output);
      result.is_manhattan_ = true;
      result.dirty_ = false;
      result.unsorted_ = false;
    } else {
      sort();
      rvalue.sort();
      try {
        result.is_manhattan_ = applyBoolean45OpOnVectors<Unit, op>(result.data_, data_, rvalue.data_);
      } catch (std::string str) {
        std::string msg = "GTL 45 Boolean error, precision insufficient to represent edge intersection coordinate value.";
        if(str == msg) {
          result.clear();
          typedef typename coordinate_traits<Unit>::manhattan_area_type Unit2;
          typedef typename polygon_45_formation<Unit2>::Vertex45Compact Vertex45Compact2;
          typedef std::vector<Vertex45Compact2> Data2;
          Data2 rvalue_data, lvalue_data, result_data;
          rvalue_data.reserve(rvalue.data_.size());
          lvalue_data.reserve(data_.size());
          for(std::size_t i = 0 ; i < data_.size(); ++i) {
            const Vertex45Compact& vi = data_[i];
            Vertex45Compact2 ci;
            ci.pt = point_data<Unit2>(x(vi.pt), y(vi.pt));
            ci.count = typename polygon_45_formation<Unit2>::Vertex45Count
              ( vi.count[0], vi.count[1], vi.count[2], vi.count[3]);
            lvalue_data.push_back(ci);
          }
          for(std::size_t i = 0 ; i < rvalue.data_.size(); ++i) {
            const Vertex45Compact& vi = rvalue.data_[i];
            Vertex45Compact2 ci;
            ci.pt = (point_data<Unit2>(x(vi.pt), y(vi.pt)));
            ci.count = typename polygon_45_formation<Unit2>::Vertex45Count
              ( vi.count[0], vi.count[1], vi.count[2], vi.count[3]);
            rvalue_data.push_back(ci);
          }
          scale_up_vertex_45_compact_range(lvalue_data.begin(), lvalue_data.end(), 2);
          scale_up_vertex_45_compact_range(rvalue_data.begin(), rvalue_data.end(), 2);
          bool result_is_manhattan = applyBoolean45OpOnVectors<Unit2, op>(result_data,
                                                                          lvalue_data,
                                                                          rvalue_data );
          if(!result_is_manhattan) {
            typename polygon_45_formation<Unit2>::Polygon45Formation pf(false);
            //std::cout << "FORMING POLYGONS\n";
            std::vector<polygon_45_with_holes_data<Unit2> > container;
            pf.scan(container, result_data.begin(), result_data.end());
            Data2 error_data_out;
            std::vector<rectangle_data<Unit2> > pos_error_rects;
            std::vector<rectangle_data<Unit2> > neg_error_rects;
            for(std::size_t i = 0; i < container.size(); ++i) {
              get_error_rects(pos_error_rects, neg_error_rects, container[i]);
            }
            for(std::size_t i = 0; i < pos_error_rects.size(); ++i) {
              insert_rectangle_into_vector_45(result_data, pos_error_rects[i], false);
              insert_rectangle_into_vector_45(error_data_out, pos_error_rects[i], false);
            }
            for(std::size_t i = 0; i < neg_error_rects.size(); ++i) {
              insert_rectangle_into_vector_45(result_data, neg_error_rects[i], true);
              insert_rectangle_into_vector_45(error_data_out, neg_error_rects[i], false);
            }
            scale_down_vertex_45_compact_range_blindly(error_data_out.begin(), error_data_out.end(), 2);
            for(std::size_t i = 0 ; i < error_data_out.size(); ++i) {
              const Vertex45Compact2& vi = error_data_out[i];
              Vertex45Compact ci;
              ci.pt.x(static_cast<Unit>(x(vi.pt)));
              ci.pt.y(static_cast<Unit>(y(vi.pt)));
              ci.count = typename polygon_45_formation<Unit>::Vertex45Count
              ( vi.count[0], vi.count[1], vi.count[2], vi.count[3]);
              result.error_data_.push_back(ci);
            }
            Data2 new_result_data;
            polygon_sort(result_data.begin(), result_data.end());
            applyUnary45OpOnVectors<Unit2, 0>(new_result_data, result_data); //OR operation
            result_data.swap(new_result_data);
          }
          scale_down_vertex_45_compact_range_blindly(result_data.begin(), result_data.end(), 2);
          //result.data_.reserve(result_data.size());
          for(std::size_t i = 0 ; i < result_data.size(); ++i) {
            const Vertex45Compact2& vi = result_data[i];
            Vertex45Compact ci;
            ci.pt.x(static_cast<Unit>(x(vi.pt)));
            ci.pt.y(static_cast<Unit>(y(vi.pt)));
            ci.count = typename polygon_45_formation<Unit>::Vertex45Count
              ( vi.count[0], vi.count[1], vi.count[2], vi.count[3]);
            result.data_.push_back(ci);
          }
          result.is_manhattan_ = result_is_manhattan;
          result.dirty_ = false;
          result.unsorted_ = false;
        } else { throw str; }
      }
      //std::cout << "DONE SCANNING\n";
    }
  }

  template <typename Unit>
  template <int op>
  inline void polygon_45_set_data<Unit>::applyAdaptiveUnary_() const {
    polygon_45_set_data<Unit> result;
    result.error_data_ = error_data_;
    if(is_manhattan()) {
      //convert each into polygon_90_set data and call boolean operations
      polygon_90_set_data<Unit> l90sd(VERTICAL);
      for(typename value_type::const_iterator itr = data_.begin(); itr != data_.end(); ++itr) {
        if((*itr).count[3] == 0) continue; //skip all non vertical edges
        l90sd.insert(std::make_pair((*itr).pt.x(), std::make_pair<Unit, int>((*itr).pt.y(), (*itr).count[3])), false, VERTICAL);
      }
      l90sd.sort();
#ifdef BOOST_POLYGON_MSVC
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
      if(op == 0) {
        l90sd.clean();
      } else if (op == 1) {
        l90sd.self_intersect();
      } else if (op == 3) {
        l90sd.self_xor();
      }
#ifdef BOOST_POLYGON_MSVC
#pragma warning (pop)
#endif
      result.data_.clear();
      result.insert(l90sd);
      result.is_manhattan_ = true;
      result.dirty_ = false;
      result.unsorted_ = false;
    } else {
      sort();
      try {
        result.is_manhattan_ = applyUnary45OpOnVectors<Unit, op>(result.data_, data_);
      } catch (std::string str) {
        std::string msg = "GTL 45 Boolean error, precision insufficient to represent edge intersection coordinate value.";
        if(str == msg) {
          result.clear();
          typedef typename coordinate_traits<Unit>::manhattan_area_type Unit2;
          typedef typename polygon_45_formation<Unit2>::Vertex45Compact Vertex45Compact2;
          typedef std::vector<Vertex45Compact2> Data2;
          Data2 lvalue_data, result_data;
          lvalue_data.reserve(data_.size());
          for(std::size_t i = 0 ; i < data_.size(); ++i) {
            const Vertex45Compact& vi = data_[i];
            Vertex45Compact2 ci;
            ci.pt.x(static_cast<Unit>(x(vi.pt)));
            ci.pt.y(static_cast<Unit>(y(vi.pt)));
            ci.count = typename polygon_45_formation<Unit2>::Vertex45Count
              ( vi.count[0], vi.count[1], vi.count[2], vi.count[3]);
            lvalue_data.push_back(ci);
          }
          scale_up_vertex_45_compact_range(lvalue_data.begin(), lvalue_data.end(), 2);
          bool result_is_manhattan = applyUnary45OpOnVectors<Unit2, op>(result_data,
                                                                        lvalue_data );
          if(!result_is_manhattan) {
            typename polygon_45_formation<Unit2>::Polygon45Formation pf(false);
            //std::cout << "FORMING POLYGONS\n";
            std::vector<polygon_45_with_holes_data<Unit2> > container;
            pf.scan(container, result_data.begin(), result_data.end());
            Data2 error_data_out;
            std::vector<rectangle_data<Unit2> > pos_error_rects;
            std::vector<rectangle_data<Unit2> > neg_error_rects;
            for(std::size_t i = 0; i < container.size(); ++i) {
              get_error_rects(pos_error_rects, neg_error_rects, container[i]);
            }
            for(std::size_t i = 0; i < pos_error_rects.size(); ++i) {
              insert_rectangle_into_vector_45(result_data, pos_error_rects[i], false);
              insert_rectangle_into_vector_45(error_data_out, pos_error_rects[i], false);
            }
            for(std::size_t i = 0; i < neg_error_rects.size(); ++i) {
              insert_rectangle_into_vector_45(result_data, neg_error_rects[i], true);
              insert_rectangle_into_vector_45(error_data_out, neg_error_rects[i], false);
            }
            scale_down_vertex_45_compact_range_blindly(error_data_out.begin(), error_data_out.end(), 2);
            for(std::size_t i = 0 ; i < error_data_out.size(); ++i) {
              const Vertex45Compact2& vi = error_data_out[i];
              Vertex45Compact ci;
              ci.pt.x(static_cast<Unit>(x(vi.pt)));
              ci.pt.y(static_cast<Unit>(y(vi.pt)));
              ci.count = typename polygon_45_formation<Unit>::Vertex45Count
              ( vi.count[0], vi.count[1], vi.count[2], vi.count[3]);
              result.error_data_.push_back(ci);
            }
            Data2 new_result_data;
            polygon_sort(result_data.begin(), result_data.end());
            applyUnary45OpOnVectors<Unit2, 0>(new_result_data, result_data); //OR operation
            result_data.swap(new_result_data);
          }
          scale_down_vertex_45_compact_range_blindly(result_data.begin(), result_data.end(), 2);
          //result.data_.reserve(result_data.size());
          for(std::size_t i = 0 ; i < result_data.size(); ++i) {
            const Vertex45Compact2& vi = result_data[i];
            Vertex45Compact ci;
            ci.pt.x(static_cast<Unit>(x(vi.pt)));
            ci.pt.y(static_cast<Unit>(y(vi.pt)));
            ci.count = typename polygon_45_formation<Unit>::Vertex45Count
              ( vi.count[0], vi.count[1], vi.count[2], vi.count[3]);
            result.data_.push_back(ci);
          }
          result.is_manhattan_ = result_is_manhattan;
          result.dirty_ = false;
          result.unsorted_ = false;
        } else { throw str; }
      }
      //std::cout << "DONE SCANNING\n";
    }
    data_.swap(result.data_);
    error_data_.swap(result.error_data_);
    dirty_ = result.dirty_;
    unsorted_ = result.unsorted_;
    is_manhattan_ = result.is_manhattan_;
  }

  template <typename coordinate_type, typename property_type>
  class property_merge_45 {
  private:
    typedef typename coordinate_traits<coordinate_type>::manhattan_area_type big_coord;
    typedef typename polygon_45_property_merge<big_coord, property_type>::MergeSetData tsd;
    tsd tsd_;
  public:
    inline property_merge_45() : tsd_() {}
    inline property_merge_45(const property_merge_45& that) : tsd_(that.tsd_) {}
    inline property_merge_45& operator=(const property_merge_45& that) {
      tsd_ = that.tsd_;
      return *this;
    }

    inline void insert(const polygon_45_set_data<coordinate_type>& ps, property_type property) {
      ps.clean();
      polygon_45_property_merge<big_coord, property_type>::populateMergeSetData(tsd_, ps.begin(), ps.end(), property);
    }
    template <class GeoObjT>
    inline void insert(const GeoObjT& geoObj, property_type property) {
      polygon_45_set_data<coordinate_type> ps;
      ps.insert(geoObj);
      insert(ps, property);
    }

    //merge properties of input geometries and store the resulting geometries of regions
    //with unique sets of merged properties to polygons sets in a map keyed by sets of properties
    // T = std::map<std::set<property_type>, polygon_45_set_data<coordiante_type> > or
    // T = std::map<std::vector<property_type>, polygon_45_set_data<coordiante_type> >
    template <class result_type>
    inline void merge(result_type& result) {
      typedef typename result_type::key_type keytype;
      typedef std::map<keytype, polygon_45_set_data<big_coord> > bigtype;
      bigtype result_big;
      polygon_45_property_merge<big_coord, property_type>::performMerge(result_big, tsd_);
      std::vector<polygon_45_with_holes_data<big_coord> > polys;
      std::vector<rectangle_data<big_coord> > pos_error_rects;
      std::vector<rectangle_data<big_coord> > neg_error_rects;
      for(typename std::map<keytype, polygon_45_set_data<big_coord> >::iterator itr = result_big.begin();
          itr != result_big.end(); ++itr) {
        polys.clear();
        (*itr).second.get(polys);
        for(std::size_t i = 0; i < polys.size(); ++i) {
          get_error_rects(pos_error_rects, neg_error_rects, polys[i]);
        }
        (*itr).second += pos_error_rects;
        (*itr).second -= neg_error_rects;
        (*itr).second.scale_down(2);
        result[(*itr).first].insert((*itr).second);
      }
    }
  };

  //ConnectivityExtraction computes the graph of connectivity between rectangle, polygon and
  //polygon set graph nodes where an edge is created whenever the geometry in two nodes overlap
  template <typename coordinate_type>
  class connectivity_extraction_45 {
  private:
    typedef typename coordinate_traits<coordinate_type>::manhattan_area_type big_coord;
    typedef typename polygon_45_touch<big_coord>::TouchSetData tsd;
    tsd tsd_;
    unsigned int nodeCount_;
  public:
    inline connectivity_extraction_45() : tsd_(), nodeCount_(0) {}
    inline connectivity_extraction_45(const connectivity_extraction_45& that) : tsd_(that.tsd_),
                                                                          nodeCount_(that.nodeCount_) {}
    inline connectivity_extraction_45& operator=(const connectivity_extraction_45& that) {
      tsd_ = that.tsd_;
      nodeCount_ = that.nodeCount_; {}
      return *this;
    }

    //insert a polygon set graph node, the value returned is the id of the graph node
    inline unsigned int insert(const polygon_45_set_data<coordinate_type>& ps) {
      ps.clean();
      polygon_45_touch<big_coord>::populateTouchSetData(tsd_, ps.begin(), ps.end(), nodeCount_);
      return nodeCount_++;
    }
    template <class GeoObjT>
    inline unsigned int insert(const GeoObjT& geoObj) {
      polygon_45_set_data<coordinate_type> ps;
      ps.insert(geoObj);
      return insert(ps);
    }

    //extract connectivity and store the edges in the graph
    //graph must be indexable by graph node id and the indexed value must be a std::set of
    //graph node id
    template <class GraphT>
    inline void extract(GraphT& graph) {
      polygon_45_touch<big_coord>::performTouch(graph, tsd_);
    }
  };
}
}
#endif


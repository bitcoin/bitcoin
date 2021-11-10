/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_90_SET_TRAITS_HPP
#define BOOST_POLYGON_POLYGON_90_SET_TRAITS_HPP
namespace boost { namespace polygon{

  struct polygon_90_set_concept {};

  template <typename T, typename T2>
  struct traits_by_concept {};
  template <typename T>
  struct traits_by_concept<T, coordinate_concept> { typedef coordinate_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, interval_concept> { typedef interval_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, point_concept> { typedef point_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, rectangle_concept> { typedef rectangle_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, segment_concept> { typedef segment_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, polygon_90_concept> { typedef polygon_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, polygon_90_with_holes_concept> { typedef polygon_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, polygon_45_concept> { typedef polygon_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, polygon_45_with_holes_concept> { typedef polygon_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, polygon_concept> { typedef polygon_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, polygon_with_holes_concept> { typedef polygon_traits<T> type; };

  struct polygon_45_set_concept;
  struct polygon_set_concept;
  template <typename T>
  struct polygon_90_set_traits;
  template <typename T>
  struct polygon_45_set_traits;
  template <typename T>
  struct polygon_set_traits;
  template <typename T>
  struct traits_by_concept<T, polygon_90_set_concept> { typedef polygon_90_set_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, polygon_45_set_concept> { typedef polygon_45_set_traits<T> type; };
  template <typename T>
  struct traits_by_concept<T, polygon_set_concept> { typedef polygon_set_traits<T> type; };

  template <typename T, typename T2>
  struct get_coordinate_type {
    typedef typename traits_by_concept<T, T2>::type traits_type;
    typedef typename traits_type::coordinate_type type;
  };
  //want to prevent recursive template definition syntax errors, so duplicate get_coordinate_type
  template <typename T, typename T2>
  struct get_coordinate_type_2 {
    typedef typename traits_by_concept<T, T2>::type traits_type;
    typedef typename traits_type::coordinate_type type;
  };
  template <typename T>
  struct get_coordinate_type<T, undefined_concept> {
    typedef typename get_coordinate_type_2<typename std::iterator_traits
                                           <typename T::iterator>::value_type,
                                           typename geometry_concept<typename std::iterator_traits
                                                                     <typename T::iterator>::value_type>::type>::type type; };

  template <typename T, typename T2>
  struct get_iterator_type_2 {
    typedef const T* type;
    static type begin(const T& t) { return &t; }
    static type end(const T& t) { const T* tp = &t; ++tp; return tp; }
  };
  template <typename T>
  struct get_iterator_type {
    typedef get_iterator_type_2<T, typename geometry_concept<T>::type> indirect_type;
    typedef typename indirect_type::type type;
    static type begin(const T& t) { return indirect_type::begin(t); }
    static type end(const T& t) { return indirect_type::end(t); }
  };
  template <typename T>
  struct get_iterator_type_2<T, undefined_concept> {
    typedef typename T::const_iterator type;
    static type begin(const T& t) { return t.begin(); }
    static type end(const T& t) { return t.end(); }
  };

//   //helpers for allowing polygon 45 and containers of polygon 45 to behave interchangably in polygon_45_set_traits
//   template <typename T, typename T2>
//   struct get_coordinate_type_45 {};
//   template <typename T, typename T2>
//   struct get_coordinate_type_2_45 {};
//   template <typename T>
//   struct get_coordinate_type_45<T, void> {
//     typedef typename get_coordinate_type_2_45< typename T::value_type, typename geometry_concept<typename T::value_type>::type >::type type; };
//   template <typename T>
//   struct get_coordinate_type_45<T, polygon_45_concept> { typedef typename polygon_traits<T>::coordinate_type type; };
//   template <typename T>
//   struct get_coordinate_type_45<T, polygon_45_with_holes_concept> { typedef typename polygon_traits<T>::coordinate_type type; };
//   template <typename T>
//   struct get_coordinate_type_2_45<T, polygon_45_concept> { typedef typename polygon_traits<T>::coordinate_type type; };
//   template <typename T>
//   struct get_coordinate_type_2_45<T, polygon_45_with_holes_concept> { typedef typename polygon_traits<T>::coordinate_type type; };
//   template <typename T, typename T2>
//   struct get_iterator_type_45 {};
//   template <typename T>
//   struct get_iterator_type_45<T, void> {
//     typedef typename T::const_iterator type;
//     static type begin(const T& t) { return t.begin(); }
//     static type end(const T& t) { return t.end(); }
//   };
//   template <typename T>
//   struct get_iterator_type_45<T, polygon_45_concept> {
//     typedef const T* type;
//     static type begin(const T& t) { return &t; }
//     static type end(const T& t) { const T* tp = &t; ++tp; return tp; }
//   };
//   template <typename T>
//   struct get_iterator_type_45<T, polygon_45_with_holes_concept> {
//     typedef const T* type;
//     static type begin(const T& t) { return &t; }
//     static type end(const T& t) { const T* tp = &t; ++tp; return tp; }
//   };
//   template <typename T>
//   struct get_iterator_type_45<T, polygon_90_set_concept> {
//     typedef const T* type;
//     static type begin(const T& t) { return &t; }
//     static type end(const T& t) { const T* tp = &t; ++tp; return tp; }
//   };

  template <typename T>
  struct polygon_90_set_traits {
    typedef typename get_coordinate_type<T, typename geometry_concept<T>::type >::type coordinate_type;
    typedef get_iterator_type<T> indirection_type;
    typedef typename get_iterator_type<T>::type iterator_type;
    typedef T operator_arg_type;

    static inline iterator_type begin(const T& polygon_set) {
      return indirection_type::begin(polygon_set);
    }

    static inline iterator_type end(const T& polygon_set) {
      return indirection_type::end(polygon_set);
    }

    static inline orientation_2d orient(const T&) { return HORIZONTAL; }

    static inline bool clean(const T&) { return false; }

    static inline bool sorted(const T&) { return false; }
  };

  template <typename T>
  struct is_manhattan_polygonal_concept { typedef gtl_no type; };
  template <>
  struct is_manhattan_polygonal_concept<rectangle_concept> { typedef gtl_yes type; };
  template <>
  struct is_manhattan_polygonal_concept<polygon_90_concept> { typedef gtl_yes type; };
  template <>
  struct is_manhattan_polygonal_concept<polygon_90_with_holes_concept> { typedef gtl_yes type; };
  template <>
  struct is_manhattan_polygonal_concept<polygon_90_set_concept> { typedef gtl_yes type; };

  template <typename T>
  struct is_polygon_90_set_type {
    typedef typename is_manhattan_polygonal_concept<typename geometry_concept<T>::type>::type type;
  };
  template <typename T>
  struct is_polygon_90_set_type<std::list<T> > {
    typedef typename gtl_or<
      typename is_manhattan_polygonal_concept<typename geometry_concept<std::list<T> >::type>::type,
      typename is_manhattan_polygonal_concept<typename geometry_concept<typename std::list<T>::value_type>::type>::type>::type type;
  };
  template <typename T>
  struct is_polygon_90_set_type<std::vector<T> > {
    typedef typename gtl_or<
      typename is_manhattan_polygonal_concept<typename geometry_concept<std::vector<T> >::type>::type,
      typename is_manhattan_polygonal_concept<typename geometry_concept<typename std::vector<T>::value_type>::type>::type>::type type;
  };

  template <typename T>
  struct is_mutable_polygon_90_set_type {
    typedef typename gtl_same_type<polygon_90_set_concept, typename geometry_concept<T>::type>::type type;
  };
  template <typename T>
  struct is_mutable_polygon_90_set_type<std::list<T> > {
    typedef typename gtl_or<
      typename gtl_same_type<polygon_90_set_concept, typename geometry_concept<std::list<T> >::type>::type,
      typename is_manhattan_polygonal_concept<typename geometry_concept<typename std::list<T>::value_type>::type>::type>::type type;
  };
  template <typename T>
  struct is_mutable_polygon_90_set_type<std::vector<T> > {
    typedef typename gtl_or<
      typename gtl_same_type<polygon_90_set_concept, typename geometry_concept<std::vector<T> >::type>::type,
      typename is_manhattan_polygonal_concept<typename geometry_concept<typename std::vector<T>::value_type>::type>::type>::type type;
  };

//   //specialization for rectangle, polygon_90 and polygon_90_with_holes types
//   template <typename T>
//   struct polygon_90_set_traits
//     typedef typename geometry_concept<T>::type concept_type;
//     typedef typename get_coordinate_type<T, concept_type>::type coordinate_type;
//     typedef iterator_geometry_to_set<concept_type, T> iterator_type;
//     typedef T operator_arg_type;

//     static inline iterator_type begin(const T& polygon_set) {
//       return iterator_geometry_to_set<concept_type, T>(polygon_set, LOW, HORIZONTAL);
//     }

//     static inline iterator_type end(const T& polygon_set) {
//       return iterator_geometry_to_set<concept_type, T>(polygon_set, HIGH, HORIZONTAL);
//     }

//     static inline orientation_2d orient(const T& polygon_set) { return HORIZONTAL; }

//     static inline bool clean(const T& polygon_set) { return false; }

//     static inline bool sorted(const T& polygon_set) { return false; }

//   };

//   //specialization for containers of recangle, polygon_90, polygon_90_with_holes
//   template <typename T>
//   struct polygon_90_set_traits<T, typename is_manhattan_polygonal_concept<typename std::iterator_traits<typename T::iterator>::value_type>::type> {
//     typedef typename std::iterator_traits<typename T::iterator>::value_type geometry_type;
//     typedef typename geometry_concept<geometry_type>::type concept_type;
//     typedef typename get_coordinate_type<geometry_type, concept_type>::type coordinate_type;
//     typedef iterator_geometry_range_to_set<concept_type, typename T::const_iterator> iterator_type;
//     typedef T operator_arg_type;

//     static inline iterator_type begin(const T& polygon_set) {
//       return iterator_type(polygon_set.begin(), HORIZONTAL);
//     }

//     static inline iterator_type end(const T& polygon_set) {
//       return iterator_type(polygon_set.end(), HORIZONTAL);
//     }

//     static inline orientation_2d orient(const T& polygon_set) { return HORIZONTAL; }

//     static inline bool clean(const T& polygon_set) { return false; }

//     static inline bool sorted(const T& polygon_set) { return false; }

//   };

  //get dispatch functions
  template <typename output_container_type, typename pst>
  void get_90_dispatch(output_container_type& output, const pst& ps,
                       orientation_2d orient, rectangle_concept ) {
    form_rectangles(output, ps.begin(), ps.end(), orient, rectangle_concept());
  }

  template <typename output_container_type, typename pst>
  void get_90_dispatch(output_container_type& output, const pst& ps,
                       orientation_2d orient, polygon_90_concept tag) {
    get_polygons(output, ps.begin(), ps.end(), orient, true, tag);
  }

  template <typename output_container_type, typename pst>
  void get_90_dispatch(output_container_type& output, const pst& ps,
                       orientation_2d orient, polygon_90_with_holes_concept tag) {
    get_polygons(output, ps.begin(), ps.end(), orient, false, tag);
  }

  //by default works with containers of rectangle, polygon or polygon with holes
  //must be specialized to work with anything else
  template <typename T>
  struct polygon_90_set_mutable_traits {};
  template <typename T>
  struct polygon_90_set_mutable_traits<std::list<T> > {
    typedef typename geometry_concept<T>::type concept_type;
    template <typename input_iterator_type>
    static inline void set(std::list<T>& polygon_set, input_iterator_type input_begin, input_iterator_type input_end, orientation_2d orient) {
      polygon_set.clear();
      polygon_90_set_data<typename polygon_90_set_traits<std::list<T> >::coordinate_type> ps(orient);
      ps.reserve(std::distance(input_begin, input_end));
      ps.insert(input_begin, input_end, orient);
      ps.clean();
      get_90_dispatch(polygon_set, ps, orient, concept_type());
    }
  };
  template <typename T>
  struct polygon_90_set_mutable_traits<std::vector<T> > {
    typedef typename geometry_concept<T>::type concept_type;
    template <typename input_iterator_type>
    static inline void set(std::vector<T>& polygon_set, input_iterator_type input_begin, input_iterator_type input_end, orientation_2d orient) {
      polygon_set.clear();
      size_t num_ele = std::distance(input_begin, input_end);
      polygon_set.reserve(num_ele);
      polygon_90_set_data<typename polygon_90_set_traits<std::list<T> >::coordinate_type> ps(orient);
      ps.reserve(num_ele);
      ps.insert(input_begin, input_end, orient);
      ps.clean();
      get_90_dispatch(polygon_set, ps, orient, concept_type());
    }
  };

  template <typename T>
  struct polygon_90_set_mutable_traits<polygon_90_set_data<T> > {

    template <typename input_iterator_type>
    static inline void set(polygon_90_set_data<T>& polygon_set,
                           input_iterator_type input_begin, input_iterator_type input_end,
                           orientation_2d orient) {
      polygon_set.clear();
      polygon_set.reserve(std::distance(input_begin, input_end));
      polygon_set.insert(input_begin, input_end, orient);
    }

  };

  template <typename T>
  struct polygon_90_set_traits<polygon_90_set_data<T> > {
    typedef typename polygon_90_set_data<T>::coordinate_type coordinate_type;
    typedef typename polygon_90_set_data<T>::iterator_type iterator_type;
    typedef typename polygon_90_set_data<T>::operator_arg_type operator_arg_type;

    static inline iterator_type begin(const polygon_90_set_data<T>& polygon_set) {
      return polygon_set.begin();
    }

    static inline iterator_type end(const polygon_90_set_data<T>& polygon_set) {
      return polygon_set.end();
    }

    static inline orientation_2d orient(const polygon_90_set_data<T>& polygon_set) { return polygon_set.orient(); }

    static inline bool clean(const polygon_90_set_data<T>& polygon_set) { polygon_set.clean(); return true; }

    static inline bool sorted(const polygon_90_set_data<T>& polygon_set) { polygon_set.sort(); return true; }

  };

  template <typename T>
  struct is_polygon_90_set_concept { };
  template <>
  struct is_polygon_90_set_concept<polygon_90_set_concept> { typedef gtl_yes type; };
  template <>
  struct is_polygon_90_set_concept<rectangle_concept> { typedef gtl_yes type; };
  template <>
  struct is_polygon_90_set_concept<polygon_90_concept> { typedef gtl_yes type; };
  template <>
  struct is_polygon_90_set_concept<polygon_90_with_holes_concept> { typedef gtl_yes type; };

  template <typename T>
  struct is_mutable_polygon_90_set_concept { typedef gtl_no type; };
  template <>
  struct is_mutable_polygon_90_set_concept<polygon_90_set_concept> { typedef gtl_yes type; };

  template <typename T>
  struct geometry_concept<polygon_90_set_data<T> > { typedef polygon_90_set_concept type; };

  //template <typename T>
  //typename enable_if<typename is_polygon_90_set_type<T>::type, void>::type
  //print_is_polygon_90_set_concept(const T& t) { std::cout << "is polygon 90 set concept\n"; }
  //template <typename T>
  //typename enable_if<typename is_mutable_polygon_90_set_type<T>::type, void>::type
  //print_is_mutable_polygon_90_set_concept(const T& t) { std::cout << "is mutable polygon 90 set concept\n"; }
}
}
#endif

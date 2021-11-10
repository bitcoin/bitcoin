/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_SET_TRAITS_HPP
#define BOOST_POLYGON_POLYGON_SET_TRAITS_HPP
namespace boost { namespace polygon{

  struct polygon_set_concept {};

  //default definition of polygon set traits works for any model of polygon , polygon with holes or any vector or list thereof
  template <typename T>
  struct polygon_set_traits {
    typedef typename get_coordinate_type<T, typename geometry_concept<T>::type >::type coordinate_type;
    typedef typename get_iterator_type<T>::type iterator_type;
    typedef T operator_arg_type;

    static inline iterator_type begin(const T& polygon_set) {
      return get_iterator_type<T>::begin(polygon_set);
    }

    static inline iterator_type end(const T& polygon_set) {
      return get_iterator_type<T>::end(polygon_set);
    }

    static inline bool clean(const T& ) { return false; }

    static inline bool sorted(const T& ) { return false; }
  };

  template <typename T>
  struct is_polygonal_concept { typedef gtl_no type; };
  template <>
  struct is_polygonal_concept<polygon_concept> { typedef gtl_yes type; };
  template <>
  struct is_polygonal_concept<polygon_with_holes_concept> { typedef gtl_yes type; };
  template <>
  struct is_polygonal_concept<polygon_set_concept> { typedef gtl_yes type; };

  template <typename T>
  struct is_polygon_set_type {
    typedef typename is_polygonal_concept<typename geometry_concept<T>::type>::type type;
  };
  template <typename T>
  struct is_polygon_set_type<std::list<T> > {
    typedef typename gtl_or<
      typename is_polygonal_concept<typename geometry_concept<std::list<T> >::type>::type,
      typename is_polygonal_concept<typename geometry_concept<typename std::list<T>::value_type>::type>::type>::type type;
  };
  template <typename T>
  struct is_polygon_set_type<std::vector<T> > {
    typedef typename gtl_or<
      typename is_polygonal_concept<typename geometry_concept<std::vector<T> >::type>::type,
      typename is_polygonal_concept<typename geometry_concept<typename std::vector<T>::value_type>::type>::type>::type type;
  };

  template <typename T>
  struct is_mutable_polygon_set_type {
    typedef typename gtl_same_type<polygon_set_concept, typename geometry_concept<T>::type>::type type;
  };
  template <typename T>
  struct is_mutable_polygon_set_type<std::list<T> > {
    typedef typename gtl_or<
      typename gtl_same_type<polygon_set_concept, typename geometry_concept<std::list<T> >::type>::type,
      typename is_polygonal_concept<typename geometry_concept<typename std::list<T>::value_type>::type>::type>::type type;
  };
  template <typename T>
  struct is_mutable_polygon_set_type<std::vector<T> > {
    typedef typename gtl_or<
      typename gtl_same_type<polygon_set_concept, typename geometry_concept<std::vector<T> >::type>::type,
      typename is_polygonal_concept<typename geometry_concept<typename std::vector<T>::value_type>::type>::type>::type type;
  };

  template <typename T>
  struct polygon_set_mutable_traits {};
  template <typename T>
  struct polygon_set_mutable_traits<std::list<T> > {
    template <typename input_iterator_type>
    static inline void set(std::list<T>& polygon_set, input_iterator_type input_begin, input_iterator_type input_end) {
      polygon_set.clear();
      polygon_set_data<typename polygon_set_traits<std::list<T> >::coordinate_type> ps;
      ps.reserve(std::distance(input_begin, input_end));
      ps.insert(input_begin, input_end);
      ps.get(polygon_set);
    }
  };
  template <typename T>
  struct polygon_set_mutable_traits<std::vector<T> > {
    template <typename input_iterator_type>
    static inline void set(std::vector<T>& polygon_set, input_iterator_type input_begin, input_iterator_type input_end) {
      polygon_set.clear();
      size_t num_ele = std::distance(input_begin, input_end);
      polygon_set.reserve(num_ele);
      polygon_set_data<typename polygon_set_traits<std::list<T> >::coordinate_type> ps;
      ps.reserve(num_ele);
      ps.insert(input_begin, input_end);
      ps.get(polygon_set);
    }
  };

  template <typename T>
  struct polygon_set_mutable_traits<polygon_set_data<T> > {
    template <typename input_iterator_type>
    static inline void set(polygon_set_data<T>& polygon_set,
                           input_iterator_type input_begin, input_iterator_type input_end) {
      polygon_set.set(input_begin, input_end);
    }
  };
  template <typename T>
  struct polygon_set_traits<polygon_set_data<T> > {
    typedef typename polygon_set_data<T>::coordinate_type coordinate_type;
    typedef typename polygon_set_data<T>::iterator_type iterator_type;
    typedef typename polygon_set_data<T>::operator_arg_type operator_arg_type;

    static inline iterator_type begin(const polygon_set_data<T>& polygon_set) {
      return polygon_set.begin();
    }

    static inline iterator_type end(const polygon_set_data<T>& polygon_set) {
      return polygon_set.end();
    }

    static inline bool clean(const polygon_set_data<T>& polygon_set) { polygon_set.clean(); return true; }

    static inline bool sorted(const polygon_set_data<T>& polygon_set) { polygon_set.sort(); return true; }

  };
}
}
#endif

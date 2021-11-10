/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_45_SET_CONCEPT_HPP
#define BOOST_POLYGON_POLYGON_45_SET_CONCEPT_HPP
#include "polygon_45_set_data.hpp"
#include "polygon_45_set_traits.hpp"
#include "detail/polygon_45_touch.hpp"
namespace boost { namespace polygon{

  template <typename T, typename T2>
  struct is_either_polygon_45_set_type {
    typedef typename gtl_or<typename is_polygon_45_set_type<T>::type, typename is_polygon_45_set_type<T2>::type >::type type;
  };

  template <typename T>
  struct is_polygon_45_or_90_set_type {
    typedef typename gtl_or<typename is_polygon_45_set_type<T>::type, typename is_polygon_90_set_type<T>::type >::type type;
  };

  template <typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_polygon_45_or_90_set_type<polygon_set_type>::type>::type,
                       typename polygon_45_set_traits<polygon_set_type>::iterator_type>::type
  begin_45_set_data(const polygon_set_type& polygon_set) {
    return polygon_45_set_traits<polygon_set_type>::begin(polygon_set);
  }

  template <typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_polygon_45_or_90_set_type<polygon_set_type>::type>::type,
                       typename polygon_45_set_traits<polygon_set_type>::iterator_type>::type
  end_45_set_data(const polygon_set_type& polygon_set) {
    return polygon_45_set_traits<polygon_set_type>::end(polygon_set);
  }

  template <typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_polygon_45_set_type<polygon_set_type>::type>::type,
                       bool>::type
  clean(const polygon_set_type& polygon_set) {
    return polygon_45_set_traits<polygon_set_type>::clean(polygon_set);
  }

  //assign
  template <typename polygon_set_type_1, typename polygon_set_type_2>
  typename enable_if< typename gtl_and< typename gtl_if<typename is_mutable_polygon_45_set_type<polygon_set_type_1>::type>::type,
                                         typename gtl_if<typename is_polygon_45_or_90_set_type<polygon_set_type_2>::type>::type>::type,
                       polygon_set_type_1>::type &
  assign(polygon_set_type_1& lvalue, const polygon_set_type_2& rvalue) {
    polygon_45_set_mutable_traits<polygon_set_type_1>::set(lvalue, begin_45_set_data(rvalue), end_45_set_data(rvalue));
    return lvalue;
  }

  //get trapezoids
  template <typename output_container_type, typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_polygon_45_set_type<polygon_set_type>::type>::type,
                       void>::type
  get_trapezoids(output_container_type& output, const polygon_set_type& polygon_set) {
    clean(polygon_set);
    polygon_45_set_data<typename polygon_45_set_traits<polygon_set_type>::coordinate_type> ps;
    assign(ps, polygon_set);
    ps.get_trapezoids(output);
  }

  //get trapezoids
  template <typename output_container_type, typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_polygon_45_set_type<polygon_set_type>::type>::type,
                       void>::type
  get_trapezoids(output_container_type& output, const polygon_set_type& polygon_set, orientation_2d slicing_orientation) {
    clean(polygon_set);
    polygon_45_set_data<typename polygon_45_set_traits<polygon_set_type>::coordinate_type> ps;
    assign(ps, polygon_set);
    ps.get_trapezoids(output, slicing_orientation);
  }

  //equivalence
  template <typename polygon_set_type_1, typename polygon_set_type_2>
  typename enable_if< typename gtl_and_3<typename gtl_if<typename is_polygon_45_or_90_set_type<polygon_set_type_1>::type>::type,
                                          typename gtl_if<typename is_polygon_45_or_90_set_type<polygon_set_type_2>::type>::type,
                                          typename gtl_if<typename is_either_polygon_45_set_type<polygon_set_type_1,
                                                                                                 polygon_set_type_2>::type>::type>::type,
                       bool>::type
  equivalence(const polygon_set_type_1& lvalue,
              const polygon_set_type_2& rvalue) {
    polygon_45_set_data<typename polygon_45_set_traits<polygon_set_type_1>::coordinate_type> ps1;
    assign(ps1, lvalue);
    polygon_45_set_data<typename polygon_45_set_traits<polygon_set_type_2>::coordinate_type> ps2;
    assign(ps2, rvalue);
    return ps1 == ps2;
  }

  //clear
  template <typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_mutable_polygon_45_set_type<polygon_set_type>::type>::type,
                       void>::type
  clear(polygon_set_type& polygon_set) {
    polygon_45_set_data<typename polygon_45_set_traits<polygon_set_type>::coordinate_type> ps;
    assign(polygon_set, ps);
  }

  //empty
  template <typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_mutable_polygon_45_set_type<polygon_set_type>::type>::type,
                       bool>::type
  empty(const polygon_set_type& polygon_set) {
    if(clean(polygon_set)) return begin_45_set_data(polygon_set) == end_45_set_data(polygon_set);
    polygon_45_set_data<typename polygon_45_set_traits<polygon_set_type>::coordinate_type> ps;
    assign(ps, polygon_set);
    ps.clean();
    return ps.empty();
  }

  //extents
  template <typename polygon_set_type, typename rectangle_type>
  typename enable_if<
    typename gtl_and< typename gtl_if<typename is_mutable_polygon_45_set_type<polygon_set_type>::type>::type,
                      typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
    bool>::type
  extents(rectangle_type& extents_rectangle,
          const polygon_set_type& polygon_set) {
    clean(polygon_set);
    polygon_45_set_data<typename polygon_45_set_traits<polygon_set_type>::coordinate_type> ps;
    assign(ps, polygon_set);
    return ps.extents(extents_rectangle);
  }

  //area
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::area_type>::type
  area(const polygon_set_type& polygon_set) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    typedef polygon_45_with_holes_data<Unit> p_type;
    typedef typename coordinate_traits<Unit>::area_type area_type;
    std::vector<p_type> polys;
    assign(polys, polygon_set);
    area_type retval = (area_type)0;
    for(std::size_t i = 0; i < polys.size(); ++i) {
      retval += area(polys[i]);
    }
    return retval;
  }

  //interact
  template <typename polygon_set_type_1, typename polygon_set_type_2>
  typename enable_if <
    typename gtl_and< typename gtl_if<typename is_mutable_polygon_45_set_type<polygon_set_type_1>::type>::type,
                      typename gtl_if<typename is_polygon_45_or_90_set_type<polygon_set_type_2>::type>::type >::type,
    polygon_set_type_1>::type&
  interact(polygon_set_type_1& polygon_set_1, const polygon_set_type_2& polygon_set_2) {
    typedef typename polygon_45_set_traits<polygon_set_type_1>::coordinate_type Unit;
    std::vector<polygon_45_data<Unit> > polys;
    assign(polys, polygon_set_1);
    std::vector<std::set<int> > graph(polys.size()+1, std::set<int>());
    connectivity_extraction_45<Unit> ce;
    ce.insert(polygon_set_2);
    for(std::size_t i = 0; i < polys.size(); ++i){
      ce.insert(polys[i]);
    }
    ce.extract(graph);
    clear(polygon_set_1);
    polygon_45_set_data<Unit> ps;
    for(std::set<int>::iterator itr = graph[0].begin(); itr != graph[0].end(); ++itr){
      ps.insert(polys[(*itr)-1]);
    }
    assign(polygon_set_1, ps);
    return polygon_set_1;
  }

//   //self_intersect
//   template <typename polygon_set_type>
//   typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type>::type,
//                        polygon_set_type>::type &
//   self_intersect(polygon_set_type& polygon_set) {
//     typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
//     //TODO
//   }

  template <typename polygon_set_type, typename coord_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  resize(polygon_set_type& polygon_set, coord_type resizing,
         RoundingOption rounding = CLOSEST, CornerOption corner = INTERSECTION) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    clean(polygon_set);
    polygon_45_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.resize(resizing, rounding, corner);
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  bloat(polygon_set_type& polygon_set,
        typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type bloating) {
    return resize(polygon_set, static_cast<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>(bloating));
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  shrink(polygon_set_type& polygon_set,
        typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type shrinking) {
    return resize(polygon_set, -(typename polygon_45_set_traits<polygon_set_type>::coordinate_type)shrinking);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  grow_and(polygon_set_type& polygon_set,
        typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type bloating) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    std::vector<polygon_45_data<Unit> > polys;
    assign(polys, polygon_set);
    clear(polygon_set);
    polygon_45_set_data<Unit> ps;
    for(std::size_t i = 0; i < polys.size(); ++i) {
      polygon_45_set_data<Unit> tmpPs;
      tmpPs.insert(polys[i]);
      bloat(tmpPs, bloating);
      tmpPs.clean(); //apply implicit OR on tmp polygon set
      ps.insert(tmpPs);
    }
    ps.self_intersect();
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  scale_up(polygon_set_type& polygon_set,
           typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type factor) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    clean(polygon_set);
    polygon_45_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.scale_up(factor);
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  scale_down(polygon_set_type& polygon_set,
           typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type factor) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    clean(polygon_set);
    polygon_45_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.scale_down(factor);
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  scale(polygon_set_type& polygon_set, double factor) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    clean(polygon_set);
    polygon_45_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.scale(factor);
    assign(polygon_set, ps);
    return polygon_set;
  }

  //self_intersect
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  self_intersect(polygon_set_type& polygon_set) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_45_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.self_intersect();
    assign(polygon_set, ps);
    return polygon_set;
  }

  //self_xor
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  self_xor(polygon_set_type& polygon_set) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_45_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.self_xor();
    assign(polygon_set, ps);
    return polygon_set;
  }

  //transform
  template <typename polygon_set_type, typename transformation_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  transform(polygon_set_type& polygon_set,
            const transformation_type& transformation) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    clean(polygon_set);
    polygon_45_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.transform(transformation);
    assign(polygon_set, ps);
    return polygon_set;
  }

  //keep
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_45_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  keep(polygon_set_type& polygon_set,
       typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::area_type min_area,
       typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::area_type max_area,
       typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type min_width,
       typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type max_width,
       typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type min_height,
       typename coordinate_traits<typename polygon_45_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type max_height) {
    typedef typename polygon_45_set_traits<polygon_set_type>::coordinate_type Unit;
    typedef typename coordinate_traits<Unit>::unsigned_area_type uat;
    std::list<polygon_45_data<Unit> > polys;
    assign(polys, polygon_set);
    typename std::list<polygon_45_data<Unit> >::iterator itr_nxt;
    for(typename std::list<polygon_45_data<Unit> >::iterator itr = polys.begin(); itr != polys.end(); itr = itr_nxt){
      itr_nxt = itr;
      ++itr_nxt;
      rectangle_data<Unit> bbox;
      extents(bbox, *itr);
      uat pwidth = delta(bbox, HORIZONTAL);
      if(pwidth > min_width && pwidth <= max_width){
        uat pheight = delta(bbox, VERTICAL);
        if(pheight > min_height && pheight <= max_height){
          typename coordinate_traits<Unit>::area_type parea = area(*itr);
          if(parea <= max_area && parea >= min_area) {
            continue;
          }
        }
      }
      polys.erase(itr);
    }
    assign(polygon_set, polys);
    return polygon_set;
  }

  template <typename T>
  struct view_of<polygon_90_set_concept, T> {
    typedef typename get_coordinate_type<T, typename geometry_concept<T>::type >::type coordinate_type;
    T* tp;
    std::vector<polygon_90_with_holes_data<coordinate_type> > polys;
    view_of(T& obj) : tp(&obj), polys() {
      std::vector<polygon_with_holes_data<coordinate_type> > gpolys;
      assign(gpolys, obj);
      for(typename std::vector<polygon_with_holes_data<coordinate_type> >::iterator itr = gpolys.begin();
          itr != gpolys.end(); ++itr) {
        polys.push_back(polygon_90_with_holes_data<coordinate_type>());
        assign(polys.back(), view_as<polygon_90_with_holes_concept>(*itr));
      }
    }
    view_of(const T& obj) : tp(), polys() {
      std::vector<polygon_with_holes_data<coordinate_type> > gpolys;
      assign(gpolys, obj);
      for(typename std::vector<polygon_with_holes_data<coordinate_type> >::iterator itr = gpolys.begin();
          itr != gpolys.end(); ++itr) {
        polys.push_back(polygon_90_with_holes_data<coordinate_type>());
        assign(polys.back(), view_as<polygon_90_with_holes_concept>(*itr));
      }
    }

    typedef typename std::vector<polygon_90_with_holes_data<coordinate_type> >::const_iterator iterator_type;
    typedef view_of operator_arg_type;

    inline iterator_type begin() const {
      return polys.begin();
    }

    inline iterator_type end() const {
      return polys.end();
    }

    inline orientation_2d orient() const { return HORIZONTAL; }

    inline bool clean() const { return false; }

    inline bool sorted() const { return false; }

    inline T& get() { return *tp; }

  };

  template <typename T>
  struct polygon_90_set_traits<view_of<polygon_90_set_concept, T> > {
    typedef typename view_of<polygon_90_set_concept, T>::coordinate_type coordinate_type;
    typedef typename view_of<polygon_90_set_concept, T>::iterator_type iterator_type;
    typedef view_of<polygon_90_set_concept, T> operator_arg_type;

    static inline iterator_type begin(const view_of<polygon_90_set_concept, T>& polygon_set) {
      return polygon_set.begin();
    }

    static inline iterator_type end(const view_of<polygon_90_set_concept, T>& polygon_set) {
      return polygon_set.end();
    }

    static inline orientation_2d orient(const view_of<polygon_90_set_concept, T>& polygon_set) {
      return polygon_set.orient(); }

    static inline bool clean(const view_of<polygon_90_set_concept, T>& polygon_set) {
      return polygon_set.clean(); }

    static inline bool sorted(const view_of<polygon_90_set_concept, T>& polygon_set) {
      return polygon_set.sorted(); }

  };

  template <typename T>
  struct geometry_concept<view_of<polygon_90_set_concept, T> > {
    typedef polygon_90_set_concept type;
  };

  template <typename T>
  struct get_coordinate_type<view_of<polygon_90_set_concept, T>, polygon_90_set_concept> {
    typedef typename view_of<polygon_90_set_concept, T>::coordinate_type type;
  };
  template <typename T>
  struct get_iterator_type_2<view_of<polygon_90_set_concept, T>, polygon_90_set_concept> {
    typedef typename view_of<polygon_90_set_concept, T>::iterator_type type;
    static type begin(const view_of<polygon_90_set_concept, T>& t) { return t.begin(); }
    static type end(const view_of<polygon_90_set_concept, T>& t) { return t.end(); }
  };

}
}
#include "detail/polygon_45_set_view.hpp"
#endif

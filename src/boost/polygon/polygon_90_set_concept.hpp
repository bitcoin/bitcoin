/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_90_SET_CONCEPT_HPP
#define BOOST_POLYGON_POLYGON_90_SET_CONCEPT_HPP
#include "polygon_90_set_data.hpp"
#include "polygon_90_set_traits.hpp"
namespace boost { namespace polygon{

  template <typename polygon_set_type>
  typename enable_if< typename is_polygon_90_set_type<polygon_set_type>::type,
                       typename polygon_90_set_traits<polygon_set_type>::iterator_type>::type
  begin_90_set_data(const polygon_set_type& polygon_set) {
    return polygon_90_set_traits<polygon_set_type>::begin(polygon_set);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_polygon_90_set_type<polygon_set_type>::type,
                       typename polygon_90_set_traits<polygon_set_type>::iterator_type>::type
  end_90_set_data(const polygon_set_type& polygon_set) {
    return polygon_90_set_traits<polygon_set_type>::end(polygon_set);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_polygon_90_set_type<polygon_set_type>::type,
                       orientation_2d>::type
  scanline_orientation(const polygon_set_type& polygon_set) {
    return polygon_90_set_traits<polygon_set_type>::orient(polygon_set);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_polygon_90_set_type<polygon_set_type>::type,
                       bool>::type
  clean(const polygon_set_type& polygon_set) {
    return polygon_90_set_traits<polygon_set_type>::clean(polygon_set);
  }

  //assign
  template <typename polygon_set_type_1, typename polygon_set_type_2>
  typename enable_if <
    typename gtl_and<
      typename is_mutable_polygon_90_set_type<polygon_set_type_1>::type,
      typename is_polygon_90_set_type<polygon_set_type_2>::type>::type,
    polygon_set_type_1>::type &
  assign(polygon_set_type_1& lvalue, const polygon_set_type_2& rvalue) {
    polygon_90_set_mutable_traits<polygon_set_type_1>::set(lvalue, begin_90_set_data(rvalue), end_90_set_data(rvalue),
                                                           scanline_orientation(rvalue));
    return lvalue;
  }

  template <typename T1, typename T2>
  struct are_not_both_rectangle_concept { typedef gtl_yes type; };
  template <>
  struct are_not_both_rectangle_concept<rectangle_concept, rectangle_concept> { typedef gtl_no type; };

  //equivalence
  template <typename polygon_set_type_1, typename polygon_set_type_2>
  typename enable_if< typename gtl_and_3<
    typename is_polygon_90_set_type<polygon_set_type_1>::type,
    typename is_polygon_90_set_type<polygon_set_type_2>::type,
    typename are_not_both_rectangle_concept<typename geometry_concept<polygon_set_type_1>::type,
                                            typename geometry_concept<polygon_set_type_2>::type>::type>::type,
                       bool>::type
  equivalence(const polygon_set_type_1& lvalue,
              const polygon_set_type_2& rvalue) {
    polygon_90_set_data<typename polygon_90_set_traits<polygon_set_type_1>::coordinate_type> ps1;
    assign(ps1, lvalue);
    polygon_90_set_data<typename polygon_90_set_traits<polygon_set_type_2>::coordinate_type> ps2;
    assign(ps2, rvalue);
    return ps1 == ps2;
  }


  //get rectangle tiles (slicing orientation is vertical)
  template <typename output_container_type, typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_polygon_90_set_type<polygon_set_type>::type>::type,
                       void>::type
  get_rectangles(output_container_type& output, const polygon_set_type& polygon_set) {
    clean(polygon_set);
    polygon_90_set_data<typename polygon_90_set_traits<polygon_set_type>::coordinate_type> ps(VERTICAL);
    assign(ps, polygon_set);
    ps.get_rectangles(output);
  }

  //get rectangle tiles
  template <typename output_container_type, typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_polygon_90_set_type<polygon_set_type>::type>::type,
                       void>::type
  get_rectangles(output_container_type& output, const polygon_set_type& polygon_set, orientation_2d slicing_orientation) {
    clean(polygon_set);
    polygon_90_set_data<typename polygon_90_set_traits<polygon_set_type>::coordinate_type> ps;
    assign(ps, polygon_set);
    ps.get_rectangles(output, slicing_orientation);
  }

  //get: min_rectangles max_rectangles
  template <typename output_container_type, typename polygon_set_type>
  typename enable_if <typename gtl_and<
    typename is_polygon_90_set_type<polygon_set_type>::type,
    typename gtl_same_type<rectangle_concept,
                           typename geometry_concept
                           <typename std::iterator_traits
                            <typename output_container_type::iterator>::value_type>::type>::type>::type,
                       void>::type
  get_max_rectangles(output_container_type& output, const polygon_set_type& polygon_set) {
    std::vector<rectangle_data<typename polygon_90_set_traits<polygon_set_type>::coordinate_type> > rects;
    assign(rects, polygon_set);
    MaxCover<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::getMaxCover(output, rects, scanline_orientation(polygon_set));
  }

  //clear
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       void>::type
  clear(polygon_set_type& polygon_set) {
    polygon_90_set_data<typename polygon_90_set_traits<polygon_set_type>::coordinate_type> ps(scanline_orientation(polygon_set));
    assign(polygon_set, ps);
  }

  //empty
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       bool>::type
  empty(const polygon_set_type& polygon_set) {
    if(clean(polygon_set)) return begin_90_set_data(polygon_set) == end_90_set_data(polygon_set);
    polygon_90_set_data<typename polygon_90_set_traits<polygon_set_type>::coordinate_type> ps;
    assign(ps, polygon_set);
    ps.clean();
    return ps.empty();
  }

  //extents
  template <typename polygon_set_type, typename rectangle_type>
  typename enable_if <typename gtl_and< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                                         typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       bool>::type
  extents(rectangle_type& extents_rectangle,
                             const polygon_set_type& polygon_set) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    return ps.extents(extents_rectangle);
  }

  //area
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::manhattan_area_type>::type
  area(const polygon_set_type& polygon_set) {
    typedef rectangle_data<typename polygon_90_set_traits<polygon_set_type>::coordinate_type> rectangle_type;
    typedef typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::manhattan_area_type area_type;
    std::vector<rectangle_type> rects;
    assign(rects, polygon_set);
    area_type retval = (area_type)0;
    for(std::size_t i = 0; i < rects.size(); ++i) {
      retval += (area_type)area(rects[i]);
    }
    return retval;
  }

  //interact
  template <typename polygon_set_type_1, typename polygon_set_type_2>
  typename enable_if <typename gtl_and< typename is_mutable_polygon_90_set_type<polygon_set_type_1>::type,
                                         typename is_mutable_polygon_90_set_type<polygon_set_type_2>::type>::type,
                       polygon_set_type_1>::type&
  interact(polygon_set_type_1& polygon_set_1, const polygon_set_type_2& polygon_set_2) {
    typedef typename polygon_90_set_traits<polygon_set_type_1>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps(scanline_orientation(polygon_set_2));
    polygon_90_set_data<Unit> ps2(ps);
    ps.insert(polygon_set_1);
    ps2.insert(polygon_set_2);
    ps.interact(ps2);
    assign(polygon_set_1, ps);
    return polygon_set_1;
  }

  //self_intersect
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  self_intersect(polygon_set_type& polygon_set) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.self_intersect();
    assign(polygon_set, ps);
    return polygon_set;
  }

  //self_xor
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  self_xor(polygon_set_type& polygon_set) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.self_xor();
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  bloat(polygon_set_type& polygon_set,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type bloating) {
    return bloat(polygon_set, bloating, bloating, bloating, bloating);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  bloat(polygon_set_type& polygon_set, orientation_2d orient,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type bloating) {
    if(orient == orientation_2d(HORIZONTAL))
      return bloat(polygon_set, bloating, bloating, 0, 0);
    return bloat(polygon_set, 0, 0, bloating, bloating);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  bloat(polygon_set_type& polygon_set, orientation_2d orient,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type low_bloating,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type high_bloating) {
    if(orient == orientation_2d(HORIZONTAL))
      return bloat(polygon_set, low_bloating, high_bloating, 0, 0);
    return bloat(polygon_set, 0, 0, low_bloating, high_bloating);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  bloat(polygon_set_type& polygon_set, direction_2d dir,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type bloating) {
    if(dir == direction_2d(EAST))
      return bloat(polygon_set, 0, bloating, 0, 0);
    if(dir == direction_2d(WEST))
      return bloat(polygon_set, bloating, 0, 0, 0);
    if(dir == direction_2d(SOUTH))
      return bloat(polygon_set, 0, 0, bloating, 0);
    return bloat(polygon_set, 0, 0, 0, bloating);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  bloat(polygon_set_type& polygon_set,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type west_bloating,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type east_bloating,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type south_bloating,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type north_bloating) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.bloat(west_bloating, east_bloating, south_bloating, north_bloating);
    ps.clean();
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  shrink(polygon_set_type& polygon_set,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type shrinking) {
    return shrink(polygon_set, shrinking, shrinking, shrinking, shrinking);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  shrink(polygon_set_type& polygon_set, orientation_2d orient,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type shrinking) {
    if(orient == orientation_2d(HORIZONTAL))
      return shrink(polygon_set, shrinking, shrinking, 0, 0);
    return shrink(polygon_set, 0, 0, shrinking, shrinking);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  shrink(polygon_set_type& polygon_set, orientation_2d orient,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type low_shrinking,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type high_shrinking) {
    if(orient == orientation_2d(HORIZONTAL))
      return shrink(polygon_set, low_shrinking, high_shrinking, 0, 0);
    return shrink(polygon_set, 0, 0, low_shrinking, high_shrinking);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  shrink(polygon_set_type& polygon_set, direction_2d dir,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type shrinking) {
    if(dir == direction_2d(EAST))
      return shrink(polygon_set, 0, shrinking, 0, 0);
    if(dir == direction_2d(WEST))
      return shrink(polygon_set, shrinking, 0, 0, 0);
    if(dir == direction_2d(SOUTH))
      return shrink(polygon_set, 0, 0, shrinking, 0);
    return shrink(polygon_set, 0, 0, 0, shrinking);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  shrink(polygon_set_type& polygon_set,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type west_shrinking,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type east_shrinking,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type south_shrinking,
        typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type north_shrinking) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.shrink(west_shrinking, east_shrinking, south_shrinking, north_shrinking);
    ps.clean();
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type, typename coord_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  resize(polygon_set_type& polygon_set, coord_type resizing) {
    if(resizing > 0) {
      return bloat(polygon_set, resizing);
    }
    if(resizing < 0) {
      return shrink(polygon_set, -resizing);
    }
    return polygon_set;
  }

  //positive or negative values allow for any and all directions of sizing
  template <typename polygon_set_type, typename coord_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  resize(polygon_set_type& polygon_set, coord_type west, coord_type east, coord_type south, coord_type north) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.resize(west, east, south, north);
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  grow_and(polygon_set_type& polygon_set,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type bloating) {
    return grow_and(polygon_set, bloating, bloating, bloating, bloating);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  grow_and(polygon_set_type& polygon_set, orientation_2d orient,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type bloating) {
    if(orient == orientation_2d(HORIZONTAL))
      return grow_and(polygon_set, bloating, bloating, 0, 0);
    return grow_and(polygon_set, 0, 0, bloating, bloating);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  grow_and(polygon_set_type& polygon_set, orientation_2d orient,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type low_bloating,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type high_bloating) {
    if(orient == orientation_2d(HORIZONTAL))
      return grow_and(polygon_set, low_bloating, high_bloating, 0, 0);
    return grow_and(polygon_set, 0, 0, low_bloating, high_bloating);
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  grow_and(polygon_set_type& polygon_set, direction_2d dir,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type bloating) {
    if(dir == direction_2d(EAST))
      return grow_and(polygon_set, 0, bloating, 0, 0);
    if(dir == direction_2d(WEST))
      return grow_and(polygon_set, bloating, 0, 0, 0);
    if(dir == direction_2d(SOUTH))
      return grow_and(polygon_set, 0, 0, bloating, 0);
    return grow_and(polygon_set, 0, 0, 0, bloating);
  }

  template <typename polygon_set_type>
  typename enable_if< typename gtl_if<typename is_mutable_polygon_90_set_type<polygon_set_type>::type>::type,
                       polygon_set_type>::type &
  grow_and(polygon_set_type& polygon_set,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type west_bloating,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type east_bloating,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type south_bloating,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type north_bloating) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    std::vector<polygon_90_data<Unit> > polys;
    assign(polys, polygon_set);
    clear(polygon_set);
    polygon_90_set_data<Unit> ps(scanline_orientation(polygon_set));
    for(std::size_t i = 0; i < polys.size(); ++i) {
      polygon_90_set_data<Unit> tmpPs(scanline_orientation(polygon_set));
      tmpPs.insert(polys[i]);
      bloat(tmpPs, west_bloating, east_bloating, south_bloating, north_bloating);
      tmpPs.clean(); //apply implicit OR on tmp polygon set
      ps.insert(tmpPs);
    }
    self_intersect(ps);
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  scale_up(polygon_set_type& polygon_set,
           typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>
           ::unsigned_area_type factor) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.scale_up(factor);
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  scale_down(polygon_set_type& polygon_set,
             typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>
             ::unsigned_area_type factor) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.scale_down(factor);
    assign(polygon_set, ps);
    return polygon_set;
  }

  template <typename polygon_set_type, typename scaling_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  scale(polygon_set_type& polygon_set,
        const scaling_type& scaling) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.scale(scaling);
    assign(polygon_set, ps);
    return polygon_set;
  }

  struct y_p_s_move : gtl_yes {};

  //move
  template <typename polygon_set_type>
  typename enable_if< typename gtl_and<y_p_s_move, typename gtl_if<typename is_mutable_polygon_90_set_type<polygon_set_type>::type>::type>::type,
                      polygon_set_type>::type &
  move(polygon_set_type& polygon_set,
  orientation_2d orient, typename polygon_90_set_traits<polygon_set_type>::coordinate_type displacement) {
    if(orient == HORIZONTAL)
      return move(polygon_set, displacement, 0);
    else
      return move(polygon_set, 0, displacement);
  }

  struct y_p_s_move2 : gtl_yes {};

  template <typename polygon_set_type>
  typename enable_if< typename gtl_and<y_p_s_move2, typename gtl_if<typename is_mutable_polygon_90_set_type<polygon_set_type>::type>::type>::type,
                      polygon_set_type>::type &
  move(polygon_set_type& polygon_set, typename polygon_90_set_traits<polygon_set_type>::coordinate_type x_displacement,
  typename polygon_90_set_traits<polygon_set_type>::coordinate_type y_displacement) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.move(x_displacement, y_displacement);
    ps.clean();
    assign(polygon_set, ps);
    return polygon_set;
  }

  //transform
  template <typename polygon_set_type, typename transformation_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  transform(polygon_set_type& polygon_set,
            const transformation_type& transformation) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    polygon_90_set_data<Unit> ps;
    assign(ps, polygon_set);
    ps.transform(transformation);
    ps.clean();
    assign(polygon_set, ps);
    return polygon_set;
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
  }

  //keep
  template <typename polygon_set_type>
  typename enable_if< typename is_mutable_polygon_90_set_type<polygon_set_type>::type,
                       polygon_set_type>::type &
  keep(polygon_set_type& polygon_set,
       typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type min_area,
       typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type max_area,
       typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type min_width,
       typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type max_width,
       typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type min_height,
       typename coordinate_traits<typename polygon_90_set_traits<polygon_set_type>::coordinate_type>::unsigned_area_type max_height) {
    typedef typename polygon_90_set_traits<polygon_set_type>::coordinate_type Unit;
    typedef typename coordinate_traits<Unit>::unsigned_area_type uat;
    std::list<polygon_90_data<Unit> > polys;
    assign(polys, polygon_set);
    clear(polygon_set);
    typename std::list<polygon_90_data<Unit> >::iterator itr_nxt;
    for(typename std::list<polygon_90_data<Unit> >::iterator itr = polys.begin(); itr != polys.end(); itr = itr_nxt){
      itr_nxt = itr;
      ++itr_nxt;
      rectangle_data<Unit> bbox;
      extents(bbox, *itr);
      uat pwidth = delta(bbox, HORIZONTAL);
      if(pwidth > min_width && pwidth <= max_width){
        uat pheight = delta(bbox, VERTICAL);
        if(pheight > min_height && pheight <= max_height){
          uat parea = area(*itr);
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


}
}
#include "detail/polygon_90_set_view.hpp"
#endif

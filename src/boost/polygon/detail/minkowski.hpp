/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
namespace boost { namespace polygon { namespace detail {

template <typename coordinate_type>
struct minkowski_offset {
  typedef point_data<coordinate_type> point;
  typedef polygon_set_data<coordinate_type> polygon_set;
  typedef polygon_with_holes_data<coordinate_type> polygon;
  typedef std::pair<point, point> edge;

  static void convolve_two_segments(std::vector<point>& figure, const edge& a, const edge& b) {
    figure.clear();
    figure.push_back(point(a.first));
    figure.push_back(point(a.first));
    figure.push_back(point(a.second));
    figure.push_back(point(a.second));
    convolve(figure[0], b.second);
    convolve(figure[1], b.first);
    convolve(figure[2], b.first);
    convolve(figure[3], b.second);
  }

  template <typename itrT1, typename itrT2>
  static void convolve_two_point_sequences(polygon_set& result, itrT1 ab, itrT1 ae, itrT2 bb, itrT2 be) {
    if(ab == ae || bb == be)
      return;
    point first_a = *ab;
    point prev_a = *ab;
    std::vector<point> vec;
    polygon poly;
    ++ab;
    for( ; ab != ae; ++ab) {
      point first_b = *bb;
      point prev_b = *bb;
      itrT2 tmpb = bb;
      ++tmpb;
      for( ; tmpb != be; ++tmpb) {
        convolve_two_segments(vec, std::make_pair(prev_b, *tmpb), std::make_pair(prev_a, *ab));
        set_points(poly, vec.begin(), vec.end());
        result.insert(poly);
        prev_b = *tmpb;
      }
      prev_a = *ab;
    }
  }

  template <typename itrT>
  static void convolve_point_sequence_with_polygons(polygon_set& result, itrT b, itrT e, const std::vector<polygon>& polygons) {
    for(std::size_t i = 0; i < polygons.size(); ++i) {
      convolve_two_point_sequences(result, b, e, begin_points(polygons[i]), end_points(polygons[i]));
      for(typename polygon_with_holes_traits<polygon>::iterator_holes_type itrh = begin_holes(polygons[i]);
          itrh != end_holes(polygons[i]); ++itrh) {
        convolve_two_point_sequences(result, b, e, begin_points(*itrh), end_points(*itrh));
      }
    }
  }

  static void convolve_two_polygon_sets(polygon_set& result, const polygon_set& a, const polygon_set& b) {
    result.clear();
    std::vector<polygon> a_polygons;
    std::vector<polygon> b_polygons;
    a.get(a_polygons);
    b.get(b_polygons);
    for(std::size_t ai = 0; ai < a_polygons.size(); ++ai) {
      convolve_point_sequence_with_polygons(result, begin_points(a_polygons[ai]),
                                            end_points(a_polygons[ai]), b_polygons);
      for(typename polygon_with_holes_traits<polygon>::iterator_holes_type itrh = begin_holes(a_polygons[ai]);
          itrh != end_holes(a_polygons[ai]); ++itrh) {
        convolve_point_sequence_with_polygons(result, begin_points(*itrh),
                                              end_points(*itrh), b_polygons);
      }
      for(std::size_t bi = 0; bi < b_polygons.size(); ++bi) {
        polygon tmp_poly = a_polygons[ai];
        result.insert(convolve(tmp_poly, *(begin_points(b_polygons[bi]))));
        tmp_poly = b_polygons[bi];
        result.insert(convolve(tmp_poly, *(begin_points(a_polygons[ai]))));
      }
    }
  }
};

}
  template<typename T>
  inline polygon_set_data<T>&
  polygon_set_data<T>::resize(coordinate_type resizing, bool corner_fill_arc, unsigned int num_circle_segments) {
    using namespace ::boost::polygon::operators;
    if(!corner_fill_arc) {
      if(resizing < 0)
        return shrink(-resizing);
      if(resizing > 0)
        return bloat(resizing);
      return *this;
    }
    if(resizing == 0) return *this;
    if(empty()) return *this;
    if(num_circle_segments < 3) num_circle_segments = 4;
    rectangle_data<coordinate_type> rect;
    extents(rect);
    if(resizing < 0) {
      ::boost::polygon::bloat(rect, 10);
      (*this) = rect - (*this); //invert
    }
    //make_arc(std::vector<point_data< T> >& return_points,
    //point_data< double> start, point_data< double>  end,
    //point_data< double> center,  double r, unsigned int num_circle_segments)
    std::vector<point_data<coordinate_type> > circle;
    point_data<double> center(0.0, 0.0), start(0.0, (double)resizing);
    make_arc(circle, start, start, center, std::abs((double)resizing),
             num_circle_segments);
    polygon_data<coordinate_type> poly;
    set_points(poly, circle.begin(), circle.end());
    polygon_set_data<coordinate_type> offset_set;
    offset_set += poly;
    polygon_set_data<coordinate_type> result;
    detail::minkowski_offset<coordinate_type>::convolve_two_polygon_sets
      (result, *this, offset_set);
    if(resizing < 0) {
      result = result & rect;//eliminate overhang
      result = result ^ rect;//invert
    }
    *this = result;
    return *this;
  }

}}

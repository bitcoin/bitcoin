/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_SCAN_ARBITRARY_HPP
#define BOOST_POLYGON_SCAN_ARBITRARY_HPP
#ifdef BOOST_POLYGON_DEBUG_FILE
#include <fstream>
#endif
#include "polygon_sort_adaptor.hpp"
namespace boost { namespace polygon{

  template <typename Unit>
  class line_intersection : public scanline_base<Unit> {
  private:
    typedef typename scanline_base<Unit>::Point Point;

    //the first point is the vertex and and second point establishes the slope of an edge eminating from the vertex
    //typedef std::pair<Point, Point> half_edge;
    typedef typename scanline_base<Unit>::half_edge half_edge;

    //scanline comparator functor
    typedef typename scanline_base<Unit>::less_half_edge less_half_edge;
    typedef typename scanline_base<Unit>::less_point less_point;

    //when parallel half edges are encounterd the set of segments is expanded
    //when a edge leaves the scanline it is removed from the set
    //when the set is empty the element is removed from the map
    typedef int segment_id;
    typedef std::pair<half_edge, std::set<segment_id> > scanline_element;
    typedef std::map<half_edge, std::set<segment_id>, less_half_edge> edge_scanline;
    typedef typename edge_scanline::iterator iterator;

//     std::map<Unit, std::set<segment_id> > vertical_data_;
//     edge_scanline edge_scanline_;
//     Unit x_;
//     int just_before_;
//     segment_id segment_id_;
//     std::vector<std::pair<half_edge, int> > event_edges_;
//     std::set<Point> intersection_queue_;
  public:
//     inline line_intersection() : vertical_data_(), edge_scanline_(), x_((std::numeric_limits<Unit>::max)()), just_before_(0), segment_id_(0), event_edges_(), intersection_queue_() {
//       less_half_edge lessElm(&x_, &just_before_);
//       edge_scanline_ = edge_scanline(lessElm);
//     }
//     inline line_intersection(const line_intersection& that) : vertical_data_(), edge_scanline_(), x_(), just_before_(), segment_id_(), event_edges_(), intersection_queue_() { (*this) = that; }
//     inline line_intersection& operator=(const line_intersection& that) {
//       x_ = that.x_;
//       just_before_ = that.just_before_;
//       segment_id_ = that.segment_id_;

//       //I cannot simply copy that.edge_scanline_ to this edge_scanline_ becuase the functor store pointers to other members!
//       less_half_edge lessElm(&x_, &just_before_);
//       edge_scanline_ = edge_scanline(lessElm);

//       edge_scanline_.insert(that.edge_scanline_.begin(), that.edge_scanline_.end());
//       return *this;
//     }

//     static inline void between(Point pt, Point pt1, Point pt2) {
//       less_point lp;
//       if(lp(pt1, pt2))
//         return lp(pt, pt2) && lp(pt1, pt);
//       return lp(pt, pt1) && lp(pt2, pt);
//     }

    template <typename iT>
    static inline void compute_histogram_in_y(iT begin, iT end, std::size_t size, std::vector<std::pair<Unit, std::pair<std::size_t, std::size_t> > >& histogram) {
      std::vector<std::pair<Unit, int> > ends;
      ends.reserve(size * 2);
      for(iT itr = begin ; itr != end; ++itr) {
        int count = (*itr).first.first.y() < (*itr).first.second.y() ? 1 : -1;
        ends.push_back(std::make_pair((*itr).first.first.y(), count));
        ends.push_back(std::make_pair((*itr).first.second.y(), -count));
      }
      polygon_sort(ends.begin(), ends.end());
      histogram.reserve(ends.size());
      histogram.push_back(std::make_pair(ends.front().first, std::make_pair(0, 0)));
      for(typename std::vector<std::pair<Unit, int> >::iterator itr = ends.begin(); itr != ends.end(); ++itr) {
        if((*itr).first != histogram.back().first) {
          histogram.push_back(std::make_pair((*itr).first, histogram.back().second));
        }
        if((*itr).second < 0)
          histogram.back().second.second -= (*itr).second;
        histogram.back().second.first += (*itr).second;
      }
    }

    template <typename iT>
    static inline void compute_y_cuts(std::vector<Unit>& y_cuts, iT begin, iT end, std::size_t size) {
      if(begin == end) return;
      if(size < 30) return; //30 is empirically chosen, but the algorithm is not sensitive to this constant
      std::size_t min_cut = size;
      iT cut = begin;
      std::size_t position = 0;
      std::size_t cut_size = 0;
      std::size_t histogram_size = std::distance(begin, end);
      for(iT itr = begin; itr != end; ++itr, ++position) {
        if(position < histogram_size / 3)
          continue;
        if(histogram_size - position < histogram_size / 3) break;
        if((*itr).second.first < min_cut) {
          cut = itr;
          min_cut = (*cut).second.first;
          cut_size = position;
        }
      }
      if(cut_size == 0 || (*cut).second.first > size / 9) //nine is empirically chosen
        return;
      compute_y_cuts(y_cuts, begin, cut, (*cut).second.first + (*cut).second.second);
      y_cuts.push_back((*cut).first);
      compute_y_cuts(y_cuts, cut, end, size - (*cut).second.second);
    }

    template <typename iT>
    static inline void validate_scan_divide_and_conquer(std::vector<std::set<Point> >& intersection_points,
                                                        iT begin, iT end) {
      std::vector<std::pair<Unit, std::pair<std::size_t, std::size_t> > > histogram;
      compute_histogram_in_y(begin, end, std::distance(begin, end), histogram);
      std::vector<Unit> y_cuts;
      compute_y_cuts(y_cuts, histogram.begin(), histogram.end(), std::distance(begin, end));
      std::map<Unit, std::vector<std::pair<half_edge, segment_id> > > bins;
      bins[histogram.front().first] = std::vector<std::pair<half_edge, segment_id> >();
      for(typename std::vector<Unit>::iterator itr = y_cuts.begin(); itr != y_cuts.end(); ++itr) {
        bins[*itr] = std::vector<std::pair<half_edge, segment_id> >();
      }
      for(iT itr = begin; itr != end; ++itr) {
        typename std::map<Unit, std::vector<std::pair<half_edge, segment_id> > >::iterator lb =
          bins.lower_bound((std::min)((*itr).first.first.y(), (*itr).first.second.y()));
        if(lb != bins.begin())
          --lb;
        typename std::map<Unit, std::vector<std::pair<half_edge, segment_id> > >::iterator ub =
          bins.upper_bound((std::max)((*itr).first.first.y(), (*itr).first.second.y()));
        for( ; lb != ub; ++lb) {
          (*lb).second.push_back(*itr);
        }
      }
      validate_scan(intersection_points, bins[histogram.front().first].begin(), bins[histogram.front().first].end());
      for(typename std::vector<Unit>::iterator itr = y_cuts.begin(); itr != y_cuts.end(); ++itr) {
        validate_scan(intersection_points, bins[*itr].begin(), bins[*itr].end(), *itr);
      }
    }

    template <typename iT>
    static inline void validate_scan(std::vector<std::set<Point> >& intersection_points,
                                     iT begin, iT end) {
      validate_scan(intersection_points, begin, end, (std::numeric_limits<Unit>::min)());
    }
    //quadratic algorithm to do same work as optimal scan for cross checking
    template <typename iT>
    static inline void validate_scan(std::vector<std::set<Point> >& intersection_points,
                                     iT begin, iT end, Unit min_y) {
      std::vector<Point> pts;
      std::vector<std::pair<half_edge, segment_id> > data(begin, end);
      for(std::size_t i = 0; i < data.size(); ++i) {
        if(data[i].first.second < data[i].first.first) {
          std::swap(data[i].first.first, data[i].first.second);
        }
      }
      typename scanline_base<Unit>::compute_intersection_pack pack_;
      polygon_sort(data.begin(), data.end());
      //find all intersection points
      for(typename std::vector<std::pair<half_edge, segment_id> >::iterator outer = data.begin();
          outer != data.end(); ++outer) {
        const half_edge& he1 = (*outer).first;
        //its own end points
        pts.push_back(he1.first);
        pts.push_back(he1.second);
        std::set<Point>& segmentpts = intersection_points[(*outer).second];
        for(typename std::set<Point>::iterator itr = segmentpts.begin(); itr != segmentpts.end(); ++itr) {
          if ((*itr).y() >= min_y) {
            pts.push_back(*itr);
          }
        }
        bool have_first_y = he1.first.y() >= min_y && he1.second.y() >= min_y;
        for(typename std::vector<std::pair<half_edge, segment_id> >::iterator inner = outer;
            inner != data.end(); ++inner) {
          const half_edge& he2 = (*inner).first;
          if(have_first_y || (he2.first.y() >= min_y && he2.second.y() >= min_y)) {
            //at least one segment has a low y value within the range
            if(he1 == he2) continue;
            if((std::min)(he2. first.get(HORIZONTAL),
                          he2.second.get(HORIZONTAL)) >=
               (std::max)(he1.second.get(HORIZONTAL),
                          he1.first.get(HORIZONTAL)))
              break;
            if(he1.first == he2.first || he1.second == he2.second)
              continue;
            Point intersection;
            if(pack_.compute_intersection(intersection, he1, he2)) {
              //their intersection point
              pts.push_back(intersection);
              intersection_points[(*inner).second].insert(intersection);
              intersection_points[(*outer).second].insert(intersection);
            }
          }
        }
      }
      polygon_sort(pts.begin(), pts.end());
      typename std::vector<Point>::iterator newend = std::unique(pts.begin(), pts.end());
      typename std::vector<Point>::iterator lfinger = pts.begin();
      //find all segments that interact with intersection points
      for(typename std::vector<std::pair<half_edge, segment_id> >::iterator outer = data.begin();
          outer != data.end(); ++outer) {
        const half_edge& he1 = (*outer).first;
        segment_id id1 = (*outer).second;
        //typedef rectangle_data<Unit> Rectangle;
        //Rectangle rect1;
        //set_points(rect1, he1.first, he1.second);
        //typename std::vector<Point>::iterator itr = lower_bound(pts.begin(), newend, (std::min)(he1.first, he1.second));
        //typename std::vector<Point>::iterator itr2 = upper_bound(pts.begin(), newend, (std::max)(he1.first, he1.second));
        Point startpt = (std::min)(he1.first, he1.second);
        Point stoppt = (std::max)(he1.first, he1.second);
        //while(itr != newend && itr != pts.begin() && (*itr).get(HORIZONTAL) >= (std::min)(he1.first.get(HORIZONTAL), he1.second.get(HORIZONTAL))) --itr;
        //while(itr2 != newend && (*itr2).get(HORIZONTAL) <= (std::max)(he1.first.get(HORIZONTAL), he1.second.get(HORIZONTAL))) ++itr2;
        //itr = pts.begin();
        //itr2 = pts.end();
        while(lfinger != newend && (*lfinger).x() < startpt.x()) ++lfinger;
        for(typename std::vector<Point>::iterator itr = lfinger ; itr != newend && (*itr).x() <= stoppt.x(); ++itr) {
          if(scanline_base<Unit>::intersects_grid(*itr, he1))
            intersection_points[id1].insert(*itr);
        }
      }
    }

    template <typename iT, typename property_type>
    static inline void validate_scan(std::vector<std::pair<half_edge, std::pair<property_type, int> > >& output_segments,
                                     iT begin, iT end) {
      std::vector<std::pair<property_type, int> > input_properties;
      std::vector<std::pair<half_edge, int> > input_segments, intermediate_segments;
      int index = 0;
      for( ; begin != end; ++begin) {
        input_properties.push_back((*begin).second);
        input_segments.push_back(std::make_pair((*begin).first, index++));
      }
      validate_scan(intermediate_segments, input_segments.begin(), input_segments.end());
      for(std::size_t i = 0; i < intermediate_segments.size(); ++i) {
        output_segments.push_back(std::make_pair(intermediate_segments[i].first,
                                                 input_properties[intermediate_segments[i].second]));
        less_point lp;
        if(lp(output_segments.back().first.first, output_segments.back().first.second) !=
           lp(input_segments[intermediate_segments[i].second].first.first,
              input_segments[intermediate_segments[i].second].first.second)) {
          //edge changed orientation, invert count on edge
          output_segments.back().second.second *= -1;
        }
        if(!scanline_base<Unit>::is_vertical(input_segments[intermediate_segments[i].second].first) &&
           scanline_base<Unit>::is_vertical(output_segments.back().first)) {
          output_segments.back().second.second *= -1;
        }
        if(lp(output_segments.back().first.second, output_segments.back().first.first)) {
          std::swap(output_segments.back().first.first, output_segments.back().first.second);
        }
      }
    }

    template <typename iT>
    static inline void validate_scan(std::vector<std::pair<half_edge, int> >& output_segments,
                                     iT begin, iT end) {
      std::vector<std::set<Point> > intersection_points(std::distance(begin, end));
      validate_scan_divide_and_conquer(intersection_points, begin, end);
      //validate_scan(intersection_points, begin, end);
      segment_intersections(output_segments, intersection_points, begin, end);
//       std::pair<segment_id, segment_id> offenders;
//       if(!verify_scan(offenders, output_segments.begin(), output_segments.end())) {
//         std::cout << "break here!\n";
//         for(typename std::set<Point>::iterator itr = intersection_points[offenders.first].begin();
//             itr != intersection_points[offenders.first].end(); ++itr) {
//           std::cout << (*itr).x() << " " << (*itr).y() << " ";
//         } std::cout << "\n";
//         for(typename std::set<Point>::iterator itr = intersection_points[offenders.second].begin();
//             itr != intersection_points[offenders.second].end(); ++itr) {
//           std::cout << (*itr).x() << " " << (*itr).y() << " ";
//         } std::cout << "\n";
//         exit(1);
//       }
    }

    //quadratic algorithm to find intersections
    template <typename iT, typename segment_id>
    static inline bool verify_scan(std::pair<segment_id, segment_id>& offenders,
                                   iT begin, iT end) {

      std::vector<std::pair<half_edge, segment_id> > data(begin, end);
      for(std::size_t i = 0; i < data.size(); ++i) {
        if(data[i].first.second < data[i].first.first) {
          std::swap(data[i].first.first, data[i].first.second);
        }
      }
      polygon_sort(data.begin(), data.end());
      for(typename std::vector<std::pair<half_edge, segment_id> >::iterator outer = data.begin();
          outer != data.end(); ++outer) {
        const half_edge& he1 = (*outer).first;
        segment_id id1 = (*outer).second;
        for(typename std::vector<std::pair<half_edge, segment_id> >::iterator inner = outer;
            inner != data.end(); ++inner) {
          const half_edge& he2 = (*inner).first;
          if(he1 == he2) continue;
          if((std::min)(he2. first.get(HORIZONTAL),
                        he2.second.get(HORIZONTAL)) >
             (std::max)(he1.second.get(HORIZONTAL),
                        he1.first.get(HORIZONTAL)))
            break;
          segment_id id2 = (*inner).second;
          if(scanline_base<Unit>::intersects(he1, he2)) {
            offenders.first = id1;
            offenders.second = id2;
            //std::cout << he1.first.x() << " " << he1.first.y() << " " << he1.second.x() << " " << he1.second.y() << " " << he2.first.x() << " " << he2.first.y() << " " << he2.second.x() << " " << he2.second.y() << "\n";
            return false;
          }
        }
      }
      return true;
    }

    class less_point_down_slope {
    public:
      typedef Point first_argument_type;
      typedef Point second_argument_type;
      typedef bool result_type;
      inline less_point_down_slope() {}
      inline bool operator () (const Point& pt1, const Point& pt2) const {
        if(pt1.get(HORIZONTAL) < pt2.get(HORIZONTAL)) return true;
        if(pt1.get(HORIZONTAL) == pt2.get(HORIZONTAL)) {
          if(pt1.get(VERTICAL) > pt2.get(VERTICAL)) return true;
        }
        return false;
      }
    };

    template <typename iT>
    static inline void segment_edge(std::vector<std::pair<half_edge, int> >& output_segments,
                                    const half_edge& , segment_id id, iT begin, iT end) {
      iT current = begin;
      iT next = begin;
      ++next;
      while(next != end) {
        output_segments.push_back(std::make_pair(half_edge(*current, *next), id));
        current = next;
        ++next;
      }
    }

    template <typename iT>
    static inline void segment_intersections(std::vector<std::pair<half_edge, int> >& output_segments,
                                             std::vector<std::set<Point> >& intersection_points,
                                             iT begin, iT end) {
      for(iT iter = begin; iter != end; ++iter) {
        //less_point lp;
        const half_edge& he = (*iter).first;
        //if(lp(he.first, he.second)) {
        //  //it is the begin event
          segment_id id = (*iter).second;
          const std::set<Point>& pts = intersection_points[id];
          Point hpt(he.first.get(HORIZONTAL)+1, he.first.get(VERTICAL));
          if(!scanline_base<Unit>::is_vertical(he) && scanline_base<Unit>::less_slope(he.first.get(HORIZONTAL), he.first.get(VERTICAL),
                                            he.second, hpt)) {
            //slope is below horizontal
            std::vector<Point> tmpPts;
            tmpPts.reserve(pts.size());
            tmpPts.insert(tmpPts.end(), pts.begin(), pts.end());
            less_point_down_slope lpds;
            polygon_sort(tmpPts.begin(), tmpPts.end(), lpds);
            segment_edge(output_segments, he, id, tmpPts.begin(), tmpPts.end());
          } else {
            segment_edge(output_segments, he, id, pts.begin(), pts.end());
          }
          //}
      }
    }

//     //iT iterator over unsorted pair<Point> representing line segments of input
//     //output_segments is populated with fully intersected output line segment half
//     //edges and the index of the input segment that they are assoicated with
//     //duplicate output half edges with different ids will be generated in the case
//     //that parallel input segments intersection
//     //outputs are in sorted order and include both begin and end events for
//     //each segment
//     template <typename iT>
//     inline void scan(std::vector<std::pair<half_edge, int> >& output_segments,
//                      iT begin, iT end) {
//       std::map<segment_id, std::set<Point> > intersection_points;
//       scan(intersection_points, begin, end);
//       segment_intersections(output_segments, intersection_points, begin, end);
//     }

//     //iT iterator over sorted sequence of half edge, segment id pairs representing segment begin and end points
//     //intersection points provides a mapping from input segment id (vector index) to the set
//     //of intersection points assocated with that input segment
//     template <typename iT>
//     inline void scan(std::map<segment_id, std::set<Point> >& intersection_points,
//                      iT begin, iT end) {
//       for(iT iter = begin; iter != end; ++iter) {
//         const std::pair<half_edge, int>& elem = *iter;
//         const half_edge& he = elem.first;
//         Unit current_x = he.first.get(HORIZONTAL);
//         if(current_x != x_) {
//           process_scan_event(intersection_points);
//           while(!intersection_queue_.empty() &&
//                 (*(intersection_queue_.begin()).get(HORIZONTAL) < current_x)) {
//             x_ = *(intersection_queue_.begin()).get(HORIZONTAL);
//             process_intersections_at_scan_event(intersection_points);
//           }
//           x_ = current_x;
//         }
//         event_edges_.push_back(elem);
//       }
//       process_scan_event(intersection_points);
//     }

//     inline iterator lookup(const half_edge& he) {
//       return edge_scanline_.find(he);
//     }

//     inline void insert_into_scanline(const half_edge& he, int id) {
//       edge_scanline_[he].insert(id);
//     }

//     inline void lookup_and_remove(const half_edge& he, int id) {
//       iterator remove_iter = lookup(he);
//       if(remove_iter == edge_scanline_.end()) {
//         //std::cout << "failed to find removal segment in scanline\n";
//         return;
//       }
//       std::set<segment_id>& ids = (*remove_iter).second;
//       std::set<segment_id>::iterator id_iter = ids.find(id);
//       if(id_iter == ids.end()) {
//         //std::cout << "failed to find removal segment id in scanline set\n";
//         return;
//       }
//       ids.erase(id_iter);
//       if(ids.empty())
//         edge_scanline_.erase(remove_iter);
//     }

//     static inline void update_segments(std::map<segment_id, std::set<Point> >& intersection_points,
//                                        const std::set<segment_id>& segments, Point pt) {
//       for(std::set<segment_id>::const_iterator itr = segments.begin(); itr != segments.end(); ++itr) {
//         intersection_points[*itr].insert(pt);
//       }
//     }

//     inline void process_intersections_at_scan_event(std::map<segment_id, std::set<Point> >& intersection_points) {
//       //there may be additional intersection points at this x location that haven't been
//       //found yet if vertical or near vertical line segments intersect more than
//       //once before the next x location
//       just_before_ = true;
//       std::set<iterator> intersecting_elements;
//       std::set<Unit> intersection_locations;
//       typedef typename std::set<Point>::iterator intersection_iterator;
//       intersection_iterator iter;
//       //first find all secondary intersection locations and all scanline iterators
//       //that are intersecting
//       for(iter = intersection_queue_.begin();
//           iter != intersection_queue_.end() && (*iter).get(HORIZONTAL) == x_; ++iter) {
//         Point pt = *iter;
//         Unit y = pt.get(VERTICAL);
//         intersection_locations.insert(y);
//         //if x_ is max there can be only end events and no sloping edges
//         if(x_ != (std::numeric_limits<Unit>::max)()) {
//           //deal with edges that project to the right of scanline
//           //first find the edges in the scanline adjacent to primary intersectin points
//           //lookup segment in scanline at pt
//           iterator itr = edge_scanline_.lower_bound(half_edge(pt, Point(x_+1, y)));
//           //look above pt in scanline until reaching end or segment that doesn't intersect
//           //1x1 grid upper right of pt
//           //look below pt in scanline until reaching begin or segment that doesn't interset
//           //1x1 grid upper right of pt

//           //second find edges in scanline on the y interval of each edge found in the previous
//           //step for x_ to x_ + 1

//           //third find overlaps in the y intervals of all found edges to find all
//           //secondary intersection points

//         }
//       }
//       //erase the intersection points from the queue
//       intersection_queue_.erase(intersection_queue_.begin(), iter);
//       std::vector<scanline_element> insertion_edges;
//       insertion_edges.reserve(intersecting_elements.size());
//       std::vector<std::pair<Unit, iterator> > sloping_ends;
//       //do all the work of updating the output of all intersecting
//       for(typename std::set<iterator>::iterator inter_iter = intersecting_elements.begin();
//           inter_iter != intersecting_elements.end(); ++inter_iter) {
//         //if it is horizontal update it now and continue
//         if(is_horizontal((*inter_iter).first)) {
//           update_segments(intersection_points, (*inter_iter).second, Point(x_, (*inter_iter).first.get(VERTICAL)));
//         } else {
//           //if x_ is max there can be only end events and no sloping edges
//           if(x_ != (std::numeric_limits<Unit>::max)()) {
//             //insert its end points into the vector of sloping ends
//             const half_edge& he = (*inter_iter).first;
//             Unit y = evalAtXforY(x_, he.first, he.second);
//             Unit y2 = evalAtXforY(x_+1, he.first, he.second);
//             if(y2 >= y) y2 +=1; //we round up, in exact case we don't worry about overbite of one
//             else y += 1; //downward sloping round up
//             sloping_ends.push_back(std::make_pair(y, inter_iter));
//             sloping_ends.push_back(std::make_pair(y2, inter_iter));
//           }
//         }
//       }

//       //merge sloping element data
//       polygon_sort(sloping_ends.begin(), sloping_ends.end());
//       std::map<Unit, std::set<iterator> > sloping_elements;
//       std::set<iterator> merge_elements;
//       for(typename std::vector<std::pair<Unit, iterator> >::iterator slop_iter = sloping_ends.begin();
//           slop_iter == sloping_ends.end(); ++slop_iter) {
//         //merge into sloping elements
//         typename std::set<iterator>::iterator merge_iterator = merge_elements.find((*slop_iter).second);
//         if(merge_iterator == merge_elements.end()) {
//           merge_elements.insert((*slop_iter).second);
//         } else {
//           merge_elements.erase(merge_iterator);
//         }
//         sloping_elements[(*slop_iter).first] = merge_elements;
//       }

//       //scan intersection points
//       typename std::map<Unit, std::set<segment_id> >::iterator vertical_iter = vertical_data_.begin();
//       typename std::map<Unit, std::set<iterator> >::iterator sloping_iter = sloping_elements.begin();
//       for(typename std::set<Unit>::iterator position_iter = intersection_locations.begin();
//           position_iter == intersection_locations.end(); ++position_iter) {
//         //look for vertical segments that intersect this point and update them
//         Unit y = *position_iter;
//         Point pt(x_, y);
//         //handle vertical segments
//         if(vertical_iter != vertical_data_.end()) {
//           typename std::map<Unit, std::set<segment_id> >::iterator next_vertical = vertical_iter;
//           for(++next_vertical; next_vertical != vertical_data_.end() &&
//                 (*next_vertical).first < y; ++next_vertical) {
//             vertical_iter = next_vertical;
//           }
//           if((*vertical_iter).first < y && !(*vertical_iter).second.empty()) {
//             update_segments(intersection_points, (*vertical_iter).second, pt);
//             ++vertical_iter;
//             if(vertical_iter != vertical_data_.end() && (*vertical_iter).first == y)
//               update_segments(intersection_points, (*vertical_iter).second, pt);
//           }
//         }
//         //handle sloping segments
//         if(sloping_iter != sloping_elements.end()) {
//           typename std::map<Unit, std::set<iterator> >::iterator next_sloping = sloping_iter;
//           for(++next_sloping; next_sloping != sloping_elements.end() &&
//                 (*next_sloping).first < y; ++next_sloping) {
//             sloping_iter = next_sloping;
//           }
//           if((*sloping_iter).first < y && !(*sloping_iter).second.empty()) {
//             for(typename std::set<iterator>::iterator element_iter = (*sloping_iter).second.begin();
//                 element_iter != (*sloping_iter).second.end(); ++element_iter) {
//               const half_edge& he = (*element_iter).first;
//               if(intersects_grid(pt, he)) {
//                 update_segments(intersection_points, (*element_iter).second, pt);
//               }
//             }
//             ++sloping_iter;
//             if(sloping_iter != sloping_elements.end() && (*sloping_iter).first == y &&
//                !(*sloping_iter).second.empty()) {
//               for(typename std::set<iterator>::iterator element_iter = (*sloping_iter).second.begin();
//                   element_iter != (*sloping_iter).second.end(); ++element_iter) {
//                 const half_edge& he = (*element_iter).first;
//                 if(intersects_grid(pt, he)) {
//                   update_segments(intersection_points, (*element_iter).second, pt);
//                 }
//               }
//             }
//           }
//         }
//       }

//       //erase and reinsert edges into scanline with check for future intersection
//     }

//     inline void process_scan_event(std::map<segment_id, std::set<Point> >& intersection_points) {
//       just_before_ = true;

//       //process end events by removing those segments from the scanline
//       //and insert vertices of all events into intersection queue
//       Point prev_point((std::numeric_limits<Unit>::min)(), (std::numeric_limits<Unit>::min)());
//       less_point lp;
//       std::set<segment_id> vertical_ids;
//       vertical_data_.clear();
//       for(std::size_t i = 0; i < event_edges_.size(); ++i) {
//         segment_id id = event_edges_[i].second;
//         const half_edge& he = event_edges_[i].first;
//         //vertical half edges are handled during intersection processing because
//         //they cannot be inserted into the scanline
//         if(!is_vertical(he)) {
//           if(lp(he.second, he.first)) {
//             //half edge is end event
//             lookup_and_remove(he, id);
//           } else {
//             //half edge is begin event
//             insert_into_scanline(he, id);
//             //note that they will be immediately removed and reinserted after
//             //handling their intersection (vertex)
//             //an optimization would allow them to be processed specially to avoid the redundant
//             //removal and reinsertion
//           }
//         } else {
//           //common case if you are lucky
//           //update the map of y to set of segment id
//           if(lp(he.second, he.first)) {
//             //half edge is end event
//             std::set<segment_id>::iterator itr = vertical_ids.find(id);
//             if(itr == vertical_ids.end()) {
//               //std::cout << "Failed to find end event id in vertical ids\n";
//             } else {
//               vertical_ids.erase(itr);
//               vertical_data_[he.first.get(HORIZONTAL)] = vertical_ids;
//             }
//           } else {
//             //half edge is a begin event
//             vertical_ids.insert(id);
//             vertical_data_[he.first.get(HORIZONTAL)] = vertical_ids;
//           }
//         }
//         //prevent repeated insertion of same vertex into intersection queue
//         if(prev_point != he.first)
//           intersection_queue_.insert(he.first);
//         else
//           prev_point = he.first;
//         // process intersections at scan event
//         process_intersections_at_scan_event(intersection_points);
//       }
//       event_edges_.clear();
//     }

  public:
    template <typename stream_type>
    static inline bool test_validate_scan(stream_type& stdcout) {
      std::vector<std::pair<half_edge, segment_id> > input, edges;
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(0, 10)), 0));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(10, 10)), 1));
      std::pair<segment_id, segment_id> result;
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail1 " << result.first << " " << result.second << "\n";
        return false;
      }
      input.push_back(std::make_pair(half_edge(Point(0, 5), Point(5, 5)), 2));
      edges.clear();
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 " << result.first << " " << result.second << "\n";
        return false;
      }
      input.pop_back();
      input.push_back(std::make_pair(half_edge(Point(1, 0), Point(11, 11)), input.size()));
      edges.clear();
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail3 " << result.first << " " << result.second << "\n";
        return false;
      }
      input.push_back(std::make_pair(half_edge(Point(1, 0), Point(10, 11)), input.size()));
      edges.clear();
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail4 " << result.first << " " << result.second << "\n";
        return false;
      }
      input.pop_back();
      input.push_back(std::make_pair(half_edge(Point(1, 2), Point(11, 11)), input.size()));
      edges.clear();
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail5 " << result.first << " " << result.second << "\n";
        return false;
      }
      input.push_back(std::make_pair(half_edge(Point(0, 5), Point(0, 11)), input.size()));
      edges.clear();
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail6 " << result.first << " " << result.second << "\n";
        return false;
      }
      input.pop_back();
      for(std::size_t i = 0; i < input.size(); ++i) {
        std::swap(input[i].first.first, input[i].first.second);
      }
      edges.clear();
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail5 2 " << result.first << " " << result.second << "\n";
        return false;
      }
      for(std::size_t i = 0; i < input.size(); ++i) {
        input[i].first.first = Point(input[i].first.first.get(HORIZONTAL) * -1,
                                     input[i].first.first.get(VERTICAL) * -1);
        input[i].first.second = Point(input[i].first.second.get(HORIZONTAL) * -1,
                                     input[i].first.second.get(VERTICAL) * -1);
      }
      edges.clear();
      validate_scan(edges, input.begin(), input.end());
      stdcout << edges.size() << "\n";
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail5 3 " << result.first << " " << result.second << "\n";
        return false;
      }
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(5, 7), Point(7, 6)), 0));
      input.push_back(std::make_pair(half_edge(Point(2, 4), Point(6, 7)), 1));
            validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 1 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(3, 2), Point(1, 7)), 0));
      input.push_back(std::make_pair(half_edge(Point(0, 6), Point(7, 4)), 1));
            validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 2 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(6, 6), Point(1, 0)), 0));
      input.push_back(std::make_pair(half_edge(Point(3, 6), Point(2, 3)), 1));
            validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 3 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(7, 0)), 0));
      input.push_back(std::make_pair(half_edge(Point(6, 0), Point(2, 0)), 1));
            validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 4 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(-17333131 - -17208131, -10316869 - -10191869), Point(0, 0)), 0));
      input.push_back(std::make_pair(half_edge(Point(-17291260 - -17208131, -10200000 - -10191869), Point(-17075000 - -17208131, -10200000 - -10191869)), 1));
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 5 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(-17333131, -10316869), Point(-17208131, -10191869)), 0));
      input.push_back(std::make_pair(half_edge(Point(-17291260, -10200000), Point(-17075000, -10200000)), 1));
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 6 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(-9850009+9853379, -286971+290340), Point(-12777869+9853379, -3214831+290340)), 0));
      input.push_back(std::make_pair(half_edge(Point(-5223510+9853379, -290340+290340), Point(-9858140+9853379, -290340+290340)), 1));
      validate_scan(edges, input.begin(), input.end());
      print(edges);
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 7 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(-9850009, -286971), Point(-12777869, -3214831)), 0));
      input.push_back(std::make_pair(half_edge(Point(-5223510, -290340), Point(-9858140, -290340)), 1));
      validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 8 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      //3 3 2 2: 0; 4 2 0 6: 1; 0 3 6 3: 2; 4 1 5 5: 3;
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(3, 3), Point(2, 2)), 0));
      input.push_back(std::make_pair(half_edge(Point(4, 2), Point(0, 6)), 1));
      input.push_back(std::make_pair(half_edge(Point(0, 3), Point(6, 3)), 2));
      input.push_back(std::make_pair(half_edge(Point(4, 1), Point(5, 5)), 3));
            validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail4 1 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      //5 7 1 3: 0; 4 5 2 1: 1; 2 5 2 1: 2; 4 1 5 3: 3;
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(5, 7), Point(1, 3)), 0));
      input.push_back(std::make_pair(half_edge(Point(4, 5), Point(2, 1)), 1));
      input.push_back(std::make_pair(half_edge(Point(2, 5), Point(2, 1)), 2));
      input.push_back(std::make_pair(half_edge(Point(4, 1), Point(5, 3)), 3));
            validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail4 2 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      //1 0 -4 -1: 0; 0 0 2 -1: 1;
      input.clear();
      edges.clear();
      input.push_back(std::make_pair(half_edge(Point(1, 0), Point(-4, -1)), 0));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(2, -1)), 1));
            validate_scan(edges, input.begin(), input.end());
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "s fail2 5 " << result.first << " " << result.second << "\n";
        print(input);
        print(edges);
        return false;
      }
      Unit min_c =0;
      Unit max_c =0;
      for(unsigned int outer = 0; outer < 1000; ++outer) {
        input.clear();
        for(unsigned int i = 0; i < 4; ++i) {
          Unit x1 = rand();
          Unit x2 = rand();
          Unit y1 = rand();
          Unit y2 = rand();
          int neg1 = rand() % 2;
          if(neg1) x1 *= -1;
          int neg2 = rand() % 2;
          if(neg2) x2 *= -1;
          int neg3 = rand() % 2;
          if(neg3) y1 *= -1;
          int neg4 = rand() % 2;
          if(neg4) y2 *= -1;
          if(x1 < min_c) min_c = x1;
          if(x2 < min_c) min_c = x2;
          if(y1 < min_c) min_c = y1;
          if(y2 < min_c) min_c = y2;
          if(x1 > max_c) max_c = x1;
          if(x2 > max_c) max_c = x2;
          if(y1 > max_c) max_c = y1;
          if(y2 > max_c) max_c = y2;
          Point pt1(x1, y1);
          Point pt2(x2, y2);
          if(pt1 != pt2)
            input.push_back(std::make_pair(half_edge(pt1, pt2), i));
        }
        edges.clear();
        validate_scan(edges, input.begin(), input.end());
        if(!verify_scan(result, edges.begin(), edges.end())) {
          stdcout << "s fail9 " << outer << ": " << result.first << " " << result.second << "\n";
          print(input);
          print(edges);
          return false;
        }
      }
      return true;
    }

    //static void print(const std::pair<half_edge, segment_id>& segment) {
      //std::cout << segment.first.first << " " << segment.first.second << ": " << segment.second << "; ";
    //}
    static void print(const std::vector<std::pair<half_edge, segment_id> >& vec) {
      for(std::size_t i = 0; i < vec.size(); ++ i) {
      //  print(vec[i]);
      }
      //std::cout << "\n";
    }

    template <typename stream_type>
    static inline bool test_verify_scan(stream_type& stdcout) {
      std::vector<std::pair<half_edge, segment_id> > edges;
      edges.push_back(std::make_pair(half_edge(Point(0, 0), Point(0, 10)), 0));
      edges.push_back(std::make_pair(half_edge(Point(0, 0), Point(10, 10)), 1));
      std::pair<segment_id, segment_id> result;
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "fail1\n";
        return false;
      }
      edges.push_back(std::make_pair(half_edge(Point(0, 5), Point(5, 5)), 2));
      if(verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "fail2\n";
        return false;
      }
      edges.pop_back();
      edges.push_back(std::make_pair(half_edge(Point(1, 0), Point(11, 11)), (segment_id)edges.size()));
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "fail3\n";
        return false;
      }
      edges.push_back(std::make_pair(half_edge(Point(1, 0), Point(10, 11)), (segment_id)edges.size()));
      if(verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "fail4\n";
        return false;
      }
      edges.pop_back();
      edges.push_back(std::make_pair(half_edge(Point(1, 2), Point(11, 11)), (segment_id)edges.size()));
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "fail5 " << result.first << " " << result.second << "\n";
        return false;
      }
      edges.push_back(std::make_pair(half_edge(Point(0, 5), Point(0, 11)), (segment_id)edges.size()));
      if(verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "fail6 " << result.first << " " << result.second << "\n";
        return false;
      }
      edges.pop_back();
      for(std::size_t i = 0; i < edges.size(); ++i) {
        std::swap(edges[i].first.first, edges[i].first.second);
      }
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "fail5 2 " << result.first << " " << result.second << "\n";
        return false;
      }
      for(std::size_t i = 0; i < edges.size(); ++i) {
        edges[i].first.first = Point(edges[i].first.first.get(HORIZONTAL) * -1,
                                     edges[i].first.first.get(VERTICAL) * -1);
        edges[i].first.second = Point(edges[i].first.second.get(HORIZONTAL) * -1,
                                     edges[i].first.second.get(VERTICAL) * -1);
      }
      if(!verify_scan(result, edges.begin(), edges.end())) {
        stdcout << "fail5 3 " << result.first << " " << result.second << "\n";
        return false;
      }
      return true;
    }

  };

  //scanline consumes the "flattened" fully intersected line segments produced by
  //a pass of line_intersection along with property and count information and performs a
  //useful operation like booleans or property merge or connectivity extraction
  template <typename Unit, typename property_type, typename keytype = std::set<property_type> >
  class scanline : public scanline_base<Unit> {
  public:
    //definitions
    typedef typename scanline_base<Unit>::Point Point;

    //the first point is the vertex and and second point establishes the slope of an edge eminating from the vertex
    //typedef std::pair<Point, Point> half_edge;
    typedef typename scanline_base<Unit>::half_edge half_edge;

    //scanline comparator functor
    typedef typename scanline_base<Unit>::less_half_edge less_half_edge;
    typedef typename scanline_base<Unit>::less_point less_point;

    typedef keytype property_set;
    //this is the data type used internally to store the combination of property counts at a given location
    typedef std::vector<std::pair<property_type, int> > property_map;
    //this data structure assocates a property and count to a half edge
    typedef std::pair<half_edge, std::pair<property_type, int> > vertex_property;
    //this data type is used internally to store the combined property data for a given half edge
    typedef std::pair<half_edge, property_map> vertex_data;
    //this data type stores the combination of many half edges
    typedef std::vector<vertex_property> property_merge_data;
    //this data structure stores end points of edges in the scanline
    typedef std::set<Point, less_point> end_point_queue;

    //this is the output data type that is created by the scanline before it is post processed based on content of property sets
    typedef std::pair<half_edge, std::pair<property_set, property_set> > half_edge_property;

    //this is the scanline data structure
    typedef std::map<half_edge, property_map, less_half_edge> scanline_type;
    typedef std::pair<half_edge, property_map> scanline_element;
    typedef typename scanline_type::iterator iterator;
    typedef typename scanline_type::const_iterator const_iterator;

    //data
    scanline_type scan_data_;
    std::vector<iterator> removal_set_; //edges to be removed at the current scanline stop
    std::vector<scanline_element> insertion_set_; //edge to be inserted after current scanline stop
    end_point_queue end_point_queue_;
    Unit x_;
    Unit y_;
    int just_before_;
    typename scanline_base<Unit>::evalAtXforYPack evalAtXforYPack_;
  public:
    inline scanline() : scan_data_(), removal_set_(), insertion_set_(), end_point_queue_(),
                        x_((std::numeric_limits<Unit>::max)()), y_((std::numeric_limits<Unit>::max)()), just_before_(false), evalAtXforYPack_() {
      less_half_edge lessElm(&x_, &just_before_, &evalAtXforYPack_);
      scan_data_ = scanline_type(lessElm);
    }
    inline scanline(const scanline& that) : scan_data_(), removal_set_(), insertion_set_(), end_point_queue_(),
                                            x_((std::numeric_limits<Unit>::max)()), y_((std::numeric_limits<Unit>::max)()), just_before_(false), evalAtXforYPack_() {
      (*this) = that; }
    inline scanline& operator=(const scanline& that) {
      x_ = that.x_;
      y_ = that.y_;
      just_before_ = that.just_before_;
      end_point_queue_ = that.end_point_queue_;
      //I cannot simply copy that.scanline_type to this scanline_type becuase the functor store pointers to other members!
      less_half_edge lessElm(&x_, &just_before_);
      scan_data_ = scanline_type(lessElm);

      scan_data_.insert(that.scan_data_.begin(), that.scan_data_.end());
      return *this;
    }

    template <typename result_type, typename result_functor>
    void write_out(result_type& result, result_functor rf, const half_edge& he,
                   const property_map& pm_left, const property_map& pm_right) {
      //std::cout << "write out ";
      //std::cout << he.first << ", " << he.second << "\n";
      property_set ps_left, ps_right;
      set_unique_property(ps_left, pm_left);
      set_unique_property(ps_right, pm_right);
      if(ps_left != ps_right) {
        //std::cout << "!equivalent\n";
        rf(result, he, ps_left, ps_right);
      }
    }

    template <typename result_type, typename result_functor, typename iT>
    iT handle_input_events(result_type& result, result_functor rf, iT begin, iT end) {
      //typedef typename high_precision_type<Unit>::type high_precision;
      //for each event
      property_map vertical_properties_above;
      property_map vertical_properties_below;
      half_edge vertical_edge_above;
      half_edge vertical_edge_below;
      std::vector<scanline_element> insertion_elements;
      //current_iter should increase monotonically toward end as we process scanline stop
      iterator current_iter = scan_data_.begin();
      just_before_ = true;
      Unit y = (std::numeric_limits<Unit>::min)();
      bool first_iteration = true;
      //we want to return from inside the loop when we hit end or new x
#ifdef BOOST_POLYGON_MSVC
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
      while(true) {
        if(begin == end || (!first_iteration && ((*begin).first.first.get(VERTICAL) != y ||
                                                 (*begin).first.first.get(HORIZONTAL) != x_))) {
          //lookup iterator range in scanline for elements coming in from the left
          //that end at this y
          Point pt(x_, y);
          //grab the properties coming in from below
          property_map properties_below;
          if(current_iter != scan_data_.end()) {
            //make sure we are looking at element in scanline just below y
            //if(evalAtXforY(x_, (*current_iter).first.first, (*current_iter).first.second) != y) {
            if(scanline_base<Unit>::on_above_or_below(Point(x_, y), (*current_iter).first) != 0) {
              Point e2(pt);
              if(e2.get(VERTICAL) != (std::numeric_limits<Unit>::max)())
                e2.set(VERTICAL, e2.get(VERTICAL) + 1);
              else
                e2.set(VERTICAL, e2.get(VERTICAL) - 1);
              half_edge vhe(pt, e2);
              current_iter = scan_data_.lower_bound(vhe);
            }
            if(current_iter != scan_data_.end()) {
              //get the bottom iterator for elements at this point
              //while(evalAtXforY(x_, (*current_iter).first.first, (*current_iter).first.second) >= (high_precision)y &&
              while(scanline_base<Unit>::on_above_or_below(Point(x_, y), (*current_iter).first) != 1 &&
                    current_iter != scan_data_.begin()) {
                --current_iter;
              }
              //if(evalAtXforY(x_, (*current_iter).first.first, (*current_iter).first.second) >= (high_precision)y) {
              if(scanline_base<Unit>::on_above_or_below(Point(x_, y), (*current_iter).first) != 1) {
                properties_below.clear();
              } else {
                properties_below = (*current_iter).second;
                //move back up to y or one past y
                ++current_iter;
              }
            }
          }
          std::vector<iterator> edges_from_left;
          while(current_iter != scan_data_.end() &&
                //can only be true if y is integer
                //evalAtXforY(x_, (*current_iter).first.first, (*current_iter).first.second) == y) {
                scanline_base<Unit>::on_above_or_below(Point(x_, y), (*current_iter).first) == 0) {
            //removal_set_.push_back(current_iter);
            ++current_iter;
          }
          //merge vertical count with count from below
          if(!vertical_properties_below.empty()) {
            merge_property_maps(vertical_properties_below, properties_below);
            //write out vertical edge
            write_out(result, rf, vertical_edge_below, properties_below, vertical_properties_below);
          } else {
            merge_property_maps(vertical_properties_below, properties_below);
          }
          //iteratively add intertion element counts to count from below
          //and write them to insertion set
          for(std::size_t i = 0; i < insertion_elements.size(); ++i) {
            if(i == 0) {
              merge_property_maps(insertion_elements[i].second, vertical_properties_below);
              write_out(result, rf, insertion_elements[i].first, insertion_elements[i].second, vertical_properties_below);
            } else {
              merge_property_maps(insertion_elements[i].second, insertion_elements[i-1].second);
              write_out(result, rf, insertion_elements[i].first, insertion_elements[i].second, insertion_elements[i-1].second);
            }
            insertion_set_.push_back(insertion_elements[i]);
          }
          if((begin == end || (*begin).first.first.get(HORIZONTAL) != x_)) {
            if(vertical_properties_above.empty()) {
              return begin;
            } else {
              y = vertical_edge_above.second.get(VERTICAL);
              vertical_properties_below.clear();
              vertical_properties_above.swap(vertical_properties_below);
              vertical_edge_below = vertical_edge_above;
              insertion_elements.clear();
              continue;
            }
          }
          vertical_properties_below.clear();
          vertical_properties_above.swap(vertical_properties_below);
          vertical_edge_below = vertical_edge_above;
          insertion_elements.clear();
        }
        if(begin != end) {
          const vertex_property& vp = *begin;
          const half_edge& he = vp.first;
          y = he.first.get(VERTICAL);
          first_iteration = false;
          if(! vertical_properties_below.empty() &&
             vertical_edge_below.second.get(VERTICAL) < y) {
            y = vertical_edge_below.second.get(VERTICAL);
            continue;
          }
          if(scanline_base<Unit>::is_vertical(he)) {
            update_property_map(vertical_properties_above, vp.second);
            vertical_edge_above = he;
          } else {
            if(insertion_elements.empty() ||
               insertion_elements.back().first != he) {
              insertion_elements.push_back(scanline_element(he, property_map()));
            }
            update_property_map(insertion_elements.back().second, vp.second);
          }
          ++begin;
        }
      }
#ifdef BOOST_POLYGON_MSVC
#pragma warning (pop)
#endif

    }

    inline void erase_end_events(typename end_point_queue::iterator epqi) {
      end_point_queue_.erase(end_point_queue_.begin(), epqi);
      for(typename std::vector<iterator>::iterator retire_itr = removal_set_.begin();
          retire_itr != removal_set_.end(); ++retire_itr) {
        scan_data_.erase(*retire_itr);
      }
      removal_set_.clear();
    }


    inline void remove_retired_edges_from_scanline() {
      just_before_ = true;
      typename end_point_queue::iterator epqi = end_point_queue_.begin();
      Unit current_x = x_;
      Unit previous_x = x_;
      while(epqi != end_point_queue_.end() &&
            (*epqi).get(HORIZONTAL) <= current_x) {
        x_ = (*epqi).get(HORIZONTAL);
        if(x_ != previous_x) erase_end_events(epqi);
        previous_x = x_;
        //lookup elements
        Point e2(*epqi);
        if(e2.get(VERTICAL) != (std::numeric_limits<Unit>::max)())
          e2.set(VERTICAL, e2.get(VERTICAL) + 1);
        else
          e2.set(VERTICAL, e2.get(VERTICAL) - 1);
        half_edge vhe_e(*epqi, e2);
        iterator current_iter = scan_data_.lower_bound(vhe_e);
        while(current_iter != scan_data_.end() && (*current_iter).first.second == (*epqi)) {
          //evalAtXforY(x_, (*current_iter).first.first, (*current_iter).first.second) == (*epqi).get(VERTICAL)) {
          removal_set_.push_back(current_iter);
          ++current_iter;
        }
        ++epqi;
      }
      x_ = current_x;
      erase_end_events(epqi);
    }

    inline void insert_new_edges_into_scanline() {
      just_before_ = false;
      for(typename std::vector<scanline_element>::iterator insert_itr = insertion_set_.begin();
          insert_itr != insertion_set_.end(); ++insert_itr) {
        scan_data_.insert(*insert_itr);
        end_point_queue_.insert((*insert_itr).first.second);
      }
      insertion_set_.clear();
    }

    //iterator over range of vertex property elements and call result functor
    //passing edge to be output, the merged data on both sides and the result
    template <typename result_type, typename result_functor, typename iT>
    void scan(result_type& result, result_functor rf, iT begin, iT end) {
      while(begin != end) {
        x_ = (*begin).first.first.get(HORIZONTAL); //update scanline stop location
        //print_scanline();
        --x_;
        remove_retired_edges_from_scanline();
        ++x_;
        begin = handle_input_events(result, rf, begin, end);
        remove_retired_edges_from_scanline();
        //print_scanline();
        insert_new_edges_into_scanline();
      }
      //print_scanline();
      x_ = (std::numeric_limits<Unit>::max)();
      remove_retired_edges_from_scanline();
    }

    //inline void print_scanline() {
    //  std::cout << "scanline at " << x_ << ": ";
    //  for(iterator itr = scan_data_.begin(); itr != scan_data_.end(); ++itr) {
    //    const scanline_element& se = *itr;
    //    const half_edge& he = se.first;
    //    const property_map& mp = se.second;
    //    std::cout << he.first << ", " << he.second << " ( ";
    //    for(std::size_t i = 0; i < mp.size(); ++i) {
    //      std::cout << mp[i].first << ":" << mp[i].second << " ";
    //    } std::cout << ") ";
    //  } std::cout << "\n";
    //}

    static inline void merge_property_maps(property_map& mp, const property_map& mp2) {
      property_map newmp;
      newmp.reserve(mp.size() + mp2.size());
      unsigned int i = 0;
      unsigned int j = 0;
      while(i != mp.size() && j != mp2.size()) {
        if(mp[i].first < mp2[j].first) {
          newmp.push_back(mp[i]);
          ++i;
        } else if(mp[i].first > mp2[j].first) {
          newmp.push_back(mp2[j]);
          ++j;
        } else {
          int count = mp[i].second;
          count += mp2[j].second;
          if(count) {
            newmp.push_back(mp[i]);
            newmp.back().second = count;
          }
          ++i;
          ++j;
        }
      }
      while(i != mp.size()) {
        newmp.push_back(mp[i]);
        ++i;
      }
      while(j != mp2.size()) {
        newmp.push_back(mp2[j]);
        ++j;
      }
      mp.swap(newmp);
    }

    static inline void update_property_map(property_map& mp, const std::pair<property_type, int>& prop_data) {
      property_map newmp;
      newmp.reserve(mp.size() +1);
      bool consumed = false;
      for(std::size_t i = 0; i < mp.size(); ++i) {
        if(!consumed && prop_data.first == mp[i].first) {
          consumed = true;
          int count = prop_data.second + mp[i].second;
          if(count)
            newmp.push_back(std::make_pair(prop_data.first, count));
        } else if(!consumed && prop_data.first < mp[i].first) {
          consumed = true;
          newmp.push_back(prop_data);
          newmp.push_back(mp[i]);
        } else {
          newmp.push_back(mp[i]);
        }
      }
      if(!consumed) newmp.push_back(prop_data);
      mp.swap(newmp);
    }

    static inline void set_unique_property(property_set& unqiue_property, const property_map& property) {
      unqiue_property.clear();
      for(typename property_map::const_iterator itr = property.begin(); itr != property.end(); ++itr) {
        if((*itr).second > 0)
          unqiue_property.insert(unqiue_property.end(), (*itr).first);
      }
    }

    static inline bool common_vertex(const half_edge& he1, const half_edge& he2) {
      return he1.first == he2.first ||
        he1.first == he2.second ||
        he1.second == he2.first ||
        he1.second == he2.second;
    }

    typedef typename scanline_base<Unit>::vertex_half_edge vertex_half_edge;
    template <typename iT>
    static inline void convert_segments_to_vertex_half_edges(std::vector<vertex_half_edge>& output, iT begin, iT end) {
      for( ; begin != end; ++begin) {
        const half_edge& he = (*begin).first;
        int count = (*begin).second;
        output.push_back(vertex_half_edge(he.first, he.second, count));
        output.push_back(vertex_half_edge(he.second, he.first, -count));
      }
      polygon_sort(output.begin(), output.end());
    }

    class test_functor {
    public:
      inline test_functor() {}
      inline void operator()(std::vector<std::pair<half_edge, std::pair<property_set, property_set> > >& result,
                             const half_edge& he, const property_set& ps_left, const property_set& ps_right) {
        result.push_back(std::make_pair(he, std::make_pair(ps_left, ps_right)));
      }
    };
    template <typename stream_type>
    static inline bool test_scanline(stream_type& stdcout) {
      std::vector<std::pair<half_edge, std::pair<property_set, property_set> > > result;
      std::vector<std::pair<half_edge, std::pair<property_type, int> > > input;
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(0, 10)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(10, 0)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 10), Point(10, 10)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(10, 10)), std::make_pair(0, -1)));
      scanline sl;
      test_functor tf;
      sl.scan(result, tf, input.begin(), input.end());
      stdcout << "scanned\n";
      for(std::size_t i = 0; i < result.size(); ++i) {
        stdcout << result[i].first.first << ", " << result[i].first.second << "; ";
      } stdcout << "\n";
      input.clear();
      result.clear();
      input.push_back(std::make_pair(half_edge(Point(-1, -1), Point(10, 0)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(-1, -1), Point(0, 10)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(0, 10), Point(11, 11)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(11, 11)), std::make_pair(0, 1)));
      scanline sl2;
      sl2.scan(result, tf, input.begin(), input.end());
      stdcout << "scanned\n";
      for(std::size_t i = 0; i < result.size(); ++i) {
        stdcout << result[i].first.first << ", " << result[i].first.second << "; ";
      } stdcout << "\n";
      input.clear();
      result.clear();
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(0, 10)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(10, 0)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 10), Point(10, 10)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(1, 1), Point(8, 2)), std::make_pair(1, 1)));
      input.push_back(std::make_pair(half_edge(Point(1, 1), Point(2, 8)), std::make_pair(1, -1)));
      input.push_back(std::make_pair(half_edge(Point(2, 8), Point(9, 9)), std::make_pair(1, -1)));
      input.push_back(std::make_pair(half_edge(Point(8, 2), Point(9, 9)), std::make_pair(1, 1)));
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(10, 10)), std::make_pair(0, -1)));
      scanline sl3;
      sl3.scan(result, tf, input.begin(), input.end());
      stdcout << "scanned\n";
      for(std::size_t i = 0; i < result.size(); ++i) {
        stdcout << result[i].first.first << ", " << result[i].first.second << "; ";
      } stdcout << "\n";
      input.clear();
      result.clear();
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(0, 10)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(10, 0)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 10), Point(10, 10)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(1, 1), Point(8, 2)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(1, 1), Point(2, 8)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(2, 8), Point(9, 9)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(8, 2), Point(9, 9)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(10, 10)), std::make_pair(0, -1)));
      scanline sl4;
      sl4.scan(result, tf, input.begin(), input.end());
      stdcout << "scanned\n";
      for(std::size_t i = 0; i < result.size(); ++i) {
        stdcout << result[i].first.first << ", " << result[i].first.second << "; ";
      } stdcout << "\n";
      input.clear();
      result.clear();
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(10, 0)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(9, 1)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(1, 9)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(0, 10)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 10), Point(10, 10)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(1, 9), Point(10, 10)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(9, 1), Point(10, 10)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(10, 10)), std::make_pair(0, -1)));
      scanline sl5;
      sl5.scan(result, tf, input.begin(), input.end());
      stdcout << "scanned\n";
      for(std::size_t i = 0; i < result.size(); ++i) {
        stdcout << result[i].first.first << ", " << result[i].first.second << "; ";
      } stdcout << "\n";
      input.clear();
      result.clear();
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(10, 0)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(9, 1)), std::make_pair(1, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(1, 9)), std::make_pair(1, -1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(0, 10)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 10), Point(10, 10)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(1, 9), Point(10, 10)), std::make_pair(1, -1)));
      input.push_back(std::make_pair(half_edge(Point(9, 1), Point(10, 10)), std::make_pair(1, 1)));
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(10, 10)), std::make_pair(0, -1)));
      scanline sl6;
      sl6.scan(result, tf, input.begin(), input.end());
      stdcout << "scanned\n";
      for(std::size_t i = 0; i < result.size(); ++i) {
        stdcout << result[i].first.first << ", " << result[i].first.second << "; ";
      } stdcout << "\n";
      input.clear();
      result.clear();
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(10, 0)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(9, 1)), std::make_pair(1, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(1, 9)), std::make_pair(1, -1)));
      input.push_back(std::make_pair(half_edge(Point(0, 0), Point(0, 10)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 10), Point(10, 10)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(0, 20), Point(10, 20)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 20), Point(9, 21)), std::make_pair(1, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 20), Point(1, 29)), std::make_pair(1, -1)));
      input.push_back(std::make_pair(half_edge(Point(0, 20), Point(0, 30)), std::make_pair(0, 1)));
      input.push_back(std::make_pair(half_edge(Point(0, 30), Point(10, 30)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(1, 9), Point(10, 10)), std::make_pair(1, -1)));
      input.push_back(std::make_pair(half_edge(Point(1, 29), Point(10, 30)), std::make_pair(1, -1)));
      input.push_back(std::make_pair(half_edge(Point(9, 1), Point(10, 10)), std::make_pair(1, 1)));
      input.push_back(std::make_pair(half_edge(Point(9, 21), Point(10, 30)), std::make_pair(1, 1)));
      input.push_back(std::make_pair(half_edge(Point(10, 20), Point(10, 30)), std::make_pair(0, -1)));
      input.push_back(std::make_pair(half_edge(Point(10, 20), Point(10, 30)), std::make_pair(0, -1)));
      scanline sl7;
      sl7.scan(result, tf, input.begin(), input.end());
      stdcout << "scanned\n";
      for(std::size_t i = 0; i < result.size(); ++i) {
        stdcout << result[i].first.first << ", " << result[i].first.second << "; ";
      } stdcout << "\n";
      input.clear();
      result.clear();
      input.push_back(std::make_pair(half_edge(Point(-1, -1), Point(10, 0)), std::make_pair(0, 1))); //a
      input.push_back(std::make_pair(half_edge(Point(-1, -1), Point(0, 10)), std::make_pair(0, -1))); //a
      input.push_back(std::make_pair(half_edge(Point(0, 10), Point(11, 11)), std::make_pair(0, -1))); //a
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(20, 0)), std::make_pair(0, 1))); //b
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(11, 11)), std::make_pair(0, -1))); //b
      input.push_back(std::make_pair(half_edge(Point(10, 0), Point(11, 11)), std::make_pair(0, 1))); //a
      input.push_back(std::make_pair(half_edge(Point(11, 11), Point(20, 10)), std::make_pair(0, -1))); //b
      input.push_back(std::make_pair(half_edge(Point(20, 0), Point(30, 0)), std::make_pair(0, 1))); //c
      input.push_back(std::make_pair(half_edge(Point(20, 0), Point(20, 10)), std::make_pair(0, -1))); //b
      input.push_back(std::make_pair(half_edge(Point(20, 0), Point(20, 10)), std::make_pair(0, 1))); //c
      input.push_back(std::make_pair(half_edge(Point(20, 10), Point(30, 10)), std::make_pair(0, -1))); //c
      input.push_back(std::make_pair(half_edge(Point(30, 0), Point(30, 10)), std::make_pair(0, -1))); //c
      scanline sl8;
      sl8.scan(result, tf, input.begin(), input.end());
      stdcout << "scanned\n";
      for(std::size_t i = 0; i < result.size(); ++i) {
        stdcout << result[i].first.first << ", " << result[i].first.second << "; ";
      } stdcout << "\n";
      return true;
    }

  };

  template <typename Unit>
  class merge_output_functor {
  public:
    typedef typename scanline_base<Unit>::half_edge half_edge;
    merge_output_functor() {}
    template <typename result_type, typename key_type>
    void operator()(result_type& result, const half_edge& edge, const key_type& left, const key_type& right) {
      typename std::pair<half_edge, int> elem;
      elem.first = edge;
      elem.second = 1;
      if(edge.second < edge.first) elem.second *= -1;
      if(scanline_base<Unit>::is_vertical(edge)) elem.second *= -1;
      if(!left.empty())
        result[left].insert_clean(elem);
      elem.second *= -1;
      if(!right.empty())
        result[right].insert_clean(elem);
    }
  };

  template <typename Unit, typename property_type, typename key_type = std::set<property_type>,
            typename output_functor_type = merge_output_functor<Unit> >
  class property_merge : public scanline_base<Unit> {
  protected:
    typedef typename scanline_base<Unit>::Point Point;

    //the first point is the vertex and and second point establishes the slope of an edge eminating from the vertex
    //typedef std::pair<Point, Point> half_edge;
    typedef typename scanline_base<Unit>::half_edge half_edge;

    //scanline comparator functor
    typedef typename scanline_base<Unit>::less_half_edge less_half_edge;
    typedef typename scanline_base<Unit>::less_point less_point;

    //this data structure assocates a property and count to a half edge
    typedef std::pair<half_edge, std::pair<property_type, int> > vertex_property;
    //this data type stores the combination of many half edges
    typedef std::vector<vertex_property> property_merge_data;

    //this is the data type used internally to store the combination of property counts at a given location
    typedef std::vector<std::pair<property_type, int> > property_map;
    //this data type is used internally to store the combined property data for a given half edge
    typedef std::pair<half_edge, property_map> vertex_data;

    property_merge_data pmd;
    typename scanline_base<Unit>::evalAtXforYPack evalAtXforYPack_;

    template<typename vertex_data_type>
    class less_vertex_data {
      typename scanline_base<Unit>::evalAtXforYPack* pack_;
    public:
      less_vertex_data() : pack_() {}
      less_vertex_data(typename scanline_base<Unit>::evalAtXforYPack* pack) : pack_(pack) {}
      bool operator() (const vertex_data_type& lvalue, const vertex_data_type& rvalue) const {
        less_point lp;
        if(lp(lvalue.first.first, rvalue.first.first)) return true;
        if(lp(rvalue.first.first, lvalue.first.first)) return false;
        Unit x = lvalue.first.first.get(HORIZONTAL);
        int just_before_ = 0;
        less_half_edge lhe(&x, &just_before_, pack_);
        return lhe(lvalue.first, rvalue.first);
      }
    };


    inline void sort_property_merge_data() {
      less_vertex_data<vertex_property> lvd(&evalAtXforYPack_);
      polygon_sort(pmd.begin(), pmd.end(), lvd);
    }
  public:
    inline property_merge_data& get_property_merge_data() { return pmd; }
    inline property_merge() : pmd(), evalAtXforYPack_() {}
    inline property_merge(const property_merge& pm) : pmd(pm.pmd), evalAtXforYPack_(pm.evalAtXforYPack_) {}
    inline property_merge& operator=(const property_merge& pm) { pmd = pm.pmd; return *this; }

    template <typename polygon_type>
    void insert(const polygon_type& polygon_object, const property_type& property_value, bool is_hole = false) {
      insert(polygon_object, property_value, is_hole, typename geometry_concept<polygon_type>::type());
    }

    //result type should be std::map<std::set<property_type>, polygon_set_type>
    //or std::map<std::vector<property_type>, polygon_set_type>
    template <typename result_type>
    void merge(result_type& result) {
      if(pmd.empty()) return;
      //intersect data
      property_merge_data tmp_pmd;
      line_intersection<Unit>::validate_scan(tmp_pmd, pmd.begin(), pmd.end());
      pmd.swap(tmp_pmd);
      sort_property_merge_data();
      scanline<Unit, property_type, key_type> sl;
      output_functor_type mof;
      sl.scan(result, mof, pmd.begin(), pmd.end());
    }

    inline bool verify1() {
      std::pair<int, int> offenders;
      std::vector<std::pair<half_edge, int> > lines;
      int count = 0;
      for(std::size_t i = 0; i < pmd.size(); ++i) {
        lines.push_back(std::make_pair(pmd[i].first, count++));
      }
      if(!line_intersection<Unit>::verify_scan(offenders, lines.begin(), lines.end())) {
        //stdcout << "Intersection failed!\n";
        //stdcout << offenders.first << " " << offenders.second << "\n";
        return false;
      }
      std::vector<Point> pts;
      for(std::size_t i = 0; i < lines.size(); ++i) {
        pts.push_back(lines[i].first.first);
        pts.push_back(lines[i].first.second);
      }
      polygon_sort(pts.begin(), pts.end());
      for(std::size_t i = 0; i < pts.size(); i+=2) {
        if(pts[i] != pts[i+1]) {
          //stdcout << "Non-closed figures after line intersection!\n";
          return false;
        }
      }
      return true;
    }

    void clear() {*this = property_merge();}

  protected:
    template <typename polygon_type>
    void insert(const polygon_type& polygon_object, const property_type& property_value, bool is_hole,
                polygon_concept ) {
      bool first_iteration = true;
      bool second_iteration = true;
      Point first_point;
      Point second_point;
      Point previous_previous_point;
      Point previous_point;
      Point current_point;
      direction_1d winding_dir = winding(polygon_object);
      for(typename polygon_traits<polygon_type>::iterator_type itr = begin_points(polygon_object);
          itr != end_points(polygon_object); ++itr) {
        assign(current_point, *itr);
        if(first_iteration) {
          first_iteration = false;
          first_point = previous_point = current_point;
        } else if(second_iteration) {
          if(previous_point != current_point) {
            second_iteration = false;
            previous_previous_point = previous_point;
            second_point = previous_point = current_point;
          }
        } else {
          if(previous_point != current_point) {
            create_vertex(pmd, previous_point, current_point, winding_dir,
                          is_hole, property_value);
            previous_previous_point = previous_point;
            previous_point = current_point;
          }
        }
      }
      current_point = first_point;
      if(!first_iteration && !second_iteration) {
        if(previous_point != current_point) {
          create_vertex(pmd, previous_point, current_point, winding_dir,
                        is_hole, property_value);
          previous_previous_point = previous_point;
          previous_point = current_point;
        }
        current_point = second_point;
        create_vertex(pmd, previous_point, current_point, winding_dir,
                      is_hole, property_value);
        previous_previous_point = previous_point;
        previous_point = current_point;
      }
    }

    template <typename polygon_with_holes_type>
    void insert(const polygon_with_holes_type& polygon_with_holes_object, const property_type& property_value, bool is_hole,
                polygon_with_holes_concept) {
      insert(polygon_with_holes_object, property_value, is_hole, polygon_concept());
      for(typename polygon_with_holes_traits<polygon_with_holes_type>::iterator_holes_type itr =
            begin_holes(polygon_with_holes_object);
          itr != end_holes(polygon_with_holes_object); ++itr) {
        insert(*itr, property_value, !is_hole, polygon_concept());
      }
    }

    template <typename rectangle_type>
    void insert(const rectangle_type& rectangle_object, const property_type& property_value, bool is_hole,
                rectangle_concept ) {
      polygon_90_data<Unit> poly;
      assign(poly, rectangle_object);
      insert(poly, property_value, is_hole, polygon_concept());
    }

  public: //change to private when done testing

    static inline void create_vertex(property_merge_data& pmd,
                                     const Point& current_point,
                                     const Point& next_point,
                                     direction_1d winding,
                                     bool is_hole, const property_type& property) {
      if(current_point == next_point) return;
      vertex_property current_vertex;
      current_vertex.first.first = current_point;
      current_vertex.first.second = next_point;
      current_vertex.second.first = property;
      int multiplier = 1;
      if(winding == CLOCKWISE)
        multiplier = -1;
      if(is_hole)
        multiplier *= -1;
      if(current_point < next_point) {
        multiplier *= -1;
        std::swap(current_vertex.first.first, current_vertex.first.second);
      }
      current_vertex.second.second = multiplier * (euclidean_distance(next_point, current_point, HORIZONTAL) == 0 ? -1: 1);
      pmd.push_back(current_vertex);
      //current_vertex.first.second = previous_point;
      //current_vertex.second.second *= -1;
      //pmd.push_back(current_vertex);
    }

    static inline void sort_vertex_half_edges(vertex_data& vertex) {
      less_half_edge_pair lessF(vertex.first);
      polygon_sort(vertex.second.begin(), vertex.second.end(), lessF);
    }

    class less_half_edge_pair {
    private:
      Point pt_;
    public:
      less_half_edge_pair(const Point& pt) : pt_(pt) {}
      bool operator()(const half_edge& e1, const half_edge& e2) {
        const Point& pt1 = e1.first;
        const Point& pt2 = e2.first;
        if(get(pt1, HORIZONTAL) ==
           get(pt_, HORIZONTAL)) {
          //vertical edge is always largest
          return false;
        }
        if(get(pt2, HORIZONTAL) ==
           get(pt_, HORIZONTAL)) {
          //if half edge 1 is not vertical its slope is less than that of half edge 2
          return get(pt1, HORIZONTAL) != get(pt2, HORIZONTAL);
        }
        return scanline_base<Unit>::less_slope(get(pt_, HORIZONTAL),
                                               get(pt_, VERTICAL), pt1, pt2);
      }
    };

  public:
    //test functions
    template <typename stream_type>
    static stream_type& print (stream_type& o, const property_map& c)
    {
      o << "count: {";
      for(typename property_map::const_iterator itr = c.begin(); itr != c.end(); ++itr) {
        o << ((*itr).first) << ":" << ((*itr).second) << " ";
      }
      return o << "} ";
    }


    template <typename stream_type>
    static stream_type& print (stream_type& o, const half_edge& he)
    {
      o << "half edge: (";
      o << (he.first);
      return o << ", " << (he.second) << ") ";
    }

    template <typename stream_type>
    static stream_type& print (stream_type& o, const vertex_property& c)
    {
      o << "vertex property: {";
      print(o, c.first);
      o << ", " << c.second.first << ":" << c.second.second << " ";
      return o;
    }

    template <typename stream_type>
    static stream_type& print (stream_type& o, const std::vector<vertex_property>& hev)
    {
      o << "vertex properties: {";
      for(std::size_t i = 0; i < hev.size(); ++i) {
        print(o, (hev[i])) << " ";
      }
      return o << "} ";
    }

    template <typename stream_type>
    static stream_type& print (stream_type& o, const std::vector<half_edge>& hev)
    {
      o << "half edges: {";
      for(std::size_t i = 0; i < hev.size(); ++i) {
        print(o, (hev[i])) << " ";
      }
      return o << "} ";
    }

    template <typename stream_type>
    static stream_type& print (stream_type& o, const vertex_data& v)
    {
      return print(o << "vertex: <" << (v.first) << ", ", (v.second)) << "> ";
    }

    template <typename stream_type>
    static stream_type& print (stream_type& o, const std::vector<vertex_data>& vv)
    {
      o << "vertices: {";
      for(std::size_t i = 0; i < vv.size(); ++i) {
        print(o, (vv[i])) << " ";
      }
      return o << "} ";
    }



    template <typename stream_type>
    static inline bool test_insertion(stream_type& stdcout) {
      property_merge si;
      rectangle_data<Unit> rect;
      xl(rect, 0);
      yl(rect, 1);
      xh(rect, 10);
      yh(rect, 11);

      si.insert(rect, 333);
      print(stdcout, si.pmd) << "\n";

      Point pts[4] = {Point(0, 0), Point(10,-3), Point(13, 8), Point(0, 0) };
      polygon_data<Unit> poly;
      property_merge si2;
      poly.set(pts, pts+3);
      si2.insert(poly, 444);
      si2.sort_property_merge_data();
      print(stdcout, si2.pmd) << "\n";
      property_merge si3;
      poly.set(pts, pts+4);
      si3.insert(poly, 444);
      si3.sort_property_merge_data();
      stdcout << (si2.pmd == si3.pmd) << "\n";
      std::reverse(pts, pts+4);
      property_merge si4;
      poly.set(pts, pts+4);
      si4.insert(poly, 444);
      si4.sort_property_merge_data();
      print(stdcout, si4.pmd) << "\n";
      stdcout << (si2.pmd == si4.pmd) << "\n";
      std::reverse(pts, pts+3);
      property_merge si5;
      poly.set(pts, pts+4);
      si5.insert(poly, 444);
      si5.sort_property_merge_data();
      stdcout << (si2.pmd == si5.pmd) << "\n";

      return true;
    }

    template <typename stream_type>
    static inline bool test_merge(stream_type& stdcout) {
      property_merge si;
      rectangle_data<Unit> rect;
      xl(rect, 0);
      yl(rect, 1);
      xh(rect, 10);
      yh(rect, 11);

      si.insert(rect, 333);
      std::map<std::set<property_type>, polygon_set_data<Unit> > result;
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      polygon_set_data<Unit> psd = (*(result.begin())).second;
      std::vector<polygon_data<Unit> > polys;
      psd.get(polys);
      if(polys.size() != 1) {
        stdcout << "fail merge 1\n";
        return false;
      }
      stdcout << (polys[0]) << "\n";
      si.clear();
      std::vector<Point> pts;
      pts.push_back(Point(0, 0));
      pts.push_back(Point(10, -10));
      pts.push_back(Point(10, 10));
      polygon_data<Unit> poly;
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      pts.clear();
      pts.push_back(Point(5, 0));
      pts.push_back(Point(-5, -10));
      pts.push_back(Point(-5, 10));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      psd = (*(result.begin())).second;
      stdcout << psd << "\n";
      polys.clear();
      psd.get(polys);
      if(polys.size() != 1) {
        stdcout << "fail merge 2\n";
        return false;
      }
      //Polygon { -4 -1, 3 3, -2 3 }
      //Polygon { 0 -4, -4 -2, -2 1 }
      si.clear();
      pts.clear();
      pts.push_back(Point(-4, -1));
      pts.push_back(Point(3, 3));
      pts.push_back(Point(-2, 3));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      pts.clear();
      pts.push_back(Point(0, -4));
      pts.push_back(Point(-4, -2));
      pts.push_back(Point(-2, 1));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      psd = (*(result.begin())).second;
      stdcout << psd << "\n";
      polys.clear();
      psd.get(polys);
      if(polys.size() != 1) {
        stdcout << "fail merge 3\n";
        return false;
      }
      stdcout << "Polygon { -2 2, -2 2, 1 4 } \n";
      stdcout << "Polygon { 2 4, 2 -4, -3 1 } \n";
      si.clear();
      pts.clear();
      pts.push_back(Point(-2, 2));
      pts.push_back(Point(-2, 2));
      pts.push_back(Point(1, 4));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      pts.clear();
      pts.push_back(Point(2, 4));
      pts.push_back(Point(2, -4));
      pts.push_back(Point(-3, 1));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      psd = (*(result.begin())).second;
      stdcout << psd << "\n";
      polys.clear();
      psd.get(polys);
      if(polys.size() != 1) {
        stdcout << "fail merge 4\n";
        return false;
      }
      stdcout << (polys[0]) << "\n";
      stdcout << "Polygon { -4 0, -2 -3, 3 -4 } \n";
      stdcout << "Polygon { -1 1, 1 -2, -4 -3 } \n";
      si.clear();
      pts.clear();
      pts.push_back(Point(-4, 0));
      pts.push_back(Point(-2, -3));
      pts.push_back(Point(3, -4));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      pts.clear();
      pts.push_back(Point(-1, 1));
      pts.push_back(Point(1, -2));
      pts.push_back(Point(-4, -3));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      psd = (*(result.begin())).second;
      stdcout << psd << "\n";
      polys.clear();
      psd.get(polys);
      if(polys.size() != 1) {
        stdcout << "fail merge 5\n";
        return false;
      }
      stdcout << "Polygon { 2 2, -2 0, 0 1 }  \n";
      stdcout << "Polygon { 4 -2, 3 -1, 2 3 }  \n";
      si.clear();
      pts.clear();
      pts.push_back(Point(2, 2));
      pts.push_back(Point(-2, 0));
      pts.push_back(Point(0, 1));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      pts.clear();
      pts.push_back(Point(4, -2));
      pts.push_back(Point(3, -1));
      pts.push_back(Point(2, 3));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      if(!result.empty()) {
        psd = (*(result.begin())).second;
        stdcout << psd << "\n";
        polys.clear();
        psd.get(polys);
        if(polys.size() != 1) {
          stdcout << "fail merge 6\n";
          return false;
        }
        stdcout << (polys[0]) << "\n";
      }
      stdcout << "Polygon { 0 2, 3 -1, 4 1 }  \n";
      stdcout << "Polygon { -4 3, 3 3, 4 2 }  \n";
      si.clear();
      pts.clear();
      pts.push_back(Point(0, 2));
      pts.push_back(Point(3, -1));
      pts.push_back(Point(4, 1));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      pts.clear();
      pts.push_back(Point(-4, 3));
      pts.push_back(Point(3, 3));
      pts.push_back(Point(4, 2));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      if(!result.empty()) {
        psd = (*(result.begin())).second;
        stdcout << psd << "\n";
        polys.clear();
        psd.get(polys);
        if(polys.size() == 0) {
          stdcout << "fail merge 7\n";
          return false;
        }
        stdcout << (polys[0]) << "\n";
      }
stdcout << "Polygon { 1 -2, -1 4, 3 -2 }   \n";
stdcout << "Polygon { 0 -3, 3 1, -3 -4 }   \n";
      si.clear();
      pts.clear();
      pts.push_back(Point(1, -2));
      pts.push_back(Point(-1, 4));
      pts.push_back(Point(3, -2));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      pts.clear();
      pts.push_back(Point(0, -3));
      pts.push_back(Point(3, 1));
      pts.push_back(Point(-3, -4));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      if(!result.empty()) {
        psd = (*(result.begin())).second;
        stdcout << psd << "\n";
        polys.clear();
        psd.get(polys);
        if(polys.size() == 0) {
          stdcout << "fail merge 8\n";
          return false;
        }
        stdcout << (polys[0]) << "\n";
      }
stdcout << "Polygon { 2 2, 3 0, -3 4 }  \n";
stdcout << "Polygon { -2 -2, 0 0, -1 -1 }  \n";
      si.clear();
      pts.clear();
      pts.push_back(Point(2, 2));
      pts.push_back(Point(3, 0));
      pts.push_back(Point(-3, 4));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      pts.clear();
      pts.push_back(Point(-2, -2));
      pts.push_back(Point(0, 0));
      pts.push_back(Point(-1, -1));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      if(!result.empty()) {
        psd = (*(result.begin())).second;
        stdcout << psd << "\n";
        polys.clear();
        psd.get(polys);
        if(polys.size() == 0) {
          stdcout << "fail merge 9\n";
          return false;
        }
        stdcout << (polys[0]) << "\n";
      }
      si.clear();
      pts.clear();
      //5624841,17616200,75000,9125000
      //pts.push_back(Point(5624841,75000));
      //pts.push_back(Point(5624841,9125000));
      //pts.push_back(Point(17616200,9125000));
      //pts.push_back(Point(17616200,75000));
pts.push_back(Point(12262940, 6652520 )); pts.push_back(Point(12125750, 6652520 )); pts.push_back(Point(12121272, 6652961 )); pts.push_back(Point(12112981, 6656396 )); pts.push_back(Point(12106636, 6662741 )); pts.push_back(Point(12103201, 6671032 )); pts.push_back(Point(12103201, 6680007 )); pts.push_back(Point(12106636, 6688298 ));
pts.push_back(Point(12109500, 6691780 )); pts.push_back(Point(12748600, 7330890 )); pts.push_back(Point(15762600, 7330890 )); pts.push_back(Point(15904620, 7472900 )); pts.push_back(Point(15909200, 7473030 )); pts.push_back(Point(15935830, 7476006 )); pts.push_back(Point(15992796, 7499602 )); pts.push_back(Point(16036397, 7543203 ));
pts.push_back(Point(16059993, 7600169 )); pts.push_back(Point(16059993, 7661830 )); pts.push_back(Point(16036397, 7718796 )); pts.push_back(Point(15992796, 7762397 )); pts.push_back(Point(15935830, 7785993 )); pts.push_back(Point(15874169, 7785993 )); pts.push_back(Point(15817203, 7762397 )); pts.push_back(Point(15773602, 7718796 ));
pts.push_back(Point(15750006, 7661830 )); pts.push_back(Point(15747030, 7635200 )); pts.push_back(Point(15746900, 7630620 )); pts.push_back(Point(15670220, 7553930 )); pts.push_back(Point(14872950, 7553930 )); pts.push_back(Point(14872950, 7626170 ));
pts.push_back(Point(14869973, 7661280 )); pts.push_back(Point(14846377, 7718246 )); pts.push_back(Point(14802776, 7761847 )); pts.push_back(Point(14745810, 7785443 )); pts.push_back(Point(14684149, 7785443 )); pts.push_back(Point(14627183, 7761847 )); pts.push_back(Point(14583582, 7718246 ));
pts.push_back(Point(14559986, 7661280 )); pts.push_back(Point(14557070, 7636660 )); pts.push_back(Point(14556670, 7625570 )); pts.push_back(Point(13703330, 7625570 )); pts.push_back(Point(13702930, 7636660 )); pts.push_back(Point(13699993, 7661830 )); pts.push_back(Point(13676397, 7718796 ));
pts.push_back(Point(13632796, 7762397 )); pts.push_back(Point(13575830, 7785993 )); pts.push_back(Point(13514169, 7785993 )); pts.push_back(Point(13457203, 7762397 )); pts.push_back(Point(13436270, 7745670 )); pts.push_back(Point(13432940, 7742520 )); pts.push_back(Point(12963760, 7742520 ));
pts.push_back(Point(12959272, 7742961 )); pts.push_back(Point(12950981, 7746396 )); pts.push_back(Point(12944636, 7752741 )); pts.push_back(Point(12941201, 7761032 )); pts.push_back(Point(12941201, 7770007 )); pts.push_back(Point(12944636, 7778298 )); pts.push_back(Point(12947490, 7781780 ));
pts.push_back(Point(13425330, 8259620 )); pts.push_back(Point(15601330, 8259620 )); pts.push_back(Point(15904620, 8562900 )); pts.push_back(Point(15909200, 8563030 )); pts.push_back(Point(15935830, 8566006 )); pts.push_back(Point(15992796, 8589602 )); pts.push_back(Point(16036397, 8633203 ));
pts.push_back(Point(16059993, 8690169 )); pts.push_back(Point(16059993, 8751830 )); pts.push_back(Point(16036397, 8808796 )); pts.push_back(Point(15992796, 8852397 )); pts.push_back(Point(15935830, 8875993 )); pts.push_back(Point(15874169, 8875993 )); pts.push_back(Point(15817203, 8852397 )); pts.push_back(Point(15773602, 8808796 ));
pts.push_back(Point(15750006, 8751830 )); pts.push_back(Point(15747030, 8725200 )); pts.push_back(Point(15746900, 8720620 )); pts.push_back(Point(15508950, 8482660 )); pts.push_back(Point(14689890, 8482660 )); pts.push_back(Point(14685412, 8483101 )); pts.push_back(Point(14677121, 8486536 ));
pts.push_back(Point(14670776, 8492881 )); pts.push_back(Point(14667341, 8501172 )); pts.push_back(Point(14667341, 8510147 )); pts.push_back(Point(14670776, 8518438 )); pts.push_back(Point(14673630, 8521920 )); pts.push_back(Point(14714620, 8562900 )); pts.push_back(Point(14719200, 8563030 )); pts.push_back(Point(14745830, 8566006 ));
pts.push_back(Point(14802796, 8589602 )); pts.push_back(Point(14846397, 8633203 )); pts.push_back(Point(14869993, 8690169 )); pts.push_back(Point(14869993, 8751830 )); pts.push_back(Point(14846397, 8808796 )); pts.push_back(Point(14802796, 8852397 )); pts.push_back(Point(14745830, 8875993 )); pts.push_back(Point(14684169, 8875993 ));
pts.push_back(Point(14627203, 8852397 )); pts.push_back(Point(14583602, 8808796 )); pts.push_back(Point(14560006, 8751830 )); pts.push_back(Point(14557030, 8725200 )); pts.push_back(Point(14556900, 8720620 )); pts.push_back(Point(14408270, 8571980 )); pts.push_back(Point(13696320, 8571980 )); pts.push_back(Point(13696320, 8675520 ));
pts.push_back(Point(13699963, 8690161 )); pts.push_back(Point(13699963, 8751818 )); pts.push_back(Point(13676368, 8808781 )); pts.push_back(Point(13632771, 8852378 )); pts.push_back(Point(13575808, 8875973 )); pts.push_back(Point(13514151, 8875973 )); pts.push_back(Point(13457188, 8852378 )); pts.push_back(Point(13436270, 8835670 )); pts.push_back(Point(13432940, 8832520 ));
pts.push_back(Point(13281760, 8832520 )); pts.push_back(Point(13277272, 8832961 )); pts.push_back(Point(13268981, 8836396 )); pts.push_back(Point(13262636, 8842741 )); pts.push_back(Point(13259201, 8851032 )); pts.push_back(Point(13259201, 8860007 )); pts.push_back(Point(13262636, 8868298 )); pts.push_back(Point(13265500, 8871780 ));
pts.push_back(Point(13518710, 9125000 )); pts.push_back(Point(16270720, 9125000 )); pts.push_back(Point(16270720, 8939590 )); pts.push_back(Point(17120780, 8939590 )); pts.push_back(Point(17120780, 9125000 )); pts.push_back(Point(17616200, 9125000 )); pts.push_back(Point(17616200,   75000 )); pts.push_back(Point(16024790,   75000 ));
pts.push_back(Point(16021460,   80700 )); pts.push_back(Point(16016397,   88796 )); pts.push_back(Point(15972796,  132397 )); pts.push_back(Point(15915830,  155993 )); pts.push_back(Point(15908730,  157240 )); pts.push_back(Point(15905000,  157800 )); pts.push_back(Point(15516800,  546000 )); pts.push_back(Point(15905000,  934200 ));
pts.push_back(Point(15908730,  934760 )); pts.push_back(Point(15915830,  936006 )); pts.push_back(Point(15972796,  959602 )); pts.push_back(Point(16016397, 1003203 )); pts.push_back(Point(16039993, 1060169 )); pts.push_back(Point(16039993, 1121830 )); pts.push_back(Point(16016397, 1178796 )); pts.push_back(Point(15972796, 1222397 ));
pts.push_back(Point(15915830, 1245993 )); pts.push_back(Point(15854169, 1245993 )); pts.push_back(Point(15797203, 1222397 )); pts.push_back(Point(15753602, 1178796 )); pts.push_back(Point(15730006, 1121830 )); pts.push_back(Point(15728760, 1114730 )); pts.push_back(Point(15728200, 1111000 )); pts.push_back(Point(15363500,  746300 ));
pts.push_back(Point(14602620,  746300 )); pts.push_back(Point(14598142,  746741 )); pts.push_back(Point(14589851,  750176 )); pts.push_back(Point(14583506,  756521 )); pts.push_back(Point(14580071,  764812 )); pts.push_back(Point(14580071,  773787 )); pts.push_back(Point(14583506,  782078 )); pts.push_back(Point(14586360,  785560 ));
pts.push_back(Point(14586370,  785560 )); pts.push_back(Point(14735000,  934200 )); pts.push_back(Point(14738730,  934760 )); pts.push_back(Point(14745830,  936006 )); pts.push_back(Point(14802796,  959602 )); pts.push_back(Point(14846397, 1003203 )); pts.push_back(Point(14869993, 1060169 ));
pts.push_back(Point(14870450, 1062550 )); pts.push_back(Point(14872170, 1071980 )); pts.push_back(Point(14972780, 1071980 )); pts.push_back(Point(15925000, 2024200 )); pts.push_back(Point(15928730, 2024760 )); pts.push_back(Point(15935830, 2026006 )); pts.push_back(Point(15992796, 2049602 ));
pts.push_back(Point(16036397, 2093203 )); pts.push_back(Point(16059993, 2150169 )); pts.push_back(Point(16059993, 2211830 )); pts.push_back(Point(16036397, 2268796 )); pts.push_back(Point(15992796, 2312397 )); pts.push_back(Point(15935830, 2335993 )); pts.push_back(Point(15874169, 2335993 ));
pts.push_back(Point(15817203, 2312397 )); pts.push_back(Point(15773602, 2268796 )); pts.push_back(Point(15750006, 2211830 )); pts.push_back(Point(15748760, 2204730 )); pts.push_back(Point(15748200, 2201000 )); pts.push_back(Point(14869220, 1322020 )); pts.push_back(Point(14088350, 1322020 ));
pts.push_back(Point(14083862, 1322461 )); pts.push_back(Point(14075571, 1325896 )); pts.push_back(Point(14069226, 1332241 )); pts.push_back(Point(14065791, 1340532 )); pts.push_back(Point(14065791, 1349507 )); pts.push_back(Point(14069226, 1357798 )); pts.push_back(Point(14072080, 1361280 ));
pts.push_back(Point(14072090, 1361280 )); pts.push_back(Point(14735000, 2024200 )); pts.push_back(Point(14738730, 2024760 )); pts.push_back(Point(14745830, 2026006 )); pts.push_back(Point(14802796, 2049602 )); pts.push_back(Point(14846397, 2093203 )); pts.push_back(Point(14869993, 2150169 ));
pts.push_back(Point(14869993, 2211830 )); pts.push_back(Point(14846397, 2268796 )); pts.push_back(Point(14802796, 2312397 )); pts.push_back(Point(14745830, 2335993 )); pts.push_back(Point(14684169, 2335993 )); pts.push_back(Point(14627203, 2312397 )); pts.push_back(Point(14583602, 2268796 )); pts.push_back(Point(14560006, 2211830 ));
pts.push_back(Point(14558760, 2204730 )); pts.push_back(Point(14558200, 2201000 )); pts.push_back(Point(13752220, 1395020 )); pts.push_back(Point(12991340, 1395020 )); pts.push_back(Point(12986862, 1395461 )); pts.push_back(Point(12978571, 1398896 )); pts.push_back(Point(12972226, 1405241 ));
pts.push_back(Point(12968791, 1413532 )); pts.push_back(Point(12968791, 1422507 )); pts.push_back(Point(12972226, 1430798 )); pts.push_back(Point(12975080, 1434280 )); pts.push_back(Point(12975090, 1434280 )); pts.push_back(Point(13565000, 2024200 )); pts.push_back(Point(13568730, 2024760 )); pts.push_back(Point(13575830, 2026006 ));
pts.push_back(Point(13632796, 2049602 )); pts.push_back(Point(13676397, 2093203 )); pts.push_back(Point(13699993, 2150169 )); pts.push_back(Point(13699993, 2211830 )); pts.push_back(Point(13676397, 2268796 )); pts.push_back(Point(13632796, 2312397 )); pts.push_back(Point(13575830, 2335993 ));
pts.push_back(Point(13514169, 2335993 )); pts.push_back(Point(13457203, 2312397 )); pts.push_back(Point(13413602, 2268796 )); pts.push_back(Point(13390006, 2211830 )); pts.push_back(Point(13388760, 2204730 )); pts.push_back(Point(13388200, 2201000 )); pts.push_back(Point(12655220, 1468020 ));
pts.push_back(Point(11894340, 1468020 )); pts.push_back(Point(11889862, 1468461 )); pts.push_back(Point(11881571, 1471896 )); pts.push_back(Point(11875226, 1478241 )); pts.push_back(Point(11871791, 1486532 )); pts.push_back(Point(11871791, 1495507 ));
pts.push_back(Point(11875226, 1503798 )); pts.push_back(Point(11878090, 1507280 )); pts.push_back(Point(12395000, 2024200 )); pts.push_back(Point(12398730, 2024760 )); pts.push_back(Point(12405830, 2026006 )); pts.push_back(Point(12462796, 2049602 )); pts.push_back(Point(12506397, 2093203 ));
pts.push_back(Point(12529993, 2150169 )); pts.push_back(Point(12529993, 2211830 )); pts.push_back(Point(12506397, 2268796 )); pts.push_back(Point(12462796, 2312397 )); pts.push_back(Point(12405830, 2335993 )); pts.push_back(Point(12344169, 2335993 ));
pts.push_back(Point(12287203, 2312397 )); pts.push_back(Point(12243602, 2268796 )); pts.push_back(Point(12220006, 2211830 )); pts.push_back(Point(12218760, 2204730 )); pts.push_back(Point(12218200, 2201000 )); pts.push_back(Point(11558220, 1541020 ));
pts.push_back(Point(10797340, 1541020 )); pts.push_back(Point(10792862, 1541461 )); pts.push_back(Point(10784571, 1544896 )); pts.push_back(Point(10778226, 1551241 )); pts.push_back(Point(10774791, 1559532 )); pts.push_back(Point(10774791, 1568507 )); pts.push_back(Point(10778226, 1576798 )); pts.push_back(Point(10781080, 1580280 ));
pts.push_back(Point(10781090, 1580280 )); pts.push_back(Point(11225000, 2024200 )); pts.push_back(Point(11228730, 2024760 )); pts.push_back(Point(11235830, 2026006 )); pts.push_back(Point(11292796, 2049602 )); pts.push_back(Point(11336397, 2093203 )); pts.push_back(Point(11359993, 2150169 ));
pts.push_back(Point(11359993, 2211830 )); pts.push_back(Point(11336397, 2268796 )); pts.push_back(Point(11292796, 2312397 )); pts.push_back(Point(11235830, 2335993 )); pts.push_back(Point(11174169, 2335993 )); pts.push_back(Point(11117203, 2312397 )); pts.push_back(Point(11073602, 2268796 )); pts.push_back(Point(11050006, 2211830 ));
pts.push_back(Point(11048760, 2204730 )); pts.push_back(Point(11048200, 2201000 )); pts.push_back(Point(10461220, 1614020 )); pts.push_back(Point( 5647400, 1614020 )); pts.push_back(Point( 5642912, 1614461 ));
pts.push_back(Point( 5634621, 1617896 )); pts.push_back(Point( 5628276, 1624241 )); pts.push_back(Point( 5624841, 1632532 )); pts.push_back(Point( 5624841, 1641507 )); pts.push_back(Point( 5628276, 1649798 )); pts.push_back(Point( 5631130, 1653280 ));
pts.push_back(Point( 5688490, 1710640 )); pts.push_back(Point( 9722350, 1710640 )); pts.push_back(Point(10034620, 2022900 )); pts.push_back(Point(10039200, 2023030 )); pts.push_back(Point(10065830, 2026006 )); pts.push_back(Point(10122796, 2049602 ));
pts.push_back(Point(10166397, 2093203 )); pts.push_back(Point(10189993, 2150169 )); pts.push_back(Point(10189993, 2211830 )); pts.push_back(Point(10166397, 2268796 )); pts.push_back(Point(10158620, 2279450 )); pts.push_back(Point(10158620, 2404900 )); pts.push_back(Point(10548950, 2795240 ));
pts.push_back(Point(15586950, 2795240 )); pts.push_back(Point(15904620, 3112900 )); pts.push_back(Point(15909200, 3113030 )); pts.push_back(Point(15935830, 3116006 )); pts.push_back(Point(15992796, 3139602 )); pts.push_back(Point(16036397, 3183203 )); pts.push_back(Point(16059993, 3240169 )); pts.push_back(Point(16059993, 3301830 ));
pts.push_back(Point(16036397, 3358796 )); pts.push_back(Point(15992796, 3402397 )); pts.push_back(Point(15935830, 3425993 )); pts.push_back(Point(15874169, 3425993 )); pts.push_back(Point(15817203, 3402397 )); pts.push_back(Point(15773602, 3358796 )); pts.push_back(Point(15750006, 3301830 )); pts.push_back(Point(15747030, 3275200 ));
pts.push_back(Point(15746900, 3270620 )); pts.push_back(Point(15494570, 3018280 )); pts.push_back(Point(14675510, 3018280 )); pts.push_back(Point(14671032, 3018721 )); pts.push_back(Point(14662741, 3022156 )); pts.push_back(Point(14656396, 3028501 )); pts.push_back(Point(14652961, 3036792 ));
pts.push_back(Point(14652961, 3045767 )); pts.push_back(Point(14656396, 3054058 )); pts.push_back(Point(14659260, 3057540 )); pts.push_back(Point(14714620, 3112900 )); pts.push_back(Point(14719200, 3113030 )); pts.push_back(Point(14745830, 3116006 )); pts.push_back(Point(14802796, 3139602 ));
pts.push_back(Point(14846397, 3183203 )); pts.push_back(Point(14869993, 3240169 )); pts.push_back(Point(14869993, 3301830 )); pts.push_back(Point(14846397, 3358796 )); pts.push_back(Point(14802796, 3402397 )); pts.push_back(Point(14745830, 3425993 )); pts.push_back(Point(14684169, 3425993 )); pts.push_back(Point(14627203, 3402397 ));
pts.push_back(Point(14583602, 3358796 )); pts.push_back(Point(14560006, 3301830 )); pts.push_back(Point(14557030, 3275200 )); pts.push_back(Point(14556900, 3270620 )); pts.push_back(Point(14370700, 3084410 )); pts.push_back(Point(13702830, 3084410 )); pts.push_back(Point(13702830, 3263160 ));
pts.push_back(Point(13700003, 3302210 )); pts.push_back(Point(13676407, 3359176 )); pts.push_back(Point(13632806, 3402777 )); pts.push_back(Point(13575840, 3426373 )); pts.push_back(Point(13514179, 3426373 )); pts.push_back(Point(13457213, 3402777 )); pts.push_back(Point(13413612, 3359176 ));
pts.push_back(Point(13390016, 3302210 )); pts.push_back(Point(13387030, 3275200 )); pts.push_back(Point(13386900, 3270620 )); pts.push_back(Point(13266840, 3150550 )); pts.push_back(Point(12532920, 3150550 )); pts.push_back(Point(12532920, 3264990 )); pts.push_back(Point(12529993, 3301820 ));
pts.push_back(Point(12506397, 3358786 )); pts.push_back(Point(12462796, 3402387 )); pts.push_back(Point(12405830, 3425983 )); pts.push_back(Point(12344169, 3425983 )); pts.push_back(Point(12287203, 3402387 )); pts.push_back(Point(12243602, 3358786 )); pts.push_back(Point(12220006, 3301820 )); pts.push_back(Point(12217030, 3275200 ));
pts.push_back(Point(12216900, 3270620 )); pts.push_back(Point(12157460, 3211170 )); pts.push_back(Point(11362030, 3211170 )); pts.push_back(Point(11360250, 3220520 )); pts.push_back(Point(11359993, 3221830 )); pts.push_back(Point(11336397, 3278796 ));
pts.push_back(Point(11292796, 3322397 )); pts.push_back(Point(11235830, 3345993 )); pts.push_back(Point(11174169, 3345993 )); pts.push_back(Point(11117203, 3322397 )); pts.push_back(Point(11096270, 3305670 )); pts.push_back(Point(11092940, 3302520 )); pts.push_back(Point(10680760, 3302520 ));
pts.push_back(Point(10676272, 3302961 )); pts.push_back(Point(10667981, 3306396 )); pts.push_back(Point(10661636, 3312741 )); pts.push_back(Point(10658201, 3321032 )); pts.push_back(Point(10658201, 3330007 )); pts.push_back(Point(10661636, 3338298 )); pts.push_back(Point(10664500, 3341780 ));
pts.push_back(Point(11264260, 3941550 )); pts.push_back(Point(15643260, 3941550 )); pts.push_back(Point(15904620, 4202900 )); pts.push_back(Point(15909200, 4203030 )); pts.push_back(Point(15935830, 4206006 )); pts.push_back(Point(15992796, 4229602 ));
pts.push_back(Point(16036397, 4273203 )); pts.push_back(Point(16059993, 4330169 )); pts.push_back(Point(16059993, 4391830 )); pts.push_back(Point(16036397, 4448796 )); pts.push_back(Point(15992796, 4492397 ));
pts.push_back(Point(15935830, 4515993 )); pts.push_back(Point(15874169, 4515993 )); pts.push_back(Point(15817203, 4492397 )); pts.push_back(Point(15773602, 4448796 )); pts.push_back(Point(15750006, 4391830 )); pts.push_back(Point(15747030, 4365200 )); pts.push_back(Point(15746900, 4360620 ));
pts.push_back(Point(15550880, 4164590 )); pts.push_back(Point(14825070, 4164590 )); pts.push_back(Point(14825070, 4247610 )); pts.push_back(Point(14846397, 4273213 )); pts.push_back(Point(14869993, 4330179 )); pts.push_back(Point(14869993, 4391840 )); pts.push_back(Point(14846397, 4448806 ));
pts.push_back(Point(14802796, 4492407 )); pts.push_back(Point(14745830, 4516003 )); pts.push_back(Point(14684169, 4516003 )); pts.push_back(Point(14627203, 4492407 )); pts.push_back(Point(14583602, 4448806 )); pts.push_back(Point(14560006, 4391840 )); pts.push_back(Point(14557030, 4365200 ));
pts.push_back(Point(14556900, 4360620 )); pts.push_back(Point(14432520, 4236230 )); pts.push_back(Point(13702830, 4236230 )); pts.push_back(Point(13702830, 4352930 )); pts.push_back(Point(13699993, 4391750 )); pts.push_back(Point(13676397, 4448716 ));
pts.push_back(Point(13632796, 4492317 )); pts.push_back(Point(13575830, 4515913 )); pts.push_back(Point(13514169, 4515913 )); pts.push_back(Point(13457203, 4492317 )); pts.push_back(Point(13413602, 4448716 )); pts.push_back(Point(13390006, 4391750 )); pts.push_back(Point(13387030, 4365200 ));
pts.push_back(Point(13386900, 4360620 )); pts.push_back(Point(13334170, 4307880 )); pts.push_back(Point(12532990, 4307880 )); pts.push_back(Point(12532990, 4357550 )); pts.push_back(Point(12529993, 4391760 )); pts.push_back(Point(12506397, 4448726 )); pts.push_back(Point(12462796, 4492327 ));
pts.push_back(Point(12405830, 4515923 )); pts.push_back(Point(12344169, 4515923 )); pts.push_back(Point(12287203, 4492327 )); pts.push_back(Point(12243602, 4448726 )); pts.push_back(Point(12220006, 4391760 )); pts.push_back(Point(12217970, 4378710 )); pts.push_back(Point(12216810, 4368500 ));
pts.push_back(Point(11363190, 4368500 )); pts.push_back(Point(11362030, 4378710 )); pts.push_back(Point(11359983, 4391828 )); pts.push_back(Point(11336388, 4448791 )); pts.push_back(Point(11292791, 4492388 )); pts.push_back(Point(11235828, 4515983 )); pts.push_back(Point(11174171, 4515983 )); pts.push_back(Point(11117208, 4492388 ));
pts.push_back(Point(11096270, 4475670 )); pts.push_back(Point(11092940, 4472520 )); pts.push_back(Point(11057750, 4472520 )); pts.push_back(Point(11053272, 4472961 )); pts.push_back(Point(11044981, 4476396 )); pts.push_back(Point(11038636, 4482741 )); pts.push_back(Point(11035201, 4491032 ));
pts.push_back(Point(11035201, 4500007 )); pts.push_back(Point(11038636, 4508298 )); pts.push_back(Point(11041490, 4511780 )); pts.push_back(Point(11573490, 5043780 )); pts.push_back(Point(15655490, 5043780 )); pts.push_back(Point(15904620, 5292900 ));
pts.push_back(Point(15909200, 5293030 )); pts.push_back(Point(15935830, 5296006 )); pts.push_back(Point(15992796, 5319602 )); pts.push_back(Point(16036397, 5363203 )); pts.push_back(Point(16059993, 5420169 )); pts.push_back(Point(16059993, 5481830 )); pts.push_back(Point(16036397, 5538796 ));
pts.push_back(Point(15992796, 5582397 )); pts.push_back(Point(15935830, 5605993 )); pts.push_back(Point(15874169, 5605993 )); pts.push_back(Point(15817203, 5582397 )); pts.push_back(Point(15773602, 5538796 )); pts.push_back(Point(15750006, 5481830 )); pts.push_back(Point(15747030, 5455200 ));
pts.push_back(Point(15746900, 5450620 )); pts.push_back(Point(15563110, 5266820 )); pts.push_back(Point(14857380, 5266820 )); pts.push_back(Point(14857380, 5382430 )); pts.push_back(Point(14869993, 5420179 )); pts.push_back(Point(14869993, 5481840 )); pts.push_back(Point(14846397, 5538806 )); pts.push_back(Point(14802796, 5582407 ));
pts.push_back(Point(14745830, 5606003 )); pts.push_back(Point(14684169, 5606003 )); pts.push_back(Point(14627203, 5582407 )); pts.push_back(Point(14583602, 5538806 )); pts.push_back(Point(14560006, 5481840 )); pts.push_back(Point(14557030, 5455200 )); pts.push_back(Point(14556900, 5450620 )); pts.push_back(Point(14444750, 5338460 ));
pts.push_back(Point(13702890, 5338460 )); pts.push_back(Point(13702890, 5364400 )); pts.push_back(Point(13699993, 5401800 )); pts.push_back(Point(13676397, 5458766 )); pts.push_back(Point(13632796, 5502367 )); pts.push_back(Point(13575830, 5525963 )); pts.push_back(Point(13514169, 5525963 )); pts.push_back(Point(13457203, 5502367 ));
pts.push_back(Point(13413602, 5458766 )); pts.push_back(Point(13390006, 5401800 )); pts.push_back(Point(13389230, 5397620 )); pts.push_back(Point(13387590, 5388060 )); pts.push_back(Point(12532960, 5388060 )); pts.push_back(Point(12532960, 5446220 )); pts.push_back(Point(12529993, 5481820 ));
pts.push_back(Point(12506397, 5538786 )); pts.push_back(Point(12462796, 5582387 )); pts.push_back(Point(12405830, 5605983 )); pts.push_back(Point(12344169, 5605983 )); pts.push_back(Point(12287203, 5582387 )); pts.push_back(Point(12266270, 5565670 )); pts.push_back(Point(12262940, 5562520 )); pts.push_back(Point(11737750, 5562520 ));
pts.push_back(Point(11733272, 5562961 )); pts.push_back(Point(11724981, 5566396 )); pts.push_back(Point(11718636, 5572741 )); pts.push_back(Point(11715201, 5581032 )); pts.push_back(Point(11715201, 5590007 )); pts.push_back(Point(11718636, 5598298 )); pts.push_back(Point(11721500, 5601780 ));
pts.push_back(Point(12287760, 6168050 )); pts.push_back(Point(15689760, 6168050 )); pts.push_back(Point(15904620, 6382900 )); pts.push_back(Point(15909200, 6383030 )); pts.push_back(Point(15935830, 6386006 )); pts.push_back(Point(15992796, 6409602 ));
pts.push_back(Point(16036397, 6453203 )); pts.push_back(Point(16059993, 6510169 )); pts.push_back(Point(16059993, 6571830 )); pts.push_back(Point(16036397, 6628796 )); pts.push_back(Point(15992796, 6672397 )); pts.push_back(Point(15935830, 6695993 )); pts.push_back(Point(15874169, 6695993 ));
pts.push_back(Point(15817203, 6672397 )); pts.push_back(Point(15773602, 6628796 )); pts.push_back(Point(15750006, 6571830 )); pts.push_back(Point(15747030, 6545200 )); pts.push_back(Point(15746900, 6540620 )); pts.push_back(Point(15597380, 6391090 )); pts.push_back(Point(14858060, 6391090 ));
pts.push_back(Point(14858060, 6473860 )); pts.push_back(Point(14869993, 6510179 )); pts.push_back(Point(14869993, 6571840 )); pts.push_back(Point(14846397, 6628806 )); pts.push_back(Point(14802796, 6672407 )); pts.push_back(Point(14745830, 6696003 )); pts.push_back(Point(14684169, 6696003 ));
pts.push_back(Point(14627203, 6672407 )); pts.push_back(Point(14583602, 6628806 )); pts.push_back(Point(14560006, 6571840 )); pts.push_back(Point(14557030, 6545200 )); pts.push_back(Point(14556900, 6540620 )); pts.push_back(Point(14479020, 6462730 ));
pts.push_back(Point(13702990, 6462730 )); pts.push_back(Point(13702990, 6537170 )); pts.push_back(Point(13700003, 6571840 )); pts.push_back(Point(13676407, 6628806 )); pts.push_back(Point(13632806, 6672407 )); pts.push_back(Point(13575840, 6696003 ));
pts.push_back(Point(13514179, 6696003 )); pts.push_back(Point(13457213, 6672407 )); pts.push_back(Point(13413612, 6628806 )); pts.push_back(Point(13390016, 6571840 )); pts.push_back(Point(13387040, 6545550 )); pts.push_back(Point(13386710, 6534380 ));
pts.push_back(Point(12533290, 6534380 )); pts.push_back(Point(12532960, 6545550 )); pts.push_back(Point(12529983, 6571828 )); pts.push_back(Point(12506388, 6628791 )); pts.push_back(Point(12462791, 6672388 )); pts.push_back(Point(12405828, 6695983 ));
pts.push_back(Point(12344171, 6695983 )); pts.push_back(Point(12287208, 6672388 )); pts.push_back(Point(12266270, 6655670 ));
      poly.set(pts.begin(), pts.end());
      si.insert(poly, 444);
      result.clear();
      si.merge(result);
      si.verify1();
      print(stdcout, si.pmd) << "\n";
      if(!result.empty()) {
        psd = (*(result.begin())).second;
        stdcout << psd << "\n";
        std::vector<Point> outpts;
        for(typename polygon_set_data<Unit>::iterator_type itr = psd.begin();
            itr != psd.end(); ++itr) {
          outpts.push_back((*itr).first.first);
          outpts.push_back((*itr).first.second);
        }
        polygon_sort(outpts.begin(), outpts.end());
        for(std::size_t i = 0; i < outpts.size(); i+=2) {
          if(outpts[i] != outpts[i+1]) {
            stdcout << "Polygon set not a closed figure\n";
            stdcout << i << "\n";
            stdcout << outpts[i] << " " << outpts[i+1] << "\n";
            return 0;
          }
        }
        polys.clear();
        psd.get(polys);
        if(polys.size() == 0) {
          stdcout << "fail merge 10\n";
          return false;
        }
        stdcout << (polys[0]) << "\n";
      }
      for(unsigned int i = 0; i < 10; ++i) {
        stdcout << "random case # " << i << "\n";
        si.clear();
        pts.clear();
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        polygon_data<Unit> poly1;
        poly1.set(pts.begin(), pts.end());
        stdcout << poly1 << "\n";
        si.insert(poly1, 444);
        pts.clear();
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        polygon_data<Unit> poly2;
        poly2.set(pts.begin(), pts.end());
        stdcout << poly2 << "\n";
        si.insert(poly2, 444);
        result.clear();
        si.merge(result);
        print(stdcout, si.pmd) << "\n";
        if(!result.empty()) {
          psd = (*(result.begin())).second;
          stdcout << psd << "\n";
          polys.clear();
          psd.get(polys);
          if(polys.size() == 0) {
            si.clear();
            si.insert(poly1, 333);
            result.clear();
            si.merge(result);
            psd = (*(result.begin())).second;
            std::vector<polygon_data<Unit> > polys1;
            psd.get(polys1);
            si.clear();
            si.insert(poly2, 333);
            result.clear();
            si.merge(result);
            psd = (*(result.begin())).second;
            std::vector<polygon_data<Unit> > polys2;
            psd.get(polys2);
            if(!polys1.empty() || !polys2.empty()) {
              stdcout << "fail random merge " << i << "\n";
              return false;
            }
          }
        }
        if(!polys.empty())
          stdcout << polys.size() << ": " << (polys[0]) << "\n";
      }
      return true;
    }

    template <typename stream_type>
    static inline bool check_rectangle_trio(rectangle_data<Unit> rect1, rectangle_data<Unit> rect2, rectangle_data<Unit> rect3, stream_type& stdcout) {
        property_merge si;
        std::map<std::set<property_type>, polygon_set_data<Unit> > result;
        std::vector<polygon_data<Unit> > polys;
        property_merge_90<property_type, Unit> si90;
        std::map<std::set<property_type>, polygon_90_set_data<Unit> > result90;
        std::vector<polygon_data<Unit> > polys90;
        si.insert(rect1, 111);
        si90.insert(rect1, 111);
        stdcout << rect1 << "\n";
        si.insert(rect2, 222);
        si90.insert(rect2, 222);
        stdcout << rect2 << "\n";
        si.insert(rect3, 333);
        si90.insert(rect3, 333);
        stdcout << rect3 << "\n";
        si.merge(result);
        si90.merge(result90);
        if(result.size() != result90.size()) {
          stdcout << "merge failed with size mismatch\n";
          return 0;
        }
        typename std::map<std::set<property_type>, polygon_90_set_data<Unit> >::iterator itr90 = result90.begin();
        for(typename std::map<std::set<property_type>, polygon_set_data<Unit> >::iterator itr = result.begin();
            itr != result.end(); ++itr) {
          for(typename std::set<property_type>::const_iterator set_itr = (*itr).first.begin();
              set_itr != (*itr).first.end(); ++set_itr) {
            stdcout << (*set_itr) << " ";
          } stdcout << ") \n";
          polygon_set_data<Unit> psd = (*itr).second;
          polygon_90_set_data<Unit> psd90 = (*itr90).second;
          polys.clear();
          polys90.clear();
          psd.get(polys);
          psd90.get(polys90);
          if(polys.size() != polys90.size()) {
            stdcout << "merge failed with polygon count mismatch\n";
            stdcout << psd << "\n";
            for(std::size_t j = 0; j < polys.size(); ++j) {
              stdcout << polys[j] << "\n";
            }
            stdcout << "reference\n";
            for(std::size_t j = 0; j < polys90.size(); ++j) {
              stdcout << polys90[j] << "\n";
            }
            return 0;
          }
          bool failed = false;
          for(std::size_t j = 0; j < polys.size(); ++j) {
            stdcout << polys[j] << "\n";
            stdcout << polys90[j] << "\n";
#ifdef BOOST_POLYGON_ICC
#pragma warning (push)
#pragma warning (disable:1572)
#endif
            if(area(polys[j]) != area(polys90[j])) {
#ifdef BOOST_POLYGON_ICC
#pragma warning (pop)
#endif
              stdcout << "merge failed with area mismatch\n";
              failed = true;
            }
          }
          if(failed) return 0;
          ++itr90;
        }
        return true;
    }

    template <typename stream_type>
    static inline bool test_manhattan_intersection(stream_type& stdcout) {
      rectangle_data<Unit> rect1, rect2, rect3;
      set_points(rect1, (Point(-1, 2)), (Point(1, 4)));
      set_points(rect2, (Point(-1, 2)), (Point(2, 3)));
      set_points(rect3, (Point(-3, 0)), (Point(4, 2)));
      if(!check_rectangle_trio(rect1, rect2, rect3, stdcout)) {
        return false;
      }
      for(unsigned int i = 0; i < 100; ++i) {
        property_merge si;
        std::map<std::set<property_type>, polygon_set_data<Unit> > result;
        std::vector<polygon_data<Unit> > polys;
        property_merge_90<property_type, Unit> si90;
        std::map<std::set<property_type>, polygon_90_set_data<Unit> > result90;
        std::vector<polygon_data<Unit> > polys90;
        stdcout << "random case # " << i << "\n";
        set_points(rect1, (Point(rand()%9-4, rand()%9-4)), (Point(rand()%9-4, rand()%9-4)));
        set_points(rect2, (Point(rand()%9-4, rand()%9-4)), (Point(rand()%9-4, rand()%9-4)));
        set_points(rect3, (Point(rand()%9-4, rand()%9-4)), (Point(rand()%9-4, rand()%9-4)));
        if(!check_rectangle_trio(rect1, rect2, rect3, stdcout)) {
          return false;
        }
      }
      return true;
    }

    template <typename stream_type>
    static inline bool test_intersection(stream_type& stdcout) {
      property_merge si;
      rectangle_data<Unit> rect;
      xl(rect, 0);
      yl(rect, 10);
      xh(rect, 30);
      yh(rect, 20);
      si.insert(rect, 333);
      xl(rect, 10);
      yl(rect, 0);
      xh(rect, 20);
      yh(rect, 30);
      si.insert(rect, 444);
      xl(rect, 15);
      yl(rect, 0);
      xh(rect, 25);
      yh(rect, 30);
      si.insert(rect, 555);
      std::map<std::set<property_type>, polygon_set_data<Unit> > result;
      si.merge(result);
      print(stdcout, si.pmd) << "\n";
      for(typename std::map<std::set<property_type>, polygon_set_data<Unit> >::iterator itr = result.begin();
          itr != result.end(); ++itr) {
        stdcout << "( ";
        for(typename std::set<property_type>::const_iterator set_itr = (*itr).first.begin();
            set_itr != (*itr).first.end(); ++set_itr) {
          stdcout << (*set_itr) << " ";
        } stdcout << ") \n";
        polygon_set_data<Unit> psd = (*itr).second;
        stdcout << psd << "\n";
        std::vector<polygon_data<Unit> > polys;
        psd.get(polys);
        for(std::size_t i = 0; i < polys.size(); ++i) {
          stdcout << polys[i] << "\n";
        }
      }
      std::vector<Point> pts;
      std::vector<polygon_data<Unit> > polys;
      for(unsigned int i = 0; i < 10; ++i) {
        property_merge si2;
        stdcout << "random case # " << i << "\n";
        si.clear();
        pts.clear();
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        polygon_data<Unit> poly1;
        poly1.set(pts.begin(), pts.end());
        stdcout << poly1 << "\n";
        si.insert(poly1, 444);
        si2.insert(poly1, 333);
        pts.clear();
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        polygon_data<Unit> poly2;
        poly2.set(pts.begin(), pts.end());
        stdcout << poly2 << "\n";
        si.insert(poly2, 444);
        si2.insert(poly2, 444);
        pts.clear();
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        pts.push_back(Point(rand()%9-4, rand()%9-4));
        polygon_data<Unit> poly3;
        poly3.set(pts.begin(), pts.end());
        stdcout << poly3 << "\n";
        si.insert(poly3, 444);
        si2.insert(poly3, 555);
        result.clear();
        std::map<std::set<property_type>, polygon_set_data<Unit> > result2;
        si.merge(result);
        si2.merge(result2);
        stdcout << "merged result\n";
      for(typename std::map<std::set<property_type>, polygon_set_data<Unit> >::iterator itr = result.begin();
          itr != result.end(); ++itr) {
        stdcout << "( ";
        for(typename std::set<property_type>::const_iterator set_itr = (*itr).first.begin();
            set_itr != (*itr).first.end(); ++set_itr) {
          stdcout << (*set_itr) << " ";
        } stdcout << ") \n";
        polygon_set_data<Unit> psd = (*itr).second;
        stdcout << psd << "\n";
        std::vector<polygon_data<Unit> > polys2;
        psd.get(polys2);
        for(std::size_t ii = 0; ii < polys2.size(); ++ii) {
          stdcout << polys2[ii] << "\n";
        }
      }
      stdcout << "intersected pmd\n";
      print(stdcout, si2.pmd) << "\n";
      stdcout << "intersected result\n";
      for(typename std::map<std::set<property_type>, polygon_set_data<Unit> >::iterator itr = result2.begin();
          itr != result2.end(); ++itr) {
        stdcout << "( ";
        for(typename std::set<property_type>::const_iterator set_itr = (*itr).first.begin();
            set_itr != (*itr).first.end(); ++set_itr) {
          stdcout << (*set_itr) << " ";
        } stdcout << ") \n";
        polygon_set_data<Unit> psd = (*itr).second;
        stdcout << psd << "\n";
        std::vector<polygon_data<Unit> > polys2;
        psd.get(polys2);
        for(std::size_t ii = 0; ii < polys2.size(); ++ii) {
          stdcout << polys2[ii] << "\n";
        }
      }
        si.clear();
        for(typename std::map<std::set<property_type>, polygon_set_data<Unit> >::iterator itr = result2.begin();
            itr != result2.end(); ++itr) {
          polys.clear();
          (*itr).second.get(polys);
          for(std::size_t j = 0; j < polys.size(); ++j) {
            si.insert(polys[j], 444);
          }
        }
        result2.clear();
        si.merge(result2);
      stdcout << "remerged result\n";
      for(typename std::map<std::set<property_type>, polygon_set_data<Unit> >::iterator itr = result2.begin();
          itr != result2.end(); ++itr) {
        stdcout << "( ";
        for(typename std::set<property_type>::const_iterator set_itr = (*itr).first.begin();
            set_itr != (*itr).first.end(); ++set_itr) {
          stdcout << (*set_itr) << " ";
        } stdcout << ") \n";
        polygon_set_data<Unit> psd = (*itr).second;
        stdcout << psd << "\n";
        std::vector<polygon_data<Unit> > polys2;
        psd.get(polys2);
        for(std::size_t ii = 0; ii < polys2.size(); ++ii) {
          stdcout << polys2[ii] << "\n";
        }
      }
      std::vector<polygon_data<Unit> > polys2;
      polys.clear();
      (*(result.begin())).second.get(polys);
      (*(result2.begin())).second.get(polys2);
      if(!(polys == polys2)) {
          stdcout << "failed intersection check # " << i << "\n";
          return false;
        }
      }
      return true;
    }
  };

  template <typename Unit>
  class arbitrary_boolean_op : public scanline_base<Unit> {
  private:

    typedef int property_type;
    typedef typename scanline_base<Unit>::Point Point;

    //the first point is the vertex and and second point establishes the slope of an edge eminating from the vertex
    //typedef std::pair<Point, Point> half_edge;
    typedef typename scanline_base<Unit>::half_edge half_edge;

    //scanline comparator functor
    typedef typename scanline_base<Unit>::less_half_edge less_half_edge;
    typedef typename scanline_base<Unit>::less_point less_point;

    //this data structure assocates a property and count to a half edge
    typedef std::pair<half_edge, std::pair<property_type, int> > vertex_property;
    //this data type stores the combination of many half edges
    typedef std::vector<vertex_property> property_merge_data;

    //this is the data type used internally to store the combination of property counts at a given location
    typedef std::vector<std::pair<property_type, int> > property_map;
    //this data type is used internally to store the combined property data for a given half edge
    typedef std::pair<half_edge, property_map> vertex_data;

    property_merge_data pmd;
    typename scanline_base<Unit>::evalAtXforYPack evalAtXforYPack_;

    template<typename vertex_data_type>
    class less_vertex_data {
      typename scanline_base<Unit>::evalAtXforYPack* pack_;
    public:
      less_vertex_data() : pack_() {}
      less_vertex_data(typename scanline_base<Unit>::evalAtXforYPack* pack) : pack_(pack) {}
      bool operator()(const vertex_data_type& lvalue, const vertex_data_type& rvalue) const {
        less_point lp;
        if(lp(lvalue.first.first, rvalue.first.first)) return true;
        if(lp(rvalue.first.first, lvalue.first.first)) return false;
        Unit x = lvalue.first.first.get(HORIZONTAL);
        int just_before_ = 0;
        less_half_edge lhe(&x, &just_before_, pack_);
        return lhe(lvalue.first, rvalue.first);
      }
    };

    template <typename result_type, typename key_type, int op_type>
    class boolean_output_functor {
    public:
      boolean_output_functor() {}
      void operator()(result_type& result, const half_edge& edge, const key_type& left, const key_type& right) {
        typename std::pair<half_edge, int> elem;
        elem.first = edge;
        elem.second = 1;
        if(edge.second < edge.first) elem.second *= -1;
        if(scanline_base<Unit>::is_vertical(edge)) elem.second *= -1;
#ifdef BOOST_POLYGON_MSVC
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
        if(op_type == 0) { //OR
          if(!left.empty() && right.empty()) {
            result.insert_clean(elem);
          } else if(!right.empty() && left.empty()) {
            elem.second *= -1;
            result.insert_clean(elem);
          }
        } else if(op_type == 1) { //AND
          if(left.size() == 2 && right.size() != 2) {
            result.insert_clean(elem);
          } else if(right.size() == 2 && left.size() != 2) {
            elem.second *= -1;
            result.insert_clean(elem);
          }
        } else if(op_type == 2) { //XOR
          if(left.size() == 1 && right.size() != 1) {
            result.insert_clean(elem);
          } else if(right.size() == 1 && left.size() != 1) {
            elem.second *= -1;
            result.insert_clean(elem);
          }
        } else { //SUBTRACT
          if(left.size() == 1) {
            if((*(left.begin())) == 0) {
              result.insert_clean(elem);
            }
          }
#ifdef BOOST_POLYGON_MSVC
#pragma warning (pop)
#endif
          if(right.size() == 1) {
            if((*(right.begin())) == 0) {
              elem.second *= -1;
              result.insert_clean(elem);
            }
          }
        }
      }
    };

    inline void sort_property_merge_data() {
      less_vertex_data<vertex_property> lvd(&evalAtXforYPack_);
      polygon_sort(pmd.begin(), pmd.end(), lvd);
    }
  public:
    inline arbitrary_boolean_op() : pmd(), evalAtXforYPack_() {}
    inline arbitrary_boolean_op(const arbitrary_boolean_op& pm) : pmd(pm.pmd), evalAtXforYPack_(pm.evalAtXforYPack_) {}
    inline arbitrary_boolean_op& operator=(const arbitrary_boolean_op& pm) { pmd = pm.pmd; return *this; }

    enum BOOLEAN_OP_TYPE {
      BOOLEAN_OR = 0,
      BOOLEAN_AND = 1,
      BOOLEAN_XOR = 2,
      BOOLEAN_NOT = 3
    };
    template <typename result_type, typename iT1, typename iT2>
    inline void execute(result_type& result, iT1 b1, iT1 e1, iT2 b2, iT2 e2, int op) {
      //intersect data
      insert(b1, e1, 0);
      insert(b2, e2, 1);
      property_merge_data tmp_pmd;
      //#define BOOST_POLYGON_DEBUG_FILE
#ifdef BOOST_POLYGON_DEBUG_FILE
      std::fstream debug_file;
      debug_file.open("gtl_debug.txt", std::ios::out);
      property_merge<Unit, property_type, std::vector<property_type> >::print(debug_file, pmd);
      debug_file.close();
#endif
      if(pmd.empty())
        return;
      line_intersection<Unit>::validate_scan(tmp_pmd, pmd.begin(), pmd.end());
      pmd.swap(tmp_pmd);
      sort_property_merge_data();
      scanline<Unit, property_type, std::vector<property_type> > sl;
      if(op == BOOLEAN_OR) {
        boolean_output_functor<result_type, std::vector<property_type>, 0> bof;
        sl.scan(result, bof, pmd.begin(), pmd.end());
      } else if(op == BOOLEAN_AND) {
        boolean_output_functor<result_type, std::vector<property_type>, 1> bof;
        sl.scan(result, bof, pmd.begin(), pmd.end());
      } else if(op == BOOLEAN_XOR) {
        boolean_output_functor<result_type, std::vector<property_type>, 2> bof;
        sl.scan(result, bof, pmd.begin(), pmd.end());
      } else if(op == BOOLEAN_NOT) {
        boolean_output_functor<result_type, std::vector<property_type>, 3> bof;
        sl.scan(result, bof, pmd.begin(), pmd.end());
      }
    }

    inline void clear() {*this = arbitrary_boolean_op();}

  private:
    template <typename iT>
    void insert(iT b, iT e, int id) {
      for(;
          b != e; ++b) {
        pmd.push_back(vertex_property(half_edge((*b).first.first, (*b).first.second),
                                      std::pair<property_type, int>(id, (*b).second)));
      }
    }

  };

  template <typename Unit, typename stream_type>
  bool test_arbitrary_boolean_op(stream_type& stdcout) {
    polygon_set_data<Unit> psd;
    rectangle_data<Unit> rect;
    set_points(rect, point_data<Unit>(0, 0), point_data<Unit>(10, 10));
    psd.insert(rect);
    polygon_set_data<Unit> psd2;
    set_points(rect, point_data<Unit>(5, 5), point_data<Unit>(15, 15));
    psd2.insert(rect);
    std::vector<polygon_data<Unit> > pv;
    pv.clear();
    arbitrary_boolean_op<Unit> abo;
    polygon_set_data<Unit> psd3;
    abo.execute(psd3, psd.begin(), psd.end(), psd2.begin(), psd2.end(), arbitrary_boolean_op<Unit>::BOOLEAN_OR);
    psd3.get(pv);
    for(std::size_t i = 0; i < pv.size(); ++i) {
      stdcout << pv[i] << "\n";
    }
    pv.clear();
    abo.clear();
    psd3.clear();
    abo.execute(psd3, psd.begin(), psd.end(), psd2.begin(), psd2.end(), arbitrary_boolean_op<Unit>::BOOLEAN_AND);
    psd3.get(pv);
    for(std::size_t i = 0; i < pv.size(); ++i) {
      stdcout << pv[i] << "\n";
    }
    pv.clear();
    abo.clear();
    psd3.clear();
    abo.execute(psd3, psd.begin(), psd.end(), psd2.begin(), psd2.end(), arbitrary_boolean_op<Unit>::BOOLEAN_XOR);
    psd3.get(pv);
    for(std::size_t i = 0; i < pv.size(); ++i) {
      stdcout << pv[i] << "\n";
    }
    pv.clear();
    abo.clear();
    psd3.clear();
    abo.execute(psd3, psd.begin(), psd.end(), psd2.begin(), psd2.end(), arbitrary_boolean_op<Unit>::BOOLEAN_NOT);
    psd3.get(pv);
    for(std::size_t i = 0; i < pv.size(); ++i) {
      stdcout << pv[i] << "\n";
    }
    return true;
  }














  template <typename Unit, typename property_type>
  class arbitrary_connectivity_extraction : public scanline_base<Unit> {
  private:

    typedef typename scanline_base<Unit>::Point Point;

    //the first point is the vertex and and second point establishes the slope of an edge eminating from the vertex
    //typedef std::pair<Point, Point> half_edge;
    typedef typename scanline_base<Unit>::half_edge half_edge;

    //scanline comparator functor
    typedef typename scanline_base<Unit>::less_half_edge less_half_edge;
    typedef typename scanline_base<Unit>::less_point less_point;

    //this data structure assocates a property and count to a half edge
    typedef std::pair<half_edge, std::pair<property_type, int> > vertex_property;
    //this data type stores the combination of many half edges
    typedef std::vector<vertex_property> property_merge_data;

    //this is the data type used internally to store the combination of property counts at a given location
    typedef std::vector<std::pair<property_type, int> > property_map;
    //this data type is used internally to store the combined property data for a given half edge
    typedef std::pair<half_edge, property_map> vertex_data;

    property_merge_data pmd;
    typename scanline_base<Unit>::evalAtXforYPack evalAtXforYPack_;

    template<typename vertex_data_type>
    class less_vertex_data {
      typename scanline_base<Unit>::evalAtXforYPack* pack_;
    public:
      less_vertex_data() : pack_() {}
      less_vertex_data(typename scanline_base<Unit>::evalAtXforYPack* pack) : pack_(pack) {}
      bool operator()(const vertex_data_type& lvalue, const vertex_data_type& rvalue) const {
        less_point lp;
        if(lp(lvalue.first.first, rvalue.first.first)) return true;
        if(lp(rvalue.first.first, lvalue.first.first)) return false;
        Unit x = lvalue.first.first.get(HORIZONTAL);
        int just_before_ = 0;
        less_half_edge lhe(&x, &just_before_, pack_);
        return lhe(lvalue.first, rvalue.first);
      }
    };


    template <typename cT>
    static void process_previous_x(cT& output) {
      std::map<point_data<Unit>, std::set<property_type> >& y_prop_map = output.first.second;
      if(y_prop_map.empty()) return;
      Unit x = output.first.first;
      for(typename std::map<point_data<Unit>, std::set<property_type> >::iterator itr =
            y_prop_map.begin(); itr != y_prop_map.end(); ++itr) {
        if((*itr).first.x() < x) {
          y_prop_map.erase(y_prop_map.begin(), itr);
          continue;
        }
        for(typename std::set<property_type>::iterator inner_itr = itr->second.begin();
            inner_itr != itr->second.end(); ++inner_itr) {
          std::set<property_type>& output_edges = (*(output.second))[*inner_itr];
          typename std::set<property_type>::iterator inner_inner_itr = inner_itr;
          ++inner_inner_itr;
          for( ; inner_inner_itr != itr->second.end(); ++inner_inner_itr) {
            output_edges.insert(output_edges.end(), *inner_inner_itr);
            std::set<property_type>& output_edges_2 = (*(output.second))[*inner_inner_itr];
            output_edges_2.insert(output_edges_2.end(), *inner_itr);
          }
        }
      }
    }

    template <typename result_type, typename key_type>
    class connectivity_extraction_output_functor {
    public:
      connectivity_extraction_output_functor() {}
      void operator()(result_type& result, const half_edge& edge, const key_type& left, const key_type& right) {
        Unit& x = result.first.first;
        std::map<point_data<Unit>, std::set<property_type> >& y_prop_map = result.first.second;
        point_data<Unit> pt = edge.first;
        if(pt.x() != x) process_previous_x(result);
        x = pt.x();
        std::set<property_type>& output_set = y_prop_map[pt];
        {
          for(typename key_type::const_iterator itr1 =
                left.begin(); itr1 != left.end(); ++itr1) {
            output_set.insert(output_set.end(), *itr1);
          }
          for(typename key_type::const_iterator itr2 =
                right.begin(); itr2 != right.end(); ++itr2) {
            output_set.insert(output_set.end(), *itr2);
          }
        }
        std::set<property_type>& output_set2 = y_prop_map[edge.second];
        for(typename key_type::const_iterator itr1 =
              left.begin(); itr1 != left.end(); ++itr1) {
          output_set2.insert(output_set2.end(), *itr1);
        }
        for(typename key_type::const_iterator itr2 =
              right.begin(); itr2 != right.end(); ++itr2) {
          output_set2.insert(output_set2.end(), *itr2);
        }
      }
    };

    inline void sort_property_merge_data() {
      less_vertex_data<vertex_property> lvd(&evalAtXforYPack_);
      polygon_sort(pmd.begin(), pmd.end(), lvd);
    }
  public:
    inline arbitrary_connectivity_extraction() : pmd(), evalAtXforYPack_() {}
    inline arbitrary_connectivity_extraction
    (const arbitrary_connectivity_extraction& pm) : pmd(pm.pmd), evalAtXforYPack_(pm.evalAtXforYPack_) {}
    inline arbitrary_connectivity_extraction& operator=
      (const arbitrary_connectivity_extraction& pm) { pmd = pm.pmd; return *this; }

    template <typename result_type>
    inline void execute(result_type& result) {
      //intersect data
      property_merge_data tmp_pmd;
      line_intersection<Unit>::validate_scan(tmp_pmd, pmd.begin(), pmd.end());
      pmd.swap(tmp_pmd);
      sort_property_merge_data();
      scanline<Unit, property_type, std::vector<property_type> > sl;
      std::pair<std::pair<Unit, std::map<point_data<Unit>, std::set<property_type> > >,
        result_type*> output
        (std::make_pair(std::make_pair((std::numeric_limits<Unit>::max)(),
                                       std::map<point_data<Unit>,
                                       std::set<property_type> >()), &result));
      connectivity_extraction_output_functor<std::pair<std::pair<Unit,
        std::map<point_data<Unit>, std::set<property_type> > >, result_type*>,
        std::vector<property_type> > ceof;
      sl.scan(output, ceof, pmd.begin(), pmd.end());
      process_previous_x(output);
    }

    inline void clear() {*this = arbitrary_connectivity_extraction();}

    template <typename iT>
    void populateTouchSetData(iT begin, iT end,
                                     property_type property) {
      for( ; begin != end; ++begin) {
        pmd.push_back(vertex_property(half_edge((*begin).first.first, (*begin).first.second),
                                      std::pair<property_type, int>(property, (*begin).second)));
      }
    }

  };

}
}
#endif

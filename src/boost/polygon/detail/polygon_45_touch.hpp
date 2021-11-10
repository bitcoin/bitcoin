/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_45_TOUCH_HPP
#define BOOST_POLYGON_POLYGON_45_TOUCH_HPP
namespace boost { namespace polygon{

  template <typename Unit>
  struct polygon_45_touch {

    typedef point_data<Unit> Point;
    typedef typename coordinate_traits<Unit>::manhattan_area_type LongUnit;

    template <typename property_map>
    static inline void merge_property_maps(property_map& mp, const property_map& mp2, bool subtract = false) {
      property_map newmp;
      newmp.reserve(mp.size() + mp2.size());
      std::size_t i = 0;
      std::size_t j = 0;
      while(i != mp.size() && j != mp2.size()) {
        if(mp[i].first < mp2[j].first) {
          newmp.push_back(mp[i]);
          ++i;
        } else if(mp[i].first > mp2[j].first) {
          newmp.push_back(mp2[j]);
          if(subtract) newmp.back().second *= -1;
          ++j;
        } else {
          int count = mp[i].second;
          if(subtract) count -= mp2[j].second;
          else count += mp2[j].second;
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
        if(subtract) newmp.back().second *= -1;
        ++j;
      }
      mp.swap(newmp);
    }

    class CountTouch {
    public:
      inline CountTouch() : counts() {}
      //inline CountTouch(int count) { counts[0] = counts[1] = count; }
      //inline CountTouch(int count1, int count2) { counts[0] = count1; counts[1] = count2; }
      inline CountTouch(const CountTouch& count) : counts(count.counts) {}
      inline bool operator==(const CountTouch& count) const { return counts == count.counts; }
      inline bool operator!=(const CountTouch& count) const { return !((*this) == count); }
      //inline CountTouch& operator=(int count) { counts[0] = counts[1] = count; return *this; }
      inline CountTouch& operator=(const CountTouch& count) { counts = count.counts; return *this; }
      inline int& operator[](int index) {
        std::vector<std::pair<int, int> >::iterator itr =
            std::lower_bound(counts.begin(), counts.end(),
                             std::make_pair(index, int(0)));
        if(itr != counts.end() && itr->first == index) {
            return itr->second;
        }
        itr = counts.insert(itr, std::make_pair(index, int(0)));
        return itr->second;
      }
//       inline int operator[](int index) const {
//         std::vector<std::pair<int, int> >::const_iterator itr = counts.begin();
//         for( ; itr != counts.end() && itr->first <= index; ++itr) {
//           if(itr->first == index) {
//             return itr->second;
//           }
//         }
//         return 0;
//       }
      inline CountTouch& operator+=(const CountTouch& count){
        merge_property_maps(counts, count.counts, false);
        return *this;
      }
      inline CountTouch& operator-=(const CountTouch& count){
        merge_property_maps(counts, count.counts, true);
        return *this;
      }
      inline CountTouch operator+(const CountTouch& count) const {
        return CountTouch(*this)+=count;
      }
      inline CountTouch operator-(const CountTouch& count) const {
        return CountTouch(*this)-=count;
      }
      inline CountTouch invert() const {
        CountTouch retval;
        retval -= *this;
        return retval;
      }
      std::vector<std::pair<int, int> > counts;
    };

    typedef std::pair<std::pair<Unit, std::map<Unit, std::set<int> > >, std::map<int, std::set<int> > > map_graph_o;
    typedef std::pair<std::pair<Unit, std::map<Unit, std::set<int> > >, std::vector<std::set<int> > > vector_graph_o;

    template <typename cT>
    static void process_previous_x(cT& output) {
      std::map<Unit, std::set<int> >& y_prop_map = output.first.second;
      for(typename std::map<Unit, std::set<int> >::iterator itr = y_prop_map.begin();
          itr != y_prop_map.end(); ++itr) {
        for(std::set<int>::iterator inner_itr = itr->second.begin();
            inner_itr != itr->second.end(); ++inner_itr) {
          std::set<int>& output_edges = (*(output.second))[*inner_itr];
          std::set<int>::iterator inner_inner_itr = inner_itr;
          ++inner_inner_itr;
          for( ; inner_inner_itr != itr->second.end(); ++inner_inner_itr) {
            output_edges.insert(output_edges.end(), *inner_inner_itr);
            std::set<int>& output_edges_2 = (*(output.second))[*inner_inner_itr];
            output_edges_2.insert(output_edges_2.end(), *inner_itr);
          }
        }
      }
      y_prop_map.clear();
    }

    struct touch_45_output_functor {
      template <typename cT>
      void operator()(cT& output, const CountTouch& count1, const CountTouch& count2,
                      const Point& pt, int , direction_1d ) {
        Unit& x = output.first.first;
        std::map<Unit, std::set<int> >& y_prop_map = output.first.second;
        if(pt.x() != x) process_previous_x(output);
        x = pt.x();
        std::set<int>& output_set = y_prop_map[pt.y()];
        for(std::vector<std::pair<int, int> >::const_iterator itr1 = count1.counts.begin();
            itr1 != count1.counts.end(); ++itr1) {
          if(itr1->second > 0) {
            output_set.insert(output_set.end(), itr1->first);
          }
        }
        for(std::vector<std::pair<int, int> >::const_iterator itr2 = count2.counts.begin();
            itr2 != count2.counts.end(); ++itr2) {
          if(itr2->second > 0) {
            output_set.insert(output_set.end(), itr2->first);
          }
        }
      }
    };
    typedef typename std::pair<Point,
                               typename boolean_op_45<Unit>::template Scan45CountT<CountTouch> > Vertex45Compact;
    typedef std::vector<Vertex45Compact> TouchSetData;

    struct lessVertex45Compact {
      bool operator()(const Vertex45Compact& l, const Vertex45Compact& r) {
        return l.first < r.first;
      }
    };

//     template <typename TSD>
//     static void print_tsd(TSD& tsd) {
//       for(std::size_t i = 0; i < tsd.size(); ++i) {
//         std::cout << tsd[i].first << ": ";
//         for(unsigned int r = 0; r < 4; ++r) {
//           std::cout << r << " { ";
//           for(std::vector<std::pair<int, int> >::iterator itr = tsd[i].second[r].counts.begin();
//               itr != tsd[i].second[r].counts.end(); ++itr) {
//             std::cout << itr->first << "," << itr->second << " ";
//           } std::cout << "} ";
//         }
//       } std::cout << std::endl;
//     }

//     template <typename T>
//     static void print_scanline(T& t) {
//       for(typename T::iterator itr = t.begin(); itr != t.end(); ++itr) {
//         std::cout << itr->x << "," << itr->y << " " << itr->rise << " ";
//         for(std::vector<std::pair<int, int> >::iterator itr2 = itr->count.counts.begin();
//             itr2 != itr->count.counts.end(); ++itr2) {
//           std::cout << itr2->first << ":" << itr2->second << " ";
//         } std::cout << std::endl;
//       }
//     }

    template <typename graph_type>
    static void performTouch(graph_type& graph, TouchSetData& tsd) {

      polygon_sort(tsd.begin(), tsd.end(), lessVertex45Compact());
      typedef std::vector<std::pair<Point, typename boolean_op_45<Unit>::template Scan45CountT<CountTouch> > > TSD;
      TSD tsd_;
      tsd_.reserve(tsd.size());
      for(typename TouchSetData::iterator itr = tsd.begin(); itr != tsd.end(); ) {
        typename TouchSetData::iterator itr2 = itr;
        ++itr2;
        for(; itr2 != tsd.end() && itr2->first == itr->first; ++itr2) {
          (itr->second) += (itr2->second); //accumulate
        }
        tsd_.push_back(std::make_pair(itr->first, itr->second));
        itr = itr2;
      }
      std::pair<std::pair<Unit, std::map<Unit, std::set<int> > >, graph_type*> output
        (std::make_pair(std::make_pair((std::numeric_limits<Unit>::max)(), std::map<Unit, std::set<int> >()), &graph));
      typename boolean_op_45<Unit>::template Scan45<CountTouch, touch_45_output_functor> scanline;
      for(typename TSD::iterator itr = tsd_.begin(); itr != tsd_.end(); ) {
        typename TSD::iterator itr2 = itr;
        ++itr2;
        while(itr2 != tsd_.end() && itr2->first.x() == itr->first.x()) {
          ++itr2;
        }
        scanline.scan(output, itr, itr2);
        itr = itr2;
      }
      process_previous_x(output);
    }

    template <typename iT>
    static void populateTouchSetData(TouchSetData& tsd, iT begin, iT end, int nodeCount) {
      for( ; begin != end; ++begin) {
        Vertex45Compact vertex;
        vertex.first = typename Vertex45Compact::first_type(begin->pt.x() * 2, begin->pt.y() * 2);
        tsd.push_back(vertex);
        for(unsigned int i = 0; i < 4; ++i) {
          if(begin->count[i]) {
            tsd.back().second[i][nodeCount] += begin->count[i];
          }
        }
      }
    }

  };


}
}
#endif

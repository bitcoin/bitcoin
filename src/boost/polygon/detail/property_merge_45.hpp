/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_PROPERTY_MERGE_45_HPP
#define BOOST_POLYGON_PROPERTY_MERGE_45_HPP
namespace boost { namespace polygon{

  template <typename Unit, typename property_type>
  struct polygon_45_property_merge {

    typedef point_data<Unit> Point;
    typedef typename coordinate_traits<Unit>::manhattan_area_type LongUnit;

    template <typename property_map>
    static inline void merge_property_maps(property_map& mp, const property_map& mp2, bool subtract = false) {
      polygon_45_touch<Unit>::merge_property_maps(mp, mp2, subtract);
    }

    class CountMerge {
    public:
      inline CountMerge() : counts() {}
      //inline CountMerge(int count) { counts[0] = counts[1] = count; }
      //inline CountMerge(int count1, int count2) { counts[0] = count1; counts[1] = count2; }
      inline CountMerge(const CountMerge& count) : counts(count.counts) {}
      inline bool operator==(const CountMerge& count) const { return counts == count.counts; }
      inline bool operator!=(const CountMerge& count) const { return !((*this) == count); }
      //inline CountMerge& operator=(int count) { counts[0] = counts[1] = count; return *this; }
      inline CountMerge& operator=(const CountMerge& count) { counts = count.counts; return *this; }
      inline int& operator[](property_type index) {
        std::vector<std::pair<int, int> >::iterator itr = lower_bound(counts.begin(), counts.end(), std::make_pair(index, int(0)));
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
      inline CountMerge& operator+=(const CountMerge& count){
        merge_property_maps(counts, count.counts, false);
        return *this;
      }
      inline CountMerge& operator-=(const CountMerge& count){
        merge_property_maps(counts, count.counts, true);
        return *this;
      }
      inline CountMerge operator+(const CountMerge& count) const {
        return CountMerge(*this)+=count;
      }
      inline CountMerge operator-(const CountMerge& count) const {
        return CountMerge(*this)-=count;
      }
      inline CountMerge invert() const {
        CountMerge retval;
        retval -= *this;
        return retval;
      }
      std::vector<std::pair<property_type, int> > counts;
    };

    //output is a std::map<std::set<property_type>, polygon_45_set_data<Unit> >
    struct merge_45_output_functor {
      template <typename cT>
      void operator()(cT& output, const CountMerge& count1, const CountMerge& count2,
                      const Point& pt, int rise, direction_1d end) {
        typedef typename cT::key_type keytype;
        keytype left;
        keytype right;
        int edgeType = end == LOW ? -1 : 1;
        for(typename std::vector<std::pair<property_type, int> >::const_iterator itr = count1.counts.begin();
            itr != count1.counts.end(); ++itr) {
          left.insert(left.end(), (*itr).first);
        }
        for(typename std::vector<std::pair<property_type, int> >::const_iterator itr = count2.counts.begin();
            itr != count2.counts.end(); ++itr) {
          right.insert(right.end(), (*itr).first);
        }
        if(left == right) return;
        if(!left.empty()) {
          //std::cout << pt.x() << " " << pt.y() << " " << rise << " " << edgeType << std::endl;
          output[left].insert_clean(typename boolean_op_45<Unit>::Vertex45(pt, rise, -edgeType));
        }
        if(!right.empty()) {
          //std::cout << pt.x() << " " << pt.y() << " " << rise << " " << -edgeType << std::endl;
          output[right].insert_clean(typename boolean_op_45<Unit>::Vertex45(pt, rise, edgeType));
        }
      }
    };

    typedef typename std::pair<Point,
                               typename boolean_op_45<Unit>::template Scan45CountT<CountMerge> > Vertex45Compact;
    typedef std::vector<Vertex45Compact> MergeSetData;

    struct lessVertex45Compact {
      bool operator()(const Vertex45Compact& l, const Vertex45Compact& r) {
        return l.first < r.first;
      }
    };

    template <typename output_type>
    static void performMerge(output_type& result, MergeSetData& tsd) {

      polygon_sort(tsd.begin(), tsd.end(), lessVertex45Compact());
      typedef std::vector<std::pair<Point, typename boolean_op_45<Unit>::template Scan45CountT<CountMerge> > > TSD;
      TSD tsd_;
      tsd_.reserve(tsd.size());
      for(typename MergeSetData::iterator itr = tsd.begin(); itr != tsd.end(); ) {
        typename MergeSetData::iterator itr2 = itr;
        ++itr2;
        for(; itr2 != tsd.end() && itr2->first == itr->first; ++itr2) {
          (itr->second) += (itr2->second); //accumulate
        }
        tsd_.push_back(std::make_pair(itr->first, itr->second));
        itr = itr2;
      }
      typename boolean_op_45<Unit>::template Scan45<CountMerge, merge_45_output_functor> scanline;
      for(typename TSD::iterator itr = tsd_.begin(); itr != tsd_.end(); ) {
        typename TSD::iterator itr2 = itr;
        ++itr2;
        while(itr2 != tsd_.end() && itr2->first.x() == itr->first.x()) {
          ++itr2;
        }
        scanline.scan(result, itr, itr2);
        itr = itr2;
      }
    }

    template <typename iT>
    static void populateMergeSetData(MergeSetData& tsd, iT begin, iT end, property_type property) {
      for( ; begin != end; ++begin) {
        Vertex45Compact vertex;
        vertex.first = typename Vertex45Compact::first_type(begin->pt.x() * 2, begin->pt.y() * 2);
        tsd.push_back(vertex);
        for(unsigned int i = 0; i < 4; ++i) {
          if(begin->count[i]) {
            tsd.back().second[i][property] += begin->count[i];
          }
        }
      }
    }

  };



}
}

#endif

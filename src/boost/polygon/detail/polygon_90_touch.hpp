/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_90_TOUCH_HPP
#define BOOST_POLYGON_POLYGON_90_TOUCH_HPP
namespace boost { namespace polygon{

  template <typename Unit>
  struct touch_90_operation {
    typedef interval_data<Unit> Interval;

    class TouchScanEvent {
    private:
      typedef std::map<Unit, std::set<int> > EventData;
      EventData eventData_;
    public:

      // The TouchScanEvent::iterator is a lazy algorithm that accumulates
      // polygon ids in a set as it is incremented through the
      // scan event data structure.
      // The iterator provides a forward iterator semantic only.
      class iterator {
      private:
        typename EventData::const_iterator itr_;
        std::pair<Interval, std::set<int> > ivlIds_;
        bool incremented_;
      public:
        inline iterator() : itr_(), ivlIds_(), incremented_(false) {}
        inline iterator(typename EventData::const_iterator itr,
                        Unit prevPos, Unit curPos, const std::set<int>& ivlIds) : itr_(itr), ivlIds_(), incremented_(false) {
          ivlIds_.second = ivlIds;
          ivlIds_.first = Interval(prevPos, curPos);
        }
        inline iterator(const iterator& that) : itr_(), ivlIds_(), incremented_(false) { (*this) = that; }
        inline iterator& operator=(const iterator& that) {
          itr_ = that.itr_;
          ivlIds_.first = that.ivlIds_.first;
          ivlIds_.second = that.ivlIds_.second;
          incremented_ = that.incremented_;
          return *this;
        }
        inline bool operator==(const iterator& that) { return itr_ == that.itr_; }
        inline bool operator!=(const iterator& that) { return itr_ != that.itr_; }
        inline iterator& operator++() {
          //std::cout << "increment\n";
          //std::cout << "state\n";
          //for(std::set<int>::iterator itr = ivlIds_.second.begin(); itr != ivlIds_.second.end(); ++itr) {
          //   std::cout << (*itr) << " ";
          //} std::cout << std::endl;
          //std::cout << "update\n";
          for(std::set<int>::const_iterator itr = (*itr_).second.begin();
              itr != (*itr_).second.end(); ++itr) {
            //std::cout << (*itr) <<  " ";
            std::set<int>::iterator lb = ivlIds_.second.find(*itr);
            if(lb != ivlIds_.second.end()) {
              ivlIds_.second.erase(lb);
            } else {
              ivlIds_.second.insert(*itr);
            }
          }
          //std::cout << std::endl;
          //std::cout << "new state\n";
          //for(std::set<int>::iterator itr = ivlIds_.second.begin(); itr != ivlIds_.second.end(); ++itr) {
          //   std::cout << (*itr) << " ";
          //} std::cout << std::endl;
          ++itr_;
          //ivlIds_.first = Interval(ivlIds_.first.get(HIGH), itr_->first);
          incremented_ = true;
          return *this;
        }
        inline const iterator operator++(int){
          iterator tmpItr(*this);
          ++(*this);
          return tmpItr;
        }
        inline std::pair<Interval, std::set<int> >& operator*() {
          if(incremented_) ivlIds_.first = Interval(ivlIds_.first.get(HIGH), itr_->first);
          incremented_ = false;
          if(ivlIds_.second.empty())(++(*this));
          if(incremented_) ivlIds_.first = Interval(ivlIds_.first.get(HIGH), itr_->first);
          incremented_ = false;
          return ivlIds_; }
      };

      inline TouchScanEvent() : eventData_() {}
      template<class iT>
      inline TouchScanEvent(iT begin, iT end) : eventData_() {
        for( ; begin != end; ++begin){
          insert(*begin);
        }
      }
      inline TouchScanEvent(const TouchScanEvent& that) : eventData_(that.eventData_) {}
      inline TouchScanEvent& operator=(const TouchScanEvent& that){
        eventData_ = that.eventData_;
        return *this;
      }

      //Insert an interval polygon id into the EventData
      inline void insert(const std::pair<Interval, int>& intervalId){
        insert(intervalId.first.low(), intervalId.second);
        insert(intervalId.first.high(), intervalId.second);
      }

      //Insert an position and polygon id into EventData
      inline void insert(Unit pos, int id) {
        typename EventData::iterator lb = eventData_.lower_bound(pos);
        if(lb != eventData_.end() && lb->first == pos) {
          std::set<int>& mr (lb->second);
          std::set<int>::iterator mri = mr.find(id);
          if(mri == mr.end()) {
            mr.insert(id);
          } else {
            mr.erase(id);
          }
        } else {
          lb = eventData_.insert(lb, std::pair<Unit, std::set<int> >(pos, std::set<int>()));
          (*lb).second.insert(id);
        }
      }

      //merge this scan event with that by inserting its data
      inline void insert(const TouchScanEvent& that){
        typename EventData::const_iterator itr;
        for(itr = that.eventData_.begin(); itr != that.eventData_.end(); ++itr) {
          eventData_[(*itr).first].insert(itr->second.begin(), itr->second.end());
        }
      }

      //Get the begin iterator over event data
      inline iterator begin() const {
        //std::cout << "begin\n";
        if(eventData_.empty()) return end();
        typename EventData::const_iterator itr = eventData_.begin();
        Unit pos = itr->first;
        const std::set<int>& idr = itr->second;
        ++itr;
        return iterator(itr, pos, itr->first, idr);
      }

      //Get the end iterator over event data
      inline iterator end() const { return iterator(eventData_.end(), 0, 0, std::set<int>()); }

      inline void clear() { eventData_.clear(); }

      inline Interval extents() const {
        if(eventData_.empty()) return Interval();
        return Interval((*(eventData_.begin())).first, (*(eventData_.rbegin())).first);
      }
    };

    //declaration of a map of scan events by coordinate value used to store all the
    //polygon data for a single layer input into the scanline algorithm
    typedef std::pair<std::map<Unit, TouchScanEvent>, std::map<Unit, TouchScanEvent> > TouchSetData;

    class TouchOp {
    public:
      typedef std::map<Unit, std::set<int> > ScanData;
      typedef std::pair<Unit, std::set<int> > ElementType;
    protected:
      ScanData scanData_;
      typename ScanData::iterator nextItr_;
    public:
      inline TouchOp () : scanData_(), nextItr_() { nextItr_ = scanData_.end(); }
      inline TouchOp (const TouchOp& that) : scanData_(that.scanData_), nextItr_() { nextItr_ = scanData_.begin(); }
      inline TouchOp& operator=(const TouchOp& that);

      //moves scanline forward
      inline void advanceScan() { nextItr_ = scanData_.begin(); }

      //proceses the given interval and std::set<int> data
      //the output data structre is a graph, the indicies in the vector correspond to graph nodes,
      //the integers in the set are vector indicies and are the nodes with which that node shares an edge
      template <typename graphT>
      inline void processInterval(graphT& outputContainer, Interval ivl, const std::set<int>& ids, bool leadingEdge) {
        //print();
        typename ScanData::iterator lowItr = lookup_(ivl.low());
        typename ScanData::iterator highItr = lookup_(ivl.high());
        //std::cout << "Interval: " << ivl << std::endl;
        //for(std::set<int>::const_iterator itr = ids.begin(); itr != ids.end(); ++itr)
        //   std::cout << (*itr) << " ";
        //std::cout << std::endl;
        //add interval to scan data if it is past the end
        if(lowItr == scanData_.end()) {
          //std::cout << "case0" << std::endl;
          lowItr = insert_(ivl.low(), ids);
          evaluateBorder_(outputContainer, ids, ids);
          highItr = insert_(ivl.high(), std::set<int>());
          return;
        }
        //ensure that highItr points to the end of the ivl
        if(highItr == scanData_.end() || (*highItr).first > ivl.high()) {
          //std::cout << "case1" << std::endl;
          //std::cout << highItr->first << std::endl;
          std::set<int> value = std::set<int>();
          if(highItr != scanData_.begin()) {
            --highItr;
            //std::cout << highItr->first << std::endl;
            //std::cout << "high set size " << highItr->second.size() << std::endl;
            value = highItr->second;
          }
          nextItr_ = highItr;
          highItr = insert_(ivl.high(), value);
        } else {
          //evaluate border with next higher interval
          //std::cout << "case1a" << std::endl;
          if(leadingEdge)evaluateBorder_(outputContainer, highItr->second, ids);
        }
        //split the low interval if needed
        if(lowItr->first > ivl.low()) {
          //std::cout << "case2" << std::endl;
          if(lowItr != scanData_.begin()) {
            //std::cout << "case3" << std::endl;
            --lowItr;
            nextItr_ = lowItr;
            //std::cout << lowItr->first << " " << lowItr->second.size() << std::endl;
            lowItr = insert_(ivl.low(), lowItr->second);
          } else {
            //std::cout << "case4" << std::endl;
            nextItr_ = lowItr;
            lowItr = insert_(ivl.low(), std::set<int>());
          }
        } else {
          //evaluate border with next higher interval
          //std::cout << "case2a" << std::endl;
          typename ScanData::iterator nextLowerItr = lowItr;
          if(leadingEdge && nextLowerItr != scanData_.begin()){
            --nextLowerItr;
            evaluateBorder_(outputContainer, nextLowerItr->second, ids);
          }
        }
        //std::cout << "low: " << lowItr->first << " high: " << highItr->first << std::endl;
        //print();
        //process scan data intersecting interval
        for(typename ScanData::iterator itr = lowItr; itr != highItr; ){
          //std::cout << "case5" << std::endl;
          //std::cout << itr->first << std::endl;
          std::set<int>& beforeIds = itr->second;
          ++itr;
          evaluateInterval_(outputContainer, beforeIds, ids, leadingEdge);
        }
        //print();
        //merge the bottom interval with the one below if they have the same count
        if(lowItr != scanData_.begin()){
          //std::cout << "case6" << std::endl;
          typename ScanData::iterator belowLowItr = lowItr;
          --belowLowItr;
          if(belowLowItr->second == lowItr->second) {
            //std::cout << "case7" << std::endl;
            scanData_.erase(lowItr);
          }
        }
        //merge the top interval with the one above if they have the same count
        if(highItr != scanData_.begin()) {
          //std::cout << "case8" << std::endl;
          typename ScanData::iterator beforeHighItr = highItr;
          --beforeHighItr;
          if(beforeHighItr->second == highItr->second) {
            //std::cout << "case9" << std::endl;
            scanData_.erase(highItr);
            highItr = beforeHighItr;
            ++highItr;
          }
        }
        //print();
        nextItr_ = highItr;
      }

//       inline void print() const {
//         for(typename ScanData::const_iterator itr = scanData_.begin(); itr != scanData_.end(); ++itr) {
//           std::cout << itr->first << ": ";
//           for(std::set<int>::const_iterator sitr = itr->second.begin();
//               sitr != itr->second.end(); ++sitr){
//             std::cout << *sitr << " ";
//           }
//           std::cout << std::endl;
//         }
//       }

    private:
      inline typename ScanData::iterator lookup_(Unit pos){
        if(nextItr_ != scanData_.end() && nextItr_->first >= pos) {
          return nextItr_;
        }
        return nextItr_ = scanData_.lower_bound(pos);
      }

      inline typename ScanData::iterator insert_(Unit pos, const std::set<int>& ids){
        //std::cout << "inserting " << ids.size() << " ids at: " << pos << std::endl;
        return nextItr_ = scanData_.insert(nextItr_, std::pair<Unit, std::set<int> >(pos, ids));
      }

      template <typename graphT>
      inline void evaluateInterval_(graphT& outputContainer, std::set<int>& ids,
                                    const std::set<int>& changingIds, bool leadingEdge) {
        for(std::set<int>::const_iterator ciditr = changingIds.begin(); ciditr != changingIds.end(); ++ciditr){
          //std::cout << "evaluateInterval " << (*ciditr) << std::endl;
          evaluateId_(outputContainer, ids, *ciditr, leadingEdge);
        }
      }
      template <typename graphT>
      inline void evaluateBorder_(graphT& outputContainer, const std::set<int>& ids, const std::set<int>& changingIds) {
        for(std::set<int>::const_iterator ciditr = changingIds.begin(); ciditr != changingIds.end(); ++ciditr){
          //std::cout << "evaluateBorder " << (*ciditr) << std::endl;
          evaluateBorderId_(outputContainer, ids, *ciditr);
        }
      }
      template <typename graphT>
      inline void evaluateBorderId_(graphT& outputContainer, const std::set<int>& ids, int changingId) {
        for(std::set<int>::const_iterator scanItr = ids.begin(); scanItr != ids.end(); ++scanItr) {
          //std::cout << "create edge: " << changingId << " " << *scanItr << std::endl;
          if(changingId != *scanItr){
            outputContainer[changingId].insert(*scanItr);
            outputContainer[*scanItr].insert(changingId);
          }
        }
      }
      template <typename graphT>
      inline void evaluateId_(graphT& outputContainer, std::set<int>& ids, int changingId, bool leadingEdge) {
        //std::cout << "changingId: " << changingId << std::endl;
        //for( std::set<int>::iterator itr = ids.begin(); itr != ids.end(); ++itr){
        //   std::cout << *itr << " ";
        //}std::cout << std::endl;
        std::set<int>::iterator lb = ids.lower_bound(changingId);
        if(lb == ids.end() || (*lb) != changingId) {
          if(leadingEdge) {
            //std::cout << "insert\n";
            //insert and add to output
            for(std::set<int>::iterator scanItr = ids.begin(); scanItr != ids.end(); ++scanItr) {
              //std::cout << "create edge: " << changingId << " " << *scanItr << std::endl;
              if(changingId != *scanItr){
                outputContainer[changingId].insert(*scanItr);
                outputContainer[*scanItr].insert(changingId);
              }
            }
            ids.insert(changingId);
          }
        } else {
          if(!leadingEdge){
            //std::cout << "erase\n";
            ids.erase(lb);
          }
        }
      }
    };

    template <typename graphT>
    static inline void processEvent(graphT& outputContainer, TouchOp& op, const TouchScanEvent& data, bool leadingEdge) {
      for(typename TouchScanEvent::iterator itr = data.begin(); itr != data.end(); ++itr) {
        //std::cout << "processInterval" << std::endl;
        op.processInterval(outputContainer, (*itr).first, (*itr).second, leadingEdge);
      }
    }

    template <typename graphT>
    static inline void performTouch(graphT& outputContainer, const TouchSetData& data) {
      typename std::map<Unit, TouchScanEvent>::const_iterator leftItr = data.first.begin();
      typename std::map<Unit, TouchScanEvent>::const_iterator rightItr = data.second.begin();
      typename std::map<Unit, TouchScanEvent>::const_iterator leftEnd = data.first.end();
      typename std::map<Unit, TouchScanEvent>::const_iterator rightEnd = data.second.end();
      TouchOp op;
      while(leftItr != leftEnd || rightItr != rightEnd) {
        //std::cout << "loop" << std::endl;
        op.advanceScan();
        //rightItr cannont be at end if leftItr is not at end
        if(leftItr != leftEnd && rightItr != rightEnd &&
           leftItr->first <= rightItr->first) {
          //std::cout << "case1" << std::endl;
          //std::cout << leftItr ->first << std::endl;
          processEvent(outputContainer, op, leftItr->second, true);
          ++leftItr;
        } else {
          //std::cout << "case2" << std::endl;
          //std::cout << rightItr ->first << std::endl;
          processEvent(outputContainer, op, rightItr->second, false);
          ++rightItr;
        }
      }
    }

    template <class iT>
    static inline void populateTouchSetData(TouchSetData& data, iT beginData, iT endData, int id) {
      Unit prevPos = ((std::numeric_limits<Unit>::max)());
      Unit prevY = prevPos;
      int count = 0;
      for(iT itr = beginData; itr != endData; ++itr) {
        Unit pos = (*itr).first;
        if(pos != prevPos) {
          prevPos = pos;
          prevY = (*itr).second.first;
          count = (*itr).second.second;
          continue;
        }
        Unit y = (*itr).second.first;
        if(count != 0 && y != prevY) {
          std::pair<Interval, int> element(Interval(prevY, y), id);
          if(count > 0) {
            data.first[pos].insert(element);
          } else {
            data.second[pos].insert(element);
          }
        }
        prevY = y;
        count += (*itr).second.second;
      }
    }

    static inline void populateTouchSetData(TouchSetData& data, const std::vector<std::pair<Unit, std::pair<Unit, int> > >& inputData, int id) {
      populateTouchSetData(data, inputData.begin(), inputData.end(), id);
    }

  };
}
}
#endif

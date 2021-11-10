/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_BOOLEAN_OP_HPP
#define BOOST_POLYGON_BOOLEAN_OP_HPP
namespace boost { namespace polygon{
namespace boolean_op {

  //BooleanOp is the generic boolean operation scanline algorithm that provides
  //all the simple boolean set operations on manhattan data.  By templatizing
  //the intersection count of the input and algorithm internals it is extensible
  //to multi-layer scans, properties and other advanced scanline operations above
  //and beyond simple booleans.
  //T must cast to int
  template <class T, typename Unit>
  class BooleanOp {
  public:
    typedef std::map<Unit, T> ScanData;
    typedef std::pair<Unit, T> ElementType;
  protected:
    ScanData scanData_;
    typename ScanData::iterator nextItr_;
    T nullT_;
  public:
    inline BooleanOp () : scanData_(), nextItr_(), nullT_() { nextItr_ = scanData_.end(); nullT_ = 0; }
    inline BooleanOp (T nullT) : scanData_(), nextItr_(), nullT_(nullT) { nextItr_ = scanData_.end(); }
    inline BooleanOp (const BooleanOp& that) : scanData_(that.scanData_), nextItr_(),
                                               nullT_(that.nullT_) { nextItr_ = scanData_.begin(); }
    inline BooleanOp& operator=(const BooleanOp& that);

    //moves scanline forward
    inline void advanceScan() { nextItr_ = scanData_.begin(); }

    //proceses the given interval and T data
    //appends output edges to cT
    template <class cT>
    inline void processInterval(cT& outputContainer, interval_data<Unit> ivl, T deltaCount);

  private:
    inline typename ScanData::iterator lookup_(Unit pos){
      if(nextItr_ != scanData_.end() && nextItr_->first >= pos) {
        return nextItr_;
      }
      return nextItr_ = scanData_.lower_bound(pos);
    }
    inline typename ScanData::iterator insert_(Unit pos, T count){
      return nextItr_ = scanData_.insert(nextItr_, ElementType(pos, count));
    }
    template <class cT>
    inline void evaluateInterval_(cT& outputContainer, interval_data<Unit> ivl, T beforeCount, T afterCount);
  };

  class BinaryAnd {
  public:
    inline BinaryAnd() {}
    inline bool operator()(int a, int b) { return (a > 0) & (b > 0); }
  };
  class BinaryOr {
  public:
    inline BinaryOr() {}
    inline bool operator()(int a, int b) { return (a > 0) | (b > 0); }
  };
  class BinaryNot {
  public:
    inline BinaryNot() {}
    inline bool operator()(int a, int b) { return (a > 0) & !(b > 0); }
  };
  class BinaryXor {
  public:
    inline BinaryXor() {}
    inline bool operator()(int a, int b) { return (a > 0) ^ (b > 0); }
  };

  //BinaryCount is an array of two deltaCounts coming from two different layers
  //of scan event data.  It is the merged count of the two suitable for consumption
  //as the template argument of the BooleanOp algorithm because BinaryCount casts to int.
  //T is a binary functor object that evaluates the array of counts and returns a logical
  //result of some operation on those values.
  //BinaryCount supports many of the operators that work with int, particularly the
  //binary operators, but cannot support less than or increment.
  template <class T>
  class BinaryCount {
  public:
    inline BinaryCount()
#ifndef BOOST_POLYGON_MSVC
      : counts_()
#endif
    { counts_[0] = counts_[1] = 0; }
    // constructs from two integers
    inline BinaryCount(int countL, int countR)
#ifndef BOOST_POLYGON_MSVC
      : counts_()
#endif
    { counts_[0] = countL, counts_[1] = countR; }
    inline BinaryCount& operator=(int count) { counts_[0] = count, counts_[1] = count; return *this; }
    inline BinaryCount& operator=(const BinaryCount& that);
    inline BinaryCount(const BinaryCount& that)
#ifndef BOOST_POLYGON_MSVC
      : counts_()
#endif
    { *this = that; }
    inline bool operator==(const BinaryCount& that) const;
    inline bool operator!=(const BinaryCount& that) const { return !((*this) == that);}
    inline BinaryCount& operator+=(const BinaryCount& that);
    inline BinaryCount& operator-=(const BinaryCount& that);
    inline BinaryCount operator+(const BinaryCount& that) const;
    inline BinaryCount operator-(const BinaryCount& that) const;
    inline BinaryCount operator-() const;
    inline int& operator[](bool index) { return counts_[index]; }

    //cast to int operator evaluates data using T binary functor
    inline operator int() const { return T()(counts_[0], counts_[1]); }
  private:
    int counts_[2];
  };

  class UnaryCount {
  public:
    inline UnaryCount() : count_(0) {}
    // constructs from two integers
    inline explicit UnaryCount(int count) : count_(count) {}
    inline UnaryCount& operator=(int count) { count_ = count; return *this; }
    inline UnaryCount& operator=(const UnaryCount& that) { count_ = that.count_; return *this; }
    inline UnaryCount(const UnaryCount& that) : count_(that.count_) {}
    inline bool operator==(const UnaryCount& that) const { return count_ == that.count_; }
    inline bool operator!=(const UnaryCount& that) const { return !((*this) == that);}
    inline UnaryCount& operator+=(const UnaryCount& that) { count_ += that.count_; return *this; }
    inline UnaryCount& operator-=(const UnaryCount& that) { count_ -= that.count_; return *this; }
    inline UnaryCount operator+(const UnaryCount& that) const { UnaryCount tmp(*this); tmp += that; return tmp; }
    inline UnaryCount operator-(const UnaryCount& that) const { UnaryCount tmp(*this); tmp -= that; return tmp; }
    inline UnaryCount operator-() const { UnaryCount tmp; return tmp - *this; }

    //cast to int operator evaluates data using T binary functor
    inline operator int() const { return count_ % 2; }
  private:
    int count_;
  };

  template <class T, typename Unit>
  inline BooleanOp<T, Unit>& BooleanOp<T, Unit>::operator=(const BooleanOp& that) {
    scanData_ = that.scanData_;
    nextItr_ = scanData_.begin();
    nullT_ = that.nullT_;
    return *this;
  }

  //appends output edges to cT
  template <class T, typename Unit>
  template <class cT>
  inline void BooleanOp<T, Unit>::processInterval(cT& outputContainer, interval_data<Unit> ivl, T deltaCount) {
    typename ScanData::iterator lowItr = lookup_(ivl.low());
    typename ScanData::iterator highItr = lookup_(ivl.high());
    //add interval to scan data if it is past the end
    if(lowItr == scanData_.end()) {
      lowItr = insert_(ivl.low(), deltaCount);
      highItr = insert_(ivl.high(), nullT_);
      evaluateInterval_(outputContainer, ivl, nullT_, deltaCount);
      return;
    }
    //ensure that highItr points to the end of the ivl
    if(highItr == scanData_.end() || (*highItr).first > ivl.high()) {
      T value = nullT_;
      if(highItr != scanData_.begin()) {
        --highItr;
        value = highItr->second;
      }
      nextItr_ = highItr;
      highItr = insert_(ivl.high(), value);
    }
    //split the low interval if needed
    if(lowItr->first > ivl.low()) {
      if(lowItr != scanData_.begin()) {
        --lowItr;
        nextItr_ = lowItr;
        lowItr = insert_(ivl.low(), lowItr->second);
      } else {
        nextItr_ = lowItr;
        lowItr = insert_(ivl.low(), nullT_);
      }
    }
    //process scan data intersecting interval
    for(typename ScanData::iterator itr = lowItr; itr != highItr; ){
      T beforeCount = itr->second;
      T afterCount = itr->second += deltaCount;
      Unit low = itr->first;
      ++itr;
      Unit high = itr->first;
      evaluateInterval_(outputContainer, interval_data<Unit>(low, high), beforeCount, afterCount);
    }
    //merge the bottom interval with the one below if they have the same count
    if(lowItr != scanData_.begin()){
      typename ScanData::iterator belowLowItr = lowItr;
      --belowLowItr;
      if(belowLowItr->second == lowItr->second) {
        scanData_.erase(lowItr);
      }
    }
    //merge the top interval with the one above if they have the same count
    if(highItr != scanData_.begin()) {
      typename ScanData::iterator beforeHighItr = highItr;
      --beforeHighItr;
      if(beforeHighItr->second == highItr->second) {
        scanData_.erase(highItr);
        highItr = beforeHighItr;
        ++highItr;
      }
    }
    nextItr_ = highItr;
  }

  template <class T, typename Unit>
  template <class cT>
  inline void BooleanOp<T, Unit>::evaluateInterval_(cT& outputContainer, interval_data<Unit> ivl,
                                              T beforeCount, T afterCount) {
    bool before = (int)beforeCount > 0;
    bool after = (int)afterCount > 0;
    int value =  (!before & after) - (before & !after);
    if(value) {
      outputContainer.insert(outputContainer.end(), std::pair<interval_data<Unit>, int>(ivl, value));
    }
  }

  template <class T>
  inline BinaryCount<T>& BinaryCount<T>::operator=(const BinaryCount<T>& that) {
    counts_[0] = that.counts_[0];
    counts_[1] = that.counts_[1];
    return *this;
  }
  template <class T>
  inline bool BinaryCount<T>::operator==(const BinaryCount<T>& that) const {
    return counts_[0] == that.counts_[0] &&
      counts_[1] == that.counts_[1];
  }
  template <class T>
  inline BinaryCount<T>& BinaryCount<T>::operator+=(const BinaryCount<T>& that) {
    counts_[0] += that.counts_[0];
    counts_[1] += that.counts_[1];
    return *this;
  }
  template <class T>
  inline BinaryCount<T>& BinaryCount<T>::operator-=(const BinaryCount<T>& that) {
    counts_[0] += that.counts_[0];
    counts_[1] += that.counts_[1];
    return *this;
  }
  template <class T>
  inline BinaryCount<T> BinaryCount<T>::operator+(const BinaryCount<T>& that) const {
    BinaryCount retVal(*this);
    retVal += that;
    return retVal;
  }
  template <class T>
  inline BinaryCount<T> BinaryCount<T>::operator-(const BinaryCount<T>& that) const {
    BinaryCount retVal(*this);
    retVal -= that;
    return retVal;
  }
  template <class T>
  inline BinaryCount<T> BinaryCount<T>::operator-() const {
    return BinaryCount<T>() - *this;
  }


  template <class T, typename Unit, typename iterator_type_1, typename iterator_type_2>
  inline void applyBooleanBinaryOp(std::vector<std::pair<Unit, std::pair<Unit, int> > >& output,
                                   //const std::vector<std::pair<Unit, std::pair<Unit, int> > >& input1,
                                   //const std::vector<std::pair<Unit, std::pair<Unit, int> > >& input2,
                                   iterator_type_1 itr1, iterator_type_1 itr1_end,
                                   iterator_type_2 itr2, iterator_type_2 itr2_end,
                                   T defaultCount) {
    BooleanOp<T, Unit> boolean(defaultCount);
    //typename std::vector<std::pair<Unit, std::pair<Unit, int> > >::const_iterator itr1 = input1.begin();
    //typename std::vector<std::pair<Unit, std::pair<Unit, int> > >::const_iterator itr2 = input2.begin();
    std::vector<std::pair<interval_data<Unit>, int> > container;
    //output.reserve((std::max)(input1.size(), input2.size()));

    //consider eliminating dependecy on limits with bool flag for initial state
    Unit UnitMax = (std::numeric_limits<Unit>::max)();
    Unit prevCoord = UnitMax;
    Unit prevPosition = UnitMax;
    T count(defaultCount);
    //define the starting point
    if(itr1 != itr1_end) {
      prevCoord = (*itr1).first;
      prevPosition = (*itr1).second.first;
      count[0] += (*itr1).second.second;
    }
    if(itr2 != itr2_end) {
      if((*itr2).first < prevCoord ||
         ((*itr2).first == prevCoord && (*itr2).second.first < prevPosition)) {
        prevCoord = (*itr2).first;
        prevPosition = (*itr2).second.first;
        count = defaultCount;
        count[1] += (*itr2).second.second;
        ++itr2;
      } else if((*itr2).first == prevCoord && (*itr2).second.first == prevPosition) {
        count[1] += (*itr2).second.second;
        ++itr2;
        if(itr1 != itr1_end) ++itr1;
      } else {
        if(itr1 != itr1_end) ++itr1;
      }
    } else {
      if(itr1 != itr1_end) ++itr1;
    }

    while(itr1 != itr1_end || itr2 != itr2_end) {
      Unit curCoord = UnitMax;
      Unit curPosition = UnitMax;
      T curCount(defaultCount);
      if(itr1 != itr1_end) {
        curCoord = (*itr1).first;
        curPosition = (*itr1).second.first;
        curCount[0] += (*itr1).second.second;
      }
      if(itr2 != itr2_end) {
        if((*itr2).first < curCoord ||
           ((*itr2).first == curCoord && (*itr2).second.first < curPosition)) {
          curCoord = (*itr2).first;
          curPosition = (*itr2).second.first;
          curCount = defaultCount;
          curCount[1] += (*itr2).second.second;
          ++itr2;
        } else if((*itr2).first == curCoord && (*itr2).second.first == curPosition) {
          curCount[1] += (*itr2).second.second;
          ++itr2;
          if(itr1 != itr1_end) ++itr1;
        } else {
          if(itr1 != itr1_end) ++itr1;
        }
      } else {
        ++itr1;
      }

      if(prevCoord != curCoord) {
        boolean.advanceScan();
        prevCoord = curCoord;
        prevPosition = curPosition;
        count = curCount;
        continue;
      }
      if(curPosition != prevPosition && count != defaultCount) {
        interval_data<Unit> ivl(prevPosition, curPosition);
        container.clear();
        boolean.processInterval(container, ivl, count);
        for(std::size_t i = 0; i < container.size(); ++i) {
          std::pair<interval_data<Unit>, int>& element = container[i];
          if(!output.empty() && output.back().first == prevCoord &&
             output.back().second.first == element.first.low() &&
             output.back().second.second == element.second * -1) {
            output.pop_back();
          } else {
            output.push_back(std::pair<Unit, std::pair<Unit, int> >(prevCoord, std::pair<Unit, int>(element.first.low(),
                                                                                                    element.second)));
          }
          output.push_back(std::pair<Unit, std::pair<Unit, int> >(prevCoord, std::pair<Unit, int>(element.first.high(),
                                                                                                  element.second * -1)));
        }
      }
      prevPosition = curPosition;
      count += curCount;
    }
  }

  template <class T, typename Unit>
  inline void applyBooleanBinaryOp(std::vector<std::pair<Unit, std::pair<Unit, int> > >& inputOutput,
                                   const std::vector<std::pair<Unit, std::pair<Unit, int> > >& input2,
                                   T defaultCount) {
    std::vector<std::pair<Unit, std::pair<Unit, int> > > output;
    applyBooleanBinaryOp(output, inputOutput, input2, defaultCount);
    if(output.size() < inputOutput.size() / 2) {
      inputOutput = std::vector<std::pair<Unit, std::pair<Unit, int> > >();
    } else {
      inputOutput.clear();
    }
    inputOutput.insert(inputOutput.end(), output.begin(), output.end());
  }

  template <typename count_type = int>
  struct default_arg_workaround {
    template <typename Unit>
    static inline void applyBooleanOr(std::vector<std::pair<Unit, std::pair<Unit, int> > >& input) {
      BooleanOp<count_type, Unit> booleanOr;
      std::vector<std::pair<interval_data<Unit>, int> > container;
      std::vector<std::pair<Unit, std::pair<Unit, int> > > output;
      output.reserve(input.size());
      //consider eliminating dependecy on limits with bool flag for initial state
      Unit UnitMax = (std::numeric_limits<Unit>::max)();
      Unit prevPos = UnitMax;
      Unit prevY = UnitMax;
      int count = 0;
      for(typename std::vector<std::pair<Unit, std::pair<Unit, int> > >::iterator itr = input.begin();
          itr != input.end(); ++itr) {
        Unit pos = (*itr).first;
        Unit y = (*itr).second.first;
        if(pos != prevPos) {
          booleanOr.advanceScan();
          prevPos = pos;
          prevY = y;
          count = (*itr).second.second;
          continue;
        }
        if(y != prevY && count != 0) {
          interval_data<Unit> ivl(prevY, y);
          container.clear();
          booleanOr.processInterval(container, ivl, count_type(count));
          for(std::size_t i = 0; i < container.size(); ++i) {
            std::pair<interval_data<Unit>, int>& element = container[i];
            if(!output.empty() && output.back().first == prevPos &&
               output.back().second.first == element.first.low() &&
               output.back().second.second == element.second * -1) {
              output.pop_back();
            } else {
              output.push_back(std::pair<Unit, std::pair<Unit, int> >(prevPos, std::pair<Unit, int>(element.first.low(),
                                                                                                    element.second)));
            }
            output.push_back(std::pair<Unit, std::pair<Unit, int> >(prevPos, std::pair<Unit, int>(element.first.high(),
                                                                                                  element.second * -1)));
          }
        }
        prevY = y;
        count += (*itr).second.second;
      }
      if(output.size() < input.size() / 2) {
        input = std::vector<std::pair<Unit, std::pair<Unit, int> > >();
      } else {
      input.clear();
      }
      input.insert(input.end(), output.begin(), output.end());
    }
  };

}

}

}
#endif

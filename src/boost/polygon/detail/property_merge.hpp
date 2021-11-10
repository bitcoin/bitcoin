/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_PROPERTY_MERGE_HPP
#define BOOST_POLYGON_PROPERTY_MERGE_HPP
namespace boost { namespace polygon{

template <typename coordinate_type>
class property_merge_point {
private:
  coordinate_type x_, y_;
public:
  inline property_merge_point() : x_(), y_() {}
  inline property_merge_point(coordinate_type x, coordinate_type y) : x_(x), y_(y) {}
  //use builtin assign and copy
  inline bool operator==(const property_merge_point& that) const { return x_ == that.x_ && y_ == that.y_; }
  inline bool operator!=(const property_merge_point& that) const { return !((*this) == that); }
  inline bool operator<(const property_merge_point& that) const {
    if(x_ < that.x_) return true;
    if(x_ > that.x_) return false;
    return y_ < that.y_;
  }
  inline coordinate_type x() const { return x_; }
  inline coordinate_type y() const { return y_; }
  inline void x(coordinate_type value) { x_ = value; }
  inline void y(coordinate_type value) { y_ = value; }
};

template <typename coordinate_type>
class property_merge_interval {
private:
  coordinate_type low_, high_;
public:
  inline property_merge_interval() : low_(), high_() {}
  inline property_merge_interval(coordinate_type low, coordinate_type high) : low_(low), high_(high) {}
  //use builtin assign and copy
  inline bool operator==(const property_merge_interval& that) const { return low_ == that.low_ && high_ == that.high_; }
  inline bool operator!=(const property_merge_interval& that) const { return !((*this) == that); }
  inline bool operator<(const property_merge_interval& that) const {
    if(low_ < that.low_) return true;
    if(low_ > that.low_) return false;
    return high_ < that.high_;
  }
  inline coordinate_type low() const { return low_; }
  inline coordinate_type high() const { return high_; }
  inline void low(coordinate_type value) { low_ = value; }
  inline void high(coordinate_type value) { high_ = value; }
};

template <typename coordinate_type, typename property_type, typename polygon_set_type, typename keytype = std::set<property_type> >
class merge_scanline {
public:
  //definitions

  typedef keytype property_set;
  typedef std::vector<std::pair<property_type, int> > property_map;
  typedef std::pair<property_merge_point<coordinate_type>, std::pair<property_type, int> > vertex_property;
  typedef std::pair<property_merge_point<coordinate_type>, property_map> vertex_data;
  typedef std::vector<vertex_property> property_merge_data;
  //typedef std::map<property_set, polygon_set_type> Result;
  typedef std::map<coordinate_type, property_map> scanline_type;
  typedef typename scanline_type::iterator scanline_iterator;
  typedef std::pair<property_merge_interval<coordinate_type>, std::pair<property_set, property_set> > edge_property;
  typedef std::vector<edge_property> edge_property_vector;

  //static public member functions

  template <typename iT, typename orientation_2d_type>
  static inline void
  populate_property_merge_data(property_merge_data& pmd, iT input_begin, iT input_end,
                               const property_type& property, orientation_2d_type orient) {
    for( ; input_begin != input_end; ++input_begin) {
      std::pair<property_merge_point<coordinate_type>, std::pair<property_type, int> > element;
      if(orient == HORIZONTAL)
        element.first = property_merge_point<coordinate_type>((*input_begin).second.first, (*input_begin).first);
      else
        element.first = property_merge_point<coordinate_type>((*input_begin).first, (*input_begin).second.first);
      element.second.first = property;
      element.second.second = (*input_begin).second.second;
      pmd.push_back(element);
    }
  }

  //public member functions

  merge_scanline() : output(), scanline(), currentVertex(), tmpVector(), previousY(), countFromBelow(), scanlinePosition() {}
  merge_scanline(const merge_scanline& that) :
    output(that.output),
    scanline(that.scanline),
    currentVertex(that.currentVertex),
    tmpVector(that.tmpVector),
    previousY(that.previousY),
    countFromBelow(that.countFromBelow),
    scanlinePosition(that.scanlinePosition)
  {}
  merge_scanline& operator=(const merge_scanline& that) {
    output = that.output;
    scanline = that.scanline;
    currentVertex = that.currentVertex;
    tmpVector = that.tmpVector;
    previousY = that.previousY;
    countFromBelow = that.countFromBelow;
    scanlinePosition = that.scanlinePosition;
    return *this;
  }

  template <typename result_type>
  inline void perform_merge(result_type& result, property_merge_data& data) {
    if(data.empty()) return;
    //sort
    polygon_sort(data.begin(), data.end(), less_vertex_data<vertex_property>());
    //scanline
    bool firstIteration = true;
    scanlinePosition = scanline.end();
    for(std::size_t i = 0; i < data.size(); ++i) {
      if(firstIteration) {
        mergeProperty(currentVertex.second, data[i].second);
        currentVertex.first = data[i].first;
        firstIteration = false;
      } else {
        if(data[i].first != currentVertex.first) {
          if(data[i].first.x() != currentVertex.first.x()) {
            processVertex(output);
            //std::cout << scanline.size() << " ";
            countFromBelow.clear(); //should already be clear
            writeOutput(currentVertex.first.x(), result, output);
            currentVertex.second.clear();
            mergeProperty(currentVertex.second, data[i].second);
            currentVertex.first = data[i].first;
            //std::cout << assertRedundant(scanline) << "/" << scanline.size() << " ";
          } else {
            processVertex(output);
            currentVertex.second.clear();
            mergeProperty(currentVertex.second, data[i].second);
            currentVertex.first = data[i].first;
          }
        } else {
          mergeProperty(currentVertex.second, data[i].second);
        }
      }
    }
    processVertex(output);
    writeOutput(currentVertex.first.x(), result, output);
    //std::cout << assertRedundant(scanline) << "/" << scanline.size() << "\n";
    //std::cout << scanline.size() << "\n";
  }

private:
  //private supporting types

  template <class T>
  class less_vertex_data {
  public:
    less_vertex_data() {}
    bool operator()(const T& lvalue, const T& rvalue) const {
      if(lvalue.first.x() < rvalue.first.x()) return true;
      if(lvalue.first.x() > rvalue.first.x()) return false;
      if(lvalue.first.y() < rvalue.first.y()) return true;
      return false;
    }
  };

  template <typename T>
  struct lessPropertyCount {
    lessPropertyCount() {}
    bool operator()(const T& a, const T& b) {
      return a.first < b.first;
    }
  };

  //private static member functions

  static inline void mergeProperty(property_map& lvalue, std::pair<property_type, int>& rvalue) {
    typename property_map::iterator itr = std::lower_bound(lvalue.begin(), lvalue.end(), rvalue,
                                                          lessPropertyCount<std::pair<property_type, int> >());
    if(itr == lvalue.end() ||
       (*itr).first != rvalue.first) {
      lvalue.insert(itr, rvalue);
    } else {
      (*itr).second += rvalue.second;
      if((*itr).second == 0)
        lvalue.erase(itr);
    }
//     if(assertSorted(lvalue)) {
//       std::cout << "in mergeProperty\n";
//       exit(0);
//     }
  }

//   static inline bool assertSorted(property_map& pset) {
//     bool result = false;
//     for(std::size_t i = 1; i < pset.size(); ++i) {
//       if(pset[i] < pset[i-1]) {
//         std::cout << "Out of Order Error ";
//         result = true;
//       }
//       if(pset[i].first == pset[i-1].first) {
//         std::cout << "Duplicate Property Error ";
//         result = true;
//       }
//       if(pset[0].second == 0 || pset[1].second == 0) {
//         std::cout << "Empty Property Error ";
//         result = true;
//       }
//     }
//     return result;
//   }

  static inline void setProperty(property_set& pset, property_map& pmap) {
    for(typename property_map::iterator itr = pmap.begin(); itr != pmap.end(); ++itr) {
      if((*itr).second > 0) {
        pset.insert(pset.end(), (*itr).first);
      }
    }
  }

  //private data members

  edge_property_vector output;
  scanline_type scanline;
  vertex_data currentVertex;
  property_map tmpVector;
  coordinate_type previousY;
  property_map countFromBelow;
  scanline_iterator scanlinePosition;

  //private member functions

  inline void mergeCount(property_map& lvalue, property_map& rvalue) {
    typename property_map::iterator litr = lvalue.begin();
    typename property_map::iterator ritr = rvalue.begin();
    tmpVector.clear();
    while(litr != lvalue.end() && ritr != rvalue.end()) {
      if((*litr).first <= (*ritr).first) {
        if(!tmpVector.empty() &&
           (*litr).first == tmpVector.back().first) {
          tmpVector.back().second += (*litr).second;
        } else {
          tmpVector.push_back(*litr);
        }
        ++litr;
      } else if((*ritr).first <= (*litr).first) {
        if(!tmpVector.empty() &&
           (*ritr).first == tmpVector.back().first) {
          tmpVector.back().second += (*ritr).second;
        } else {
          tmpVector.push_back(*ritr);
        }
        ++ritr;
      }
    }
    while(litr != lvalue.end()) {
      if(!tmpVector.empty() &&
         (*litr).first == tmpVector.back().first) {
        tmpVector.back().second += (*litr).second;
      } else {
        tmpVector.push_back(*litr);
      }
      ++litr;
    }
    while(ritr != rvalue.end()) {
      if(!tmpVector.empty() &&
         (*ritr).first == tmpVector.back().first) {
        tmpVector.back().second += (*ritr).second;
      } else {
        tmpVector.push_back(*ritr);
      }
      ++ritr;
    }
    lvalue.clear();
    for(std::size_t i = 0; i < tmpVector.size(); ++i) {
      if(tmpVector[i].second != 0) {
        lvalue.push_back(tmpVector[i]);
      }
    }
//     if(assertSorted(lvalue)) {
//       std::cout << "in mergeCount\n";
//       exit(0);
//     }
  }

  inline void processVertex(edge_property_vector& output) {
    if(!countFromBelow.empty()) {
      //we are processing an interval of change in scanline state between
      //previous vertex position and current vertex position where
      //count from below represents the change on the interval
      //foreach scanline element from previous to current we
      //write the interval on the scanline that is changing
      //the old value and the new value to output
      property_merge_interval<coordinate_type> currentInterval(previousY, currentVertex.first.y());
      coordinate_type currentY = currentInterval.low();
      if(scanlinePosition == scanline.end() ||
         (*scanlinePosition).first != previousY) {
        scanlinePosition = scanline.lower_bound(previousY);
      }
      scanline_iterator previousScanlinePosition = scanlinePosition;
      ++scanlinePosition;
      while(scanlinePosition != scanline.end()) {
        coordinate_type elementY = (*scanlinePosition).first;
        if(elementY <= currentInterval.high()) {
          property_map& countOnLeft = (*previousScanlinePosition).second;
          edge_property element;
          output.push_back(element);
          output.back().first = property_merge_interval<coordinate_type>((*previousScanlinePosition).first, elementY);
          setProperty(output.back().second.first, countOnLeft);
          mergeCount(countOnLeft, countFromBelow);
          setProperty(output.back().second.second, countOnLeft);
          if(output.back().second.first == output.back().second.second) {
            output.pop_back(); //it was an internal vertical edge, not to be output
          }
          else if(output.size() > 1) {
            edge_property& secondToLast = output[output.size()-2];
            if(secondToLast.first.high() == output.back().first.low() &&
               secondToLast.second.first == output.back().second.first &&
               secondToLast.second.second == output.back().second.second) {
              //merge output onto previous output because properties are
              //identical on both sides implying an internal horizontal edge
              secondToLast.first.high(output.back().first.high());
              output.pop_back();
            }
          }
          if(previousScanlinePosition == scanline.begin()) {
            if(countOnLeft.empty()) {
              scanline.erase(previousScanlinePosition);
            }
          } else {
            scanline_iterator tmpitr = previousScanlinePosition;
            --tmpitr;
            if((*tmpitr).second == (*previousScanlinePosition).second)
              scanline.erase(previousScanlinePosition);
          }

        } else if(currentY < currentInterval.high()){
          //elementY > currentInterval.high()
          //split the interval between previous and current scanline elements
          std::pair<coordinate_type, property_map> elementScan;
          elementScan.first = currentInterval.high();
          elementScan.second = (*previousScanlinePosition).second;
          scanlinePosition = scanline.insert(scanlinePosition, elementScan);
          continue;
        } else {
          break;
        }
        previousScanlinePosition = scanlinePosition;
        currentY = previousY = elementY;
        ++scanlinePosition;
        if(scanlinePosition == scanline.end() &&
           currentY < currentInterval.high()) {
          //insert a new element for top of range
          std::pair<coordinate_type, property_map> elementScan;
          elementScan.first = currentInterval.high();
          scanlinePosition = scanline.insert(scanline.end(), elementScan);
        }
      }
      if(scanlinePosition == scanline.end() &&
         currentY < currentInterval.high()) {
        //handle case where we iterated to end of the scanline
        //we need to insert an element into the scanline at currentY
        //with property value coming from below
        //and another one at currentInterval.high() with empty property value
        mergeCount(scanline[currentY], countFromBelow);
        std::pair<coordinate_type, property_map> elementScan;
        elementScan.first = currentInterval.high();
        scanline.insert(scanline.end(), elementScan);

        edge_property element;
        output.push_back(element);
        output.back().first = property_merge_interval<coordinate_type>(currentY, currentInterval.high());
        setProperty(output.back().second.second, countFromBelow);
        mergeCount(countFromBelow, currentVertex.second);
      } else {
        mergeCount(countFromBelow, currentVertex.second);
        if(countFromBelow.empty()) {
          if(previousScanlinePosition == scanline.begin()) {
            if((*previousScanlinePosition).second.empty()) {
              scanline.erase(previousScanlinePosition);
              //previousScanlinePosition = scanline.end();
              //std::cout << "ERASE_A ";
            }
          } else {
            scanline_iterator tmpitr = previousScanlinePosition;
            --tmpitr;
            if((*tmpitr).second == (*previousScanlinePosition).second) {
              scanline.erase(previousScanlinePosition);
              //previousScanlinePosition = scanline.end();
              //std::cout << "ERASE_B ";
            }
          }
        }
      }
    } else {
      //count from below is empty, we are starting a new interval of change
      countFromBelow = currentVertex.second;
      scanlinePosition = scanline.lower_bound(currentVertex.first.y());
      if(scanlinePosition != scanline.end()) {
        if((*scanlinePosition).first != currentVertex.first.y()) {
          if(scanlinePosition != scanline.begin()) {
            //decrement to get the lower position of the first interval this vertex intersects
            --scanlinePosition;
            //insert a new element into the scanline for the incoming vertex
            property_map& countOnLeft = (*scanlinePosition).second;
            std::pair<coordinate_type, property_map> element(currentVertex.first.y(), countOnLeft);
            scanlinePosition = scanline.insert(scanlinePosition, element);
          } else {
            property_map countOnLeft;
            std::pair<coordinate_type, property_map> element(currentVertex.first.y(), countOnLeft);
            scanlinePosition = scanline.insert(scanlinePosition, element);
          }
        }
      } else {
        property_map countOnLeft;
        std::pair<coordinate_type, property_map> element(currentVertex.first.y(), countOnLeft);
        scanlinePosition = scanline.insert(scanlinePosition, element);
      }
    }
    previousY = currentVertex.first.y();
  }

  template <typename T>
  inline int assertRedundant(T& t) {
    if(t.empty()) return 0;
    int count = 0;
    typename T::iterator itr = t.begin();
    if((*itr).second.empty())
      ++count;
    typename T::iterator itr2 = itr;
    ++itr2;
    while(itr2 != t.end()) {
      if((*itr).second == (*itr2).second)
        ++count;
      itr = itr2;
      ++itr2;
    }
    return count;
  }

  template <typename T>
  inline void performExtract(T& result, property_merge_data& data) {
    if(data.empty()) return;
    //sort
    polygon_sort(data.begin(), data.end(), less_vertex_data<vertex_property>());

    //scanline
    bool firstIteration = true;
    scanlinePosition = scanline.end();
    for(std::size_t i = 0; i < data.size(); ++i) {
      if(firstIteration) {
        mergeProperty(currentVertex.second, data[i].second);
        currentVertex.first = data[i].first;
        firstIteration = false;
      } else {
        if(data[i].first != currentVertex.first) {
          if(data[i].first.x() != currentVertex.first.x()) {
            processVertex(output);
            //std::cout << scanline.size() << " ";
            countFromBelow.clear(); //should already be clear
            writeGraph(result, output, scanline);
            currentVertex.second.clear();
            mergeProperty(currentVertex.second, data[i].second);
            currentVertex.first = data[i].first;
          } else {
            processVertex(output);
            currentVertex.second.clear();
            mergeProperty(currentVertex.second, data[i].second);
            currentVertex.first = data[i].first;
          }
        } else {
          mergeProperty(currentVertex.second, data[i].second);
        }
      }
    }
    processVertex(output);
    writeGraph(result, output, scanline);
    //std::cout << scanline.size() << "\n";
  }

  template <typename T>
  inline void insertEdges(T& graph, property_set& p1, property_set& p2) {
    for(typename property_set::iterator itr = p1.begin(); itr != p1.end(); ++itr) {
      for(typename property_set::iterator itr2 = p2.begin(); itr2 != p2.end(); ++itr2) {
        if(*itr != *itr2) {
          graph[*itr].insert(*itr2);
          graph[*itr2].insert(*itr);
        }
      }
    }
  }

  template <typename T>
  inline void propertySetAbove(coordinate_type y, property_set& ps, T& scanline) {
    ps.clear();
    typename T::iterator itr = scanline.find(y);
    if(itr != scanline.end())
      setProperty(ps, (*itr).second);
  }

  template <typename T>
  inline void propertySetBelow(coordinate_type y, property_set& ps, T& scanline) {
    ps.clear();
    typename T::iterator itr = scanline.find(y);
    if(itr != scanline.begin()) {
      --itr;
      setProperty(ps, (*itr).second);
    }
  }

  template <typename T, typename T2>
  inline void writeGraph(T& graph, edge_property_vector& output, T2& scanline) {
    if(output.empty()) return;
    edge_property* previousEdgeP = &(output[0]);
    bool firstIteration = true;
    property_set ps;
    for(std::size_t i = 0; i < output.size(); ++i) {
      edge_property& previousEdge = *previousEdgeP;
      edge_property& edge = output[i];
      if(previousEdge.first.high() == edge.first.low()) {
        //horizontal edge
        insertEdges(graph, edge.second.first, previousEdge.second.first);
        //corner 1
        insertEdges(graph, edge.second.first, previousEdge.second.second);
        //other horizontal edge
        insertEdges(graph, edge.second.second, previousEdge.second.second);
        //corner 2
        insertEdges(graph, edge.second.second, previousEdge.second.first);
      } else {
        if(!firstIteration){
          //look up regions above previous edge
          propertySetAbove(previousEdge.first.high(), ps, scanline);
          insertEdges(graph, ps, previousEdge.second.first);
          insertEdges(graph, ps, previousEdge.second.second);
        }
        //look up regions below current edge in the scanline
        propertySetBelow(edge.first.high(), ps, scanline);
        insertEdges(graph, ps, edge.second.first);
        insertEdges(graph, ps, edge.second.second);
      }
      firstIteration = false;
      //vertical edge
      insertEdges(graph, edge.second.second, edge.second.first);
      //shared region to left
      insertEdges(graph, edge.second.second, edge.second.second);
      //shared region to right
      insertEdges(graph, edge.second.first, edge.second.first);
      previousEdgeP = &(output[i]);
    }
    edge_property& previousEdge = *previousEdgeP;
    propertySetAbove(previousEdge.first.high(), ps, scanline);
    insertEdges(graph, ps, previousEdge.second.first);
    insertEdges(graph, ps, previousEdge.second.second);
    output.clear();
  }

  template <typename Result>
  inline void writeOutput(coordinate_type x, Result& result, edge_property_vector& output) {
    for(std::size_t i = 0; i < output.size(); ++i) {
      edge_property& edge = output[i];
      //edge.second.first is the property set on the left of the edge
      if(!edge.second.first.empty()) {
        typename Result::iterator itr = result.find(edge.second.first);
        if(itr == result.end()) {
          std::pair<property_set, polygon_set_type> element(edge.second.first, polygon_set_type(VERTICAL));
          itr = result.insert(result.end(), element);
        }
        std::pair<interval_data<coordinate_type>, int> element2(interval_data<coordinate_type>(edge.first.low(), edge.first.high()), -1); //right edge of figure
        (*itr).second.insert(x, element2);
      }
      if(!edge.second.second.empty()) {
        //edge.second.second is the property set on the right of the edge
        typename Result::iterator itr = result.find(edge.second.second);
        if(itr == result.end()) {
          std::pair<property_set, polygon_set_type> element(edge.second.second, polygon_set_type(VERTICAL));
          itr = result.insert(result.end(), element);
        }
        std::pair<interval_data<coordinate_type>, int> element3(interval_data<coordinate_type>(edge.first.low(), edge.first.high()), 1); //left edge of figure
        (*itr).second.insert(x, element3);
      }
    }
    output.clear();
  }
};

}
}
#endif

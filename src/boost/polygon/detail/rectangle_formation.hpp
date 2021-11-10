/*
    Copyright 2008 Intel Corporation

    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_RECTANGLE_FORMATION_HPP
#define BOOST_POLYGON_RECTANGLE_FORMATION_HPP
namespace boost { namespace polygon{

namespace rectangle_formation {
  template <class T>
  class ScanLineToRects {
  public:
    typedef T rectangle_type;
    typedef typename rectangle_traits<T>::coordinate_type coordinate_type;
    typedef rectangle_data<coordinate_type> scan_rect_type;
  private:

    typedef std::set<scan_rect_type, less_rectangle_concept<scan_rect_type, scan_rect_type> > ScanData;
    ScanData scanData_;
    bool haveCurrentRect_;
    scan_rect_type currentRect_;
    orientation_2d orient_;
    typename rectangle_traits<T>::coordinate_type currentCoordinate_;
  public:
    inline ScanLineToRects() : scanData_(), haveCurrentRect_(), currentRect_(), orient_(), currentCoordinate_() {}

    inline ScanLineToRects(orientation_2d orient, rectangle_type model) :
      scanData_(orientation_2d(orient.to_int() ? VERTICAL : HORIZONTAL)),
      haveCurrentRect_(false), currentRect_(), orient_(orient), currentCoordinate_() {
      assign(currentRect_, model);
      currentCoordinate_ = (std::numeric_limits<coordinate_type>::max)();
    }

    template <typename CT>
    inline ScanLineToRects& processEdge(CT& rectangles, const interval_data<coordinate_type>& edge);

    inline ScanLineToRects& nextMajorCoordinate(coordinate_type currentCoordinate) {
      if(haveCurrentRect_) {
        scanData_.insert(scanData_.end(), currentRect_);
        haveCurrentRect_ = false;
      }
      currentCoordinate_ = currentCoordinate;
      return *this;
    }

  };

  template <class CT, class ST, class rectangle_type, typename interval_type, typename coordinate_type> inline CT&
  processEdge_(CT& rectangles, ST& scanData, const interval_type& edge,
               bool& haveCurrentRect, rectangle_type& currentRect, coordinate_type currentCoordinate, orientation_2d orient)
  {
    typedef typename CT::value_type result_type;
    bool edgeProcessed = false;
    if(!scanData.empty()) {

      //process all rectangles in the scanData that touch the edge
      typename ST::iterator dataIter = scanData.lower_bound(rectangle_type(edge, edge));
      //decrement beginIter until its low is less than edge's low
      while((dataIter == scanData.end() || (*dataIter).get(orient).get(LOW) > edge.get(LOW)) &&
            dataIter != scanData.begin())
        {
          --dataIter;
        }
      //process each rectangle until the low end of the rectangle
      //is greater than the high end of the edge
      while(dataIter != scanData.end() &&
            (*dataIter).get(orient).get(LOW) <= edge.get(HIGH))
        {
          const rectangle_type& rect = *dataIter;
          //if the rectangle data intersects the edge at all
          if(rect.get(orient).get(HIGH) >= edge.get(LOW)) {
            if(contains(rect.get(orient), edge, true)) {
              //this is a closing edge
              //we need to write out the intersecting rectangle and
              //insert between 0 and 2 rectangles into the scanData
              //write out rectangle
              rectangle_type tmpRect = rect;

              if(rect.get(orient.get_perpendicular()).get(LOW) < currentCoordinate) {
                //set the high coordinate perpedicular to slicing orientation
                //to the current coordinate of the scan event
                tmpRect.set(orient.get_perpendicular().get_direction(HIGH),
                            currentCoordinate);
                result_type result;
                assign(result, tmpRect);
                rectangles.insert(rectangles.end(), result);
              }
              //erase the rectangle from the scan data
              typename ST::iterator nextIter = dataIter;
              ++nextIter;
              scanData.erase(dataIter);
              if(tmpRect.get(orient).get(LOW) < edge.get(LOW)) {
                //insert a rectangle for the overhang of the bottom
                //of the rectangle back into scan data
                rectangle_type lowRect(tmpRect);
                lowRect.set(orient.get_perpendicular(), interval_data<coordinate_type>(currentCoordinate,
                                                                currentCoordinate));
                lowRect.set(orient.get_direction(HIGH), edge.get(LOW));
                scanData.insert(nextIter, lowRect);
              }
              if(tmpRect.get(orient).get(HIGH) > edge.get(HIGH)) {
                //insert a rectangle for the overhang of the top
                //of the rectangle back into scan data
                rectangle_type highRect(tmpRect);
                highRect.set(orient.get_perpendicular(), interval_data<coordinate_type>(currentCoordinate,
                                                                 currentCoordinate));
                highRect.set(orient.get_direction(LOW), edge.get(HIGH));
                scanData.insert(nextIter, highRect);
              }
              //we are done with this edge
              edgeProcessed = true;
              break;
            } else {
              //it must be an opening edge
              //assert that rect does not overlap the edge but only touches
              //write out rectangle
              rectangle_type tmpRect = rect;
              //set the high coordinate perpedicular to slicing orientation
              //to the current coordinate of the scan event
              if(tmpRect.get(orient.get_perpendicular().get_direction(LOW)) < currentCoordinate) {
                tmpRect.set(orient.get_perpendicular().get_direction(HIGH),
                            currentCoordinate);
                result_type result;
                assign(result, tmpRect);
                rectangles.insert(rectangles.end(), result);
              }
              //erase the rectangle from the scan data
              typename ST::iterator nextIter = dataIter;
              ++nextIter;
              scanData.erase(dataIter);
              dataIter = nextIter;
              if(haveCurrentRect) {
                if(currentRect.get(orient).get(HIGH) >= edge.get(LOW)){
                  if(!edgeProcessed && currentRect.get(orient.get_direction(HIGH)) > edge.get(LOW)){
                    rectangle_type tmpRect2(currentRect);
                    tmpRect2.set(orient.get_direction(HIGH), edge.get(LOW));
                    scanData.insert(nextIter, tmpRect2);
                    if(currentRect.get(orient.get_direction(HIGH)) > edge.get(HIGH)) {
                      currentRect.set(orient, interval_data<coordinate_type>(edge.get(HIGH), currentRect.get(orient.get_direction(HIGH))));
                    } else {
                      haveCurrentRect = false;
                    }
                  } else {
                    //extend the top of current rect
                    currentRect.set(orient.get_direction(HIGH),
                                    (std::max)(edge.get(HIGH),
                                               tmpRect.get(orient.get_direction(HIGH))));
                  }
                } else {
                  //insert current rect into the scanData
                  scanData.insert(nextIter, currentRect);
                  //create a new current rect
                  currentRect.set(orient.get_perpendicular(), interval_data<coordinate_type>(currentCoordinate,
                                                                      currentCoordinate));
                  currentRect.set(orient, interval_data<coordinate_type>((std::min)(tmpRect.get(orient).get(LOW),
                                                       edge.get(LOW)),
                                                                         (std::max)(tmpRect.get(orient).get(HIGH),
                                                       edge.get(HIGH))));
                }
              } else {
                haveCurrentRect = true;
                currentRect.set(orient.get_perpendicular(), interval_data<coordinate_type>(currentCoordinate,
                                                                    currentCoordinate));
                currentRect.set(orient, interval_data<coordinate_type>((std::min)(tmpRect.get(orient).get(LOW),
                                                     edge.get(LOW)),
                                                                       (std::max)(tmpRect.get(orient).get(HIGH),
                                                     edge.get(HIGH))));
              }
              //skip to nextIter position
              edgeProcessed = true;
              continue;
            }
            //edgeProcessed = true;
          }
          ++dataIter;
        } //end while edge intersects rectangle data

    }
    if(!edgeProcessed) {
      if(haveCurrentRect) {
        if(currentRect.get(orient.get_perpendicular().get_direction(HIGH))
           == currentCoordinate &&
           currentRect.get(orient.get_direction(HIGH)) >= edge.get(LOW))
          {
            if(currentRect.get(orient.get_direction(HIGH)) > edge.get(LOW)){
              rectangle_type tmpRect(currentRect);
              tmpRect.set(orient.get_direction(HIGH), edge.get(LOW));
              scanData.insert(scanData.end(), tmpRect);
              if(currentRect.get(orient.get_direction(HIGH)) > edge.get(HIGH)) {
                currentRect.set(orient,
                                interval_data<coordinate_type>(edge.get(HIGH),
                                         currentRect.get(orient.get_direction(HIGH))));
                return rectangles;
              } else {
                haveCurrentRect = false;
                return rectangles;
              }
            }
            //extend current rect
            currentRect.set(orient.get_direction(HIGH), edge.get(HIGH));
            return rectangles;
          }
        scanData.insert(scanData.end(), currentRect);
        haveCurrentRect = false;
      }
      rectangle_type tmpRect(currentRect);
      tmpRect.set(orient.get_perpendicular(), interval_data<coordinate_type>(currentCoordinate,
                                                      currentCoordinate));
      tmpRect.set(orient, edge);
      scanData.insert(tmpRect);
      return rectangles;
    }
    return rectangles;

  }

  template <class T>
  template <class CT>
  inline
  ScanLineToRects<T>& ScanLineToRects<T>::processEdge(CT& rectangles, const interval_data<coordinate_type>& edge)
  {
    processEdge_(rectangles, scanData_, edge, haveCurrentRect_, currentRect_, currentCoordinate_, orient_);
    return *this;
  }


} //namespace rectangle_formation

  template <typename T, typename T2>
  struct get_coordinate_type_for_rectangles {
    typedef typename polygon_traits<T>::coordinate_type type;
  };
  template <typename T>
  struct get_coordinate_type_for_rectangles<T, rectangle_concept> {
    typedef typename rectangle_traits<T>::coordinate_type type;
  };

  template <typename output_container, typename iterator_type, typename rectangle_concept>
  void form_rectangles(output_container& output, iterator_type begin, iterator_type end,
                       orientation_2d orient, rectangle_concept ) {
    typedef typename output_container::value_type rectangle_type;
    typedef typename get_coordinate_type_for_rectangles<rectangle_type, typename geometry_concept<rectangle_type>::type>::type Unit;
    rectangle_data<Unit> model;
    Unit prevPos = (std::numeric_limits<Unit>::max)();
    rectangle_formation::ScanLineToRects<rectangle_data<Unit> > scanlineToRects(orient, model);
    for(iterator_type itr = begin;
        itr != end; ++ itr) {
      Unit pos = (*itr).first;
      if(pos != prevPos) {
        scanlineToRects.nextMajorCoordinate(pos);
        prevPos = pos;
      }
      Unit lowy = (*itr).second.first;
      iterator_type tmp_itr = itr;
      ++itr;
      Unit highy = (*itr).second.first;
      scanlineToRects.processEdge(output, interval_data<Unit>(lowy, highy));
      if(std::abs((*itr).second.second) > 1) itr = tmp_itr; //next edge begins from this vertex
    }
  }
}
}
#endif

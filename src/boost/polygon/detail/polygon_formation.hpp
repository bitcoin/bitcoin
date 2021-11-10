/*
    Copyright 2008 Intel Corporation

    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
*/
#include<iostream>
#include<cassert>
#ifndef BOOST_POLYGON_POLYGON_FORMATION_HPP
#define BOOST_POLYGON_POLYGON_FORMATION_HPP
namespace boost { namespace polygon{

namespace polygon_formation {

  /*
   * End has two states, HEAD and TAIL as is represented by a bool
   */
  typedef bool End;

  /*
   * HEAD End is represented as false because it is the lesser state
   */
  const End HEAD = false;

  /*
   * TAIL End is represented by true because TAIL comes after head and 1 after 0
   */
  const End TAIL = true;

  /*
   * 2D turning direction, left and right sides (is a boolean value since it has two states.)
   */
  typedef bool Side;

  /*
   * LEFT Side is 0 because we inuitively think left to right; left < right
   */
  const Side LEFT = false;

  /*
   * RIGHT Side is 1 so that right > left
   */
  const Side RIGHT = true;

  /*
   * The PolyLine class is data storage and services for building and representing partial polygons.
   * As the polyline is added to it extends its storage to accomodate the data.
   * PolyLines can be joined head-to-head/head-to-tail when it is determined that two polylines are
   * part of the same polygon.
   * PolyLines keep state information about what orientation their incomplete head and tail geometry have,
   * which side of the polyline is solid and whether the polyline is joined head-to-head and tail-to-head.
   * PolyLines have nothing whatsoever to do with holes.
   * It may be valuable to collect a histogram of PolyLine lengths used by an algorithm on its typical data
   * sets and tune the allocation of the initial vector of coordinate data to be greater than or equal to
   * the mean, median, mode, or mean plus some number of standard deviation, or just generally large enough
   * to prevent too much unnecesary reallocations, but not too big that it wastes a lot of memory and degrades cache
   * performance.
   */
  template <typename Unit>
  class PolyLine {
  private:
    //data

    /*
     * ptdata_ a vector of coordiantes
     * if VERTICAL_HEAD, first coordiante is an X
     * else first coordinate is a Y
     */
    std::vector<Unit> ptdata_;

    /*
     * head and tail points to other polylines before and after this in a chain
     */
    PolyLine* headp_;
    PolyLine* tailp_;

    /*
     * state bitmask
     * bit zero is orientation, 0 H, 1 V
     * bit 1 is head connectivity, 0 for head, 1 for tail
     * bit 2 is tail connectivity, 0 for head, 1 for tail
     * bit 3 is solid to left of PolyLine when 1, right when 0
     */
    int state_;

  public:
    /*
     * default constructor (for preallocation)
     */
    PolyLine();

    /*
     * constructor that takes the orientation, coordiante and side to which there is solid
     */
    PolyLine(orientation_2d orient, Unit coord, Side side);

    //copy constructor
    PolyLine(const PolyLine& pline);

    //destructor
    ~PolyLine();

    //assignment operator
    PolyLine& operator=(const PolyLine& that);

    //equivalence operator
    bool operator==(const PolyLine& b) const;

    /*
     * valid PolyLine (only default constructed polylines are invalid.)
     */
    bool isValid() const;

    /*
     * Orientation of Head
     */
    orientation_2d headOrient() const;

    /*
     * returns true if first coordinate is an X value (first segment is vertical)
     */
    bool verticalHead() const;

    /*
     * returns the orientation_2d fo the tail
     */
    orientation_2d tailOrient() const;

    /*
     * returns true if last coordinate is an X value (last segment is vertical)
     */
    bool verticalTail() const;

    /*
     * retrun true if PolyLine has odd number of coordiantes
     */
    bool oddLength() const;

    /*
     * retrun the End of the other polyline that the specified end of this polyline is connected to
     */
    End endConnectivity(End end) const;

    /*
     * retrun true if the head of this polyline is connect to the tail of a polyline
     */
    bool headToTail() const;
    /*
     * retrun true if the head of this polyline is connect to the head of a polyline
     */
    bool headToHead() const;

    /*
     * retrun true if the tail of this polyline is connect to the tail of a polyline
     */
    bool tailToTail() const;
    /*
     * retrun true if the tail of this polyline is connect to the head of a polyline
     */
    bool tailToHead() const;

    /*
     * retrun the side on which there is solid for this polyline
     */
    Side solidSide() const;

    /*
     * retrun true if there is solid to the right of this polyline
     */
    bool solidToRight() const;

    /*
     * returns true if the polyline tail is not connected
     */
    bool active() const;

    /*
     * adds a coordinate value to the end of the polyline changing the tail orientation
     */
    PolyLine& pushCoordinate(Unit coord);

    /*
     * removes a coordinate value at the end of the polyline changing the tail orientation
     */
    PolyLine& popCoordinate();

    /*
     * extends the tail of the polyline to include the point, changing orientation if needed
     */
    PolyLine& pushPoint(const point_data<Unit>& point);

    /*
     * changes the last coordinate of the tail of the polyline by the amount of the delta
     */
    PolyLine& extendTail(Unit delta);

    /*
     * join thisEnd of this polyline to that polyline's end
     */
    PolyLine& joinTo(End thisEnd, PolyLine& that, End end);

    /*
     * join an end of this polyline to the tail of that polyline
     */
    PolyLine& joinToTail(PolyLine& that, End end);

    /*
     * join an end of this polyline to the head of that polyline
     */
    PolyLine& joinToHead(PolyLine& that, End end);

    /*
     * join the head of this polyline to the head of that polyline
     */
    //join this to that in the given way
    PolyLine& joinHeadToHead(PolyLine& that);

    /*
     * join the head of this polyline to the tail of that polyline
     */
    PolyLine& joinHeadToTail(PolyLine& that);

    /*
     * join the tail of this polyline to the head of that polyline
     */
    PolyLine& joinTailToHead(PolyLine& that);

    /*
     * join the tail of this polyline to the tail of that polyline
     */
    PolyLine& joinTailToTail(PolyLine& that);

    /*
     * dissconnect the tail at the end of the polygon
     */
    PolyLine& disconnectTails();

    /*
     * get the coordinate at one end of this polyline, by default the tail end
     */
    Unit getEndCoord(End end = TAIL) const;

    /*
     * get the point on the polyline at the given index (polylines have the same number of coordinates as points
     */
    point_data<Unit> getPoint(unsigned int index) const;

    /*
     * get the point on one end of the polyline, by default the tail
     */
    point_data<Unit> getEndPoint(End end = TAIL) const;

    /*
     * get the orientation of a segment by index
     */
    orientation_2d segmentOrient(unsigned int index = 0) const;

    /*
     * get a coordinate by index using the square bracket operator
     */
    Unit operator[] (unsigned int index) const;

    /*
     * get the number of segments/points/coordinates in the polyline
     */
    unsigned int numSegments() const;

    /*
     * get the pointer to the next polyline at one end of this
     */
    PolyLine* next(End end) const;

    /*
     * write out coordinates of this and all attached polylines to a single vector
     */
    PolyLine* writeOut(std::vector<Unit>& outVec, End startEnd = TAIL) const;

  private:
    //methods
    PolyLine& joinTo_(End thisEnd, PolyLine& that, End end);
  };

  //forward declaration
  template<bool orientT, typename Unit>
  class PolyLinePolygonData;

  //forward declaration
  template<bool orientT, typename Unit>
  class PolyLinePolygonWithHolesData;

  /*
   * ActiveTail represents an edge of an incomplete polygon.
   *
   * An ActiveTail object is the active tail end of a polyline object, which may (should) be the attached to
   * a chain of polyline objects through a pointer.  The ActiveTail class provides an abstraction between
   * and algorithm that builds polygons and the PolyLine data representation of incomplete polygons that are
   * being built.  It does this by providing an iterface to access the information about the last edge at the
   * tail of the PolyLine it is associated with.  To a polygon constructing algorithm, an ActiveTail is a floating
   * edge of an incomplete polygon and has an orientation and coordinate value, as well as knowing which side of
   * that edge is supposed to be solid or space.  Any incomplete polygon will have two active tails.  Active tails
   * may be joined together to merge two incomplete polygons into a larger incomplete polygon.  If two active tails
   * that are to be merged are the oppositve ends of the same incomplete polygon that indicates that the polygon
   * has been closed and is complete.  The active tail keeps a pointer to the other active tail of its incomplete
   * polygon so that it is easy to check this condition.  These pointers are updated when active tails are joined.
   * The active tail also keeps a list of pointers to active tail objects that serve as handles to closed holes.  In
   * this way a hole can be associated to another incomplete polygon, which will eventually be its enclosing shell,
   * or reassociate the hole to another incomplete polygon in the case that it become a hole itself.  Alternately,
   * the active tail may add a filiment to stitch a hole into a shell and "fracture" the hole out of the interior
   * of a polygon.  The active tail maintains a static output buffer to temporarily write polygon data to when
   * it outputs a figure so that outputting a polygon does not require the allocation of a temporary buffer.  This
   * static buffer should be destroyed whenever the program determines that it won't need it anymore and would prefer to
   * release the memory it has allocated back to the system.
   */
  template <typename Unit>
  class ActiveTail {
  private:
    //data
    PolyLine<Unit>* tailp_;
    ActiveTail *otherTailp_;
    std::list<ActiveTail*> holesList_;
    //Sum of all the polylines which constitute the active tail (including holes)//
    size_t polyLineSize_;
  public:

    inline size_t getPolyLineSize(){
       return polyLineSize_;
    }

    inline void setPolyLineSize(int delta){
       polyLineSize_ = delta;
    }

    inline void addPolyLineSize(int delta){
       polyLineSize_ += delta;
    }

    /*
     * iterator over coordinates of the figure
     */
    class iterator {
    private:
      const PolyLine<Unit>* pLine_;
      const PolyLine<Unit>* pLineEnd_;
      unsigned int index_;
      unsigned int indexEnd_;
      End startEnd_;
    public:
      inline iterator() : pLine_(), pLineEnd_(), index_(), indexEnd_(), startEnd_() {}
      inline iterator(const ActiveTail* at, bool isHole, orientation_2d orient) :
        pLine_(), pLineEnd_(), index_(), indexEnd_(), startEnd_() {
        //if it is a hole and orientation is vertical or it is not a hole and orientation is horizontal
        //we want to use this active tail, otherwise we want to use the other active tail
        startEnd_ = TAIL;
        if(!isHole ^ (orient == HORIZONTAL)) {
          //switch winding direction
          at = at->getOtherActiveTail();
        }
        //now we have the right winding direction
        //if it is horizontal we need to skip the first element
        pLine_ = at->getTail();

        if(at->getTail()->numSegments() > 0)
         index_ = at->getTail()->numSegments() - 1;

        if((at->getOrient() == HORIZONTAL) ^ (orient == HORIZONTAL)) {
          pLineEnd_ = at->getTail();
          indexEnd_ = pLineEnd_->numSegments() - 1;
          if(index_ == 0) {
            pLine_ = at->getTail()->next(HEAD);
            if(at->getTail()->endConnectivity(HEAD) == TAIL) {
              index_ = pLine_->numSegments() -1;
            } else {
              startEnd_ = HEAD;
              index_ = 0;
            }
          } else { --index_; }
        } else {
          pLineEnd_ = at->getOtherActiveTail()->getTail();
          if(pLineEnd_->numSegments() > 0)
          indexEnd_ = pLineEnd_->numSegments() - 1;
        }
        at->getTail()->joinTailToTail(*(at->getOtherActiveTail()->getTail()));
      }

      inline size_t size(void){
        size_t count = 0;
        End dir = startEnd_;
        PolyLine<Unit> const * currLine = pLine_;
        size_t ops = 0;
        while(currLine != pLineEnd_){
           ops++;
           count += currLine->numSegments();
           currLine = currLine->next(dir == HEAD ? TAIL : HEAD);
           dir = currLine->endConnectivity(dir == HEAD ? TAIL : HEAD);
        }
        count += pLineEnd_->numSegments();
        return count; //no. of vertices
      }

      //use bitwise copy and assign provided by the compiler
      inline iterator& operator++() {
        if(pLine_ == pLineEnd_ && index_ == indexEnd_) {
          pLine_ = 0;
          index_ = 0;
          return *this;
        }
        if(startEnd_ == HEAD) {
          ++index_;
          if(index_ == pLine_->numSegments()) {
            End end = pLine_->endConnectivity(TAIL);
            pLine_ = pLine_->next(TAIL);
            if(end == TAIL) {
              startEnd_ = TAIL;
              index_ = pLine_->numSegments() -1;
            } else {
              index_ = 0;
            }
          }
        } else {
          if(index_ == 0) {
            End end = pLine_->endConnectivity(HEAD);
            pLine_ = pLine_->next(HEAD);
            if(end == TAIL) {
              index_ = pLine_->numSegments() -1;
            } else {
              startEnd_ = HEAD;
              index_ = 0;
            }
          } else {
            --index_;
          }
        }
        return *this;
      }
      inline const iterator operator++(int) {
        iterator tmp(*this);
        ++(*this);
        return tmp;
      }
      inline bool operator==(const iterator& that) const {
        return pLine_ == that.pLine_ && index_ == that.index_;
      }
      inline bool operator!=(const iterator& that) const {
        return pLine_ != that.pLine_ || index_ != that.index_;
      }
      inline Unit operator*() { return (*pLine_)[index_]; }
    };

    /*
     * iterator over holes contained within the figure
     */
    typedef typename std::list<ActiveTail*>::const_iterator iteratorHoles;

    //default constructor
    ActiveTail();

    //constructor
    ActiveTail(orientation_2d orient, Unit coord, Side solidToRight, ActiveTail* otherTailp);

    //constructor
    ActiveTail(PolyLine<Unit>* active, ActiveTail* otherTailp);

    //copy constructor
    ActiveTail(const ActiveTail& that);

    //destructor
    ~ActiveTail();

    //assignment operator
    ActiveTail& operator=(const ActiveTail& that);

    //equivalence operator
    bool operator==(const ActiveTail& b) const;

    /*
     * comparison operators, ActiveTail objects are sortable by geometry
     */
    bool operator<(const ActiveTail& b) const;
    bool operator<=(const ActiveTail& b) const;
    bool operator>(const ActiveTail& b) const;
    bool operator>=(const ActiveTail& b) const;

    /*
     * get the pointer to the polyline that this is an active tail of
     */
    PolyLine<Unit>* getTail() const;

    /*
     * get the pointer to the polyline at the other end of the chain
     */
    PolyLine<Unit>* getOtherTail() const;

    /*
     * get the pointer to the activetail at the other end of the chain
     */
    ActiveTail* getOtherActiveTail() const;

    /*
     * test if another active tail is the other end of the chain
     */
    bool isOtherTail(const ActiveTail& b);

    /*
     * update this end of chain pointer to new polyline
     */
    ActiveTail& updateTail(PolyLine<Unit>* newTail);

    /*
     * associate a hole to this active tail by the specified policy
     */
    ActiveTail* addHole(ActiveTail* hole, bool fractureHoles);

    /*
     * get the list of holes
     */
    const std::list<ActiveTail*>& getHoles() const;

    /*
     * copy holes from that to this
     */
    void copyHoles(ActiveTail& that);

    /*
     * find out if solid to right
     */
    bool solidToRight() const;

    /*
     * get coordinate (getCoord and getCoordinate are aliases for eachother)
     */
    Unit getCoord() const;
    Unit getCoordinate() const;

    /*
     * get the tail orientation
     */
    orientation_2d getOrient() const;

    /*
     * add a coordinate to the polygon at this active tail end, properly handle degenerate edges by removing redundant coordinate
     */
    void pushCoordinate(Unit coord);

    /*
     * write the figure that this active tail points to out to the temp buffer
     */
    void writeOutFigure(std::vector<Unit>& outVec, bool isHole = false) const;

    /*
     * write the figure that this active tail points to out through iterators
     */
    void writeOutFigureItrs(iterator& beginOut, iterator& endOut, bool isHole = false, orientation_2d orient = VERTICAL) const;
    iterator begin(bool isHole, orientation_2d orient) const;
    iterator end() const;

    /*
     * write the holes that this active tail points to out through iterators
     */
    void writeOutFigureHoleItrs(iteratorHoles& beginOut, iteratorHoles& endOut) const;
    iteratorHoles beginHoles() const;
    iteratorHoles endHoles() const;

    /*
     * joins the two chains that the two active tail tails are ends of
     * checks for closure of figure and writes out polygons appropriately
     * returns a handle to a hole if one is closed
     */
    static ActiveTail* joinChains(ActiveTail* at1, ActiveTail* at2, bool solid, std::vector<Unit>& outBufferTmp);
    template <typename PolygonT>
    static ActiveTail* joinChains(ActiveTail* at1, ActiveTail* at2, bool solid, typename std::vector<PolygonT>& outBufferTmp);

    /*
     * deallocate temp buffer
     */
    static void destroyOutBuffer();

    /*
     * deallocate all polygon data this active tail points to (deep delete, call only from one of a pair of active tails)
     */
    void destroyContents();
  };

  /* allocate a polyline object */
  template <typename Unit>
  PolyLine<Unit>* createPolyLine(orientation_2d orient, Unit coord, Side side);

  /* deallocate a polyline object */
  template <typename Unit>
  void destroyPolyLine(PolyLine<Unit>* pLine);

  /* allocate an activetail object */
  template <typename Unit>
  ActiveTail<Unit>* createActiveTail();

  /* deallocate an activetail object */
  template <typename Unit>
  void destroyActiveTail(ActiveTail<Unit>* aTail);

  template<bool orientT, typename Unit>
  class PolyLineHoleData {
  private:
    ActiveTail<Unit>* p_;
  public:
    typedef Unit coordinate_type;
    typedef typename ActiveTail<Unit>::iterator compact_iterator_type;
    typedef iterator_compact_to_points<compact_iterator_type, point_data<coordinate_type> > iterator_type;
    inline PolyLineHoleData() : p_(0) {}
    inline PolyLineHoleData(ActiveTail<Unit>* p) : p_(p) {}
    //use default copy and assign
    inline compact_iterator_type begin_compact() const { return p_->begin(true, (orientT ? VERTICAL : HORIZONTAL)); }
    inline compact_iterator_type end_compact() const { return p_->end(); }
    inline iterator_type begin() const { return iterator_type(begin_compact(), end_compact()); }
    inline iterator_type end() const { return iterator_type(end_compact(), end_compact()); }
    inline std::size_t size() const {
       return p_->getPolyLineSize();
    }
    inline ActiveTail<Unit>* yield() { return p_; }
  };

  template<bool orientT, typename Unit>
  class PolyLinePolygonWithHolesData {
  private:
    ActiveTail<Unit>* p_;
  public:
    typedef Unit coordinate_type;
    typedef typename ActiveTail<Unit>::iterator compact_iterator_type;
    typedef iterator_compact_to_points<compact_iterator_type, point_data<coordinate_type> > iterator_type;
    typedef PolyLineHoleData<orientT, Unit> hole_type;
    typedef typename coordinate_traits<Unit>::area_type area_type;
    class iteratorHoles {
    private:
      typename ActiveTail<Unit>::iteratorHoles itr_;
    public:
      inline iteratorHoles() : itr_() {}
      inline iteratorHoles(typename ActiveTail<Unit>::iteratorHoles itr) : itr_(itr) {}
      //use bitwise copy and assign provided by the compiler
      inline iteratorHoles& operator++() {
        ++itr_;
        return *this;
      }
      inline const iteratorHoles operator++(int) {
        iteratorHoles tmp(*this);
        ++(*this);
        return tmp;
      }
      inline bool operator==(const iteratorHoles& that) const {
        return itr_ == that.itr_;
      }
      inline bool operator!=(const iteratorHoles& that) const {
        return itr_ != that.itr_;
      }
      inline PolyLineHoleData<orientT, Unit> operator*() { return PolyLineHoleData<orientT, Unit>(*itr_);}
    };
    typedef iteratorHoles iterator_holes_type;

    inline PolyLinePolygonWithHolesData() : p_(0) {}
    inline PolyLinePolygonWithHolesData(ActiveTail<Unit>* p) : p_(p) {}
    //use default copy and assign
    inline compact_iterator_type begin_compact() const { return p_->begin(false, (orientT ? VERTICAL : HORIZONTAL)); }
    inline compact_iterator_type end_compact() const { return p_->end(); }
    inline iterator_type begin() const { return iterator_type(begin_compact(), end_compact()); }
    inline iterator_type end() const { return iterator_type(end_compact(), end_compact()); }
    inline iteratorHoles begin_holes() const { return iteratorHoles(p_->beginHoles()); }
    inline iteratorHoles end_holes() const { return iteratorHoles(p_->endHoles()); }
    inline ActiveTail<Unit>* yield() { return p_; }
    //stub out these four required functions that will not be used but are needed for the interface
    inline std::size_t size_holes() const { return 0; }
    inline std::size_t size() const { return 0; }
  };


  template <bool orientT, typename Unit, typename polygon_concept_type>
  struct PolyLineType { };
  template <bool orientT, typename Unit>
  struct PolyLineType<orientT, Unit, polygon_90_with_holes_concept> { typedef PolyLinePolygonWithHolesData<orientT, Unit> type; };
  template <bool orientT, typename Unit>
  struct PolyLineType<orientT, Unit, polygon_45_with_holes_concept> { typedef PolyLinePolygonWithHolesData<orientT, Unit> type; };
  template <bool orientT, typename Unit>
  struct PolyLineType<orientT, Unit, polygon_with_holes_concept> { typedef PolyLinePolygonWithHolesData<orientT, Unit> type; };
  template <bool orientT, typename Unit>
  struct PolyLineType<orientT, Unit, polygon_90_concept> { typedef PolyLineHoleData<orientT, Unit> type; };
  template <bool orientT, typename Unit>
  struct PolyLineType<orientT, Unit, polygon_45_concept> { typedef PolyLineHoleData<orientT, Unit> type; };
  template <bool orientT, typename Unit>
  struct PolyLineType<orientT, Unit, polygon_concept> { typedef PolyLineHoleData<orientT, Unit> type; };

  template <bool orientT, typename Unit, typename polygon_concept_type>
  class ScanLineToPolygonItrs {
  private:
    std::map<Unit, ActiveTail<Unit>*> tailMap_;
    typedef typename PolyLineType<orientT, Unit, polygon_concept_type>::type PolyLinePolygonData;
    std::vector<PolyLinePolygonData> outputPolygons_;
    bool fractureHoles_;
  public:
    typedef typename std::vector<PolyLinePolygonData>::iterator iterator;
    inline ScanLineToPolygonItrs() : tailMap_(), outputPolygons_(), fractureHoles_(false)  {}
    /* construct a scanline with the proper offsets, protocol and options */
    inline ScanLineToPolygonItrs(bool fractureHoles) : tailMap_(), outputPolygons_(), fractureHoles_(fractureHoles) {}

    ~ScanLineToPolygonItrs() { clearOutput_(); }

    /* process all vertical edges, left and right, at a unique x coordinate, edges must be sorted low to high */
    void processEdges(iterator& beginOutput, iterator& endOutput,
                      Unit currentX, std::vector<interval_data<Unit> >& leftEdges,
                      std::vector<interval_data<Unit> >& rightEdges,
                      size_t vertexThreshold=(std::numeric_limits<size_t>::max)() );

   /**********************************************************************
    *methods implementing new polygon formation code
    *
    **********************************************************************/
    void updatePartialSimplePolygonsWithRightEdges(Unit currentX,
         const std::vector<interval_data<Unit> >& leftEdges, size_t threshold);

    void updatePartialSimplePolygonsWithLeftEdges(Unit currentX,
         const std::vector<interval_data<Unit> >& leftEdges, size_t threshold);

    void closePartialSimplePolygon(Unit, ActiveTail<Unit>*, ActiveTail<Unit>*);

    void maintainPartialSimplePolygonInvariant(iterator& ,iterator& ,Unit,
         const std::vector<interval_data<Unit> >&,
         const std::vector<interval_data<Unit> >&,
         size_t vertexThreshold=(std::numeric_limits<size_t>::max)());

    void insertNewLeftEdgeIntoTailMap(Unit, Unit, Unit,
      typename std::map<Unit, ActiveTail<Unit>*>::iterator &);
    /**********************************************************************/

    inline size_t getTailMapSize(){
       typename std::map<Unit, ActiveTail<Unit>* >::const_iterator itr;
       size_t tsize = 0;
       for(itr=tailMap_.begin(); itr!=tailMap_.end(); ++itr){
          tsize +=  (itr->second)->getPolyLineSize();
       }
       return tsize;
    }
   /*print the active tails in this map:*/
   inline void print(){
      typename std::map<Unit, ActiveTail<Unit>* >::const_iterator itr;
      printf("=========TailMap[%lu]=========\n", tailMap_.size());
      for(itr=tailMap_.begin(); itr!=tailMap_.end(); ++itr){
         std::cout<< "[" << itr->first << "] : " << std::endl;
         //print active tail//
         ActiveTail<Unit> const *t = (itr->second);
         PolyLine<Unit> const *pBegin = t->getTail();
         PolyLine<Unit> const *pEnd = t->getOtherActiveTail()->getTail();
         std::string sorient = pBegin->solidToRight() ? "RIGHT" : "LEFT";
         std::cout<< " ActiveTail.tailp_ (solid= " << sorient ;
         End dir = TAIL;
         while(pBegin!=pEnd){
            std::cout << pBegin  << "={ ";
            for(size_t i=0; i<pBegin->numSegments(); i++){
               point_data<Unit> u = pBegin->getPoint(i);
               std::cout << "(" << u.x() << "," << u.y() << ") ";
            }
            std::cout << "}  ";
            pBegin = pBegin->next(dir == HEAD ? TAIL : HEAD);
            dir = pBegin->endConnectivity(dir == HEAD ? TAIL : HEAD);
         }
         if(pEnd){
            std::cout << pEnd << "={ ";
            for(size_t i=0; i<pEnd->numSegments(); i++){
               point_data<Unit> u = pEnd->getPoint(i);
               std::cout << "(" << u.x() << "," << u.y() << ") ";
            }
            std::cout << "}  ";
         }
         std::cout << " end= " << pEnd << std::endl;
      }
   }

  private:
    void clearOutput_();
  };

  /*
   * ScanLine does all the work of stitching together polygons from incoming vertical edges
   */
//   template <typename Unit, typename polygon_concept_type>
//   class ScanLineToPolygons {
//   private:
//     ScanLineToPolygonItrs<true, Unit> scanline_;
//   public:
//     inline ScanLineToPolygons() : scanline_() {}
//     /* construct a scanline with the proper offsets, protocol and options */
//     inline ScanLineToPolygons(bool fractureHoles) : scanline_(fractureHoles) {}

//     /* process all vertical edges, left and right, at a unique x coordinate, edges must be sorted low to high */
//     inline void processEdges(std::vector<Unit>& outBufferTmp, Unit currentX, std::vector<interval_data<Unit> >& leftEdges,
//                              std::vector<interval_data<Unit> >& rightEdges) {
//       typename ScanLineToPolygonItrs<true, Unit>::iterator itr, endItr;
//       scanline_.processEdges(itr, endItr, currentX, leftEdges, rightEdges);
//       //copy data into outBufferTmp
//       while(itr != endItr) {
//         typename PolyLinePolygonData<true, Unit>::iterator pditr;
//         outBufferTmp.push_back(0);
//         unsigned int sizeIndex = outBufferTmp.size() - 1;
//         int count = 0;
//         for(pditr = (*itr).begin(); pditr != (*itr).end(); ++pditr) {
//           outBufferTmp.push_back(*pditr);
//           ++count;
//         }
//         outBufferTmp[sizeIndex] = count;
//         typename PolyLinePolygonData<true, Unit>::iteratorHoles pdHoleItr;
//         for(pdHoleItr = (*itr).beginHoles(); pdHoleItr != (*itr).endHoles(); ++pdHoleItr) {
//           outBufferTmp.push_back(0);
//           unsigned int sizeIndex2 = outBufferTmp.size() - 1;
//           int count2 = 0;
//           for(pditr = (*pdHoleItr).begin(); pditr != (*pdHoleItr).end(); ++pditr) {
//             outBufferTmp.push_back(*pditr);
//             ++count2;
//           }
//           outBufferTmp[sizeIndex2] = -count;
//         }
//         ++itr;
//       }
//     }
//   };

  const int VERTICAL_HEAD = 1, HEAD_TO_TAIL = 2, TAIL_TO_TAIL = 4, SOLID_TO_RIGHT = 8;

  //EVERY FUNCTION in this DEF file should be explicitly defined as inline

  //microsoft compiler improperly warns whenever you cast an integer to bool
  //call this function on an integer to convert it to bool without a warning
  template <class T>
  inline bool to_bool(const T& val) { return val != 0; }

  //default constructor (for preallocation)
  template <typename Unit>
  inline PolyLine<Unit>::PolyLine() : ptdata_() ,headp_(0), tailp_(0), state_(-1) {}

  //constructor
  template <typename Unit>
  inline PolyLine<Unit>::PolyLine(orientation_2d orient, Unit coord, Side side) :
    ptdata_(1, coord),
    headp_(0),
    tailp_(0),
    state_(orient.to_int() +
           (side << 3)){}

  //copy constructor
  template <typename Unit>
  inline PolyLine<Unit>::PolyLine(const PolyLine<Unit>& pline) : ptdata_(pline.ptdata_),
                                                     headp_(pline.headp_),
                                                     tailp_(pline.tailp_),
                                                     state_(pline.state_) {}

  //destructor
  template <typename Unit>
  inline PolyLine<Unit>::~PolyLine() {
    //clear out data just in case it is read later
    headp_ = tailp_ = 0;
    state_ = 0;
  }

  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::operator=(const PolyLine<Unit>& that) {
    if(!(this == &that)) {
      headp_ = that.headp_;
      tailp_ = that.tailp_;
      ptdata_ = that.ptdata_;
      state_ = that.state_;
    }
    return *this;
  }

  template <typename Unit>
  inline bool PolyLine<Unit>::operator==(const PolyLine<Unit>& b) const {
    return this == &b || (state_ == b.state_ &&
                          headp_ == b.headp_ &&
                          tailp_ == b.tailp_);
  }

  //valid PolyLine
  template <typename Unit>
  inline bool PolyLine<Unit>::isValid() const {
    return state_ > -1; }

  //first coordinate is an X value
  //first segment is vertical
  template <typename Unit>
  inline bool PolyLine<Unit>::verticalHead() const {
    return state_ & VERTICAL_HEAD;
  }

  //retrun true is PolyLine has odd number of coordiantes
  template <typename Unit>
  inline bool PolyLine<Unit>::oddLength() const {
    return to_bool((ptdata_.size()-1) % 2);
  }

  //last coordiante is an X value
  //last segment is vertical
  template <typename Unit>
  inline bool PolyLine<Unit>::verticalTail() const {
    return to_bool(verticalHead() ^ oddLength());
  }

  template <typename Unit>
  inline orientation_2d PolyLine<Unit>::tailOrient() const {
    return (verticalTail() ? VERTICAL : HORIZONTAL);
  }

  template <typename Unit>
  inline orientation_2d PolyLine<Unit>::headOrient() const {
    return (verticalHead() ? VERTICAL : HORIZONTAL);
  }

  template <typename Unit>
  inline End PolyLine<Unit>::endConnectivity(End end) const {
    //Tail should be defined as true
    if(end) { return tailToTail(); }
    return headToTail();
  }

  template <typename Unit>
  inline bool PolyLine<Unit>::headToTail() const {
    return to_bool(state_ & HEAD_TO_TAIL);
  }

  template <typename Unit>
  inline bool PolyLine<Unit>::headToHead() const {
    return to_bool(!headToTail());
  }

  template <typename Unit>
  inline bool PolyLine<Unit>::tailToHead() const {
    return to_bool(!tailToTail());
  }

  template <typename Unit>
  inline bool PolyLine<Unit>::tailToTail() const {
    return to_bool(state_ & TAIL_TO_TAIL);
  }

  template <typename Unit>
  inline Side PolyLine<Unit>::solidSide() const {
    return solidToRight(); }

  template <typename Unit>
  inline bool PolyLine<Unit>::solidToRight() const {
    return to_bool(state_ & SOLID_TO_RIGHT) != 0;
  }

  template <typename Unit>
  inline bool PolyLine<Unit>::active() const {
    return !to_bool(tailp_);
  }

  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::pushCoordinate(Unit coord) {
    ptdata_.push_back(coord);
    return *this;
  }

  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::popCoordinate() {
    ptdata_.pop_back();
    return *this;
  }

  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::pushPoint(const point_data<Unit>& point) {
     if(numSegments()){
       point_data<Unit> endPt = getEndPoint();
       //vertical is true, horizontal is false
       if((tailOrient().to_int() ? point.get(VERTICAL) == endPt.get(VERTICAL) : point.get(HORIZONTAL) == endPt.get(HORIZONTAL))) {
         //we were pushing a colinear segment
         return popCoordinate();
       }
     }
    return pushCoordinate(tailOrient().to_int() ? point.get(VERTICAL) : point.get(HORIZONTAL));
  }

  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::extendTail(Unit delta) {
    ptdata_.back() += delta;
    return *this;
  }

  //private member function that creates a link from this PolyLine to that
  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::joinTo_(End thisEnd, PolyLine<Unit>& that, End end) {
    if(thisEnd){
      tailp_ = &that;
      state_ &= ~TAIL_TO_TAIL; //clear any previous state_ of bit (for safety)
      state_ |= (end << 2); //place bit into mask
    } else {
      headp_ = &that;
      state_ &= ~HEAD_TO_TAIL; //clear any previous state_ of bit (for safety)
      state_ |= (end << 1); //place bit into mask
    }
    return *this;
  }

  //join two PolyLines (both ways of the association)
  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::joinTo(End thisEnd, PolyLine<Unit>& that, End end) {
    joinTo_(thisEnd, that, end);
    that.joinTo_(end, *this, thisEnd);
    return *this;
  }

  //convenience functions for joining PolyLines
  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::joinToTail(PolyLine<Unit>& that, End end) {
    return joinTo(TAIL, that, end);
  }
  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::joinToHead(PolyLine<Unit>& that, End end) {
    return joinTo(HEAD, that, end);
  }
  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::joinHeadToHead(PolyLine<Unit>& that) {
    return joinToHead(that, HEAD);
  }
  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::joinHeadToTail(PolyLine<Unit>& that) {
    return joinToHead(that, TAIL);
  }
  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::joinTailToHead(PolyLine<Unit>& that) {
    return joinToTail(that, HEAD);
  }
  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::joinTailToTail(PolyLine<Unit>& that) {
    return joinToTail(that, TAIL);
  }

  template <typename Unit>
  inline PolyLine<Unit>& PolyLine<Unit>::disconnectTails() {
    next(TAIL)->state_ &= !TAIL_TO_TAIL;
    next(TAIL)->tailp_ = 0;
    state_ &= !TAIL_TO_TAIL;
    tailp_ = 0;
    return *this;
  }

  template <typename Unit>
  inline Unit PolyLine<Unit>::getEndCoord(End end) const {
    if(end)
      return ptdata_.back();
    return ptdata_.front();
  }

  template <typename Unit>
  inline orientation_2d PolyLine<Unit>::segmentOrient(unsigned int index) const {
    return (to_bool((unsigned int)verticalHead() ^ (index % 2)) ? VERTICAL : HORIZONTAL);
  }

  template <typename Unit>
  inline point_data<Unit> PolyLine<Unit>::getPoint(unsigned int index) const {
    //assert(isValid() && headp_->isValid()) ("PolyLine: headp_ must be valid");
    point_data<Unit> pt;
    pt.set(HORIZONTAL, ptdata_[index]);
    pt.set(VERTICAL, ptdata_[index]);
    Unit prevCoord;
    if(index == 0) {
      prevCoord = headp_->getEndCoord(headToTail());
    } else {
      prevCoord = ptdata_[index-1];
    }
    pt.set(segmentOrient(index), prevCoord);
    return pt;
  }

  template <typename Unit>
  inline point_data<Unit> PolyLine<Unit>::getEndPoint(End end) const {
    return getPoint((end ? numSegments() - 1 : (unsigned int)0));
  }

  template <typename Unit>
  inline Unit PolyLine<Unit>::operator[] (unsigned int index) const {
    //assert(ptdata_.size() > index) ("PolyLine: out of bounds index");
    return ptdata_[index];
  }

  template <typename Unit>
  inline unsigned int PolyLine<Unit>::numSegments() const {
    return ptdata_.size();
  }

  template <typename Unit>
  inline PolyLine<Unit>* PolyLine<Unit>::next(End end) const {
    return (end ? tailp_ : headp_);
  }

  template <typename Unit>
  inline ActiveTail<Unit>::ActiveTail() : tailp_(0), otherTailp_(0), holesList_(),
   polyLineSize_(0) {}

  template <typename Unit>
  inline ActiveTail<Unit>::ActiveTail(orientation_2d orient, Unit coord, Side solidToRight, ActiveTail* otherTailp) :
    tailp_(0), otherTailp_(0), holesList_(), polyLineSize_(0) {
    tailp_ = createPolyLine(orient, coord, solidToRight);
    otherTailp_ = otherTailp;
    polyLineSize_ = tailp_->numSegments();
  }

  template <typename Unit>
  inline ActiveTail<Unit>::ActiveTail(PolyLine<Unit>* active, ActiveTail<Unit>* otherTailp) :
    tailp_(active), otherTailp_(otherTailp), holesList_(),
      polyLineSize_(0) {}

  //copy constructor
  template <typename Unit>
  inline ActiveTail<Unit>::ActiveTail(const ActiveTail<Unit>& that) : tailp_(that.tailp_), otherTailp_(that.otherTailp_), holesList_(), polyLineSize_(that.polyLineSize_) {}

  //destructor
  template <typename Unit>
  inline ActiveTail<Unit>::~ActiveTail() {
    //clear them in case the memory is read later
    tailp_ = 0; otherTailp_ = 0;
  }

  template <typename Unit>
  inline ActiveTail<Unit>& ActiveTail<Unit>::operator=(const ActiveTail<Unit>& that) {
    //self assignment is safe in this case
    tailp_ = that.tailp_;
    otherTailp_ = that.otherTailp_;
    polyLineSize_ = that.polyLineSize_;
    return *this;
  }

  template <typename Unit>
  inline bool ActiveTail<Unit>::operator==(const ActiveTail<Unit>& b) const {
    return tailp_ == b.tailp_ && otherTailp_ == b.otherTailp_;
  }

  template <typename Unit>
  inline bool ActiveTail<Unit>::operator<(const ActiveTail<Unit>& b) const {
    return tailp_->getEndPoint().get(VERTICAL) < b.tailp_->getEndPoint().get(VERTICAL);
  }

  template <typename Unit>
  inline bool ActiveTail<Unit>::operator<=(const ActiveTail<Unit>& b) const {
    return !(*this > b); }

  template <typename Unit>
  inline bool ActiveTail<Unit>::operator>(const ActiveTail<Unit>& b) const {
    return b < (*this); }

  template <typename Unit>
  inline bool ActiveTail<Unit>::operator>=(const ActiveTail<Unit>& b) const {
    return !(*this < b); }

  template <typename Unit>
  inline PolyLine<Unit>* ActiveTail<Unit>::getTail() const {
    return tailp_; }

  template <typename Unit>
  inline PolyLine<Unit>* ActiveTail<Unit>::getOtherTail() const {
    return otherTailp_->tailp_; }

  template <typename Unit>
  inline ActiveTail<Unit>* ActiveTail<Unit>::getOtherActiveTail() const {
    return otherTailp_; }

  template <typename Unit>
  inline bool ActiveTail<Unit>::isOtherTail(const ActiveTail<Unit>& b) {
    //       assert( (tailp_ == b.getOtherTail() && getOtherTail() == b.tailp_) ||
    //                     (tailp_ != b.getOtherTail() && getOtherTail() != b.tailp_))
    //         ("ActiveTail: Active tails out of sync");
    return otherTailp_ == &b;
  }

  template <typename Unit>
  inline ActiveTail<Unit>& ActiveTail<Unit>::updateTail(PolyLine<Unit>* newTail) {
    //subtract the old size and add new size//
    int delta = newTail->numSegments() - tailp_->numSegments();
    addPolyLineSize(delta);
    otherTailp_->addPolyLineSize(delta);
    tailp_ = newTail;
    return *this;
  }

  template <typename Unit>
  inline ActiveTail<Unit>* ActiveTail<Unit>::addHole(ActiveTail<Unit>* hole, bool fractureHoles) {

    if(!fractureHoles){
      holesList_.push_back(hole);
      copyHoles(*hole);
      copyHoles(*(hole->getOtherActiveTail()));
      return this;
    }
    ActiveTail<Unit>* h, *v;
    ActiveTail<Unit>* other = hole->getOtherActiveTail();
    if(other->getOrient() == VERTICAL) {
      //assert that hole.getOrient() == HORIZONTAL
      //this case should never happen
      h = hole;
      v = other;
    } else {
      //assert that hole.getOrient() == VERTICAL
      h = other;
      v = hole;
    }
    h->pushCoordinate(v->getCoordinate());
    //assert that h->getOrient() == VERTICAL
    //v->pushCoordinate(getCoordinate());
    //assert that v->getOrient() == VERTICAL
    //I can't close a figure by adding a hole, so pass zero for xMin and yMin
    std::vector<Unit> tmpVec;
    ActiveTail<Unit>::joinChains(this, h, false, tmpVec);
    return v;
  }

  template <typename Unit>
  inline const std::list<ActiveTail<Unit>*>& ActiveTail<Unit>::getHoles() const {
    return holesList_;
  }

  template <typename Unit>
  inline void ActiveTail<Unit>::copyHoles(ActiveTail<Unit>& that) {
    holesList_.splice(holesList_.end(), that.holesList_); //splice the two lists together
  }

  template <typename Unit>
  inline bool ActiveTail<Unit>::solidToRight() const {
    return getTail()->solidToRight(); }

  template <typename Unit>
  inline Unit ActiveTail<Unit>::getCoord() const {
    return getTail()->getEndCoord(); }

  template <typename Unit>
  inline Unit ActiveTail<Unit>::getCoordinate() const {
    return getCoord(); }

  template <typename Unit>
  inline orientation_2d ActiveTail<Unit>::getOrient() const {
    return getTail()->tailOrient(); }

  template <typename Unit>
  inline void ActiveTail<Unit>::pushCoordinate(Unit coord) {
    //appropriately handle any co-linear polyline segments by calling push point internally
    point_data<Unit> p;
    p.set(HORIZONTAL, coord);
    p.set(VERTICAL, coord);
    //if we are vertical assign the last coordinate (an X) to p.x, else to p.y
    p.set(getOrient().get_perpendicular(), getCoordinate());
    int oldSegments = tailp_->numSegments();
    tailp_->pushPoint(p);
    int delta = tailp_->numSegments() - oldSegments;
    addPolyLineSize(delta);
    otherTailp_->addPolyLineSize(delta);
  }


  //global utility functions
  template <typename Unit>
  inline PolyLine<Unit>* createPolyLine(orientation_2d orient, Unit coord, Side side) {
    return new PolyLine<Unit>(orient, coord, side);
  }

  template <typename Unit>
  inline void destroyPolyLine(PolyLine<Unit>* pLine) {
    delete pLine;
  }

  template <typename Unit>
  inline ActiveTail<Unit>* createActiveTail() {
    //consider replacing system allocator with ActiveTail memory pool
    return new ActiveTail<Unit>();
  }

  template <typename Unit>
  inline void destroyActiveTail(ActiveTail<Unit>* aTail) {
    delete aTail;
  }


  //no recursion, to prevent max recursion depth errors
  template <typename Unit>
  inline void ActiveTail<Unit>::destroyContents() {
    tailp_->disconnectTails();
    PolyLine<Unit>* nextPolyLinep = tailp_->next(HEAD);
    End end = tailp_->endConnectivity(HEAD);
    destroyPolyLine(tailp_);
    while(nextPolyLinep) {
      End nextEnd = nextPolyLinep->endConnectivity(!end); //get the direction of next polyLine
      PolyLine<Unit>* nextNextPolyLinep = nextPolyLinep->next(!end); //get the next polyline
      destroyPolyLine(nextPolyLinep); //destroy the current polyline
      end = nextEnd;
      nextPolyLinep = nextNextPolyLinep;
    }
  }

  template <typename Unit>
  inline typename ActiveTail<Unit>::iterator ActiveTail<Unit>::begin(bool isHole, orientation_2d orient) const {
    return iterator(this, isHole, orient);
  }

  template <typename Unit>
  inline typename ActiveTail<Unit>::iterator ActiveTail<Unit>::end() const {
    return iterator();
  }

  template <typename Unit>
  inline typename ActiveTail<Unit>::iteratorHoles ActiveTail<Unit>::beginHoles() const {
    return holesList_.begin();
  }

  template <typename Unit>
  inline typename ActiveTail<Unit>::iteratorHoles ActiveTail<Unit>::endHoles() const {
    return holesList_.end();
  }

  template <typename Unit>
  inline void ActiveTail<Unit>::writeOutFigureItrs(iterator& beginOut, iterator& endOut, bool isHole, orientation_2d orient) const {
    beginOut = begin(isHole, orient);
    endOut = end();
  }

  template <typename Unit>
  inline void ActiveTail<Unit>::writeOutFigureHoleItrs(iteratorHoles& beginOut, iteratorHoles& endOut) const {
    beginOut = beginHoles();
    endOut = endHoles();
  }

  template <typename Unit>
  inline void ActiveTail<Unit>::writeOutFigure(std::vector<Unit>& outVec, bool isHole) const {
    //we start writing out the polyLine that this active tail points to at its tail
    std::size_t size = outVec.size();
    outVec.push_back(0); //place holder for size
    PolyLine<Unit>* nextPolyLinep = 0;
    if(!isHole){
      nextPolyLinep = otherTailp_->tailp_->writeOut(outVec);
    } else {
      nextPolyLinep = tailp_->writeOut(outVec);
    }
    Unit firsty = outVec[size + 1];
    if((getOrient() == HORIZONTAL) ^ !isHole) {
      //our first coordinate is a y value, so we need to rotate it to the end
      typename std::vector<Unit>::iterator tmpItr = outVec.begin();
      tmpItr += size;
      outVec.erase(++tmpItr); //erase the 2nd element
    }
    End startEnd = tailp_->endConnectivity(HEAD);
    if(isHole) startEnd = otherTailp_->tailp_->endConnectivity(HEAD);
    while(nextPolyLinep) {
      bool nextStartEnd = nextPolyLinep->endConnectivity(!startEnd);
      nextPolyLinep = nextPolyLinep->writeOut(outVec, startEnd);
      startEnd = nextStartEnd;
    }
    if((getOrient() == HORIZONTAL) ^ !isHole) {
      //we want to push the y value onto the end since we ought to have ended with an x
      outVec.push_back(firsty); //should never be executed because we want first value to be an x
    }
    //the vector contains the coordinates of the linked list of PolyLines in the correct order
    //first element is supposed to be the size
    outVec[size] = outVec.size() - 1 - size;  //number of coordinates in vector
    //assert outVec[size] % 2 == 0 //it should be even
    //make the size negative for holes
    outVec[size] *= (isHole ? -1 : 1);
  }

  //no recursion to prevent max recursion depth errors
  template <typename Unit>
  inline PolyLine<Unit>* PolyLine<Unit>::writeOut(std::vector<Unit>& outVec, End startEnd) const {
    if(startEnd == HEAD){
      //forward order
      outVec.insert(outVec.end(), ptdata_.begin(), ptdata_.end());
      return tailp_;
    }else{
      //reverse order
      //do not reserve because we expect outVec to be large enough already
      for(int i = ptdata_.size() - 1; i >= 0; --i){
        outVec.push_back(ptdata_[i]);
      }
      //NT didn't know about this version of the API....
      //outVec.insert(outVec.end(), ptdata_.rbegin(), ptdata_.rend());
      return headp_;
    }
  }

  //solid indicates if it was joined by a solit or a space
  template <typename Unit>
  inline ActiveTail<Unit>* ActiveTail<Unit>::joinChains(ActiveTail<Unit>* at1, ActiveTail<Unit>* at2, bool solid, std::vector<Unit>& outBufferTmp)
  {
    //checks to see if we closed a figure
    if(at1->isOtherTail(*at2)){
      //value of solid tells us if we closed solid or hole
      //and output the solid or handle the hole appropriately
      //if the hole needs to fracture across horizontal partition boundary we need to notify
      //the calling context to do so
      if(solid) {
        //the chains are being joined because there is solid to the right
        //this means that if the figure is closed at this point it must be a hole
        //because otherwise it would have to have another vertex to the right of this one
        //and would not be closed at this point
        return at1;
      } else {
        //assert pG != 0
        //the figure that was closed is a shell
        at1->writeOutFigure(outBufferTmp);
        //process holes of the polygon
        at1->copyHoles(*at2); //there should not be holes on at2, but if there are, copy them over
        const std::list<ActiveTail<Unit>*>& holes = at1->getHoles();
        for(typename std::list<ActiveTail<Unit>*>::const_iterator litr = holes.begin(); litr != holes.end(); ++litr) {
          (*litr)->writeOutFigure(outBufferTmp, true);
          //delete the hole
          (*litr)->destroyContents();
          destroyActiveTail((*litr)->getOtherActiveTail());
          destroyActiveTail((*litr));
        }
        //delete the polygon
        at1->destroyContents();
        //at2 contents are the same as at1, so it should not destroy them
        destroyActiveTail(at1);
        destroyActiveTail(at2);
      }
      return 0;
    }
    //join the two partial polygons into one large partial polygon
    at1->getTail()->joinTailToTail(*(at2->getTail()));
    *(at1->getOtherActiveTail()) = ActiveTail(at1->getOtherTail(), at2->getOtherActiveTail());
    *(at2->getOtherActiveTail()) = ActiveTail(at2->getOtherTail(), at1->getOtherActiveTail());

    int accumulate = at2->getPolyLineSize() + at1->getPolyLineSize();
    (at1->getOtherActiveTail())->setPolyLineSize(accumulate);
    (at2->getOtherActiveTail())->setPolyLineSize(accumulate);

    at1->getOtherActiveTail()->copyHoles(*at1);
    at1->getOtherActiveTail()->copyHoles(*at2);
    destroyActiveTail(at1);
    destroyActiveTail(at2);
    return 0;
  }

  //solid indicates if it was joined by a solit or a space
  template <typename Unit>
  template <typename PolygonT>
  inline ActiveTail<Unit>* ActiveTail<Unit>::joinChains(ActiveTail<Unit>* at1, ActiveTail<Unit>* at2, bool solid,
                                                        std::vector<PolygonT>& outBufferTmp) {
    //checks to see if we closed a figure
    if(at1->isOtherTail(*at2)){
      //value of solid tells us if we closed solid or hole
      //and output the solid or handle the hole appropriately
      //if the hole needs to fracture across horizontal partition boundary we need to notify
      //the calling context to do so
      if(solid) {
        //the chains are being joined because there is solid to the right
        //this means that if the figure is closed at this point it must be a hole
        //because otherwise it would have to have another vertex to the right of this one
        //and would not be closed at this point
        return at1;
      } else {
        //assert pG != 0
        //the figure that was closed is a shell
        outBufferTmp.push_back(at1);
        at1->copyHoles(*at2); //there should not be holes on at2, but if there are, copy them over
      }
      return 0;
    }
    //join the two partial polygons into one large partial polygon
    at1->getTail()->joinTailToTail(*(at2->getTail()));
    *(at1->getOtherActiveTail()) = ActiveTail<Unit>(at1->getOtherTail(), at2->getOtherActiveTail());
    *(at2->getOtherActiveTail()) = ActiveTail<Unit>(at2->getOtherTail(), at1->getOtherActiveTail());

    int accumulate = at2->getPolyLineSize() + at1->getPolyLineSize();
    (at1->getOtherActiveTail())->setPolyLineSize(accumulate);
    (at2->getOtherActiveTail())->setPolyLineSize(accumulate);

    at1->getOtherActiveTail()->copyHoles(*at1);
    at1->getOtherActiveTail()->copyHoles(*at2);
    destroyActiveTail(at1);
    destroyActiveTail(at2);
    return 0;
  }

  template <class TKey, class T> inline typename std::map<TKey, T>::iterator findAtNext(std::map<TKey, T>& theMap,
                                                                                        typename std::map<TKey, T>::iterator pos, const TKey& key)
  {
    if(pos == theMap.end()) return theMap.find(key);
    //if they match the mapItr is pointing to the correct position
    if(pos->first < key) {
      return theMap.find(key);
    }
    if(pos->first > key) {
      return theMap.end();
    }
    //else they are equal and no need to do anything to the iterator
    return pos;
  }

  // createActiveTailsAsPair is called in these two end cases of geometry
  // 1. lower left concave corner
  //         ###|
  //         ###|
  //         ###|###
  //         ###|###
  // 2. lower left convex corner
  //            |###
  //            |###
  //            |
  //            |
  // In case 1 there may be a hole propigated up from the bottom.  If the fracture option is enabled
  // the two active tails that form the filament fracture line edges can become the new active tail pair
  // by pushing x and y onto them.  Otherwise the hole simply needs to be associated to one of the new active tails
  // with add hole
  template <typename Unit>
  inline std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*> createActiveTailsAsPair(Unit x, Unit y, bool solid, ActiveTail<Unit>* phole, bool fractureHoles) {
    ActiveTail<Unit>* at1 = 0;
    ActiveTail<Unit>* at2 = 0;
    if(!phole || !fractureHoles){
      at1 = createActiveTail<Unit>();
      at2 = createActiveTail<Unit>();
      (*at1) = ActiveTail<Unit>(VERTICAL, x, solid, at2);
      (*at2) = ActiveTail<Unit>(HORIZONTAL, y, !solid, at1);
      //provide a function through activeTail class to provide this
      at1->getTail()->joinHeadToHead(*(at2->getTail()));

      at1->addPolyLineSize(1);
      at2->addPolyLineSize(1);

      if(phole)
        at1->addHole(phole, fractureHoles); //assert fractureHoles == false
      return std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*>(at1, at2);
    }
    //assert phole is not null
    //assert fractureHoles is true
    if(phole->getOrient() == VERTICAL) {
      at2 = phole;
    } else {
      at2 = phole->getOtherActiveTail(); //should never be executed since orientation is expected to be vertical
    }
    //assert solid == false, we should be creating a corner with solid below and to the left if there was a hole
    at1 = at2->getOtherActiveTail();
    //assert at1 is horizontal
    at1->pushCoordinate(x);
    //assert at2 is vertical
    at2->pushCoordinate(y);

    return std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*>(at1, at2);
  }

  /*
   *     |
   *     |
   *     =
   *     |########
   *     |########  (add a new ActiveTail in the tailMap_).
   *     |########
   *     |########
   *     |########
   *     =
   *     |
   *     |
   *
   * NOTE: Call this only if you are sure that the $ledege$ is not in the tailMap_
   */
  template<bool orientT, typename Unit, typename polygon_concept_type>
  inline void ScanLineToPolygonItrs<orientT, Unit, polygon_concept_type>::
  insertNewLeftEdgeIntoTailMap(Unit currentX, Unit yBegin, Unit yEnd,
   typename std::map<Unit, ActiveTail<Unit> *>::iterator &hint){
     ActiveTail<Unit> *currentTail = NULL;
     std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*> tailPair =
      createActiveTailsAsPair(currentX, yBegin, true, currentTail,
         fractureHoles_);
     currentTail = tailPair.first;
     if(!tailMap_.empty()){
        ++hint;
     }
     hint = tailMap_.insert(hint, std::make_pair(yBegin, tailPair.second));
     currentTail->pushCoordinate(yEnd); ++hint;
     hint = tailMap_.insert(hint, std::make_pair(yEnd, currentTail));
  }

  template<bool orientT, typename Unit, typename polygon_concept_type>
  inline void ScanLineToPolygonItrs<orientT, Unit, polygon_concept_type>::
  closePartialSimplePolygon(Unit currentX, ActiveTail<Unit>*pfig,
      ActiveTail<Unit>*ppfig){
     pfig->pushCoordinate(currentX);
     ActiveTail<Unit>::joinChains(pfig, ppfig, false, outputPolygons_);
  }
  /*
   * If the invariant is maintained correctly then left edges can do the
   * following.
   *
   *               =###
   *            #######
   *            #######
   *            #######
   *            #######
   *               =###
   *               |### (input left edge)
   *               |###
   *               =###
   *            #######
   *            #######
   *               =###
   */
  template<bool orientT, typename Unit, typename polygon_concept_type>
  inline void ScanLineToPolygonItrs<orientT, Unit, polygon_concept_type>::
  updatePartialSimplePolygonsWithLeftEdges(Unit currentX,
   const std::vector<interval_data<Unit> > &leftEdges, size_t vertexThreshold){
     typename std::map<Unit, ActiveTail<Unit>* >::iterator succ, succ1;
     typename std::map<Unit, ActiveTail<Unit>* >::iterator pred, pred1, hint;
     Unit begin, end;
     ActiveTail<Unit> *pfig, *ppfig;
     std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*> tailPair;
     size_t pfig_size = 0;

     hint = tailMap_.begin();
     for(size_t i=0; i < leftEdges.size(); i++){
        begin = leftEdges[i].get(LOW); end = leftEdges[i].get(HIGH);
        succ = findAtNext(tailMap_, hint, begin);
        pred = findAtNext(tailMap_, hint, end);

        if(succ != tailMap_.end() && pred != tailMap_.end()){ //CASE-1//
           //join the corresponding active tails//
           pfig = succ->second; ppfig = pred->second;
           pfig_size = pfig->getPolyLineSize() + ppfig->getPolyLineSize();

           if(pfig_size >= vertexThreshold){
              size_t bsize = pfig->getPolyLineSize();
              size_t usize = ppfig->getPolyLineSize();

              if(usize+2 < vertexThreshold){
                 //cut-off the lower piece (succ1, succ) join (succ1, pred)//
                 succ1 = succ; --succ1;
                 assert((succ1 != tailMap_.end()) &&
                  ((succ->second)->getOtherActiveTail() == succ1->second));
                 closePartialSimplePolygon(currentX, succ1->second, succ->second);
                 tailPair = createActiveTailsAsPair<Unit>(currentX, succ1->first,
                     true, NULL, fractureHoles_);

                 //just update the succ1 with new ActiveTail<Unit>*//
                 succ1->second = tailPair.second;
                 ActiveTail<Unit>::joinChains(tailPair.first, pred->second, true,
                     outputPolygons_);
              }else if(bsize+2 < vertexThreshold){
                 //cut-off the upper piece () join ()//
                 pred1 = pred; ++pred1;
                 assert(pred1 != tailMap_.end() &&
                  ((pred1->second)->getOtherActiveTail() == pred->second));
                 closePartialSimplePolygon(currentX, pred->second, pred1->second);

                 //just update the pred1 with ActiveTail<Unit>* = pfig//
                 pred1->second = pfig;
                 pfig->pushCoordinate(currentX);
                 pfig->pushCoordinate(pred1->first);
              }else{
                 //cut both and create an left edge between (pred->first, succ1)//
                 succ1 = succ; --succ1;
                 pred1 = pred; ++pred1;
                 assert(pred1 != tailMap_.end() && succ1 != tailMap_.end());
                 assert((pred1->second)->getOtherActiveTail() == pred->second);
                 assert((succ1->second)->getOtherActiveTail() == succ->second);

                 closePartialSimplePolygon(currentX, succ1->second, succ->second);
                 closePartialSimplePolygon(currentX, pred->second, pred1->second);

                 tailPair = createActiveTailsAsPair<Unit>(currentX, succ1->first,
                     true, NULL, fractureHoles_);
                 succ1->second = tailPair.second;
                 pred1->second = tailPair.first;
                 (tailPair.first)->pushCoordinate(pred1->first);
              }
           }else{
              //just join them with closing//
              pfig->pushCoordinate(currentX);
              ActiveTail<Unit>::joinChains(pfig, ppfig, true, outputPolygons_);
           }
           hint = pred; ++hint;
           tailMap_.erase(succ); tailMap_.erase(pred);
        }else if(succ == tailMap_.end() && pred != tailMap_.end()){ //CASE-2//
           //succ is missing in the map, first insert it into the map//
           tailPair = createActiveTailsAsPair<Unit>(currentX, begin, true, NULL,
               fractureHoles_);
           hint = pred; ++hint;
           hint = tailMap_.insert(hint, std::make_pair(begin, tailPair.second));

           pfig = pred->second;
           pfig_size = pfig->getPolyLineSize() + 2;
           if(pfig_size >= vertexThreshold){
              //cut-off piece from [pred, pred1] , add [begin, pred1]//
              pred1 = pred; ++pred1;
              assert((pred1 != tailMap_.end()) &&
               ((pred1->second)->getOtherActiveTail() == pred->second));
              closePartialSimplePolygon(currentX, pred->second, pred1->second);

              //update: we need left edge between (begin, pred1->first)//
              pred1->second = tailPair.first;
              (tailPair.first)->pushCoordinate(pred1->first);
           }else{
              //just join//
              ActiveTail<Unit>::joinChains(tailPair.first, pfig,
                  true, outputPolygons_);
           }
           tailMap_.erase(pred);
        }else if(succ != tailMap_.end() && pred == tailMap_.end()){ //CASE-3//
            //pred is missing in the map, first insert it into the map//
            hint = succ; ++hint;
            hint = tailMap_.insert(hint, std::make_pair(end, (ActiveTail<Unit> *) NULL));
            pfig = succ->second;
            pfig_size = pfig->getPolyLineSize() + 2;
            if(pfig_size >= vertexThreshold){
               //this figure needs cutting here//
               succ1 = succ; --succ1;
               assert((succ1 != tailMap_.end()) &&
                  (succ1->second == pfig->getOtherActiveTail()));
               ppfig = succ1->second;
               closePartialSimplePolygon(currentX, ppfig, pfig);

               //update: we need a left edge between (succ1->first, end)//
               tailPair = createActiveTailsAsPair<Unit>(currentX, succ1->first,
                  true, NULL, fractureHoles_);
               succ1->second = tailPair.second;
               hint->second = tailPair.first;
               (tailPair.first)->pushCoordinate(end);
            }else{
               //no cutting needed//
               hint->second = pfig;
               pfig->pushCoordinate(currentX);
               pfig->pushCoordinate(end);
            }
            tailMap_.erase(succ);
        }else{
           //insert both pred and succ//
           insertNewLeftEdgeIntoTailMap(currentX, begin, end, hint);
        }
     }
  }

  template<bool orientT, typename Unit, typename polygon_concept_type>
  inline void ScanLineToPolygonItrs<orientT, Unit, polygon_concept_type>::
  updatePartialSimplePolygonsWithRightEdges(Unit currentX,
   const std::vector<interval_data<Unit> > &rightEdges, size_t vertexThreshold)
   {

     typename std::map<Unit, ActiveTail<Unit>* >::iterator succ, pred, hint;
     std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*> tailPair;
     Unit begin, end;
     size_t i = 0;
     //If rightEdges is non-empty Then tailMap_ is non-empty //
     assert(rightEdges.empty() || !tailMap_.empty() );

     while( i < rightEdges.size() ){
        //find the interval in the tailMap which contains this interval//
        pred = tailMap_.lower_bound(rightEdges[i].get(HIGH));
        assert(pred != tailMap_.end());
        succ = pred; --succ;
        assert(pred != succ);
        end =  pred->first; begin = succ->first;

        //we now have a [begin, end] //
        bool found_solid_opening = false;
        bool erase_succ = true, erase_pred = true;
        Unit solid_opening_begin = 0;
        Unit solid_opening_end = 0;
        size_t j = i+1;
        ActiveTail<Unit> *pfig = succ->second;
        ActiveTail<Unit> *ppfig = pred->second;
        size_t partial_fig_size = pfig->getPolyLineSize();
        //Invariant://
        assert(succ->second && (pfig)->getOtherActiveTail() == ppfig);

        hint = succ;
        Unit key = rightEdges[i].get(LOW);
        if(begin != key){
           found_solid_opening = true;
           solid_opening_begin = begin; solid_opening_end = key;
        }

        while(j < rightEdges.size() && rightEdges[j].get(HIGH) <= end){
           if(rightEdges[j-1].get(HIGH) != rightEdges[j].get(LOW)){
              if(!found_solid_opening){
                 found_solid_opening = true;
                 solid_opening_begin = rightEdges[j-1].get(HIGH);
                 solid_opening_end = rightEdges[j].get(LOW);
              }else{
                 ++hint;
                 insertNewLeftEdgeIntoTailMap(currentX,
                     rightEdges[j-1].get(HIGH), rightEdges[j].get(LOW), hint);
              }
           }
           j++;
        }

        //trailing edge//
        if(end != rightEdges[j-1].get(HIGH)){
           if(!found_solid_opening){
              found_solid_opening = true;
              solid_opening_begin = rightEdges[j-1].get(HIGH); solid_opening_end = end;
           }else{
              // a solid opening has been found already, we need to insert a new left
              // between [rightEdges[j-1].get(HIGH), end]
              Unit lbegin = rightEdges[j-1].get(HIGH);
              tailPair = createActiveTailsAsPair<Unit>(currentX, lbegin, true, NULL,
                  fractureHoles_);
              hint = tailMap_.insert(pred, std::make_pair(lbegin, tailPair.second));
              pred->second = tailPair.first;
              (tailPair.first)->pushCoordinate(end);
              erase_pred = false;
           }
        }

        size_t vertex_delta = ((begin != solid_opening_begin) &&
               (end != solid_opening_end)) ? 4 : 2;

        if(!found_solid_opening){
           //just close the figure, TODO: call closePartialPolygon//
           pfig->pushCoordinate(currentX);
           ActiveTail<Unit>::joinChains(pfig, ppfig, false, outputPolygons_);
           hint = pred; ++hint;
        }else if(partial_fig_size+vertex_delta >= vertexThreshold){
           //close the figure and add a pseudo left-edge//
           closePartialSimplePolygon(currentX, pfig, ppfig);
           assert(begin != solid_opening_begin || end != solid_opening_end);

           if(begin != solid_opening_begin && end != solid_opening_end){
               insertNewLeftEdgeIntoTailMap(currentX, solid_opening_begin,
                     solid_opening_end, hint);
           }else if(begin == solid_opening_begin){
              //we just need to update the succ in the tailMap_//
              tailPair = createActiveTailsAsPair<Unit>(currentX, solid_opening_begin,
                  true, NULL, fractureHoles_);
              succ->second = tailPair.second;
              hint = succ; ++hint;
              hint = tailMap_.insert(pred, std::make_pair(solid_opening_end,
                  tailPair.first));
              (tailPair.first)->pushCoordinate(solid_opening_end);
              erase_succ = false;
           }else{
              //we just need to update the pred in the tailMap_//
              tailPair = createActiveTailsAsPair<Unit>(currentX, solid_opening_begin,
                  true, NULL, fractureHoles_);
              hint = tailMap_.insert(pred, std::make_pair(solid_opening_begin,
                  tailPair.second));
              pred->second = tailPair.first;
              (tailPair.first)->pushCoordinate(solid_opening_end);
              erase_pred = false;
           }
        }else{
           //continue the figure (by adding at-most two new vertices)//
           if(begin != solid_opening_begin){
              pfig->pushCoordinate(currentX);
              pfig->pushCoordinate(solid_opening_begin);
              //insert solid_opening_begin//
              hint = succ; ++hint;
              hint = tailMap_.insert(hint, std::make_pair(solid_opening_begin, pfig));
           }else{
              erase_succ = false;
           }

           if(end != solid_opening_end){
              std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*> tailPair =
               createActiveTailsAsPair<Unit>(currentX, solid_opening_end, false,
                     NULL, fractureHoles_);
              hint = pred; ++hint;
              hint = tailMap_.insert(hint, std::make_pair(solid_opening_end,
                  tailPair.second));
              ActiveTail<Unit>::joinChains(tailPair.first, ppfig, false,
                  outputPolygons_);
           }else{
              erase_pred = false;
           }
        }

        //Remove the pred and succ if necessary//
        if(erase_succ){
           tailMap_.erase(succ);
        }
        if(erase_pred){
           tailMap_.erase(pred);
        }
        i = j;
     }
 }

 // Maintains the following invariant:
 // a. All the partial polygons formed at any state can be closed
 //    by a single edge.
 template<bool orientT, typename Unit, typename polygon_concept_type>
 inline void ScanLineToPolygonItrs<orientT, Unit, polygon_concept_type>::
 maintainPartialSimplePolygonInvariant(iterator& beginOutput,
   iterator& endOutput, Unit currentX, const std::vector<interval_data<Unit> >& l,
      const std::vector<interval_data<Unit> >& r, size_t vertexThreshold) {

      clearOutput_();
      if(!l.empty()){
         updatePartialSimplePolygonsWithLeftEdges(currentX, l, vertexThreshold);
      }

      if(!r.empty()){
         updatePartialSimplePolygonsWithRightEdges(currentX, r, vertexThreshold);
      }
      beginOutput = outputPolygons_.begin();
      endOutput = outputPolygons_.end();

  }

  //Process edges connects vertical input edges (right or left edges of figures) to horizontal edges stored as member
  //data of the scanline object.  It also creates now horizontal edges as needed to construct figures from edge data.
  //
  //There are only 12 geometric end cases where the scanline intersects a horizontal edge and even fewer unique
  //actions to take:
  // 1. Solid on both sides of the vertical partition after the current position and space on both sides before
  //         ###|###
  //         ###|###
  //            |
  //            |
  //    This case does not need to be handled because there is no vertical edge at the current x coordinate.
  //
  // 2. Solid on both sides of the vertical partition before the current position and space on both sides after
  //            |
  //            |
  //         ###|###
  //         ###|###
  //    This case does not need to be handled because there is no vertical edge at the current x coordinate.
  //
  // 3. Solid on the left of the vertical partition after the current position and space elsewhere
  //         ###|
  //         ###|
  //            |
  //            |
  //    The horizontal edge from the left is found and turns upward because of the vertical right edge to become
  //    the currently active vertical edge.
  //
  // 4. Solid on the left of the vertical partion before the current position and space elsewhere
  //            |
  //            |
  //         ###|
  //         ###|
  //    The horizontal edge from the left is found and joined to the currently active vertical edge.
  //
  // 5. Solid to the right above and below and solid to the left above current position.
  //         ###|###
  //         ###|###
  //            |###
  //            |###
  //    The horizontal edge from the left is found and joined to the currently active vertical edge,
  //    potentially closing a hole.
  //
  // 6. Solid on the left of the vertical partion before the current position and solid to the right above and below
  //            |###
  //            |###
  //         ###|###
  //         ###|###
  //    The horizontal edge from the left is found and turns upward because of the vertical right edge to become
  //    the currently active vertical edge.
  //
  // 7. Solid on the right of the vertical partition after the current position and space elsewhere
  //            |###
  //            |###
  //            |
  //            |
  //    Create two new ActiveTails, one is added to the horizontal edges and the other becomes the vertical currentTail
  //
  // 8. Solid on the right of the vertical partion before the current position and space elsewhere
  //            |
  //            |
  //            |###
  //            |###
  //    The currentTail vertical edge turns right and is added to the horizontal edges data
  //
  // 9. Solid to the right above and solid to the left above and below current position.
  //         ###|###
  //         ###|###
  //         ###|
  //         ###|
  //    The currentTail vertical edge turns right and is added to the horizontal edges data
  //
  // 10. Solid on the left of the vertical partion above and below the current position and solid to the right below
  //         ###|
  //         ###|
  //         ###|###
  //         ###|###
  //    Create two new ActiveTails, one is added to the horizontal edges data and the other becomes the vertical currentTail
  //
  // 11. Solid to the right above and solid to the left below current position.
  //            |###
  //            |###
  //         ###|
  //         ###|
  //    The currentTail vertical edge joins the horizontal edge from the left (may close a polygon)
  //    Create two new ActiveTails, one is added to the horizontal edges data and the other becomes the vertical currentTail
  //
  // 12. Solid on the left of the vertical partion above the current position and solid to the right below
  //         ###|
  //         ###|
  //            |###
  //            |###
  //    The currentTail vertical edge turns right and is added to the horizontal edges data.
  //    The horizontal edge from the left turns upward and becomes the currentTail vertical edge
  //
  template <bool orientT, typename Unit, typename polygon_concept_type>
  inline void ScanLineToPolygonItrs<orientT, Unit, polygon_concept_type>::
  processEdges(iterator& beginOutput, iterator& endOutput,
               Unit currentX, std::vector<interval_data<Unit> >& leftEdges,
               std::vector<interval_data<Unit> >& rightEdges,
               size_t vertexThreshold) {
    clearOutput_();
    typename std::map<Unit, ActiveTail<Unit>*>::iterator nextMapItr;
    //foreach edge
    unsigned int leftIndex = 0;
    unsigned int rightIndex = 0;
    bool bottomAlreadyProcessed = false;
    ActiveTail<Unit>* currentTail = 0;
    const Unit UnitMax = (std::numeric_limits<Unit>::max)();

    if(vertexThreshold < (std::numeric_limits<size_t>::max)()){
      maintainPartialSimplePolygonInvariant(beginOutput, endOutput, currentX,
         leftEdges, rightEdges, vertexThreshold);
      return;
    }

    nextMapItr = tailMap_.begin();
    while(leftIndex < leftEdges.size() || rightIndex < rightEdges.size()) {
      interval_data<Unit>  edges[2] = {interval_data<Unit> (UnitMax, UnitMax),
         interval_data<Unit> (UnitMax, UnitMax)};
      bool haveNextEdge = true;
      if(leftIndex < leftEdges.size())
        edges[0] = leftEdges[leftIndex];
      else
        haveNextEdge = false;
      if(rightIndex < rightEdges.size())
        edges[1] = rightEdges[rightIndex];
      else
        haveNextEdge = false;
      bool trailingEdge = edges[1].get(LOW) < edges[0].get(LOW);
      interval_data<Unit> & edge = edges[trailingEdge];
      interval_data<Unit> & nextEdge = edges[!trailingEdge];
      //process this edge
      if(!bottomAlreadyProcessed) {
        //assert currentTail = 0

        //process the bottom end of this edge
        typename std::map<Unit, ActiveTail<Unit>*>::iterator thisMapItr = findAtNext(tailMap_, nextMapItr, edge.get(LOW));
        if(thisMapItr != tailMap_.end()) {
          //there is an edge in the map at the low end of this edge
          //it needs to turn upward and become the current tail
          ActiveTail<Unit>* tail = thisMapItr->second;
          if(currentTail) {
            //stitch currentTail into this tail
            currentTail = tail->addHole(currentTail, fractureHoles_);
            if(!fractureHoles_)
              currentTail->pushCoordinate(currentX);
          } else {
            currentTail = tail;
            currentTail->pushCoordinate(currentX);
          }
          //assert currentTail->getOrient() == VERTICAL
          nextMapItr = thisMapItr; //set nextMapItr to the next position after this one
          ++nextMapItr;
          //remove thisMapItr from the map
          tailMap_.erase(thisMapItr);
        } else {
          //there is no edge in the map at the low end of this edge
          //we need to create one and another one to be the current vertical tail
          //if this is a trailing edge then there is space to the right of the vertical edge
          //so pass the inverse of trailingEdge to indicate solid to the right
          std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*> tailPair =
            createActiveTailsAsPair(currentX, edge.get(LOW), !trailingEdge, currentTail, fractureHoles_);
          currentTail = tailPair.first;
          tailMap_.insert(nextMapItr, std::pair<Unit, ActiveTail<Unit>*>(edge.get(LOW), tailPair.second));
          // leave nextMapItr unchanged
        }

      }
      if(haveNextEdge && edge.get(HIGH) == nextEdge.get(LOW)) {
        //the top of this edge is equal to the bottom of the next edge, process them both
        bottomAlreadyProcessed = true;
        typename std::map<Unit, ActiveTail<Unit>*>::iterator thisMapItr = findAtNext(tailMap_, nextMapItr, edge.get(HIGH));
        if(thisMapItr == tailMap_.end()) //assert this should never happen
          return;
        if(trailingEdge) {
          //geometry at this position
          //   |##
          //   |##
          // -----
          // ##|
          // ##|
          //current tail should join thisMapItr tail
          ActiveTail<Unit>* tail = thisMapItr->second;
          //pass false because they are being joined because space is to the right and it will close a solid figure
          ActiveTail<Unit>::joinChains(currentTail, tail, false, outputPolygons_);
          //two new tails are created, the vertical becomes current tail, the horizontal becomes thisMapItr tail
          //pass true becuase they are created at the lower left corner of some solid
          //pass null because there is no hole pointer possible
          std::pair<ActiveTail<Unit>*, ActiveTail<Unit>*> tailPair =
            createActiveTailsAsPair<Unit>(currentX, edge.get(HIGH), true, 0, fractureHoles_);
          currentTail = tailPair.first;
          thisMapItr->second = tailPair.second;
        } else {
          //geometry at this position
          // ##|
          // ##|
          // -----
          //   |##
          //   |##
          //current tail should turn right
          currentTail->pushCoordinate(edge.get(HIGH));
          //thisMapItr tail should turn up
          thisMapItr->second->pushCoordinate(currentX);
          //thisMapItr tail becomes current tail and current tail becomes thisMapItr tail
          std::swap(currentTail, thisMapItr->second);
        }
        nextMapItr = thisMapItr; //set nextMapItr to the next position after this one
        ++nextMapItr;
      } else {
        //there is a gap between the top of this edge and the bottom of the next, process the top of this edge
        bottomAlreadyProcessed = false;
        //process the top of this edge
        typename std::map<Unit, ActiveTail<Unit>*>::iterator thisMapItr = findAtNext(tailMap_, nextMapItr, edge.get(HIGH));
        if(thisMapItr != tailMap_.end()) {
          //thisMapItr is pointing to a horizontal edge in the map at the top of this vertical edge
          //we need to join them and potentially close a figure
          //assert currentTail != 0
          ActiveTail<Unit>* tail = thisMapItr->second;
          //pass the opositve of trailing edge to mean that they are joined because of solid to the right
          currentTail = ActiveTail<Unit>::joinChains(currentTail, tail, !trailingEdge, outputPolygons_);
          nextMapItr = thisMapItr; //set nextMapItr to the next position after this one
          ++nextMapItr;
          if(currentTail) { //figure is not closed//
            Unit nextItrY = UnitMax;
            if(nextMapItr != tailMap_.end()) {
              nextItrY = nextMapItr->first;
            }
            //for it to be a hole this must have been a left edge
            Unit leftY = UnitMax;
            if(leftIndex + 1 < leftEdges.size())
              leftY = leftEdges[leftIndex+1].get(LOW);
            Unit rightY = nextEdge.get(LOW);
            if(!haveNextEdge || (nextItrY < leftY && nextItrY < rightY)) {
              //we need to add it to the next edge above it in the map
              tail = nextMapItr->second;
              tail = tail->addHole(currentTail, fractureHoles_);
              if(fractureHoles_) {
                //some small additional work stitching in the filament
                tail->pushCoordinate(nextItrY);
                nextMapItr->second = tail;
              }
              //set current tail to null
              currentTail = 0;
            }
          }
          //delete thisMapItr from the map
          tailMap_.erase(thisMapItr);
        } else {
          //currentTail must turn right and be added into the map
          currentTail->pushCoordinate(edge.get(HIGH));
          //assert currentTail->getOrient() == HORIZONTAL
          tailMap_.insert(nextMapItr, std::pair<Unit, ActiveTail<Unit>*>(edge.get(HIGH), currentTail));
          //set currentTail to null
          currentTail = 0;
          //leave nextMapItr unchanged, it is still next
        }
      }

      //increment index
      leftIndex += !trailingEdge;
      rightIndex += trailingEdge;
    } //end while
    beginOutput = outputPolygons_.begin();
    endOutput = outputPolygons_.end();
  } //end function

  template<bool orientT, typename Unit, typename polygon_concept_type>
  inline void ScanLineToPolygonItrs<orientT, Unit, polygon_concept_type>::clearOutput_() {
    for(std::size_t i = 0; i < outputPolygons_.size(); ++i) {
      ActiveTail<Unit>* at1 = outputPolygons_[i].yield();
      const std::list<ActiveTail<Unit>*>& holes = at1->getHoles();
      for(typename std::list<ActiveTail<Unit>*>::const_iterator litr = holes.begin(); litr != holes.end(); ++litr) {
        //delete the hole
        (*litr)->destroyContents();
        destroyActiveTail((*litr)->getOtherActiveTail());
        destroyActiveTail((*litr));
      }
      //delete the polygon
      at1->destroyContents();
      //at2 contents are the same as at1, so it should not destroy them
      destroyActiveTail((at1)->getOtherActiveTail());
      destroyActiveTail(at1);
    }
    outputPolygons_.clear();
  }

} //polygon_formation namespace

  template <bool orientT, typename Unit>
  struct geometry_concept<polygon_formation::PolyLinePolygonWithHolesData<orientT, Unit> > {
    typedef polygon_90_with_holes_concept type;
  };

  template <bool orientT, typename Unit>
  struct geometry_concept<polygon_formation::PolyLineHoleData<orientT, Unit> > {
    typedef polygon_90_concept type;
  };

  //public API to access polygon formation algorithm
  template <typename output_container, typename iterator_type, typename concept_type>
  unsigned int get_polygons(output_container& container,
      iterator_type begin, iterator_type end, orientation_2d orient,
      bool fracture_holes, concept_type,
      size_t sliceThreshold = (std::numeric_limits<size_t>::max)() ) {
    typedef typename output_container::value_type polygon_type;
    typedef typename std::iterator_traits<iterator_type>::value_type::first_type coordinate_type;
    polygon_type poly;
    unsigned int countPolygons = 0;
    typedef typename geometry_concept<polygon_type>::type polygon_concept_type;
    polygon_formation::ScanLineToPolygonItrs<true, coordinate_type, polygon_concept_type> scanlineToPolygonItrsV(fracture_holes);
    polygon_formation::ScanLineToPolygonItrs<false, coordinate_type, polygon_concept_type> scanlineToPolygonItrsH(fracture_holes);
    std::vector<interval_data<coordinate_type> > leftEdges;
    std::vector<interval_data<coordinate_type> > rightEdges;
    coordinate_type prevPos = (std::numeric_limits<coordinate_type>::max)();
    coordinate_type prevY = (std::numeric_limits<coordinate_type>::max)();
    int count = 0;
    for(iterator_type itr = begin;
        itr != end; ++ itr) {
      coordinate_type pos = (*itr).first;
      if(pos != prevPos) {
        if(orient == VERTICAL) {
          typename polygon_formation::ScanLineToPolygonItrs<true, coordinate_type, polygon_concept_type>::iterator itrPoly, itrPolyEnd;
           scanlineToPolygonItrsV.processEdges(itrPoly, itrPolyEnd, prevPos,
               leftEdges, rightEdges, sliceThreshold);
          for( ; itrPoly != itrPolyEnd; ++ itrPoly) {
            ++countPolygons;
            assign(poly, *itrPoly);
            container.insert(container.end(), poly);
          }
        } else {
          typename polygon_formation::ScanLineToPolygonItrs<false, coordinate_type, polygon_concept_type>::iterator itrPoly, itrPolyEnd;
          scanlineToPolygonItrsH.processEdges(itrPoly, itrPolyEnd, prevPos,
               leftEdges, rightEdges, sliceThreshold);
          for( ; itrPoly != itrPolyEnd; ++ itrPoly) {
            ++countPolygons;
            assign(poly, *itrPoly);
            container.insert(container.end(), poly);
          }
        }
        leftEdges.clear();
        rightEdges.clear();
        prevPos = pos;
        prevY = (*itr).second.first;
        count = (*itr).second.second;
        continue;
      }
      coordinate_type y = (*itr).second.first;
      if(count != 0 && y != prevY) {
        std::pair<interval_data<coordinate_type>, int> element(interval_data<coordinate_type>(prevY, y), count);
        if(element.second == 1) {
          if(leftEdges.size() && leftEdges.back().high() == element.first.low()) {
            encompass(leftEdges.back(), element.first);
          } else {
            leftEdges.push_back(element.first);
          }
        } else {
          if(rightEdges.size() && rightEdges.back().high() == element.first.low()) {
            encompass(rightEdges.back(), element.first);
          } else {
            rightEdges.push_back(element.first);
          }
        }

      }
      prevY = y;
      count += (*itr).second.second;
    }
    if(orient == VERTICAL) {
      typename polygon_formation::ScanLineToPolygonItrs<true, coordinate_type, polygon_concept_type>::iterator itrPoly, itrPolyEnd;
      scanlineToPolygonItrsV.processEdges(itrPoly, itrPolyEnd, prevPos, leftEdges, rightEdges, sliceThreshold);
      for( ; itrPoly != itrPolyEnd; ++ itrPoly) {
        ++countPolygons;
        assign(poly, *itrPoly);
        container.insert(container.end(), poly);
      }
    } else {
      typename polygon_formation::ScanLineToPolygonItrs<false, coordinate_type, polygon_concept_type>::iterator itrPoly, itrPolyEnd;
      scanlineToPolygonItrsH.processEdges(itrPoly, itrPolyEnd, prevPos, leftEdges, rightEdges,  sliceThreshold);

      for( ; itrPoly != itrPolyEnd; ++ itrPoly) {
        ++countPolygons;
        assign(poly, *itrPoly);
        container.insert(container.end(), poly);
      }
    }
    return countPolygons;
  }

}
}
#endif

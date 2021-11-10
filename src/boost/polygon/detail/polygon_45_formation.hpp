/*
    Copyright 2008 Intel Corporation

    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_45_FORMATION_HPP
#define BOOST_POLYGON_POLYGON_45_FORMATION_HPP
namespace boost { namespace polygon{

  template <typename T, typename T2>
  struct PolyLineByConcept {};

  template <typename T>
  class PolyLine45PolygonData;
  template <typename T>
  class PolyLine45HoleData;

  //polygon45formation algorithm
  template <typename Unit>
  struct polygon_45_formation : public boolean_op_45<Unit> {
    typedef point_data<Unit> Point;
    typedef polygon_45_data<Unit> Polygon45;
    typedef polygon_45_with_holes_data<Unit> Polygon45WithHoles;
    typedef typename boolean_op_45<Unit>::Vertex45 Vertex45;
    typedef typename boolean_op_45<Unit>::lessVertex45 lessVertex45;
    typedef typename boolean_op_45<Unit>::Count2 Count2;
    typedef typename boolean_op_45<Unit>::Scan45Count Scan45Count;
    typedef std::pair<Point, Scan45Count> Scan45Vertex;
    typedef typename boolean_op_45<Unit>::template
    Scan45<Count2, typename boolean_op_45<Unit>::template boolean_op_45_output_functor<0> > Scan45;

    class PolyLine45 {
    public:
      typedef typename std::list<Point>::const_iterator iterator;

      // default constructor of point does not initialize x and y
      inline PolyLine45() : points() {} //do nothing default constructor

      // initialize a polygon from x,y values, it is assumed that the first is an x
      // and that the input is a well behaved polygon
      template<class iT>
      inline PolyLine45& set(iT inputBegin, iT inputEnd) {
        points.clear();  //just in case there was some old data there
        while(inputBegin != inputEnd) {
          points.insert(points.end(), *inputBegin);
          ++inputBegin;
        }
        return *this;
      }

      // copy constructor (since we have dynamic memory)
      inline PolyLine45(const PolyLine45& that) : points(that.points) {}

      // assignment operator (since we have dynamic memory do a deep copy)
      inline PolyLine45& operator=(const PolyLine45& that) {
        points = that.points;
        return *this;
      }

      // get begin iterator, returns a pointer to a const Unit
      inline iterator begin() const { return points.begin(); }

      // get end iterator, returns a pointer to a const Unit
      inline iterator end() const { return points.end(); }

      inline std::size_t size() const { return points.size(); }

      //public data member
      std::list<Point> points;
    };

    class ActiveTail45 {
    private:
      //data
      PolyLine45* tailp_;
      ActiveTail45 *otherTailp_;
      std::list<ActiveTail45*> holesList_;
      bool head_;
    public:

      /**
       * @brief iterator over coordinates of the figure
       */
      typedef typename PolyLine45::iterator iterator;

      /**
       * @brief iterator over holes contained within the figure
       */
      typedef typename std::list<ActiveTail45*>::const_iterator iteratorHoles;

      //default constructor
      inline ActiveTail45() : tailp_(0), otherTailp_(0), holesList_(), head_(0) {}

      //constructor
      inline ActiveTail45(const Vertex45& vertex, ActiveTail45* otherTailp = 0) :
        tailp_(0), otherTailp_(0), holesList_(), head_(0) {
        tailp_ = new PolyLine45;
        tailp_->points.push_back(vertex.pt);
        bool headArray[4] = {false, true, true, true};
        bool inverted = vertex.count == -1;
        head_ = headArray[vertex.rise+1] ^ inverted;
        otherTailp_ = otherTailp;
      }

      inline ActiveTail45(Point point, ActiveTail45* otherTailp, bool head = true) :
        tailp_(0), otherTailp_(0), holesList_(), head_(0) {
        tailp_ = new PolyLine45;
        tailp_->points.push_back(point);
        head_ = head;
        otherTailp_ = otherTailp;

      }
      inline ActiveTail45(ActiveTail45* otherTailp) :
        tailp_(0), otherTailp_(0), holesList_(), head_(0)  {
        tailp_ = otherTailp->tailp_;
        otherTailp_ = otherTailp;
      }

      //copy constructor
      inline ActiveTail45(const ActiveTail45& that) :
        tailp_(0), otherTailp_(0), holesList_(), head_(0)  { (*this) = that; }

      //destructor
      inline ~ActiveTail45() {
        destroyContents();
      }

      //assignment operator
      inline ActiveTail45& operator=(const ActiveTail45& that) {
        tailp_ = new PolyLine45(*(that.tailp_));
        head_ = that.head_;
        otherTailp_ = that.otherTailp_;
        holesList_ = that.holesList_;
        return *this;
      }

      //equivalence operator
      inline bool operator==(const ActiveTail45& b) const {
        return tailp_ == b.tailp_ && head_ == b.head_;
      }

      /**
       * @brief get the pointer to the polyline that this is an active tail of
       */
      inline PolyLine45* getTail() const { return tailp_; }

      /**
       * @brief get the pointer to the polyline at the other end of the chain
       */
      inline PolyLine45* getOtherTail() const { return otherTailp_->tailp_; }

      /**
       * @brief get the pointer to the activetail at the other end of the chain
       */
      inline ActiveTail45* getOtherActiveTail() const { return otherTailp_; }

      /**
       * @brief test if another active tail is the other end of the chain
       */
      inline bool isOtherTail(const ActiveTail45& b) const { return &b == otherTailp_; }

      /**
       * @brief update this end of chain pointer to new polyline
       */
      inline ActiveTail45& updateTail(PolyLine45* newTail) { tailp_ = newTail; return *this; }

      inline bool join(ActiveTail45* tail) {
        if(tail == otherTailp_) {
          //std::cout << "joining to other tail!\n";
          return false;
        }
        if(tail->head_ == head_) {
          //std::cout << "joining head to head!\n";
          return false;
        }
        if(!tailp_) {
          //std::cout << "joining empty tail!\n";
          return false;
        }
        if(!(otherTailp_->head_)) {
          otherTailp_->copyHoles(*tail);
          otherTailp_->copyHoles(*this);
        } else {
          tail->otherTailp_->copyHoles(*this);
          tail->otherTailp_->copyHoles(*tail);
        }
        PolyLine45* tail1 = tailp_;
        PolyLine45* tail2 = tail->tailp_;
        if(head_) std::swap(tail1, tail2);
        tail1->points.splice(tail1->points.end(), tail2->points);
        delete tail2;
        otherTailp_->tailp_ = tail1;
        tail->otherTailp_->tailp_ = tail1;
        otherTailp_->otherTailp_ = tail->otherTailp_;
        tail->otherTailp_->otherTailp_ = otherTailp_;
        tailp_ = 0;
        tail->tailp_ = 0;
        tail->otherTailp_ = 0;
        otherTailp_ = 0;
        return true;
      }

      /**
       * @brief associate a hole to this active tail by the specified policy
       */
      inline ActiveTail45* addHole(ActiveTail45* hole) {
        holesList_.push_back(hole);
        copyHoles(*hole);
        copyHoles(*(hole->otherTailp_));
        return this;
      }

      /**
       * @brief get the list of holes
       */
      inline const std::list<ActiveTail45*>& getHoles() const { return holesList_; }

      /**
       * @brief copy holes from that to this
       */
      inline void copyHoles(ActiveTail45& that) { holesList_.splice(holesList_.end(), that.holesList_); }

      /**
       * @brief find out if solid to right
       */
      inline bool solidToRight() const { return !head_; }
      inline bool solidToLeft() const { return head_; }

      /**
       * @brief get vertex
       */
      inline Point getPoint() const {
        if(head_) return tailp_->points.front();
        return tailp_->points.back();
      }

      /**
       * @brief add a coordinate to the polygon at this active tail end, properly handle degenerate edges by removing redundant coordinate
       */
      inline void pushPoint(Point point) {
        if(head_) {
          //if(tailp_->points.size() < 2) {
          //   tailp_->points.push_front(point);
          //   return;
          //}
          typename std::list<Point>::iterator iter = tailp_->points.begin();
          if(iter == tailp_->points.end()) {
            tailp_->points.push_front(point);
            return;
          }
          Unit firstY = (*iter).y();
          Unit firstX = (*iter).x();
          ++iter;
          if(iter == tailp_->points.end()) {
            tailp_->points.push_front(point);
            return;
          }
          if((iter->y() == point.y() && firstY == point.y()) ||
             (iter->x() == point.x() && firstX == point.x())){
            --iter;
            *iter = point;
          } else {
            tailp_->points.push_front(point);
          }
          return;
        }
        //if(tailp_->points.size() < 2) {
        //   tailp_->points.push_back(point);
        //   return;
        //}
        typename std::list<Point>::reverse_iterator iter = tailp_->points.rbegin();
        if(iter == tailp_->points.rend()) {
          tailp_->points.push_back(point);
          return;
        }
        Unit firstY = (*iter).y();
        Unit firstX = (*iter).x();
        ++iter;
        if(iter == tailp_->points.rend()) {
          tailp_->points.push_back(point);
          return;
        }
        if((iter->y() == point.y() && firstY == point.y()) ||
           (iter->x() == point.x() && firstX == point.x())){
          --iter;
          *iter = point;
        } else {
          tailp_->points.push_back(point);
        }
      }

      /**
       * @brief joins the two chains that the two active tail tails are ends of
       * checks for closure of figure and writes out polygons appropriately
       * returns a handle to a hole if one is closed
       */

      template <class cT>
      static inline ActiveTail45* joinChains(Point point, ActiveTail45* at1, ActiveTail45* at2, bool solid,
                                             cT& output) {
        if(at1->otherTailp_ == at2) {
          //if(at2->otherTailp_ != at1) std::cout << "half closed error\n";
          //we are closing a figure
          at1->pushPoint(point);
          at2->pushPoint(point);
          if(solid) {
            //we are closing a solid figure, write to output
            //std::cout << "test1\n";
            at1->copyHoles(*(at1->otherTailp_));
            //std::cout << "test2\n";
            //Polygon45WithHolesImpl<PolyLine45PolygonData> poly(polyData);
            //std::cout << poly << "\n";
            //std::cout << "test3\n";
            typedef typename cT::value_type pType;
            output.push_back(pType());
            typedef typename geometry_concept<pType>::type cType;
            typename PolyLineByConcept<Unit, cType>::type polyData(at1);
            assign(output.back(), polyData);
            //std::cout << "test4\n";
            //std::cout << "delete " << at1->otherTailp_ << "\n";
            //at1->print();
            //at1->otherTailp_->print();
            delete at1->otherTailp_;
            //at1->print();
            //at1->otherTailp_->print();
            //std::cout << "test5\n";
            //std::cout << "delete " << at1 << "\n";
            delete at1;
            //std::cout << "test6\n";
            return 0;
          } else {
            //we are closing a hole, return the tail end active tail of the figure
            return at1;
          }
        }
        //we are not closing a figure
        at1->pushPoint(point);
        at1->join(at2);
        delete at1;
        delete at2;
        return 0;
      }

      inline void destroyContents() {
        if(otherTailp_) {
          //std::cout << "delete p " << tailp_ << "\n";
          if(tailp_) delete tailp_;
          tailp_ = 0;
          otherTailp_->otherTailp_ = 0;
          otherTailp_->tailp_ = 0;
          otherTailp_ = 0;
        }
        for(typename std::list<ActiveTail45*>::iterator itr = holesList_.begin(); itr != holesList_.end(); ++itr) {
          //std::cout << "delete p " << (*itr) << "\n";
          if(*itr) {
            if((*itr)->otherTailp_) {
              delete (*itr)->otherTailp_;
              (*itr)->otherTailp_ = 0;
            }
            delete (*itr);
          }
          (*itr) = 0;
        }
        holesList_.clear();
      }

//       inline void print() {
//         std::cout << this << " " << tailp_ << " " << otherTailp_ << " " << holesList_.size() << " " << head_ << "\n";
//       }

      static inline std::pair<ActiveTail45*, ActiveTail45*> createActiveTail45sAsPair(Point point, bool solid,
                                                                                      ActiveTail45* phole, bool fractureHoles) {
        ActiveTail45* at1 = 0;
        ActiveTail45* at2 = 0;
        if(phole && fractureHoles) {
          //std::cout << "adding hole\n";
          at1 = phole;
          //assert solid == false, we should be creating a corner with solid below and to the left if there was a hole
          at2 = at1->getOtherActiveTail();
          at2->pushPoint(point);
          at1->pushPoint(point);
        } else {
          at1 = new ActiveTail45(point, at2, solid);
          at2 = new ActiveTail45(at1);
          at1->otherTailp_ = at2;
          at2->head_ = !solid;
          if(phole)
            at2->addHole(phole); //assert fractureHoles == false
        }
        return std::pair<ActiveTail45*, ActiveTail45*>(at1, at2);
      }

    };

    template <typename ct>
    class Vertex45CountT {
    public:
      typedef ct count_type;
      inline Vertex45CountT()
#ifndef BOOST_POLYGON_MSVC
        : counts()
#endif
      { counts[0] = counts[1] = counts[2] = counts[3] = 0; }
      //inline Vertex45CountT(ct count) { counts[0] = counts[1] = counts[2] = counts[3] = count; }
      inline Vertex45CountT(const ct& count1, const ct& count2, const ct& count3,
                           const ct& count4)
#ifndef BOOST_POLYGON_MSVC
        : counts()
#endif
      {
        counts[0] = count1;
        counts[1] = count2;
        counts[2] = count3;
        counts[3] = count4;
      }
      inline Vertex45CountT(const Vertex45& vertex)
#ifndef BOOST_POLYGON_MSVC
        : counts()
#endif
      {
        counts[0] = counts[1] = counts[2] = counts[3] = 0;
        (*this) += vertex;
      }
      inline Vertex45CountT(const Vertex45CountT& count)
#ifndef BOOST_POLYGON_MSVC
        : counts()
#endif
      {
        (*this) = count;
      }
      inline bool operator==(const Vertex45CountT& count) const {
        for(unsigned int i = 0; i < 4; ++i) {
          if(counts[i] != count.counts[i]) return false;
        }
        return true;
      }
      inline bool operator!=(const Vertex45CountT& count) const { return !((*this) == count); }
      inline Vertex45CountT& operator=(ct count) {
        counts[0] = counts[1] = counts[2] = counts[3] = count; return *this; }
      inline Vertex45CountT& operator=(const Vertex45CountT& count) {
        for(unsigned int i = 0; i < 4; ++i) {
          counts[i] = count.counts[i];
        }
        return *this;
      }
      inline ct& operator[](int index) { return counts[index]; }
      inline ct operator[](int index) const {return counts[index]; }
      inline Vertex45CountT& operator+=(const Vertex45CountT& count){
        for(unsigned int i = 0; i < 4; ++i) {
          counts[i] += count.counts[i];
        }
        return *this;
      }
      inline Vertex45CountT& operator-=(const Vertex45CountT& count){
        for(unsigned int i = 0; i < 4; ++i) {
          counts[i] -= count.counts[i];
        }
        return *this;
      }
      inline Vertex45CountT operator+(const Vertex45CountT& count) const {
        return Vertex45CountT(*this)+=count;
      }
      inline Vertex45CountT operator-(const Vertex45CountT& count) const {
        return Vertex45CountT(*this)-=count;
      }
      inline Vertex45CountT invert() const {
        return Vertex45CountT()-=(*this);
      }
      inline Vertex45CountT& operator+=(const Vertex45& element){
        counts[element.rise+1] += element.count; return *this;
      }
      inline bool is_45() const {
        return counts[0] != 0 || counts[2] != 0;
      }
    private:
      ct counts[4];
    };

    typedef Vertex45CountT<int> Vertex45Count;

//     inline std::ostream& operator<< (std::ostream& o, const Vertex45Count& c) {
//       o << c[0] << ", " << c[1] << ", ";
//       o << c[2] << ", " << c[3];
//       return o;
//     }

    template <typename ct>
    class Vertex45CompactT {
    public:
      Point pt;
      ct count;
      typedef typename boolean_op_45<Unit>::template Vertex45T<typename ct::count_type> Vertex45T;
      inline Vertex45CompactT() : pt(), count() {}
      inline Vertex45CompactT(const Point& point, int riseIn, int countIn) : pt(point), count() {
        count[riseIn+1] = countIn;
      }
      template <typename ct2>
      inline Vertex45CompactT(const typename boolean_op_45<Unit>::template Vertex45T<ct2>& vertex) : pt(vertex.pt), count() {
        count[vertex.rise+1] = vertex.count;
      }
      inline Vertex45CompactT(const Vertex45CompactT& vertex) : pt(vertex.pt), count(vertex.count) {}
      inline Vertex45CompactT& operator=(const Vertex45CompactT& vertex){
        pt = vertex.pt; count = vertex.count; return *this; }
      inline bool operator==(const Vertex45CompactT& vertex) const {
        return pt == vertex.pt && count == vertex.count; }
      inline bool operator!=(const Vertex45CompactT& vertex) const { return !((*this) == vertex); }
      inline bool operator<(const Vertex45CompactT& vertex) const {
        if(pt.x() < vertex.pt.x()) return true;
        if(pt.x() == vertex.pt.x()) {
          return pt.y() < vertex.pt.y();
        }
        return false;
      }
      inline bool operator>(const Vertex45CompactT& vertex) const { return vertex < (*this); }
      inline bool operator<=(const Vertex45CompactT& vertex) const { return !((*this) > vertex); }
      inline bool operator>=(const Vertex45CompactT& vertex) const { return !((*this) < vertex); }
      inline bool haveVertex45(int index) const { return count[index]; }
      inline Vertex45T operator[](int index) const {
        return Vertex45T(pt, index-1, count[index]); }
    };

    typedef Vertex45CompactT<Vertex45Count> Vertex45Compact;

//     inline std::ostream& operator<< (std::ostream& o, const Vertex45Compact& c) {
//       o << c.pt << ", " << c.count;
//       return o;
//     }

    class Polygon45Formation {
    private:
      //definitions
      typedef std::map<Vertex45, ActiveTail45*, lessVertex45> Polygon45FormationData;
      typedef typename Polygon45FormationData::iterator iterator;
      typedef typename Polygon45FormationData::const_iterator const_iterator;

      //data
      Polygon45FormationData scanData_;
      Unit x_;
      int justBefore_;
      int fractureHoles_;
    public:
      inline Polygon45Formation() : scanData_(), x_((std::numeric_limits<Unit>::min)()), justBefore_(false), fractureHoles_(0) {
        lessVertex45 lessElm(&x_, &justBefore_);
        scanData_ = Polygon45FormationData(lessElm);
      }
      inline Polygon45Formation(bool fractureHoles) : scanData_(), x_((std::numeric_limits<Unit>::min)()), justBefore_(false), fractureHoles_(fractureHoles) {
        lessVertex45 lessElm(&x_, &justBefore_);
        scanData_ = Polygon45FormationData(lessElm);
      }
      inline Polygon45Formation(const Polygon45Formation& that) :
        scanData_(), x_((std::numeric_limits<Unit>::min)()), justBefore_(false), fractureHoles_(0) { (*this) = that; }
      inline Polygon45Formation& operator=(const Polygon45Formation& that) {
        x_ = that.x_;
        justBefore_ = that.justBefore_;
        fractureHoles_ = that.fractureHoles_;
        lessVertex45 lessElm(&x_, &justBefore_);
        scanData_ = Polygon45FormationData(lessElm);
        for(const_iterator itr = that.scanData_.begin(); itr != that.scanData_.end(); ++itr){
          scanData_.insert(scanData_.end(), *itr);
        }
        return *this;
      }

      //cT is an output container of Polygon45 or Polygon45WithHoles
      //iT is an iterator over Vertex45 elements
      //inputBegin - inputEnd is a range of sorted iT that represents
      //one or more scanline stops worth of data
      template <class cT, class iT>
      void scan(cT& output, iT inputBegin, iT inputEnd) {
        //std::cout << "1\n";
        while(inputBegin != inputEnd) {
          //std::cout << "2\n";
          x_ = (*inputBegin).pt.x();
          //std::cout << "SCAN FORMATION " << x_ << "\n";
          //std::cout << "x_ = " << x_ << "\n";
          //std::cout << "scan line size: " << scanData_.size() << "\n";
          inputBegin = processEvent_(output, inputBegin, inputEnd);
        }
      }

    private:
      //functions
      template <class cT, class cT2>
      inline std::pair<int, ActiveTail45*> processPoint_(cT& output, cT2& elements, Point point,
                                                         Vertex45Count& counts, ActiveTail45** tails, Vertex45Count& incoming) {
        //std::cout << point << "\n";
        //std::cout << counts[0] << " ";
        //std::cout << counts[1] << " ";
        //std::cout << counts[2] << " ";
        //std::cout << counts[3] << "\n";
        //std::cout << incoming[0] << " ";
        //std::cout << incoming[1] << " ";
        //std::cout << incoming[2] << " ";
        //std::cout << incoming[3] << "\n";
        //join any closing solid corners
        ActiveTail45* returnValue = 0;
        int returnCount = 0;
        for(int i = 0; i < 3; ++i) {
          //std::cout << i << "\n";
          if(counts[i] == -1) {
            //std::cout << "fixed i\n";
            for(int j = i + 1; j < 4; ++j) {
              //std::cout << j << "\n";
              if(counts[j]) {
                if(counts[j] == 1) {
                  //std::cout << "case1: " << i << " " << j << "\n";
                  //if a figure is closed it will be written out by this function to output
                  ActiveTail45::joinChains(point, tails[i], tails[j], true, output);
                  counts[i] = 0;
                  counts[j] = 0;
                  tails[i] = 0;
                  tails[j] = 0;
                }
                break;
              }
            }
          }
        }
        //find any pairs of incoming edges that need to create pair for leading solid
        //std::cout << "checking case2\n";
        for(int i = 0; i < 3; ++i) {
          //std::cout << i << "\n";
          if(incoming[i] == 1) {
            //std::cout << "fixed i\n";
            for(int j = i + 1; j < 4; ++j) {
              //std::cout << j << "\n";
              if(incoming[j]) {
                if(incoming[j] == -1) {
                  //std::cout << "case2: " << i << " " << j << "\n";
                  //std::cout << "creating active tail pair\n";
                  std::pair<ActiveTail45*, ActiveTail45*> tailPair =
                    ActiveTail45::createActiveTail45sAsPair(point, true, 0, fractureHoles_ != 0);
                  //tailPair.first->print();
                  //tailPair.second->print();
                  if(j == 3) {
                    //vertical active tail becomes return value
                    returnValue = tailPair.first;
                    returnCount = 1;
                  } else {
                    Vertex45 vertex(point, i -1, incoming[i]);
                    //std::cout << "new element " << j-1 << " " << -1 << "\n";
                    elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, -1), tailPair.first));
                  }
                  //std::cout << "new element " << i-1 << " " << 1 << "\n";
                  elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, i -1, 1), tailPair.second));
                  incoming[i] = 0;
                  incoming[j] = 0;
                }
                break;
              }
            }
          }
        }

        //find any active tail that needs to pass through to an incoming edge
        //we expect to find no more than two pass through

        //find pass through with solid on top
        //std::cout << "checking case 3\n";
        for(int i = 0; i < 4; ++i) {
          //std::cout << i << "\n";
          if(counts[i] != 0) {
            if(counts[i] == 1) {
              //std::cout << "fixed i\n";
              for(int j = 3; j >= 0; --j) {
                if(incoming[j] != 0) {
                  if(incoming[j] == 1) {
                    //std::cout << "case3: " << i << " " << j << "\n";
                    //tails[i]->print();
                    //pass through solid on top
                    tails[i]->pushPoint(point);
                    //std::cout << "after push\n";
                    if(j == 3) {
                      returnValue = tails[i];
                      returnCount = -1;
                    } else {
                      elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, incoming[j]), tails[i]));
                    }
                    tails[i] = 0;
                    counts[i] = 0;
                    incoming[j] = 0;
                  }
                  break;
                }
              }
            }
            break;
          }
        }
        //std::cout << "checking case 4\n";
        //find pass through with solid on bottom
        for(int i = 3; i >= 0; --i) {
          if(counts[i] != 0) {
            if(counts[i] == -1) {
              for(int j = 0; j < 4; ++j) {
                if(incoming[j] != 0) {
                  if(incoming[j] == -1) {
                    //std::cout << "case4: " << i << " " << j << "\n";
                    //pass through solid on bottom
                    tails[i]->pushPoint(point);
                    if(j == 3) {
                      returnValue = tails[i];
                      returnCount = 1;
                    } else {
                      //std::cout << "new element " << j-1 << " " << incoming[j] << "\n";
                      elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, incoming[j]), tails[i]));
                    }
                    tails[i] = 0;
                    counts[i] = 0;
                    incoming[j] = 0;
                  }
                  break;
                }
              }
            }
            break;
          }
        }

        //find the end of a hole or the beginning of a hole

        //find end of a hole
        for(int i = 0; i < 3; ++i) {
          if(counts[i] != 0) {
            for(int j = i+1; j < 4; ++j) {
              if(counts[j] != 0) {
                //std::cout << "case5: " << i << " " << j << "\n";
                //we are ending a hole and may potentially close a figure and have to handle the hole
                returnValue = ActiveTail45::joinChains(point, tails[i], tails[j], false, output);
                tails[i] = 0;
                tails[j] = 0;
                counts[i] = 0;
                counts[j] = 0;
                break;
              }
            }
            break;
          }
        }
        //find beginning of a hole
        for(int i = 0; i < 3; ++i) {
          if(incoming[i] != 0) {
            for(int j = i+1; j < 4; ++j) {
              if(incoming[j] != 0) {
                //std::cout << "case6: " << i << " " << j << "\n";
                //we are beginning a empty space
                ActiveTail45* holep = 0;
                if(counts[3] == 0) holep = tails[3];
                std::pair<ActiveTail45*, ActiveTail45*> tailPair =
                  ActiveTail45::createActiveTail45sAsPair(point, false, holep, fractureHoles_ != 0);
                if(j == 3) {
                  returnValue = tailPair.first;
                  returnCount = -1;
                } else {
                  //std::cout << "new element " << j-1 << " " << incoming[j] << "\n";
                  elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, incoming[j]), tailPair.first));
                }
                //std::cout << "new element " << i-1 << " " << incoming[i] << "\n";
                elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, i -1, incoming[i]), tailPair.second));
                incoming[i] = 0;
                incoming[j] = 0;
                break;
              }
            }
            break;
          }
        }
        //assert that tails, counts and incoming are all null
        return std::pair<int, ActiveTail45*>(returnCount, returnValue);
      }

      template <class cT, class iT>
      inline iT processEvent_(cT& output, iT inputBegin, iT inputEnd) {
        //std::cout << "processEvent_\n";
        justBefore_ = true;
        //collect up all elements from the tree that are at the y
        //values of events in the input queue
        //create vector of new elements to add into tree
        ActiveTail45* verticalTail = 0;
        int verticalCount = 0;
        iT currentIter = inputBegin;
        std::vector<iterator> elementIters;
        std::vector<std::pair<Vertex45, ActiveTail45*> > elements;
        while(currentIter != inputEnd && currentIter->pt.x() == x_) {
          //std::cout << "loop\n";
          Unit currentY = (*currentIter).pt.y();
          iterator iter = lookUp_(currentY);
          //int counts[4] = {0, 0, 0, 0};
          Vertex45Count counts;
          ActiveTail45* tails[4] = {0, 0, 0, verticalTail};
          //std::cout << "finding elements in tree\n";
          while(iter != scanData_.end() &&
                iter->first.evalAtX(x_) == currentY) {
            //std::cout << "loop2\n";
            elementIters.push_back(iter);
            int index = iter->first.rise + 1;
            //std::cout << index << " " << iter->first.count << "\n";
            counts[index] = iter->first.count;
            tails[index] = iter->second;
            ++iter;
          }
          //int incoming[4] = {0, 0, 0, 0};
          Vertex45Count incoming;
          //std::cout << "aggregating\n";
          do {
            //std::cout << "loop3\n";
            Vertex45Compact currentVertex(*currentIter);
            incoming += currentVertex.count;
            ++currentIter;
          } while(currentIter != inputEnd && currentIter->pt.y() == currentY &&
                  currentIter->pt.x() == x_);
          //now counts and tails have the data from the left and
          //incoming has the data from the right at this point
          //cancel out any end points
          //std::cout << counts[0] << " ";
          //std::cout << counts[1] << " ";
          //std::cout << counts[2] << " ";
          //std::cout << counts[3] << "\n";
          //std::cout << incoming[0] << " ";
          //std::cout << incoming[1] << " ";
          //std::cout << incoming[2] << " ";
          //std::cout << incoming[3] << "\n";
          if(verticalTail) {
            counts[3] = -verticalCount;
          }
          incoming[3] *= -1;
          for(unsigned int i = 0; i < 4; ++i) incoming[i] += counts[i];
          //std::cout << "calling processPoint_\n";
          std::pair<int, ActiveTail45*> result = processPoint_(output, elements, Point(x_, currentY), counts, tails, incoming);
          verticalCount = result.first;
          verticalTail = result.second;
          //if(verticalTail) std::cout << "have vertical tail\n";
          //std::cout << "verticalCount: " << verticalCount << "\n";
          if(verticalTail && !verticalCount) {
            //we got a hole out of the point we just processed
            //iter is still at the next y element above the current y value in the tree
            //std::cout << "checking whether ot handle hole\n";
            if(currentIter == inputEnd ||
               currentIter->pt.x() != x_ ||
               currentIter->pt.y() >= iter->first.evalAtX(x_)) {
              //std::cout << "handle hole here\n";
              if(fractureHoles_) {
                //std::cout << "fracture hole here\n";
                //we need to handle the hole now and not at the next input vertex
                ActiveTail45* at = iter->second;
                Point point(x_, iter->first.evalAtX(x_));
                verticalTail->getOtherActiveTail()->pushPoint(point);
                iter->second = verticalTail->getOtherActiveTail();
                at->pushPoint(point);
                verticalTail->join(at);
                delete at;
                delete verticalTail;
                verticalTail = 0;
              } else {
                //std::cout << "push hole onto list\n";
                iter->second->addHole(verticalTail);
                verticalTail = 0;
              }
            }
          }
        }
        //std::cout << "erasing\n";
        //erase all elements from the tree
        for(typename std::vector<iterator>::iterator iter = elementIters.begin();
            iter != elementIters.end(); ++iter) {
          //std::cout << "erasing loop\n";
          scanData_.erase(*iter);
        }
        //switch comparison tie breaking policy
        justBefore_ = false;
        //add new elements into tree
        //std::cout << "inserting\n";
        for(typename std::vector<std::pair<Vertex45, ActiveTail45*> >::iterator iter = elements.begin();
            iter != elements.end(); ++iter) {
          //std::cout << "inserting loop\n";
          scanData_.insert(scanData_.end(), *iter);
        }
        //std::cout << "end processEvent\n";
        return currentIter;
      }

      inline iterator lookUp_(Unit y){
        //if just before then we need to look from 1 not -1
        return scanData_.lower_bound(Vertex45(Point(x_, y), -1+2*justBefore_, 0));
      }

    };

    template <typename stream_type>
    static inline bool testPolygon45FormationRect(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      Polygon45Formation pf(true);
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 10), 2, -1));
      data.push_back(Vertex45(Point(0, 10), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 2, -1));
      data.push_back(Vertex45(Point(10, 10), 2, 1));
      data.push_back(Vertex45(Point(10, 10), 0, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45FormationP1(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      Polygon45Formation pf(true);
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 1, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 10), 2, -1));
      data.push_back(Vertex45(Point(0, 10), 1, -1));
      data.push_back(Vertex45(Point(10, 10), 1, -1));
      data.push_back(Vertex45(Point(10, 10), 2, -1));
      data.push_back(Vertex45(Point(10, 20), 2, 1));
      data.push_back(Vertex45(Point(10, 20), 1, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }
    //polygon45set class

    template <typename stream_type>
    static inline bool testPolygon45FormationP2(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      Polygon45Formation pf(true);
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 1, -1));
      data.push_back(Vertex45(Point(10, 0), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 1, 1));
      data.push_back(Vertex45(Point(10, 10), 1, 1));
      data.push_back(Vertex45(Point(10, 10), 0, -1));
      data.push_back(Vertex45(Point(20, 10), 1, -1));
      data.push_back(Vertex45(Point(20, 10), 0, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }
    //polygon45set class

    template <typename stream_type>
    static inline bool testPolygon45FormationStar1(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      Polygon45Formation pf(true);
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      // result == 0 8 -1 1
      data.push_back(Vertex45(Point(0, 8), -1, 1));
      // result == 0 8 1 -1
      data.push_back(Vertex45(Point(0, 8), 1, -1));
      // result == 4 0 1 1
      data.push_back(Vertex45(Point(4, 0), 1, 1));
      // result == 4 0 2 1
      data.push_back(Vertex45(Point(4, 0), 2, 1));
      // result == 4 4 2 -1
      data.push_back(Vertex45(Point(4, 4), 2, -1));
      // result == 4 4 -1 -1
      data.push_back(Vertex45(Point(4, 4), -1, -1));
      // result == 4 12 1 1
      data.push_back(Vertex45(Point(4, 12), 1, 1));
      // result == 4 12 2 1
      data.push_back(Vertex45(Point(4, 12), 2, 1));
      // result == 4 16 2 -1
      data.push_back(Vertex45(Point(4, 16), 2, 1));
      // result == 4 16 -1 -1
      data.push_back(Vertex45(Point(4, 16), -1, -1));
      // result == 6 2 1 -1
      data.push_back(Vertex45(Point(6, 2), 1, -1));
      // result == 6 14 -1 1
      data.push_back(Vertex45(Point(6, 14), -1, 1));
      // result == 6 2 -1 1
      data.push_back(Vertex45(Point(6, 2), -1, 1));
      // result == 6 14 1 -1
      data.push_back(Vertex45(Point(6, 14), 1, -1));
      // result == 8 0 -1 -1
      data.push_back(Vertex45(Point(8, 0), -1, -1));
      // result == 8 0 2 -1
      data.push_back(Vertex45(Point(8, 0), 2, -1));
      // result == 8 4 2 1
      data.push_back(Vertex45(Point(8, 4), 2, 1));
      // result == 8 4 1 1
      data.push_back(Vertex45(Point(8, 4), 1, 1));
      // result == 8 12 -1 -1
      data.push_back(Vertex45(Point(8, 12), -1, -1));
      // result == 8 12 2 -1
      data.push_back(Vertex45(Point(8, 12), 2, -1));
      // result == 8 16 2 1
      data.push_back(Vertex45(Point(8, 16), 2, 1));
      // result == 8 16 1 1
      data.push_back(Vertex45(Point(8, 16), 1, 1));
      // result == 12 8 1 -1
      data.push_back(Vertex45(Point(12, 8), 1, -1));
      // result == 12 8 -1 1
      data.push_back(Vertex45(Point(12, 8), -1, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45FormationStar2(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      Polygon45Formation pf(true);
      std::vector<Polygon45> polys;
      Scan45 scan45;
      std::vector<Vertex45 > result;
      std::vector<Scan45Vertex> vertices;
      //is a Rectnagle(0, 0, 10, 10);
      Count2 count(1, 0);
      Count2 ncount(-1, 0);
      vertices.push_back(Scan45Vertex(Point(0,4), Scan45Count(Count2(0, 0), count, ncount, Count2(0, 0))));
      vertices.push_back(Scan45Vertex(Point(16,4), Scan45Count(count, ncount, Count2(0, 0), Count2(0, 0))));
      vertices.push_back(Scan45Vertex(Point(8,12), Scan45Count(ncount, Count2(0, 0), count, Count2(0, 0))));
      count = Count2(0, 1);
      ncount = count.invert();
      vertices.push_back(Scan45Vertex(Point(0,8), Scan45Count(count, ncount, Count2(0, 0), Count2(0, 0))));
      vertices.push_back(Scan45Vertex(Point(16,8), Scan45Count(Count2(0, 0), count, ncount, Count2(0, 0))));
      vertices.push_back(Scan45Vertex(Point(8,0), Scan45Count(ncount, Count2(0, 0), count, Count2(0, 0))));
      sortScan45Vector(vertices);
      stdcout << "scanning\n";
      scan45.scan(result, vertices.begin(), vertices.end());

      polygon_sort(result.begin(), result.end());
      pf.scan(polys, result.begin(), result.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45FormationStarHole1(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      Polygon45Formation pf(true);
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      // result == 0 8 -1 1
      data.push_back(Vertex45(Point(0, 8), -1, 1));
      // result == 0 8 1 -1
      data.push_back(Vertex45(Point(0, 8), 1, -1));
      // result == 4 0 1 1
      data.push_back(Vertex45(Point(4, 0), 1, 1));
      // result == 4 0 2 1
      data.push_back(Vertex45(Point(4, 0), 2, 1));
      // result == 4 4 2 -1
      data.push_back(Vertex45(Point(4, 4), 2, -1));
      // result == 4 4 -1 -1
      data.push_back(Vertex45(Point(4, 4), -1, -1));
      // result == 4 12 1 1
      data.push_back(Vertex45(Point(4, 12), 1, 1));
      // result == 4 12 2 1
      data.push_back(Vertex45(Point(4, 12), 2, 1));
      // result == 4 16 2 -1
      data.push_back(Vertex45(Point(4, 16), 2, 1));
      // result == 4 16 -1 -1
      data.push_back(Vertex45(Point(4, 16), -1, -1));
      // result == 6 2 1 -1
      data.push_back(Vertex45(Point(6, 2), 1, -1));
      // result == 6 14 -1 1
      data.push_back(Vertex45(Point(6, 14), -1, 1));
      // result == 6 2 -1 1
      data.push_back(Vertex45(Point(6, 2), -1, 1));
      // result == 6 14 1 -1
      data.push_back(Vertex45(Point(6, 14), 1, -1));
      // result == 8 0 -1 -1
      data.push_back(Vertex45(Point(8, 0), -1, -1));
      // result == 8 0 2 -1
      data.push_back(Vertex45(Point(8, 0), 2, -1));
      // result == 8 4 2 1
      data.push_back(Vertex45(Point(8, 4), 2, 1));
      // result == 8 4 1 1
      data.push_back(Vertex45(Point(8, 4), 1, 1));
      // result == 8 12 -1 -1
      data.push_back(Vertex45(Point(8, 12), -1, -1));
      // result == 8 12 2 -1
      data.push_back(Vertex45(Point(8, 12), 2, -1));
      // result == 8 16 2 1
      data.push_back(Vertex45(Point(8, 16), 2, 1));
      // result == 8 16 1 1
      data.push_back(Vertex45(Point(8, 16), 1, 1));
      // result == 12 8 1 -1
      data.push_back(Vertex45(Point(12, 8), 1, -1));
      // result == 12 8 -1 1
      data.push_back(Vertex45(Point(12, 8), -1, 1));

      data.push_back(Vertex45(Point(6, 4), 1, -1));
      data.push_back(Vertex45(Point(6, 4), 2, -1));
      data.push_back(Vertex45(Point(6, 8), -1, 1));
      data.push_back(Vertex45(Point(6, 8), 2, 1));
      data.push_back(Vertex45(Point(8, 6), -1, -1));
      data.push_back(Vertex45(Point(8, 6), 1, 1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45FormationStarHole2(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      Polygon45Formation pf(false);
      std::vector<Polygon45WithHoles> polys;
      std::vector<Vertex45> data;
      // result == 0 8 -1 1
      data.push_back(Vertex45(Point(0, 8), -1, 1));
      // result == 0 8 1 -1
      data.push_back(Vertex45(Point(0, 8), 1, -1));
      // result == 4 0 1 1
      data.push_back(Vertex45(Point(4, 0), 1, 1));
      // result == 4 0 2 1
      data.push_back(Vertex45(Point(4, 0), 2, 1));
      // result == 4 4 2 -1
      data.push_back(Vertex45(Point(4, 4), 2, -1));
      // result == 4 4 -1 -1
      data.push_back(Vertex45(Point(4, 4), -1, -1));
      // result == 4 12 1 1
      data.push_back(Vertex45(Point(4, 12), 1, 1));
      // result == 4 12 2 1
      data.push_back(Vertex45(Point(4, 12), 2, 1));
      // result == 4 16 2 -1
      data.push_back(Vertex45(Point(4, 16), 2, 1));
      // result == 4 16 -1 -1
      data.push_back(Vertex45(Point(4, 16), -1, -1));
      // result == 6 2 1 -1
      data.push_back(Vertex45(Point(6, 2), 1, -1));
      // result == 6 14 -1 1
      data.push_back(Vertex45(Point(6, 14), -1, 1));
      // result == 6 2 -1 1
      data.push_back(Vertex45(Point(6, 2), -1, 1));
      // result == 6 14 1 -1
      data.push_back(Vertex45(Point(6, 14), 1, -1));
      // result == 8 0 -1 -1
      data.push_back(Vertex45(Point(8, 0), -1, -1));
      // result == 8 0 2 -1
      data.push_back(Vertex45(Point(8, 0), 2, -1));
      // result == 8 4 2 1
      data.push_back(Vertex45(Point(8, 4), 2, 1));
      // result == 8 4 1 1
      data.push_back(Vertex45(Point(8, 4), 1, 1));
      // result == 8 12 -1 -1
      data.push_back(Vertex45(Point(8, 12), -1, -1));
      // result == 8 12 2 -1
      data.push_back(Vertex45(Point(8, 12), 2, -1));
      // result == 8 16 2 1
      data.push_back(Vertex45(Point(8, 16), 2, 1));
      // result == 8 16 1 1
      data.push_back(Vertex45(Point(8, 16), 1, 1));
      // result == 12 8 1 -1
      data.push_back(Vertex45(Point(12, 8), 1, -1));
      // result == 12 8 -1 1
      data.push_back(Vertex45(Point(12, 8), -1, 1));

      data.push_back(Vertex45(Point(6, 4), 1, -1));
      data.push_back(Vertex45(Point(6, 4), 2, -1));
      data.push_back(Vertex45(Point(6, 12), -1, 1));
      data.push_back(Vertex45(Point(6, 12), 2, 1));
      data.push_back(Vertex45(Point(10, 8), -1, -1));
      data.push_back(Vertex45(Point(10, 8), 1, 1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45Formation(stream_type& stdcout) {
      stdcout << "testing polygon formation\n";
      Polygon45Formation pf(false);
      std::vector<Polygon45WithHoles> polys;
      std::vector<Vertex45> data;

      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 100), 2, -1));
      data.push_back(Vertex45(Point(0, 100), 0, -1));
      data.push_back(Vertex45(Point(100, 0), 0, -1));
      data.push_back(Vertex45(Point(100, 0), 2, -1));
      data.push_back(Vertex45(Point(100, 100), 2, 1));
      data.push_back(Vertex45(Point(100, 100), 0, 1));

      data.push_back(Vertex45(Point(2, 2), 0, -1));
      data.push_back(Vertex45(Point(2, 2), 2, -1));
      data.push_back(Vertex45(Point(2, 10), 2, 1));
      data.push_back(Vertex45(Point(2, 10), 0, 1));
      data.push_back(Vertex45(Point(10, 2), 0, 1));
      data.push_back(Vertex45(Point(10, 2), 2, 1));
      data.push_back(Vertex45(Point(10, 10), 2, -1));
      data.push_back(Vertex45(Point(10, 10), 0, -1));

      data.push_back(Vertex45(Point(2, 12), 0, -1));
      data.push_back(Vertex45(Point(2, 12), 2, -1));
      data.push_back(Vertex45(Point(2, 22), 2, 1));
      data.push_back(Vertex45(Point(2, 22), 0, 1));
      data.push_back(Vertex45(Point(10, 12), 0, 1));
      data.push_back(Vertex45(Point(10, 12), 2, 1));
      data.push_back(Vertex45(Point(10, 22), 2, -1));
      data.push_back(Vertex45(Point(10, 22), 0, -1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon formation\n";
      return true;
    }


    class Polygon45Tiling {
    private:
      //definitions
      typedef std::map<Vertex45, ActiveTail45*, lessVertex45> Polygon45FormationData;
      typedef typename Polygon45FormationData::iterator iterator;
      typedef typename Polygon45FormationData::const_iterator const_iterator;

      //data
      Polygon45FormationData scanData_;
      Unit x_;
      int justBefore_;
    public:
      inline Polygon45Tiling() : scanData_(), x_((std::numeric_limits<Unit>::min)()), justBefore_(false) {
        lessVertex45 lessElm(&x_, &justBefore_);
        scanData_ = Polygon45FormationData(lessElm);
      }
      inline Polygon45Tiling(const Polygon45Tiling& that) :
        scanData_(), x_((std::numeric_limits<Unit>::min)()), justBefore_(false) { (*this) = that; }
      inline Polygon45Tiling& operator=(const Polygon45Tiling& that) {
        x_ = that.x_;
        justBefore_ = that.justBefore_;
        lessVertex45 lessElm(&x_, &justBefore_);
        scanData_ = Polygon45FormationData(lessElm);
        for(const_iterator itr = that.scanData_.begin(); itr != that.scanData_.end(); ++itr){
          scanData_.insert(scanData_.end(), *itr);
        }
        return *this;
      }

      //cT is an output container of Polygon45 or Polygon45WithHoles
      //iT is an iterator over Vertex45 elements
      //inputBegin - inputEnd is a range of sorted iT that represents
      //one or more scanline stops worth of data
      template <class cT, class iT>
      void scan(cT& output, iT inputBegin, iT inputEnd) {
        //std::cout << "1\n";
        while(inputBegin != inputEnd) {
          //std::cout << "2\n";
          x_ = (*inputBegin).pt.x();
          //std::cout << "SCAN FORMATION " << x_ << "\n";
          //std::cout << "x_ = " << x_ << "\n";
          //std::cout << "scan line size: " << scanData_.size() << "\n";
          inputBegin = processEvent_(output, inputBegin, inputEnd);
        }
      }

    private:
      //functions

      inline void getVerticalPair_(std::pair<ActiveTail45*, ActiveTail45*>& verticalPair,
                                   iterator previter) {
        ActiveTail45* iterTail = (*previter).second;
        Point prevPoint(x_, previter->first.evalAtX(x_));
        iterTail->pushPoint(prevPoint);
        std::pair<ActiveTail45*, ActiveTail45*> tailPair =
          ActiveTail45::createActiveTail45sAsPair(prevPoint, true, 0, false);
        verticalPair.first = iterTail;
        verticalPair.second = tailPair.first;
        (*previter).second = tailPair.second;
      }

      template <class cT, class cT2>
      inline std::pair<int, ActiveTail45*> processPoint_(cT& output, cT2& elements,
                                                         std::pair<ActiveTail45*, ActiveTail45*>& verticalPair,
                                                         iterator previter, Point point,
                                                         Vertex45Count& counts, ActiveTail45** tails, Vertex45Count& incoming) {
        //std::cout << point << "\n";
        //std::cout << counts[0] << " ";
        //std::cout << counts[1] << " ";
        //std::cout << counts[2] << " ";
        //std::cout << counts[3] << "\n";
        //std::cout << incoming[0] << " ";
        //std::cout << incoming[1] << " ";
        //std::cout << incoming[2] << " ";
        //std::cout << incoming[3] << "\n";
        //join any closing solid corners
        ActiveTail45* returnValue = 0;
        std::pair<ActiveTail45*, ActiveTail45*> verticalPairOut;
        verticalPairOut.first = 0;
        verticalPairOut.second = 0;
        int returnCount = 0;
        for(int i = 0; i < 3; ++i) {
          //std::cout << i << "\n";
          if(counts[i] == -1) {
            //std::cout << "fixed i\n";
            for(int j = i + 1; j < 4; ++j) {
              //std::cout << j << "\n";
              if(counts[j]) {
                if(counts[j] == 1) {
                  //std::cout << "case1: " << i << " " << j << "\n";
                  //if a figure is closed it will be written out by this function to output
                  ActiveTail45::joinChains(point, tails[i], tails[j], true, output);
                  counts[i] = 0;
                  counts[j] = 0;
                  tails[i] = 0;
                  tails[j] = 0;
                }
                break;
              }
            }
          }
        }
        //find any pairs of incoming edges that need to create pair for leading solid
        //std::cout << "checking case2\n";
        for(int i = 0; i < 3; ++i) {
          //std::cout << i << "\n";
          if(incoming[i] == 1) {
            //std::cout << "fixed i\n";
            for(int j = i + 1; j < 4; ++j) {
              //std::cout << j << "\n";
              if(incoming[j]) {
                if(incoming[j] == -1) {
                  //std::cout << "case2: " << i << " " << j << "\n";
                  //std::cout << "creating active tail pair\n";
                  std::pair<ActiveTail45*, ActiveTail45*> tailPair =
                    ActiveTail45::createActiveTail45sAsPair(point, true, 0, false);
                  //tailPair.first->print();
                  //tailPair.second->print();
                  if(j == 3) {
                    //vertical active tail becomes return value
                    returnValue = tailPair.first;
                    returnCount = 1;
                  } else {
                    Vertex45 vertex(point, i -1, incoming[i]);
                    //std::cout << "new element " << j-1 << " " << -1 << "\n";
                    elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, -1), tailPair.first));
                  }
                  //std::cout << "new element " << i-1 << " " << 1 << "\n";
                  elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, i -1, 1), tailPair.second));
                  incoming[i] = 0;
                  incoming[j] = 0;
                }
                break;
              }
            }
          }
        }

        //find any active tail that needs to pass through to an incoming edge
        //we expect to find no more than two pass through

        //find pass through with solid on top
        //std::cout << "checking case 3\n";
        for(int i = 0; i < 4; ++i) {
          //std::cout << i << "\n";
          if(counts[i] != 0) {
            if(counts[i] == 1) {
              //std::cout << "fixed i\n";
              for(int j = 3; j >= 0; --j) {
                if(incoming[j] != 0) {
                  if(incoming[j] == 1) {
                    //std::cout << "case3: " << i << " " << j << "\n";
                    //tails[i]->print();
                    //pass through solid on top
                    if(i != 3)
                      tails[i]->pushPoint(point);
                    //std::cout << "after push\n";
                    if(j == 3) {
                      returnValue = tails[i];
                      returnCount = -1;
                    } else {
                      verticalPairOut.first = tails[i];
                      std::pair<ActiveTail45*, ActiveTail45*> tailPair =
                        ActiveTail45::createActiveTail45sAsPair(point, true, 0, false);
                      verticalPairOut.second = tailPair.first;
                      elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, incoming[j]),
                                                                            tailPair.second));
                    }
                    tails[i] = 0;
                    counts[i] = 0;
                    incoming[j] = 0;
                  }
                  break;
                }
              }
            }
            break;
          }
        }
        //std::cout << "checking case 4\n";
        //find pass through with solid on bottom
        for(int i = 3; i >= 0; --i) {
          if(counts[i] != 0) {
            if(counts[i] == -1) {
              for(int j = 0; j < 4; ++j) {
                if(incoming[j] != 0) {
                  if(incoming[j] == -1) {
                    //std::cout << "case4: " << i << " " << j << "\n";
                    //pass through solid on bottom
                    if(i == 3) {
                      //std::cout << "new element " << j-1 << " " << incoming[j] << "\n";
                      if(j == 3) {
                        returnValue = tails[i];
                        returnCount = 1;
                      } else {
                        tails[i]->pushPoint(point);
                        elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, incoming[j]), tails[i]));
                      }
                    } else if(j == 3) {
                      if(verticalPair.first == 0) {
                        getVerticalPair_(verticalPair, previter);
                      }
                      ActiveTail45::joinChains(point, tails[i], verticalPair.first, true, output);
                      returnValue = verticalPair.second;
                      returnCount = 1;
                    } else {
                      if(verticalPair.first == 0) {
                        getVerticalPair_(verticalPair, previter);
                      }
                      ActiveTail45::joinChains(point, tails[i], verticalPair.first, true, output);
                      verticalPair.second->pushPoint(point);
                      elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, incoming[j]),
                                                                            verticalPair.second));
                    }
                    tails[i] = 0;
                    counts[i] = 0;
                    incoming[j] = 0;
                  }
                  break;
                }
              }
            }
            break;
          }
        }

        //find the end of a hole or the beginning of a hole

        //find end of a hole
        for(int i = 0; i < 3; ++i) {
          if(counts[i] != 0) {
            for(int j = i+1; j < 4; ++j) {
              if(counts[j] != 0) {
                //std::cout << "case5: " << i << " " << j << "\n";
                //we are ending a hole and may potentially close a figure and have to handle the hole
                tails[i]->pushPoint(point);
                verticalPairOut.first = tails[i];
                if(j == 3) {
                  verticalPairOut.second = tails[j];
                } else {
                  if(verticalPair.first == 0) {
                    getVerticalPair_(verticalPair, previter);
                  }
                  ActiveTail45::joinChains(point, tails[j], verticalPair.first, true, output);
                  verticalPairOut.second = verticalPair.second;
                }
                tails[i] = 0;
                tails[j] = 0;
                counts[i] = 0;
                counts[j] = 0;
                break;
              }
            }
            break;
          }
        }
        //find beginning of a hole
        for(int i = 0; i < 3; ++i) {
          if(incoming[i] != 0) {
            for(int j = i+1; j < 4; ++j) {
              if(incoming[j] != 0) {
                //std::cout << "case6: " << i << " " << j << "\n";
                //we are beginning a empty space
                if(verticalPair.first == 0) {
                  getVerticalPair_(verticalPair, previter);
                }
                verticalPair.second->pushPoint(point);
                if(j == 3) {
                  returnValue = verticalPair.first;
                  returnCount = -1;
                } else {
                  std::pair<ActiveTail45*, ActiveTail45*> tailPair =
                    ActiveTail45::createActiveTail45sAsPair(point, true, 0, false);
                  //std::cout << "new element " << j-1 << " " << incoming[j] << "\n";
                  elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, j -1, incoming[j]), tailPair.second));
                  verticalPairOut.second = tailPair.first;
                  verticalPairOut.first = verticalPair.first;
                }
                //std::cout << "new element " << i-1 << " " << incoming[i] << "\n";
                elements.push_back(std::pair<Vertex45, ActiveTail45*>(Vertex45(point, i -1, incoming[i]), verticalPair.second));
                incoming[i] = 0;
                incoming[j] = 0;
                break;
              }
            }
            break;
          }
        }
        verticalPair = verticalPairOut;
        //assert that verticalPair is either both null, or neither null
        //assert that returnValue is null if verticalPair is not null
        //assert that tails, counts and incoming are all null
        return std::pair<int, ActiveTail45*>(returnCount, returnValue);
      }

      template <class cT, class iT>
      inline iT processEvent_(cT& output, iT inputBegin, iT inputEnd) {
        //std::cout << "processEvent_\n";
        justBefore_ = true;
        //collect up all elements from the tree that are at the y
        //values of events in the input queue
        //create vector of new elements to add into tree
        ActiveTail45* verticalTail = 0;
        std::pair<ActiveTail45*, ActiveTail45*> verticalPair;
        verticalPair.first = 0;
        verticalPair.second = 0;
        int verticalCount = 0;
        iT currentIter = inputBegin;
        std::vector<iterator> elementIters;
        std::vector<std::pair<Vertex45, ActiveTail45*> > elements;
        while(currentIter != inputEnd && currentIter->pt.x() == x_) {
          //std::cout << "loop\n";
          Unit currentY = (*currentIter).pt.y();
          iterator iter = lookUp_(currentY);
          //int counts[4] = {0, 0, 0, 0};
          Vertex45Count counts;
          ActiveTail45* tails[4] = {0, 0, 0, verticalTail};
          //std::cout << "finding elements in tree\n";
          iterator previter = iter;
          if(previter != scanData_.end() &&
             previter->first.evalAtX(x_) >= currentY &&
             previter != scanData_.begin())
            --previter;
          while(iter != scanData_.end() &&
                iter->first.evalAtX(x_) == currentY) {
            //std::cout << "loop2\n";
            elementIters.push_back(iter);
            int index = iter->first.rise + 1;
            //std::cout << index << " " << iter->first.count << "\n";
            counts[index] = iter->first.count;
            tails[index] = iter->second;
            ++iter;
          }
          //int incoming[4] = {0, 0, 0, 0};
          Vertex45Count incoming;
          //std::cout << "aggregating\n";
          do {
            //std::cout << "loop3\n";
            Vertex45Compact currentVertex(*currentIter);
            incoming += currentVertex.count;
            ++currentIter;
          } while(currentIter != inputEnd && currentIter->pt.y() == currentY &&
                  currentIter->pt.x() == x_);
          //now counts and tails have the data from the left and
          //incoming has the data from the right at this point
          //cancel out any end points
          //std::cout << counts[0] << " ";
          //std::cout << counts[1] << " ";
          //std::cout << counts[2] << " ";
          //std::cout << counts[3] << "\n";
          //std::cout << incoming[0] << " ";
          //std::cout << incoming[1] << " ";
          //std::cout << incoming[2] << " ";
          //std::cout << incoming[3] << "\n";
          if(verticalTail) {
            counts[3] = -verticalCount;
          }
          incoming[3] *= -1;
          for(unsigned int i = 0; i < 4; ++i) incoming[i] += counts[i];
          //std::cout << "calling processPoint_\n";
          std::pair<int, ActiveTail45*> result = processPoint_(output, elements, verticalPair, previter,
                                                               Point(x_, currentY), counts, tails, incoming);
          verticalCount = result.first;
          verticalTail = result.second;
          if(verticalPair.first != 0 && iter != scanData_.end() &&
             (currentIter == inputEnd || currentIter->pt.x() != x_ ||
              currentIter->pt.y() > (*iter).first.evalAtX(x_))) {
            //splice vertical pair into edge above
            ActiveTail45* tailabove = (*iter).second;
            Point point(x_, (*iter).first.evalAtX(x_));
            verticalPair.second->pushPoint(point);
            ActiveTail45::joinChains(point, tailabove, verticalPair.first, true, output);
            (*iter).second = verticalPair.second;
            verticalPair.first = 0;
            verticalPair.second = 0;
          }
        }
        //std::cout << "erasing\n";
        //erase all elements from the tree
        for(typename std::vector<iterator>::iterator iter = elementIters.begin();
            iter != elementIters.end(); ++iter) {
          //std::cout << "erasing loop\n";
          scanData_.erase(*iter);
        }
        //switch comparison tie breaking policy
        justBefore_ = false;
        //add new elements into tree
        //std::cout << "inserting\n";
        for(typename std::vector<std::pair<Vertex45, ActiveTail45*> >::iterator iter = elements.begin();
            iter != elements.end(); ++iter) {
          //std::cout << "inserting loop\n";
          scanData_.insert(scanData_.end(), *iter);
        }
        //std::cout << "end processEvent\n";
        return currentIter;
      }

      inline iterator lookUp_(Unit y){
        //if just before then we need to look from 1 not -1
        return scanData_.lower_bound(Vertex45(Point(x_, y), -1+2*justBefore_, 0));
      }

    };

    template <typename stream_type>
    static inline bool testPolygon45TilingRect(stream_type& stdcout) {
      stdcout << "testing polygon tiling\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 10), 2, -1));
      data.push_back(Vertex45(Point(0, 10), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 2, -1));
      data.push_back(Vertex45(Point(10, 10), 2, 1));
      data.push_back(Vertex45(Point(10, 10), 0, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingP1(stream_type& stdcout) {
      stdcout << "testing polygon tiling\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 1, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 10), 2, -1));
      data.push_back(Vertex45(Point(0, 10), 1, -1));
      data.push_back(Vertex45(Point(10, 10), 1, -1));
      data.push_back(Vertex45(Point(10, 10), 2, -1));
      data.push_back(Vertex45(Point(10, 20), 2, 1));
      data.push_back(Vertex45(Point(10, 20), 1, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingP2(stream_type& stdcout) {
      stdcout << "testing polygon tiling\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 1, -1));
      data.push_back(Vertex45(Point(10, 0), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 1, 1));
      data.push_back(Vertex45(Point(10, 10), 1, 1));
      data.push_back(Vertex45(Point(10, 10), 0, -1));
      data.push_back(Vertex45(Point(20, 10), 1, -1));
      data.push_back(Vertex45(Point(20, 10), 0, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingP3(stream_type& stdcout) {
      stdcout << "testing polygon tiling\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 10), 2, -1));
      data.push_back(Vertex45(Point(0, 10), 0, -1));
      data.push_back(Vertex45(Point(20, 0), 0, -1));
      data.push_back(Vertex45(Point(20, 0), 2, -1));
      data.push_back(Vertex45(Point(10, 10), 1, -1));
      data.push_back(Vertex45(Point(10, 10), 0, 1));
      data.push_back(Vertex45(Point(20, 20), 1, 1));
      data.push_back(Vertex45(Point(20, 20), 2, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingP4(stream_type& stdcout) {
      stdcout << "testing polygon tiling p4\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 10), 2, -1));
      data.push_back(Vertex45(Point(0, 10), 0, -1));
      data.push_back(Vertex45(Point(10, 0), -1, 1));
      data.push_back(Vertex45(Point(10, 0), 0, -1));
      data.push_back(Vertex45(Point(20, 10), 2, 1));
      data.push_back(Vertex45(Point(20, 10), 0, 1));
      data.push_back(Vertex45(Point(20, -10), -1, -1));
      data.push_back(Vertex45(Point(20, -10), 2, -1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingP5(stream_type& stdcout) {
      stdcout << "testing polygon tiling P5\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 10), 2, -1));
      data.push_back(Vertex45(Point(0, 10), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 2, -1));
      data.push_back(Vertex45(Point(10, 10), 2, 1));
      data.push_back(Vertex45(Point(10, 10), 0, 1));

      data.push_back(Vertex45(Point(1, 1), 0, -1));
      data.push_back(Vertex45(Point(1, 1), 1, 1));
      data.push_back(Vertex45(Point(2, 1), 0, 1));
      data.push_back(Vertex45(Point(2, 1), 1, -1));
      data.push_back(Vertex45(Point(2, 2), 1, -1));
      data.push_back(Vertex45(Point(2, 2), 0, 1));
      data.push_back(Vertex45(Point(3, 2), 1, 1));
      data.push_back(Vertex45(Point(3, 2), 0, -1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingP6(stream_type& stdcout) {
      stdcout << "testing polygon tiling P6\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 10), 2, -1));
      data.push_back(Vertex45(Point(0, 10), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 0, -1));
      data.push_back(Vertex45(Point(10, 0), 2, -1));
      data.push_back(Vertex45(Point(10, 10), 2, 1));
      data.push_back(Vertex45(Point(10, 10), 0, 1));

      data.push_back(Vertex45(Point(1, 1), 0, -1));
      data.push_back(Vertex45(Point(1, 1), 2, -1));
      data.push_back(Vertex45(Point(1, 2), 2, 1));
      data.push_back(Vertex45(Point(1, 2), 0, 1));
      data.push_back(Vertex45(Point(2, 1), 0, 1));
      data.push_back(Vertex45(Point(2, 1), 2, 1));
      data.push_back(Vertex45(Point(2, 2), 2, -1));
      data.push_back(Vertex45(Point(2, 2), 0, -1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingStar1(stream_type& stdcout) {
      stdcout << "testing polygon tiling star1\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      // result == 0 8 -1 1
      data.push_back(Vertex45(Point(0, 8), -1, 1));
      // result == 0 8 1 -1
      data.push_back(Vertex45(Point(0, 8), 1, -1));
      // result == 4 0 1 1
      data.push_back(Vertex45(Point(4, 0), 1, 1));
      // result == 4 0 2 1
      data.push_back(Vertex45(Point(4, 0), 2, 1));
      // result == 4 4 2 -1
      data.push_back(Vertex45(Point(4, 4), 2, -1));
      // result == 4 4 -1 -1
      data.push_back(Vertex45(Point(4, 4), -1, -1));
      // result == 4 12 1 1
      data.push_back(Vertex45(Point(4, 12), 1, 1));
      // result == 4 12 2 1
      data.push_back(Vertex45(Point(4, 12), 2, 1));
      // result == 4 16 2 -1
      data.push_back(Vertex45(Point(4, 16), 2, 1));
      // result == 4 16 -1 -1
      data.push_back(Vertex45(Point(4, 16), -1, -1));
      // result == 6 2 1 -1
      data.push_back(Vertex45(Point(6, 2), 1, -1));
      // result == 6 14 -1 1
      data.push_back(Vertex45(Point(6, 14), -1, 1));
      // result == 6 2 -1 1
      data.push_back(Vertex45(Point(6, 2), -1, 1));
      // result == 6 14 1 -1
      data.push_back(Vertex45(Point(6, 14), 1, -1));
      // result == 8 0 -1 -1
      data.push_back(Vertex45(Point(8, 0), -1, -1));
      // result == 8 0 2 -1
      data.push_back(Vertex45(Point(8, 0), 2, -1));
      // result == 8 4 2 1
      data.push_back(Vertex45(Point(8, 4), 2, 1));
      // result == 8 4 1 1
      data.push_back(Vertex45(Point(8, 4), 1, 1));
      // result == 8 12 -1 -1
      data.push_back(Vertex45(Point(8, 12), -1, -1));
      // result == 8 12 2 -1
      data.push_back(Vertex45(Point(8, 12), 2, -1));
      // result == 8 16 2 1
      data.push_back(Vertex45(Point(8, 16), 2, 1));
      // result == 8 16 1 1
      data.push_back(Vertex45(Point(8, 16), 1, 1));
      // result == 12 8 1 -1
      data.push_back(Vertex45(Point(12, 8), 1, -1));
      // result == 12 8 -1 1
      data.push_back(Vertex45(Point(12, 8), -1, 1));
      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingStar2(stream_type& stdcout) {
      stdcout << "testing polygon tiling\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;

      Scan45 scan45;
      std::vector<Vertex45 > result;
      std::vector<Scan45Vertex> vertices;
      //is a Rectnagle(0, 0, 10, 10);
      Count2 count(1, 0);
      Count2 ncount(-1, 0);
      vertices.push_back(Scan45Vertex(Point(0,4), Scan45Count(Count2(0, 0), count, ncount, Count2(0, 0))));
      vertices.push_back(Scan45Vertex(Point(16,4), Scan45Count(count, ncount, Count2(0, 0), Count2(0, 0))));
      vertices.push_back(Scan45Vertex(Point(8,12), Scan45Count(ncount, Count2(0, 0), count, Count2(0, 0))));
      count = Count2(0, 1);
      ncount = count.invert();
      vertices.push_back(Scan45Vertex(Point(0,8), Scan45Count(count, ncount, Count2(0, 0), Count2(0, 0))));
      vertices.push_back(Scan45Vertex(Point(16,8), Scan45Count(Count2(0, 0), count, ncount, Count2(0, 0))));
      vertices.push_back(Scan45Vertex(Point(8,0), Scan45Count(ncount, Count2(0, 0), count, Count2(0, 0))));
      sortScan45Vector(vertices);
      stdcout << "scanning\n";
      scan45.scan(result, vertices.begin(), vertices.end());

      polygon_sort(result.begin(), result.end());
      pf.scan(polys, result.begin(), result.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingStarHole1(stream_type& stdcout) {
      stdcout << "testing polygon tiling star hole 1\n";
      Polygon45Tiling pf;
      std::vector<Polygon45> polys;
      std::vector<Vertex45> data;
      // result == 0 8 -1 1
      data.push_back(Vertex45(Point(0, 8), -1, 1));
      // result == 0 8 1 -1
      data.push_back(Vertex45(Point(0, 8), 1, -1));
      // result == 4 0 1 1
      data.push_back(Vertex45(Point(4, 0), 1, 1));
      // result == 4 0 2 1
      data.push_back(Vertex45(Point(4, 0), 2, 1));
      // result == 4 4 2 -1
      data.push_back(Vertex45(Point(4, 4), 2, -1));
      // result == 4 4 -1 -1
      data.push_back(Vertex45(Point(4, 4), -1, -1));
      // result == 4 12 1 1
      data.push_back(Vertex45(Point(4, 12), 1, 1));
      // result == 4 12 2 1
      data.push_back(Vertex45(Point(4, 12), 2, 1));
      // result == 4 16 2 -1
      data.push_back(Vertex45(Point(4, 16), 2, 1));
      // result == 4 16 -1 -1
      data.push_back(Vertex45(Point(4, 16), -1, -1));
      // result == 6 2 1 -1
      data.push_back(Vertex45(Point(6, 2), 1, -1));
      // result == 6 14 -1 1
      data.push_back(Vertex45(Point(6, 14), -1, 1));
      // result == 6 2 -1 1
      data.push_back(Vertex45(Point(6, 2), -1, 1));
      // result == 6 14 1 -1
      data.push_back(Vertex45(Point(6, 14), 1, -1));
      // result == 8 0 -1 -1
      data.push_back(Vertex45(Point(8, 0), -1, -1));
      // result == 8 0 2 -1
      data.push_back(Vertex45(Point(8, 0), 2, -1));
      // result == 8 4 2 1
      data.push_back(Vertex45(Point(8, 4), 2, 1));
      // result == 8 4 1 1
      data.push_back(Vertex45(Point(8, 4), 1, 1));
      // result == 8 12 -1 -1
      data.push_back(Vertex45(Point(8, 12), -1, -1));
      // result == 8 12 2 -1
      data.push_back(Vertex45(Point(8, 12), 2, -1));
      // result == 8 16 2 1
      data.push_back(Vertex45(Point(8, 16), 2, 1));
      // result == 8 16 1 1
      data.push_back(Vertex45(Point(8, 16), 1, 1));
      // result == 12 8 1 -1
      data.push_back(Vertex45(Point(12, 8), 1, -1));
      // result == 12 8 -1 1
      data.push_back(Vertex45(Point(12, 8), -1, 1));

      data.push_back(Vertex45(Point(6, 4), 1, -1));
      data.push_back(Vertex45(Point(6, 4), 2, -1));
      data.push_back(Vertex45(Point(6, 8), -1, 1));
      data.push_back(Vertex45(Point(6, 8), 2, 1));
      data.push_back(Vertex45(Point(8, 6), -1, -1));
      data.push_back(Vertex45(Point(8, 6), 1, 1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45TilingStarHole2(stream_type& stdcout) {
      stdcout << "testing polygon tiling star hole 2\n";
      Polygon45Tiling pf;
      std::vector<Polygon45WithHoles> polys;
      std::vector<Vertex45> data;
      // result == 0 8 -1 1
      data.push_back(Vertex45(Point(0, 8), -1, 1));
      // result == 0 8 1 -1
      data.push_back(Vertex45(Point(0, 8), 1, -1));
      // result == 4 0 1 1
      data.push_back(Vertex45(Point(4, 0), 1, 1));
      // result == 4 0 2 1
      data.push_back(Vertex45(Point(4, 0), 2, 1));
      // result == 4 4 2 -1
      data.push_back(Vertex45(Point(4, 4), 2, -1));
      // result == 4 4 -1 -1
      data.push_back(Vertex45(Point(4, 4), -1, -1));
      // result == 4 12 1 1
      data.push_back(Vertex45(Point(4, 12), 1, 1));
      // result == 4 12 2 1
      data.push_back(Vertex45(Point(4, 12), 2, 1));
      // result == 4 16 2 -1
      data.push_back(Vertex45(Point(4, 16), 2, 1));
      // result == 4 16 -1 -1
      data.push_back(Vertex45(Point(4, 16), -1, -1));
      // result == 6 2 1 -1
      data.push_back(Vertex45(Point(6, 2), 1, -1));
      // result == 6 14 -1 1
      data.push_back(Vertex45(Point(6, 14), -1, 1));
      // result == 6 2 -1 1
      data.push_back(Vertex45(Point(6, 2), -1, 1));
      // result == 6 14 1 -1
      data.push_back(Vertex45(Point(6, 14), 1, -1));
      // result == 8 0 -1 -1
      data.push_back(Vertex45(Point(8, 0), -1, -1));
      // result == 8 0 2 -1
      data.push_back(Vertex45(Point(8, 0), 2, -1));
      // result == 8 4 2 1
      data.push_back(Vertex45(Point(8, 4), 2, 1));
      // result == 8 4 1 1
      data.push_back(Vertex45(Point(8, 4), 1, 1));
      // result == 8 12 -1 -1
      data.push_back(Vertex45(Point(8, 12), -1, -1));
      // result == 8 12 2 -1
      data.push_back(Vertex45(Point(8, 12), 2, -1));
      // result == 8 16 2 1
      data.push_back(Vertex45(Point(8, 16), 2, 1));
      // result == 8 16 1 1
      data.push_back(Vertex45(Point(8, 16), 1, 1));
      // result == 12 8 1 -1
      data.push_back(Vertex45(Point(12, 8), 1, -1));
      // result == 12 8 -1 1
      data.push_back(Vertex45(Point(12, 8), -1, 1));

      data.push_back(Vertex45(Point(6, 4), 1, -1));
      data.push_back(Vertex45(Point(6, 4), 2, -1));
      data.push_back(Vertex45(Point(6, 12), -1, 1));
      data.push_back(Vertex45(Point(6, 12), 2, 1));
      data.push_back(Vertex45(Point(10, 8), -1, -1));
      data.push_back(Vertex45(Point(10, 8), 1, 1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }

    template <typename stream_type>
    static inline bool testPolygon45Tiling(stream_type& stdcout) {
      stdcout << "testing polygon tiling\n";
      Polygon45Tiling pf;
      std::vector<Polygon45WithHoles> polys;
      std::vector<Vertex45> data;

      data.push_back(Vertex45(Point(0, 0), 0, 1));
      data.push_back(Vertex45(Point(0, 0), 2, 1));
      data.push_back(Vertex45(Point(0, 100), 2, -1));
      data.push_back(Vertex45(Point(0, 100), 0, -1));
      data.push_back(Vertex45(Point(100, 0), 0, -1));
      data.push_back(Vertex45(Point(100, 0), 2, -1));
      data.push_back(Vertex45(Point(100, 100), 2, 1));
      data.push_back(Vertex45(Point(100, 100), 0, 1));

      data.push_back(Vertex45(Point(2, 2), 0, -1));
      data.push_back(Vertex45(Point(2, 2), 2, -1));
      data.push_back(Vertex45(Point(2, 10), 2, 1));
      data.push_back(Vertex45(Point(2, 10), 0, 1));
      data.push_back(Vertex45(Point(10, 2), 0, 1));
      data.push_back(Vertex45(Point(10, 2), 2, 1));
      data.push_back(Vertex45(Point(10, 10), 2, -1));
      data.push_back(Vertex45(Point(10, 10), 0, -1));

      data.push_back(Vertex45(Point(2, 12), 0, -1));
      data.push_back(Vertex45(Point(2, 12), 2, -1));
      data.push_back(Vertex45(Point(2, 22), 2, 1));
      data.push_back(Vertex45(Point(2, 22), 0, 1));
      data.push_back(Vertex45(Point(10, 12), 0, 1));
      data.push_back(Vertex45(Point(10, 12), 2, 1));
      data.push_back(Vertex45(Point(10, 22), 2, -1));
      data.push_back(Vertex45(Point(10, 22), 0, -1));

      polygon_sort(data.begin(), data.end());
      pf.scan(polys, data.begin(), data.end());
      stdcout << "result size: " << polys.size() << "\n";
      for(std::size_t i = 0; i < polys.size(); ++i) {
        stdcout << polys[i] << "\n";
      }
      stdcout << "done testing polygon tiling\n";
      return true;
    }
  };

  template <typename Unit>
  class PolyLine45HoleData {
  public:
    typedef typename polygon_45_formation<Unit>::ActiveTail45 ActiveTail45;
    typedef typename ActiveTail45::iterator iterator;

    typedef polygon_45_concept geometry_type;
    typedef Unit coordinate_type;
    typedef point_data<Unit> Point;
    typedef Point point_type;
    //    typedef iterator_points_to_compact<iterator, Point> compact_iterator_type;
    typedef iterator iterator_type;
    typedef typename coordinate_traits<Unit>::area_type area_type;

    inline PolyLine45HoleData() : p_(0) {}
    inline PolyLine45HoleData(ActiveTail45* p) : p_(p) {}
    //use default copy and assign
    inline iterator begin() const { return p_->getTail()->begin(); }
    inline iterator end() const { return p_->getTail()->end(); }
    inline std::size_t size() const { return 0; }
  private:
    ActiveTail45* p_;
  };

  template <typename Unit>
  class PolyLine45PolygonData {
  public:
    typedef typename polygon_45_formation<Unit>::ActiveTail45 ActiveTail45;
    typedef typename ActiveTail45::iterator iterator;
    typedef PolyLine45HoleData<Unit> holeType;

    typedef polygon_45_with_holes_concept geometry_type;
    typedef Unit coordinate_type;
    typedef point_data<Unit> Point;
    typedef Point point_type;
    //    typedef iterator_points_to_compact<iterator, Point> compact_iterator_type;
    typedef iterator iterator_type;
    typedef holeType hole_type;
    typedef typename coordinate_traits<Unit>::area_type area_type;
    class iteratorHoles {
    private:
      typename ActiveTail45::iteratorHoles itr_;
    public:
      typedef PolyLine45HoleData<Unit> holeType;
      typedef holeType value_type;
      typedef std::forward_iterator_tag iterator_category;
      typedef std::ptrdiff_t difference_type;
      typedef const value_type* pointer; //immutable
      typedef const value_type& reference; //immutable
      inline iteratorHoles() : itr_() {}
      inline iteratorHoles(typename ActiveTail45::iteratorHoles itr) : itr_(itr) {}
      inline iteratorHoles(const iteratorHoles& that) : itr_(that.itr_) {}
      inline iteratorHoles& operator=(const iteratorHoles& that) {
        itr_ = that.itr_;
        return *this;
      }
      inline bool operator==(const iteratorHoles& that) { return itr_ == that.itr_; }
      inline bool operator!=(const iteratorHoles& that) { return itr_ != that.itr_; }
      inline iteratorHoles& operator++() {
        ++itr_;
        return *this;
      }
      inline const iteratorHoles operator++(int) {
        iteratorHoles tmp = *this;
        ++(*this);
        return tmp;
      }
      inline holeType operator*() {
        return *itr_;
      }
    };
    typedef iteratorHoles iterator_holes_type;


    inline PolyLine45PolygonData() : p_(0) {}
    inline PolyLine45PolygonData(ActiveTail45* p) : p_(p) {}
    //use default copy and assign
    inline iterator begin() const { return p_->getTail()->begin(); }
    inline iterator end() const { return p_->getTail()->end(); }
    inline iteratorHoles begin_holes() const { return iteratorHoles(p_->getHoles().begin()); }
    inline iteratorHoles end_holes() const { return iteratorHoles(p_->getHoles().end()); }
    inline ActiveTail45* yield() { return p_; }
    //stub out these four required functions that will not be used but are needed for the interface
    inline std::size_t size_holes() const { return 0; }
    inline std::size_t size() const { return 0; }
  private:
    ActiveTail45* p_;
  };

  template <typename T>
  struct PolyLineByConcept<T, polygon_45_with_holes_concept> { typedef PolyLine45PolygonData<T> type; };
  template <typename T>
  struct PolyLineByConcept<T, polygon_with_holes_concept> { typedef PolyLine45PolygonData<T> type; };
  template <typename T>
  struct PolyLineByConcept<T, polygon_45_concept> { typedef PolyLine45HoleData<T> type; };
  template <typename T>
  struct PolyLineByConcept<T, polygon_concept> { typedef PolyLine45HoleData<T> type; };

  template <typename T>
  struct geometry_concept<PolyLine45PolygonData<T> > { typedef polygon_45_with_holes_concept type; };
  template <typename T>
  struct geometry_concept<PolyLine45HoleData<T> > { typedef polygon_45_concept type; };

}
}
#endif

/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_MAX_COVER_HPP
#define BOOST_POLYGON_MAX_COVER_HPP
namespace boost { namespace polygon{

  template <typename Unit>
  struct MaxCover {
    typedef interval_data<Unit> Interval;
    typedef rectangle_data<Unit> Rectangle;

    class Node {
    private:
      std::vector<Node*> children_;
      std::set<Interval> tracedPaths_;
    public:
      Rectangle rect;
      Node() : children_(), tracedPaths_(), rect() {}
      Node(const Rectangle rectIn) : children_(), tracedPaths_(), rect(rectIn) {}
      typedef typename std::vector<Node*>::iterator iterator;
      inline iterator begin() { return children_.begin(); }
      inline iterator end() { return children_.end(); }
      inline void add(Node* child) { children_.push_back(child); }
      inline bool tracedPath(const Interval& ivl) const {
        return tracedPaths_.find(ivl) != tracedPaths_.end();
      }
      inline void addPath(const Interval& ivl) {
        tracedPaths_.insert(tracedPaths_.end(), ivl);
      }
    };

    typedef std::pair<std::pair<Unit, Interval>, Node* > EdgeAssociation;

    class lessEdgeAssociation {
    public:
      typedef const EdgeAssociation& first_argument_type;
      typedef const EdgeAssociation& second_argument_type;
      typedef bool result_type;
      inline lessEdgeAssociation() {}
      inline bool operator () (const EdgeAssociation& elem1, const EdgeAssociation& elem2) const {
        if(elem1.first.first < elem2.first.first) return true;
        if(elem1.first.first > elem2.first.first) return false;
        return elem1.first.second < elem2.first.second;
      }
    };

    template <class cT>
    static inline void getMaxCover(cT& outputContainer, Node* node, orientation_2d orient) {
      Interval rectIvl = node->rect.get(orient);
      if(node->tracedPath(rectIvl)) {
        return;
      }
      node->addPath(rectIvl);
      if(node->begin() == node->end()) {
        //std::cout << "WRITE OUT 3: " << node->rect << std::endl;
        outputContainer.push_back(copy_construct<typename cT::value_type, Rectangle>(node->rect));
        return;
      }
      bool writeOut = true;
      for(typename Node::iterator itr = node->begin(); itr != node->end(); ++itr) {
        getMaxCover(outputContainer, *itr, orient, node->rect); //get rectangles down path
        Interval nodeIvl = (*itr)->rect.get(orient);
        if(contains(nodeIvl, rectIvl, true)) writeOut = false;
      }
      if(writeOut) {
        //std::cout << "WRITE OUT 2: " << node->rect << std::endl;
        outputContainer.push_back(copy_construct<typename cT::value_type, Rectangle>(node->rect));
      }
    }

    struct stack_element {
      inline stack_element() :
        node(), rect(), itr() {}
      inline stack_element(Node* n,
                           const Rectangle& r,
                           typename Node::iterator i) :
        node(n), rect(r), itr(i) {}
      Node* node;
      Rectangle rect;
      typename Node::iterator itr;
    };

    template <class cT>
    static inline void getMaxCover(cT& outputContainer, Node* node, orientation_2d orient,
                                   Rectangle rect) {
      //std::cout << "New Root\n";
      std::vector<stack_element> stack;
      typename Node::iterator itr = node->begin();
      do {
        //std::cout << "LOOP\n";
        //std::cout << node->rect << std::endl;
        Interval rectIvl = rect.get(orient);
        Interval nodeIvl = node->rect.get(orient);
        bool iresult = intersect(rectIvl, nodeIvl, false);
        bool tresult = !node->tracedPath(rectIvl);
        //std::cout << (itr != node->end()) << " " << iresult << " " << tresult << std::endl;
        Rectangle nextRect1 = Rectangle(rectIvl, rectIvl);
        Unit low = rect.get(orient.get_perpendicular()).low();
        Unit high = node->rect.get(orient.get_perpendicular()).high();
        nextRect1.set(orient.get_perpendicular(), Interval(low, high));
        if(iresult && tresult) {
          node->addPath(rectIvl);
          bool writeOut = true;
          //check further visibility beyond this node
          for(typename Node::iterator itr2 = node->begin(); itr2 != node->end(); ++itr2) {
            Interval nodeIvl3 = (*itr2)->rect.get(orient);
            //if a child of this node can contain the interval then we can extend through
            if(contains(nodeIvl3, rectIvl, true)) writeOut = false;
            //std::cout << "child " << (*itr2)->rect << std::endl;
          }
          Rectangle nextRect2 = Rectangle(rectIvl, rectIvl);
          Unit low2 = rect.get(orient.get_perpendicular()).low();
          Unit high2 = node->rect.get(orient.get_perpendicular()).high();
          nextRect2.set(orient.get_perpendicular(), Interval(low2, high2));
          if(writeOut) {
            //std::cout << "write out " << nextRect << std::endl;
            outputContainer.push_back(copy_construct<typename cT::value_type, Rectangle>(nextRect2));
          } else {
            //std::cout << "suppress " << nextRect << std::endl;
          }
        }
        if(itr != node->end() && iresult && tresult) {
          //std::cout << "recurse into child\n";
          stack.push_back(stack_element(node, rect, itr));
          rect = nextRect1;
          node = *itr;
          itr = node->begin();
        } else {
          if(!stack.empty()) {
            //std::cout << "recurse out of child\n";
            node = stack.back().node;
            rect = stack.back().rect;
            itr = stack.back().itr;
            stack.pop_back();
          } else {
            //std::cout << "empty stack\n";
            //if there were no children of the root node
//             Rectangle nextRect = Rectangle(rectIvl, rectIvl);
//             Unit low = rect.get(orient.get_perpendicular()).low();
//             Unit high = node->rect.get(orient.get_perpendicular()).high();
//             nextRect.set(orient.get_perpendicular(), Interval(low, high));
//             outputContainer.push_back(copy_construct<typename cT::value_type, Rectangle>(nextRect));
          }
          //std::cout << "increment " << (itr != node->end()) << std::endl;
          if(itr != node->end()) {
            ++itr;
            if(itr != node->end()) {
              //std::cout << "recurse into next child.\n";
              stack.push_back(stack_element(node, rect, itr));
              Interval rectIvl2 = rect.get(orient);
              Interval nodeIvl2 = node->rect.get(orient);
              /*bool iresult =*/ intersect(rectIvl2, nodeIvl2, false);
              Rectangle nextRect2 = Rectangle(rectIvl2, rectIvl2);
              Unit low2 = rect.get(orient.get_perpendicular()).low();
              Unit high2 = node->rect.get(orient.get_perpendicular()).high();
              nextRect2.set(orient.get_perpendicular(), Interval(low2, high2));
              rect = nextRect2;
              //std::cout << "rect for next child" << rect << std::endl;
              node = *itr;
              itr = node->begin();
            }
          }
        }
      } while(!stack.empty() || itr != node->end());
    }

    /*  Function recursive version of getMaxCover
        Because the code is so much simpler than the loop algorithm I retain it for clarity

    template <class cT>
    static inline void getMaxCover(cT& outputContainer, Node* node, orientation_2d orient,
                                   const Rectangle& rect) {
      Interval rectIvl = rect.get(orient);
      Interval nodeIvl = node->rect.get(orient);
      if(!intersect(rectIvl, nodeIvl, false)) {
        return;
      }
      if(node->tracedPath(rectIvl)) {
        return;
      }
      node->addPath(rectIvl);
      Rectangle nextRect(rectIvl, rectIvl);
      Unit low = rect.get(orient.get_perpendicular()).low();
      Unit high = node->rect.get(orient.get_perpendicular()).high();
      nextRect.set(orient.get_perpendicular(), Interval(low, high));
      bool writeOut = true;
      rectIvl = nextRect.get(orient);
      for(typename Node::iterator itr = node->begin(); itr != node->end(); ++itr) {
        nodeIvl = (*itr)->rect.get(orient);
        if(contains(nodeIvl, rectIvl, true)) writeOut = false;
      }
      if(writeOut) {
        outputContainer.push_back(copy_construct<typename cT::value_type, Rectangle>(nextRect));
      }
      for(typename Node::iterator itr = node->begin(); itr != node->end(); ++itr) {
        getMaxCover(outputContainer, *itr, orient, nextRect);
      }
    }
    */

    //iterator range is assummed to be in topological order meaning all node's trailing
    //edges are in sorted order
    template <class iT>
    static inline void computeDag(iT beginNode, iT endNode, orientation_2d orient,
                                  std::size_t size) {
      std::vector<EdgeAssociation> leadingEdges;
      leadingEdges.reserve(size);
      for(iT iter = beginNode; iter != endNode; ++iter) {
        Node* nodep = &(*iter);
        Unit leading = nodep->rect.get(orient.get_perpendicular()).low();
        Interval rectIvl = nodep->rect.get(orient);
        leadingEdges.push_back(EdgeAssociation(std::pair<Unit, Interval>(leading, rectIvl), nodep));
      }
      polygon_sort(leadingEdges.begin(), leadingEdges.end(), lessEdgeAssociation());
      typename std::vector<EdgeAssociation>::iterator leadingBegin = leadingEdges.begin();
      iT trailingBegin = beginNode;
      while(leadingBegin != leadingEdges.end()) {
        EdgeAssociation& leadingSegment = (*leadingBegin);
        Unit trailing = (*trailingBegin).rect.get(orient.get_perpendicular()).high();
        Interval ivl = (*trailingBegin).rect.get(orient);
        std::pair<Unit, Interval> trailingSegment(trailing, ivl);
        if(leadingSegment.first.first < trailingSegment.first) {
          ++leadingBegin;
          continue;
        }
        if(leadingSegment.first.first > trailingSegment.first) {
          ++trailingBegin;
          continue;
        }
        if(leadingSegment.first.second.high() <= trailingSegment.second.low()) {
          ++leadingBegin;
          continue;
        }
        if(trailingSegment.second.high() <= leadingSegment.first.second.low()) {
          ++trailingBegin;
          continue;
        }
        //leading segment intersects trailing segment
        (*trailingBegin).add((*leadingBegin).second);
        if(leadingSegment.first.second.high() > trailingSegment.second.high()) {
          ++trailingBegin;
          continue;
        }
        if(trailingSegment.second.high() > leadingSegment.first.second.high()) {
          ++leadingBegin;
          continue;
        }
        ++leadingBegin;
        ++trailingBegin;
      }
    }

    template <class cT>
    static inline void getMaxCover(cT& outputContainer,
                                   const std::vector<Rectangle>& rects, orientation_2d orient) {
      if(rects.empty()) return;
      std::vector<Node> nodes;
      {
        if(rects.size() == 1) {
          outputContainer.push_back(copy_construct<typename cT::value_type, Rectangle>(rects[0]));
          return;
        }
        nodes.reserve(rects.size());
        for(std::size_t i = 0; i < rects.size(); ++i) { nodes.push_back(Node(rects[i])); }
      }
      computeDag(nodes.begin(), nodes.end(), orient, nodes.size());
      for(std::size_t i = 0; i < nodes.size(); ++i) {
        getMaxCover(outputContainer, &(nodes[i]), orient);
      }
    }

  };
}
}

#endif

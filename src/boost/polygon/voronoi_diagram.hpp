// Boost.Polygon library voronoi_diagram.hpp header file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_POLYGON_VORONOI_DIAGRAM
#define BOOST_POLYGON_VORONOI_DIAGRAM

#include <vector>
#include <utility>

#include "detail/voronoi_ctypes.hpp"
#include "detail/voronoi_structures.hpp"

#include "voronoi_geometry_type.hpp"

namespace boost {
namespace polygon {

// Forward declarations.
template <typename T>
class voronoi_edge;

// Represents Voronoi cell.
// Data members:
//   1) index of the source within the initial input set
//   2) pointer to the incident edge
//   3) mutable color member
// Cell may contain point or segment site inside.
template <typename T>
class voronoi_cell {
 public:
  typedef T coordinate_type;
  typedef std::size_t color_type;
  typedef voronoi_edge<coordinate_type> voronoi_edge_type;
  typedef std::size_t source_index_type;
  typedef SourceCategory source_category_type;

  voronoi_cell(source_index_type source_index,
               source_category_type source_category) :
      source_index_(source_index),
      incident_edge_(NULL),
      color_(source_category) {}

  // Returns true if the cell contains point site, false else.
  bool contains_point() const {
    source_category_type source_category = this->source_category();
    return belongs(source_category, GEOMETRY_CATEGORY_POINT);
  }

  // Returns true if the cell contains segment site, false else.
  bool contains_segment() const {
    source_category_type source_category = this->source_category();
    return belongs(source_category, GEOMETRY_CATEGORY_SEGMENT);
  }

  source_index_type source_index() const {
    return source_index_;
  }

  source_category_type source_category() const {
    return static_cast<source_category_type>(color_ & SOURCE_CATEGORY_BITMASK);
  }

  // Degenerate cells don't have any incident edges.
  bool is_degenerate() const { return incident_edge_ == NULL; }

  voronoi_edge_type* incident_edge() { return incident_edge_; }
  const voronoi_edge_type* incident_edge() const { return incident_edge_; }
  void incident_edge(voronoi_edge_type* e) { incident_edge_ = e; }

  color_type color() const { return color_ >> BITS_SHIFT; }
  void color(color_type color) const {
    color_ &= BITS_MASK;
    color_ |= color << BITS_SHIFT;
  }

 private:
  // 5 color bits are reserved.
  enum Bits {
    BITS_SHIFT = 0x5,
    BITS_MASK = 0x1F
  };

  source_index_type source_index_;
  voronoi_edge_type* incident_edge_;
  mutable color_type color_;
};

// Represents Voronoi vertex.
// Data members:
//   1) vertex coordinates
//   2) pointer to the incident edge
//   3) mutable color member
template <typename T>
class voronoi_vertex {
 public:
  typedef T coordinate_type;
  typedef std::size_t color_type;
  typedef voronoi_edge<coordinate_type> voronoi_edge_type;

  voronoi_vertex(const coordinate_type& x, const coordinate_type& y) :
      x_(x),
      y_(y),
      incident_edge_(NULL),
      color_(0) {}

  const coordinate_type& x() const { return x_; }
  const coordinate_type& y() const { return y_; }

  bool is_degenerate() const { return incident_edge_ == NULL; }

  voronoi_edge_type* incident_edge() { return incident_edge_; }
  const voronoi_edge_type* incident_edge() const { return incident_edge_; }
  void incident_edge(voronoi_edge_type* e) { incident_edge_ = e; }

  color_type color() const { return color_ >> BITS_SHIFT; }
  void color(color_type color) const {
    color_ &= BITS_MASK;
    color_ |= color << BITS_SHIFT;
  }

 private:
  // 5 color bits are reserved.
  enum Bits {
    BITS_SHIFT = 0x5,
    BITS_MASK = 0x1F
  };

  coordinate_type x_;
  coordinate_type y_;
  voronoi_edge_type* incident_edge_;
  mutable color_type color_;
};

// Half-edge data structure. Represents Voronoi edge.
// Data members:
//   1) pointer to the corresponding cell
//   2) pointer to the vertex that is the starting
//      point of the half-edge
//   3) pointer to the twin edge
//   4) pointer to the CCW next edge
//   5) pointer to the CCW prev edge
//   6) mutable color member
template <typename T>
class voronoi_edge {
 public:
  typedef T coordinate_type;
  typedef voronoi_cell<coordinate_type> voronoi_cell_type;
  typedef voronoi_vertex<coordinate_type> voronoi_vertex_type;
  typedef voronoi_edge<coordinate_type> voronoi_edge_type;
  typedef std::size_t color_type;

  voronoi_edge(bool is_linear, bool is_primary) :
      cell_(NULL),
      vertex_(NULL),
      twin_(NULL),
      next_(NULL),
      prev_(NULL),
      color_(0) {
    if (is_linear)
      color_ |= BIT_IS_LINEAR;
    if (is_primary)
      color_ |= BIT_IS_PRIMARY;
  }

  voronoi_cell_type* cell() { return cell_; }
  const voronoi_cell_type* cell() const { return cell_; }
  void cell(voronoi_cell_type* c) { cell_ = c; }

  voronoi_vertex_type* vertex0() { return vertex_; }
  const voronoi_vertex_type* vertex0() const { return vertex_; }
  void vertex0(voronoi_vertex_type* v) { vertex_ = v; }

  voronoi_vertex_type* vertex1() { return twin_->vertex0(); }
  const voronoi_vertex_type* vertex1() const { return twin_->vertex0(); }

  voronoi_edge_type* twin() { return twin_; }
  const voronoi_edge_type* twin() const { return twin_; }
  void twin(voronoi_edge_type* e) { twin_ = e; }

  voronoi_edge_type* next() { return next_; }
  const voronoi_edge_type* next() const { return next_; }
  void next(voronoi_edge_type* e) { next_ = e; }

  voronoi_edge_type* prev() { return prev_; }
  const voronoi_edge_type* prev() const { return prev_; }
  void prev(voronoi_edge_type* e) { prev_ = e; }

  // Returns a pointer to the rotation next edge
  // over the starting point of the half-edge.
  voronoi_edge_type* rot_next() { return prev_->twin(); }
  const voronoi_edge_type* rot_next() const { return prev_->twin(); }

  // Returns a pointer to the rotation prev edge
  // over the starting point of the half-edge.
  voronoi_edge_type* rot_prev() { return twin_->next(); }
  const voronoi_edge_type* rot_prev() const { return twin_->next(); }

  // Returns true if the edge is finite (segment, parabolic arc).
  // Returns false if the edge is infinite (ray, line).
  bool is_finite() const { return vertex0() && vertex1(); }

  // Returns true if the edge is infinite (ray, line).
  // Returns false if the edge is finite (segment, parabolic arc).
  bool is_infinite() const { return !vertex0() || !vertex1(); }

  // Returns true if the edge is linear (segment, ray, line).
  // Returns false if the edge is curved (parabolic arc).
  bool is_linear() const {
    return (color_ & BIT_IS_LINEAR) ? true : false;
  }

  // Returns true if the edge is curved (parabolic arc).
  // Returns false if the edge is linear (segment, ray, line).
  bool is_curved() const {
    return (color_ & BIT_IS_LINEAR) ? false : true;
  }

  // Returns false if edge goes through the endpoint of the segment.
  // Returns true else.
  bool is_primary() const {
    return (color_ & BIT_IS_PRIMARY) ? true : false;
  }

  // Returns true if edge goes through the endpoint of the segment.
  // Returns false else.
  bool is_secondary() const {
    return (color_ & BIT_IS_PRIMARY) ? false : true;
  }

  color_type color() const { return color_ >> BITS_SHIFT; }
  void color(color_type color) const {
    color_ &= BITS_MASK;
    color_ |= color << BITS_SHIFT;
  }

 private:
  // 5 color bits are reserved.
  enum Bits {
    BIT_IS_LINEAR = 0x1,  // linear is opposite to curved
    BIT_IS_PRIMARY = 0x2,  // primary is opposite to secondary

    BITS_SHIFT = 0x5,
    BITS_MASK = 0x1F
  };

  voronoi_cell_type* cell_;
  voronoi_vertex_type* vertex_;
  voronoi_edge_type* twin_;
  voronoi_edge_type* next_;
  voronoi_edge_type* prev_;
  mutable color_type color_;
};

template <typename T>
struct voronoi_diagram_traits {
  typedef T coordinate_type;
  typedef voronoi_cell<coordinate_type> cell_type;
  typedef voronoi_vertex<coordinate_type> vertex_type;
  typedef voronoi_edge<coordinate_type> edge_type;
  class vertex_equality_predicate_type {
   public:
    enum { ULPS = 128 };
    bool operator()(const vertex_type& v1, const vertex_type& v2) const {
      return (ulp_cmp(v1.x(), v2.x(), ULPS) ==
              detail::ulp_comparison<T>::EQUAL) &&
             (ulp_cmp(v1.y(), v2.y(), ULPS) ==
              detail::ulp_comparison<T>::EQUAL);
    }
   private:
    typename detail::ulp_comparison<T> ulp_cmp;
  };
};

// Voronoi output data structure.
// CCW ordering is used on the faces perimeter and around the vertices.
template <typename T, typename TRAITS = voronoi_diagram_traits<T> >
class voronoi_diagram {
 public:
  typedef typename TRAITS::coordinate_type coordinate_type;
  typedef typename TRAITS::cell_type cell_type;
  typedef typename TRAITS::vertex_type vertex_type;
  typedef typename TRAITS::edge_type edge_type;

  typedef std::vector<cell_type> cell_container_type;
  typedef typename cell_container_type::const_iterator const_cell_iterator;

  typedef std::vector<vertex_type> vertex_container_type;
  typedef typename vertex_container_type::const_iterator const_vertex_iterator;

  typedef std::vector<edge_type> edge_container_type;
  typedef typename edge_container_type::const_iterator const_edge_iterator;

  voronoi_diagram() {}

  void clear() {
    cells_.clear();
    vertices_.clear();
    edges_.clear();
  }

  const cell_container_type& cells() const {
    return cells_;
  }

  const vertex_container_type& vertices() const {
    return vertices_;
  }

  const edge_container_type& edges() const {
    return edges_;
  }

  std::size_t num_cells() const {
    return cells_.size();
  }

  std::size_t num_edges() const {
    return edges_.size();
  }

  std::size_t num_vertices() const {
    return vertices_.size();
  }

  void _reserve(std::size_t num_sites) {
    cells_.reserve(num_sites);
    vertices_.reserve(num_sites << 1);
    edges_.reserve((num_sites << 2) + (num_sites << 1));
  }

  template <typename CT>
  void _process_single_site(const detail::site_event<CT>& site) {
    cells_.push_back(cell_type(site.initial_index(), site.source_category()));
  }

  // Insert a new half-edge into the output data structure.
  // Takes as input left and right sites that form a new bisector.
  // Returns a pair of pointers to a new half-edges.
  template <typename CT>
  std::pair<void*, void*> _insert_new_edge(
      const detail::site_event<CT>& site1,
      const detail::site_event<CT>& site2) {
    // Get sites' indexes.
    std::size_t site_index1 = site1.sorted_index();
    std::size_t site_index2 = site2.sorted_index();

    bool is_linear = is_linear_edge(site1, site2);
    bool is_primary = is_primary_edge(site1, site2);

    // Create a new half-edge that belongs to the first site.
    edges_.push_back(edge_type(is_linear, is_primary));
    edge_type& edge1 = edges_.back();

    // Create a new half-edge that belongs to the second site.
    edges_.push_back(edge_type(is_linear, is_primary));
    edge_type& edge2 = edges_.back();

    // Add the initial cell during the first edge insertion.
    if (cells_.empty()) {
      cells_.push_back(cell_type(
          site1.initial_index(), site1.source_category()));
    }

    // The second site represents a new site during site event
    // processing. Add a new cell to the cell records.
    cells_.push_back(cell_type(
        site2.initial_index(), site2.source_category()));

    // Set up pointers to cells.
    edge1.cell(&cells_[site_index1]);
    edge2.cell(&cells_[site_index2]);

    // Set up twin pointers.
    edge1.twin(&edge2);
    edge2.twin(&edge1);

    // Return a pointer to the new half-edge.
    return std::make_pair(&edge1, &edge2);
  }

  // Insert a new half-edge into the output data structure with the
  // start at the point where two previously added half-edges intersect.
  // Takes as input two sites that create a new bisector, circle event
  // that corresponds to the intersection point of the two old half-edges,
  // pointers to those half-edges. Half-edges' direction goes out of the
  // new Voronoi vertex point. Returns a pair of pointers to a new half-edges.
  template <typename CT1, typename CT2>
  std::pair<void*, void*> _insert_new_edge(
      const detail::site_event<CT1>& site1,
      const detail::site_event<CT1>& site3,
      const detail::circle_event<CT2>& circle,
      void* data12, void* data23) {
    edge_type* edge12 = static_cast<edge_type*>(data12);
    edge_type* edge23 = static_cast<edge_type*>(data23);

    // Add a new Voronoi vertex.
    vertices_.push_back(vertex_type(circle.x(), circle.y()));
    vertex_type& new_vertex = vertices_.back();

    // Update vertex pointers of the old edges.
    edge12->vertex0(&new_vertex);
    edge23->vertex0(&new_vertex);

    bool is_linear = is_linear_edge(site1, site3);
    bool is_primary = is_primary_edge(site1, site3);

    // Add a new half-edge.
    edges_.push_back(edge_type(is_linear, is_primary));
    edge_type& new_edge1 = edges_.back();
    new_edge1.cell(&cells_[site1.sorted_index()]);

    // Add a new half-edge.
    edges_.push_back(edge_type(is_linear, is_primary));
    edge_type& new_edge2 = edges_.back();
    new_edge2.cell(&cells_[site3.sorted_index()]);

    // Update twin pointers.
    new_edge1.twin(&new_edge2);
    new_edge2.twin(&new_edge1);

    // Update vertex pointer.
    new_edge2.vertex0(&new_vertex);

    // Update Voronoi prev/next pointers.
    edge12->prev(&new_edge1);
    new_edge1.next(edge12);
    edge12->twin()->next(edge23);
    edge23->prev(edge12->twin());
    edge23->twin()->next(&new_edge2);
    new_edge2.prev(edge23->twin());

    // Return a pointer to the new half-edge.
    return std::make_pair(&new_edge1, &new_edge2);
  }

  void _build() {
    // Remove degenerate edges.
    edge_iterator last_edge = edges_.begin();
    for (edge_iterator it = edges_.begin(); it != edges_.end(); it += 2) {
      const vertex_type* v1 = it->vertex0();
      const vertex_type* v2 = it->vertex1();
      if (v1 && v2 && vertex_equality_predicate_(*v1, *v2)) {
        remove_edge(&(*it));
      } else {
        if (it != last_edge) {
          edge_type* e1 = &(*last_edge = *it);
          edge_type* e2 = &(*(last_edge + 1) = *(it + 1));
          e1->twin(e2);
          e2->twin(e1);
          if (e1->prev()) {
            e1->prev()->next(e1);
            e2->next()->prev(e2);
          }
          if (e2->prev()) {
            e1->next()->prev(e1);
            e2->prev()->next(e2);
          }
        }
        last_edge += 2;
      }
    }
    edges_.erase(last_edge, edges_.end());

    // Set up incident edge pointers for cells and vertices.
    for (edge_iterator it = edges_.begin(); it != edges_.end(); ++it) {
      it->cell()->incident_edge(&(*it));
      if (it->vertex0()) {
        it->vertex0()->incident_edge(&(*it));
      }
    }

    // Remove degenerate vertices.
    vertex_iterator last_vertex = vertices_.begin();
    for (vertex_iterator it = vertices_.begin(); it != vertices_.end(); ++it) {
      if (it->incident_edge()) {
        if (it != last_vertex) {
          *last_vertex = *it;
          vertex_type* v = &(*last_vertex);
          edge_type* e = v->incident_edge();
          do {
            e->vertex0(v);
            e = e->rot_next();
          } while (e != v->incident_edge());
        }
        ++last_vertex;
      }
    }
    vertices_.erase(last_vertex, vertices_.end());

    // Set up next/prev pointers for infinite edges.
    if (vertices_.empty()) {
      if (!edges_.empty()) {
        // Update prev/next pointers for the line edges.
        edge_iterator edge_it = edges_.begin();
        edge_type* edge1 = &(*edge_it);
        edge1->next(edge1);
        edge1->prev(edge1);
        ++edge_it;
        edge1 = &(*edge_it);
        ++edge_it;

        while (edge_it != edges_.end()) {
          edge_type* edge2 = &(*edge_it);
          ++edge_it;

          edge1->next(edge2);
          edge1->prev(edge2);
          edge2->next(edge1);
          edge2->prev(edge1);

          edge1 = &(*edge_it);
          ++edge_it;
        }

        edge1->next(edge1);
        edge1->prev(edge1);
      }
    } else {
      // Update prev/next pointers for the ray edges.
      for (cell_iterator cell_it = cells_.begin();
         cell_it != cells_.end(); ++cell_it) {
        if (cell_it->is_degenerate())
          continue;
        // Move to the previous edge while
        // it is possible in the CW direction.
        edge_type* left_edge = cell_it->incident_edge();
        while (left_edge->prev() != NULL) {
          left_edge = left_edge->prev();
          // Terminate if this is not a boundary cell.
          if (left_edge == cell_it->incident_edge())
            break;
        }

        if (left_edge->prev() != NULL)
          continue;

        edge_type* right_edge = cell_it->incident_edge();
        while (right_edge->next() != NULL)
          right_edge = right_edge->next();
        left_edge->prev(right_edge);
        right_edge->next(left_edge);
      }
    }
  }

 private:
  typedef typename cell_container_type::iterator cell_iterator;
  typedef typename vertex_container_type::iterator vertex_iterator;
  typedef typename edge_container_type::iterator edge_iterator;
  typedef typename TRAITS::vertex_equality_predicate_type
    vertex_equality_predicate_type;

  template <typename SEvent>
  bool is_primary_edge(const SEvent& site1, const SEvent& site2) const {
    bool flag1 = site1.is_segment();
    bool flag2 = site2.is_segment();
    if (flag1 && !flag2) {
      return (site1.point0() != site2.point0()) &&
             (site1.point1() != site2.point0());
    }
    if (!flag1 && flag2) {
      return (site2.point0() != site1.point0()) &&
             (site2.point1() != site1.point0());
    }
    return true;
  }

  template <typename SEvent>
  bool is_linear_edge(const SEvent& site1, const SEvent& site2) const {
    if (!is_primary_edge(site1, site2)) {
      return true;
    }
    return !(site1.is_segment() ^ site2.is_segment());
  }

  // Remove degenerate edge.
  void remove_edge(edge_type* edge) {
    // Update the endpoints of the incident edges to the second vertex.
    vertex_type* vertex = edge->vertex0();
    edge_type* updated_edge = edge->twin()->rot_next();
    while (updated_edge != edge->twin()) {
      updated_edge->vertex0(vertex);
      updated_edge = updated_edge->rot_next();
    }

    edge_type* edge1 = edge;
    edge_type* edge2 = edge->twin();

    edge_type* edge1_rot_prev = edge1->rot_prev();
    edge_type* edge1_rot_next = edge1->rot_next();

    edge_type* edge2_rot_prev = edge2->rot_prev();
    edge_type* edge2_rot_next = edge2->rot_next();

    // Update prev/next pointers for the incident edges.
    edge1_rot_next->twin()->next(edge2_rot_prev);
    edge2_rot_prev->prev(edge1_rot_next->twin());
    edge1_rot_prev->prev(edge2_rot_next->twin());
    edge2_rot_next->twin()->next(edge1_rot_prev);
  }

  cell_container_type cells_;
  vertex_container_type vertices_;
  edge_container_type edges_;
  vertex_equality_predicate_type vertex_equality_predicate_;

  // Disallow copy constructor and operator=
  voronoi_diagram(const voronoi_diagram&);
  void operator=(const voronoi_diagram&);
};
}  // polygon
}  // boost

#endif  // BOOST_POLYGON_VORONOI_DIAGRAM

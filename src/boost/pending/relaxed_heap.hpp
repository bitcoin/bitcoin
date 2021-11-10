// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#ifndef BOOST_RELAXED_HEAP_HEADER
#define BOOST_RELAXED_HEAP_HEADER

#include <boost/config/header_deprecated.hpp>
BOOST_HEADER_DEPRECATED("the standard heap functions")

#include <functional>
#include <boost/property_map/property_map.hpp>
#include <boost/optional.hpp>
#include <vector>
#include <climits> // for CHAR_BIT
#include <boost/none.hpp>

#ifdef BOOST_RELAXED_HEAP_DEBUG
#include <iostream>
#endif // BOOST_RELAXED_HEAP_DEBUG

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4355) // complaint about using 'this' to
#endif // initialize a member

namespace boost
{

template < typename IndexedType, typename Compare = std::less< IndexedType >,
    typename ID = identity_property_map >
class relaxed_heap
{
    struct group;

    typedef relaxed_heap self_type;
    typedef std::size_t rank_type;

public:
    typedef IndexedType value_type;
    typedef rank_type size_type;

private:
    /**
     * The kind of key that a group has. The actual values are discussed
     * in-depth in the documentation of the @c kind field of the @c group
     * structure. Note that the order of the enumerators *IS* important
     * and must not be changed.
     */
    enum group_key_kind
    {
        smallest_key,
        stored_key,
        largest_key
    };

    struct group
    {
        explicit group(group_key_kind kind = largest_key)
        : kind(kind), parent(this), rank(0)
        {
        }

        /** The value associated with this group. This value is only valid
         *  when @c kind!=largest_key (which indicates a deleted
         *  element). Note that the use of boost::optional increases the
         *  memory requirements slightly but does not result in extraneous
         *  memory allocations or deallocations. The optional could be
         *  eliminated when @c value_type is a model of
         *  DefaultConstructible.
         */
        ::boost::optional< value_type > value;

        /**
         * The kind of key stored at this group. This may be @c
         * smallest_key, which indicates that the key is infinitely small;
         * @c largest_key, which indicates that the key is infinitely
         * large; or @c stored_key, which means that the key is unknown,
         * but its relationship to other keys can be determined via the
         * comparison function object.
         */
        group_key_kind kind;

        /// The parent of this group. Will only be NULL for the dummy root group
        group* parent;

        /// The rank of this group. Equivalent to the number of children in
        /// the group.
        rank_type rank;

        /** The children of this group. For the dummy root group, these are
         * the roots. This is an array of length log n containing pointers
         * to the child groups.
         */
        group** children;
    };

    size_type log_base_2(size_type n) // log2 is a macro on some platforms
    {
        size_type leading_zeroes = 0;
        do
        {
            size_type next = n << 1;
            if (n == (next >> 1))
            {
                ++leading_zeroes;
                n = next;
            }
            else
            {
                break;
            }
        } while (true);
        return sizeof(size_type) * CHAR_BIT - leading_zeroes - 1;
    }

public:
    relaxed_heap(
        size_type n, const Compare& compare = Compare(), const ID& id = ID())
    : compare(compare), id(id), root(smallest_key), groups(n), smallest_value(0)
    {
        if (n == 0)
        {
            root.children = new group*[1];
            return;
        }

        log_n = log_base_2(n);
        if (log_n == 0)
            log_n = 1;
        size_type g = n / log_n;
        if (n % log_n > 0)
            ++g;
        size_type log_g = log_base_2(g);
        size_type r = log_g;

        // Reserve an appropriate amount of space for data structures, so
        // that we do not need to expand them.
        index_to_group.resize(g);
        A.resize(r + 1, 0);
        root.rank = r + 1;
        root.children = new group*[(log_g + 1) * (g + 1)];
        for (rank_type i = 0; i < r + 1; ++i)
            root.children[i] = 0;

        // Build initial heap
        size_type idx = 0;
        while (idx < g)
        {
            root.children[r] = &index_to_group[idx];
            idx = build_tree(root, idx, r, log_g + 1);
            if (idx != g)
                r = static_cast< size_type >(log_base_2(g - idx));
        }
    }

    ~relaxed_heap() { delete[] root.children; }

    void push(const value_type& x)
    {
        groups[get(id, x)] = x;
        update(x);
    }

    void update(const value_type& x)
    {
        group* a = &index_to_group[get(id, x) / log_n];
        if (!a->value || *a->value == x || compare(x, *a->value))
        {
            if (a != smallest_value)
                smallest_value = 0;
            a->kind = stored_key;
            a->value = x;
            promote(a);
        }
    }

    void remove(const value_type& x)
    {
        group* a = &index_to_group[get(id, x) / log_n];
        assert(groups[get(id, x)]);
        a->value = x;
        a->kind = smallest_key;
        promote(a);
        smallest_value = a;
        pop();
    }

    value_type& top()
    {
        find_smallest();
        assert(smallest_value->value != none);
        return *smallest_value->value;
    }

    const value_type& top() const
    {
        find_smallest();
        assert(smallest_value->value != none);
        return *smallest_value->value;
    }

    bool empty() const
    {
        find_smallest();
        return !smallest_value || (smallest_value->kind == largest_key);
    }

    bool contains(const value_type& x) const
    {
        return static_cast< bool >(groups[get(id, x)]);
    }

    void pop()
    {
        // Fill in smallest_value. This is the group x.
        find_smallest();
        group* x = smallest_value;
        smallest_value = 0;

        // Make x a leaf, giving it the smallest value within its group
        rank_type r = x->rank;
        group* p = x->parent;
        {
            assert(x->value != none);

            // Find x's group
            size_type start = get(id, *x->value) - get(id, *x->value) % log_n;
            size_type end = start + log_n;
            if (end > groups.size())
                end = groups.size();

            // Remove the smallest value from the group, and find the new
            // smallest value.
            groups[get(id, *x->value)].reset();
            x->value.reset();
            x->kind = largest_key;
            for (size_type i = start; i < end; ++i)
            {
                if (groups[i] && (!x->value || compare(*groups[i], *x->value)))
                {
                    x->kind = stored_key;
                    x->value = groups[i];
                }
            }
        }
        x->rank = 0;

        // Combine prior children of x with x
        group* y = x;
        for (size_type c = 0; c < r; ++c)
        {
            group* child = x->children[c];
            if (A[c] == child)
                A[c] = 0;
            y = combine(y, child);
        }

        // If we got back something other than x, let y take x's place
        if (y != x)
        {
            y->parent = p;
            p->children[r] = y;

            assert(r == y->rank);
            if (A[y->rank] == x)
                A[y->rank] = do_compare(y, p) ? y : 0;
        }
    }

#ifdef BOOST_RELAXED_HEAP_DEBUG
    /*************************************************************************
     * Debugging support                                                     *
     *************************************************************************/
    void dump_tree() { dump_tree(std::cout); }
    void dump_tree(std::ostream& out) { dump_tree(out, &root); }

    void dump_tree(std::ostream& out, group* p, bool in_progress = false)
    {
        if (!in_progress)
        {
            out << "digraph heap {\n"
                << "  edge[dir=\"back\"];\n";
        }

        size_type p_index = 0;
        if (p != &root)
            while (&index_to_group[p_index] != p)
                ++p_index;

        for (size_type i = 0; i < p->rank; ++i)
        {
            group* c = p->children[i];
            if (c)
            {
                size_type c_index = 0;
                if (c != &root)
                    while (&index_to_group[c_index] != c)
                        ++c_index;

                out << "  ";
                if (p == &root)
                    out << 'p';
                else
                    out << p_index;
                out << " -> ";
                if (c == &root)
                    out << 'p';
                else
                    out << c_index;
                if (A[c->rank] == c)
                    out << " [style=\"dotted\"]";
                out << ";\n";
                dump_tree(out, c, true);

                // Emit node information
                out << "  ";
                if (c == &root)
                    out << 'p';
                else
                    out << c_index;
                out << " [label=\"";
                if (c == &root)
                    out << 'p';
                else
                    out << c_index;
                out << ":";
                size_type start = c_index * log_n;
                size_type end = start + log_n;
                if (end > groups.size())
                    end = groups.size();
                while (start != end)
                {
                    if (groups[start])
                    {
                        out << " " << get(id, *groups[start]);
                        if (*groups[start] == *c->value)
                            out << "(*)";
                    }
                    ++start;
                }
                out << '"';

                if (do_compare(c, p))
                {
                    out << "  ";
                    if (c == &root)
                        out << 'p';
                    else
                        out << c_index;
                    out << ", style=\"filled\", fillcolor=\"gray\"";
                }
                out << "];\n";
            }
            else
            {
                assert(p->parent == p);
            }
        }
        if (!in_progress)
            out << "}\n";
    }

    bool valid()
    {
        // Check that the ranks in the A array match the ranks of the
        // groups stored there. Also, the active groups must be the last
        // child of their parent.
        for (size_type r = 0; r < A.size(); ++r)
        {
            if (A[r] && A[r]->rank != r)
                return false;

            if (A[r] && A[r]->parent->children[A[r]->parent->rank - 1] != A[r])
                return false;
        }

        // The root must have no value and a key of -Infinity
        if (root.kind != smallest_key)
            return false;

        return valid(&root);
    }

    bool valid(group* p)
    {
        for (size_type i = 0; i < p->rank; ++i)
        {
            group* c = p->children[i];
            if (c)
            {
                // Check link structure
                if (c->parent != p)
                    return false;
                if (c->rank != i)
                    return false;

                // A bad group must be active
                if (do_compare(c, p) && A[i] != c)
                    return false;

                // Check recursively
                if (!valid(c))
                    return false;
            }
            else
            {
                // Only the root may
                if (p != &root)
                    return false;
            }
        }
        return true;
    }

#endif // BOOST_RELAXED_HEAP_DEBUG

private:
    size_type build_tree(
        group& parent, size_type idx, size_type r, size_type max_rank)
    {
        group& this_group = index_to_group[idx];
        this_group.parent = &parent;
        ++idx;

        this_group.children = root.children + (idx * max_rank);
        this_group.rank = r;
        for (size_type i = 0; i < r; ++i)
        {
            this_group.children[i] = &index_to_group[idx];
            idx = build_tree(this_group, idx, i, max_rank);
        }
        return idx;
    }

    void find_smallest() const
    {
        group** roots = root.children;

        if (!smallest_value)
        {
            std::size_t i;
            for (i = 0; i < root.rank; ++i)
            {
                if (roots[i]
                    && (!smallest_value
                        || do_compare(roots[i], smallest_value)))
                {
                    smallest_value = roots[i];
                }
            }
            for (i = 0; i < A.size(); ++i)
            {
                if (A[i]
                    && (!smallest_value || do_compare(A[i], smallest_value)))
                    smallest_value = A[i];
            }
        }
    }

    bool do_compare(group* x, group* y) const
    {
        return (x->kind < y->kind
            || (x->kind == y->kind && x->kind == stored_key
                && compare(*x->value, *y->value)));
    }

    void promote(group* a)
    {
        assert(a != 0);
        rank_type r = a->rank;
        group* p = a->parent;
        assert(p != 0);
        if (do_compare(a, p))
        {
            // s is the rank + 1 sibling
            group* s = p->rank > r + 1 ? p->children[r + 1] : 0;

            // If a is the last child of p
            if (r == p->rank - 1)
            {
                if (!A[r])
                    A[r] = a;
                else if (A[r] != a)
                    pair_transform(a);
            }
            else
            {
                assert(s != 0);
                if (A[r + 1] == s)
                    active_sibling_transform(a, s);
                else
                    good_sibling_transform(a, s);
            }
        }
    }

    group* combine(group* a1, group* a2)
    {
        assert(a1->rank == a2->rank);
        if (do_compare(a2, a1))
            do_swap(a1, a2);
        a1->children[a1->rank++] = a2;
        a2->parent = a1;
        clean(a1);
        return a1;
    }

    void clean(group* q)
    {
        if (2 > q->rank)
            return;
        group* qp = q->children[q->rank - 1];
        rank_type s = q->rank - 2;
        group* x = q->children[s];
        group* xp = qp->children[s];
        assert(s == x->rank);

        // If x is active, swap x and xp
        if (A[s] == x)
        {
            q->children[s] = xp;
            xp->parent = q;
            qp->children[s] = x;
            x->parent = qp;
        }
    }

    void pair_transform(group* a)
    {
#if defined(BOOST_RELAXED_HEAP_DEBUG) && BOOST_RELAXED_HEAP_DEBUG > 1
        std::cerr << "- pair transform\n";
#endif
        rank_type r = a->rank;

        // p is a's parent
        group* p = a->parent;
        assert(p != 0);

        // g is p's parent (a's grandparent)
        group* g = p->parent;
        assert(g != 0);

        // a' <- A(r)
        assert(A[r] != 0);
        group* ap = A[r];
        assert(ap != 0);

        // A(r) <- nil
        A[r] = 0;

        // let a' have parent p'
        group* pp = ap->parent;
        assert(pp != 0);

        // let a' have grandparent g'
        group* gp = pp->parent;
        assert(gp != 0);

        // Remove a and a' from their parents
        assert(ap
            == pp->children[pp->rank - 1]); // Guaranteed because ap is active
        --pp->rank;

        // Guaranteed by caller
        assert(a == p->children[p->rank - 1]);
        --p->rank;

        // Note: a, ap, p, pp all have rank r
        if (do_compare(pp, p))
        {
            do_swap(a, ap);
            do_swap(p, pp);
            do_swap(g, gp);
        }

        // Assuming k(p) <= k(p')
        // make p' the rank r child of p
        assert(r == p->rank);
        p->children[p->rank++] = pp;
        pp->parent = p;

        // Combine a, ap into a rank r+1 group c
        group* c = combine(a, ap);

        // make c the rank r+1 child of g'
        assert(gp->rank > r + 1);
        gp->children[r + 1] = c;
        c->parent = gp;

#if defined(BOOST_RELAXED_HEAP_DEBUG) && BOOST_RELAXED_HEAP_DEBUG > 1
        std::cerr << "After pair transform...\n";
        dump_tree();
#endif

        if (A[r + 1] == pp)
            A[r + 1] = c;
        else
            promote(c);
    }

    void active_sibling_transform(group* a, group* s)
    {
#if defined(BOOST_RELAXED_HEAP_DEBUG) && BOOST_RELAXED_HEAP_DEBUG > 1
        std::cerr << "- active sibling transform\n";
#endif
        group* p = a->parent;
        group* g = p->parent;

        // remove a, s from their parents
        assert(s->parent == p);
        assert(p->children[p->rank - 1] == s);
        --p->rank;
        assert(p->children[p->rank - 1] == a);
        --p->rank;

        rank_type r = a->rank;
        A[r + 1] = 0;
        a = combine(p, a);
        group* c = combine(a, s);

        // make c the rank r+2 child of g
        assert(g->children[r + 2] == p);
        g->children[r + 2] = c;
        c->parent = g;
        if (A[r + 2] == p)
            A[r + 2] = c;
        else
            promote(c);
    }

    void good_sibling_transform(group* a, group* s)
    {
#if defined(BOOST_RELAXED_HEAP_DEBUG) && BOOST_RELAXED_HEAP_DEBUG > 1
        std::cerr << "- good sibling transform\n";
#endif
        rank_type r = a->rank;
        group* c = s->children[s->rank - 1];
        assert(c->rank == r);
        if (A[r] == c)
        {
#if defined(BOOST_RELAXED_HEAP_DEBUG) && BOOST_RELAXED_HEAP_DEBUG > 1
            std::cerr << "- good sibling pair transform\n";
#endif
            A[r] = 0;
            group* p = a->parent;

            // Remove c from its parent
            --s->rank;

            // Make s the rank r child of p
            s->parent = p;
            p->children[r] = s;

            // combine a, c and let the result by the rank r+1 child of p
            assert(p->rank > r + 1);
            group* x = combine(a, c);
            x->parent = p;
            p->children[r + 1] = x;

            if (A[r + 1] == s)
                A[r + 1] = x;
            else
                promote(x);

#if defined(BOOST_RELAXED_HEAP_DEBUG) && BOOST_RELAXED_HEAP_DEBUG > 1
            dump_tree(std::cerr);
#endif
            //      pair_transform(a);
        }
        else
        {
            // Clean operation
            group* p = a->parent;
            s->children[r] = a;
            a->parent = s;
            p->children[r] = c;
            c->parent = p;

            promote(a);
        }
    }

    static void do_swap(group*& x, group*& y)
    {
        group* tmp = x;
        x = y;
        y = tmp;
    }

    /// Function object that compares two values in the heap
    Compare compare;

    /// Mapping from values to indices in the range [0, n).
    ID id;

    /** The root group of the queue. This group is special because it will
     *  never store a value, but it acts as a parent to all of the
     *  roots. Thus, its list of children is the list of roots.
     */
    group root;

    /** Mapping from the group index of a value to the group associated
     *  with that value. If a value is not in the queue, then the "value"
     *  field will be empty.
     */
    std::vector< group > index_to_group;

    /** Flat data structure containing the values in each of the
     *  groups. It will be indexed via the id of the values. The groups
     *  are each log_n long, with the last group potentially being
     *  smaller.
     */
    std::vector< ::boost::optional< value_type > > groups;

    /** The list of active groups, indexed by rank. When A[r] is null,
     *  there is no active group of rank r. Otherwise, A[r] is the active
     *  group of rank r.
     */
    std::vector< group* > A;

    /** The group containing the smallest value in the queue, which must
     *  be either a root or an active group. If this group is null, then we
     *  will need to search for this group when it is needed.
     */
    mutable group* smallest_value;

    /// Cached value log_base_2(n)
    size_type log_n;
};

} // end namespace boost

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif // BOOST_RELAXED_HEAP_HEADER

/*=============================================================================
    Copyright (c) 2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_POSITION_TAGGED_MAY_01_2014_0321PM)
#define BOOST_SPIRIT_X3_POSITION_TAGGED_MAY_01_2014_0321PM

#include <boost/range/iterator_range_core.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/core/enable_if.hpp>

namespace boost { namespace spirit { namespace x3
{
    struct position_tagged
    {
        // Use this to annotate an AST with the iterator position.
        // These ids are used as a key to the position_cache (below)
        // and marks the start and end of an AST node.
        int id_first = -1;
        int id_last = -1;
    };

    template <typename Container>
    class position_cache
    {
    public:

        typedef typename Container::value_type iterator_type;

        position_cache(
            iterator_type first
          , iterator_type last)
          : first_(first), last_(last) {}

        // This will catch all nodes inheriting from position_tagged
        boost::iterator_range<iterator_type>
        position_of(position_tagged const& ast) const
        {
            return
                boost::iterator_range<iterator_type>(
                    positions.at(ast.id_first) // throws if out of range
                  , positions.at(ast.id_last)  // throws if out of range
                );
        }

        // This will catch all nodes except those inheriting from position_tagged
        template <typename AST>
        typename boost::enable_if_c<
            (!is_base_of<position_tagged, AST>::value)
          , boost::iterator_range<iterator_type>
        >::type
        position_of(AST const& /* ast */) const
        {
            // returns an empty position
            return boost::iterator_range<iterator_type>();
        }

        // This will catch all nodes except those inheriting from position_tagged
        template <typename AST>
        void annotate(AST& /* ast */, iterator_type /* first */, iterator_type /* last */, mpl::false_)
        {
            // (no-op) no need for tags
        }

        // This will catch all nodes inheriting from position_tagged
        void annotate(position_tagged& ast, iterator_type first, iterator_type last, mpl::true_)
        {
            ast.id_first = int(positions.size());
            positions.push_back(first);
            ast.id_last = int(positions.size());
            positions.push_back(last);
        }

        template <typename AST>
        void annotate(AST& ast, iterator_type first, iterator_type last)
        {
            annotate(ast, first, last, is_base_of<position_tagged, AST>());
        }

        Container const&
        get_positions() const
        {
            return positions;
        }

        iterator_type first() const { return first_; }
        iterator_type last() const { return last_; }

    private:

        Container positions;
        iterator_type first_;
        iterator_type last_;
    };

}}}

#endif

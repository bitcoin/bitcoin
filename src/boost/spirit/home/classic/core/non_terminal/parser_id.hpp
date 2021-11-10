/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_ID_HPP)
#define BOOST_SPIRIT_PARSER_ID_HPP

#if defined(BOOST_SPIRIT_DEBUG)
#   include <ostream>
#endif
#include <boost/spirit/home/classic/namespace.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  parser_id class
    //
    ///////////////////////////////////////////////////////////////////////////
    class parser_id
    {
    public:
                    parser_id()                     : p(0) {}
        explicit    parser_id(void const* prule)    : p(prule) {}
                    parser_id(std::size_t l_)       : l(l_) {}

        bool operator==(parser_id const& x) const   { return p == x.p; }
        bool operator!=(parser_id const& x) const   { return !(*this == x); }
        bool operator<(parser_id const& x) const    { return p < x.p; }
        std::size_t to_long() const                 { return l; }

    private:

        union
        {
            void const* p;
            std::size_t l;
        };
    };

    #if defined(BOOST_SPIRIT_DEBUG)
    inline std::ostream&
    operator<<(std::ostream& out, parser_id const& rid)
    {
        out << (unsigned int)rid.to_long();
        return out;
    }
    #endif

    ///////////////////////////////////////////////////////////////////////////
    //
    //  parser_tag_base class: base class of all parser tags
    //
    ///////////////////////////////////////////////////////////////////////////
    struct parser_tag_base {};
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //  parser_address_tag class: tags a parser with its address
    //
    ///////////////////////////////////////////////////////////////////////////
    struct parser_address_tag : parser_tag_base
    {
        parser_id id() const
        { return parser_id(reinterpret_cast<std::size_t>(this)); }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  parser_tag class: tags a parser with an integer ID
    //
    ///////////////////////////////////////////////////////////////////////////
    template <int N>
    struct parser_tag : parser_tag_base
    {
        static parser_id id()
        { return parser_id(std::size_t(N)); }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  dynamic_parser_tag class: tags a parser with a dynamically changeable
    //  integer ID
    //
    ///////////////////////////////////////////////////////////////////////////
    class dynamic_parser_tag : public parser_tag_base
    {
    public:
    
        dynamic_parser_tag() 
        : tag(std::size_t(0)) {}
        
        parser_id 
        id() const
        { 
            return 
                tag.to_long() 
                ? tag 
                : parser_id(reinterpret_cast<std::size_t>(this)); 
        }

        void set_id(parser_id id_) { tag = id_; } 
        
    private:
    
        parser_id tag;
    };

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif


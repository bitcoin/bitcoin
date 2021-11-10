//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_POSITION_TOKEN_MAY_13_2011_0846PM)
#define BOOST_SPIRIT_LEX_POSITION_TOKEN_MAY_13_2011_0846PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/support/attributes.hpp>
#include <boost/spirit/home/support/argument.hpp>
#include <boost/spirit/home/support/detail/lexer/generator.hpp>
#include <boost/spirit/home/support/detail/lexer/rules.hpp>
#include <boost/spirit/home/support/detail/lexer/consts.hpp>
#include <boost/spirit/home/support/utree/utree_traits_fwd.hpp>
#include <boost/spirit/home/lex/lexer/terminals.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/static_assert.hpp>

#if defined(BOOST_SPIRIT_DEBUG)
#include <iosfwd>
#endif

namespace boost { namespace spirit { namespace lex { namespace lexertl
{ 
    ///////////////////////////////////////////////////////////////////////////
    //
    //  The position_token is the type of the objects returned by the 
    //  iterator if it has been specified while instantiating the lexer object.
    //
    //    template parameters:
    //        Iterator        The type of the iterator used to access the
    //                        underlying character stream.
    //        AttributeTypes  A mpl sequence containing the types of all 
    //                        required different token values to be supported 
    //                        by this token type.
    //        HasState        A mpl::bool_ indicating, whether this token type
    //                        should support lexer states.
    //        Idtype          The type to use for the token id (defaults to 
    //                        std::size_t).
    //
    //  It is possible to use other token types with the spirit::lex 
    //  framework as well. If you plan to use a different type as your token 
    //  type, you'll need to expose the following things from your token type 
    //  to make it compatible with spirit::lex:
    //
    //    typedefs
    //        iterator_type   The type of the iterator used to access the
    //                        underlying character stream.
    //
    //        id_type         The type of the token id used.
    //
    //    methods
    //        default constructor
    //                        This should initialize the token as an end of 
    //                        input token.
    //        constructors    The prototype of the other required 
    //                        constructors should be:
    //
    //              token(int)
    //                        This constructor should initialize the token as 
    //                        an invalid token (not carrying any specific 
    //                        values)
    //
    //              where:  the int is used as a tag only and its value is 
    //                      ignored
    //
    //                        and:
    //
    //              token(Idtype id, std::size_t state, 
    //                    iterator_type first, iterator_type last);
    //
    //              where:  id:           token id
    //                      state:        lexer state this token was matched in
    //                      first, last:  pair of iterators marking the matched 
    //                                    range in the underlying input stream 
    //
    //        accessors
    //              id()      return the token id of the matched input sequence
    //              id(newid) set the token id of the token instance
    //
    //              state()   return the lexer state this token was matched in
    //
    //              value()   return the token value
    //
    //  Additionally, you will have to implement a couple of helper functions
    //  in the same namespace as the token type: a comparison operator==() to 
    //  compare your token instances, a token_is_valid() function and different 
    //  specializations of the Spirit customization point 
    //  assign_to_attribute_from_value as shown below.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator = char const*
      , typename AttributeTypes = mpl::vector0<>
      , typename HasState = mpl::true_
      , typename Idtype = std::size_t> 
    struct position_token;

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization of the token type doesn't contain any item data and
    //  doesn't support working with lexer states. Although, like all other
    //  variants of position_token, it carries a pair of iterators marking the
    //  begin and the end of the matched character sequence.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Idtype>
    struct position_token<Iterator, lex::omit, mpl::false_, Idtype>
    {
        typedef Iterator iterator_type;
        typedef iterator_range<iterator_type> iterpair_type;
        typedef mpl::false_ has_state;
        typedef Idtype id_type;
        typedef unused_type token_value_type;

        //  default constructed tokens correspond to EOI tokens
        position_token() 
          : id_(id_type(boost::lexer::npos)) {}

        //  construct an invalid token
        explicit position_token(int) 
          : id_(id_type(0)) {}

        position_token(id_type id, std::size_t) 
          : id_(id) {}

        position_token(id_type id, std::size_t, token_value_type)
          : id_(id) {}

        position_token(id_type id, std::size_t, Iterator const& first
              , Iterator const& last)
          : id_(id), matched_(first, last) {}

        //  this default conversion operator is needed to allow the direct 
        //  usage of tokens in conjunction with the primitive parsers defined 
        //  in Qi
        operator id_type() const { return id_; }

        //  Retrieve or set the token id of this token instance. 
        id_type id() const { return id_; }
        void id(id_type newid) { id_ = newid; }

        std::size_t state() const { return 0; }   // always '0' (INITIAL state)

        bool is_valid() const 
        { 
            return 0 != id_ && id_type(boost::lexer::npos) != id_; 
        }

        // access the stored iterator range of the matched input sequence
        iterator_type begin() const { return matched_.begin(); }
        iterator_type end() const { return matched_.end(); }

        iterpair_type& matched() { return matched_; }
        iterpair_type const& matched() const { return matched_; }

        token_value_type& value() { static token_value_type u; return u; }
        token_value_type const& value() const { return unused; }

#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
        // workaround for MSVC10 which has problems copying a default 
        // constructed iterator_range
        position_token& operator= (position_token const& rhs)
        {
            if (this != &rhs) 
            {
                id_ = rhs.id_;
                if (is_valid()) 
                    matched_ = rhs.matched_;
            }
            return *this;
        }
#endif

    protected:
        id_type id_;              // token id, 0 if nothing has been matched
        iterpair_type matched_;   // matched input sequence
    };

#if defined(BOOST_SPIRIT_DEBUG)
    template <typename Char, typename Traits, typename Iterator
      , typename AttributeTypes, typename HasState, typename Idtype> 
    inline std::basic_ostream<Char, Traits>& 
    operator<< (std::basic_ostream<Char, Traits>& os
      , position_token<Iterator, AttributeTypes, HasState, Idtype> const& t)
    {
        if (t.is_valid()) {
            Iterator end = t.end();
            for (Iterator it = t.begin(); it != end; ++it)
                os << *it;
        }
        else {
            os << "<invalid token>";
        }
        return os;
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization of the token type doesn't contain any item data but
    //  supports working with lexer states.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Idtype>
    struct position_token<Iterator, lex::omit, mpl::true_, Idtype>
      : position_token<Iterator, lex::omit, mpl::false_, Idtype>
    {
    private:
        typedef position_token<Iterator, lex::omit, mpl::false_, Idtype> 
            base_type;

    public:
        typedef typename base_type::id_type id_type;
        typedef Iterator iterator_type;
        typedef mpl::true_ has_state;
        typedef unused_type token_value_type;

        //  default constructed tokens correspond to EOI tokens
        position_token() : state_(boost::lexer::npos) {}

        //  construct an invalid token
        explicit position_token(int) 
          : base_type(0), state_(boost::lexer::npos) {}

        position_token(id_type id, std::size_t state)
          : base_type(id, boost::lexer::npos), state_(state) {}

        position_token(id_type id, std::size_t state, token_value_type)
          : base_type(id, boost::lexer::npos, unused)
          , state_(state) {}

        position_token(id_type id, std::size_t state
              , Iterator const& first, Iterator const& last)
          : base_type(id, boost::lexer::npos, first, last)
          , state_(state) {}

        std::size_t state() const { return state_; }

#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
        // workaround for MSVC10 which has problems copying a default 
        // constructed iterator_range
        position_token& operator= (position_token const& rhs)
        {
            if (this != &rhs) 
            {
                this->base_type::operator=(static_cast<base_type const&>(rhs));
                state_ = rhs.state_;
            }
            return *this;
        }
#endif

    protected:
        std::size_t state_;      // lexer state this token was matched in
    };

    ///////////////////////////////////////////////////////////////////////////
    // These specializations for an empty attribute list cause all token 
    // instances to expose as it attribute the iterator_range pointing to the
    // matched input sequence.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename HasState, typename Idtype>
    struct position_token<Iterator, mpl::vector<>, HasState, Idtype>
      : position_token<Iterator, lex::omit, HasState, Idtype>
    {
    private:
        typedef position_token<Iterator, lex::omit, HasState, Idtype> base_type;

    public:
        typedef typename base_type::id_type id_type;
        typedef typename base_type::iterator_type iterator_type;
        typedef typename base_type::iterpair_type iterpair_type;
        typedef HasState has_state;
        typedef iterpair_type token_value_type;

        //  default constructed tokens correspond to EOI tokens
        position_token() {}

        //  construct an invalid token
        explicit position_token(int) 
          : base_type(0) {}

        position_token(id_type id, std::size_t state)
          : base_type(id, state) {}

        position_token(id_type id, std::size_t state, token_value_type)
          : base_type(id, state, unused) {}

        position_token(id_type id, std::size_t state
              , Iterator const& first, Iterator const& last)
          : base_type(id, state, first, last) {}

        token_value_type& value() { return this->base_type::matched(); }
        token_value_type const& value() const { return this->base_type::matched(); }
    };

    template <typename Iterator, typename HasState, typename Idtype>
    struct position_token<Iterator, mpl::vector0<>, HasState, Idtype>
      : position_token<Iterator, lex::omit, HasState, Idtype>
    {
    private:
        typedef position_token<Iterator, lex::omit, HasState, Idtype> base_type;

    public:
        typedef typename base_type::id_type id_type;
        typedef typename base_type::iterator_type iterator_type;
        typedef typename base_type::iterpair_type iterpair_type;
        typedef HasState has_state;
        typedef iterpair_type token_value_type;

        //  default constructed tokens correspond to EOI tokens
        position_token() {}

        //  construct an invalid token
        explicit position_token(int) 
          : base_type(0) {}

        position_token(id_type id, std::size_t state)
          : base_type(id, state) {}

        position_token(id_type id, std::size_t state, token_value_type)
          : base_type(id, state, unused) {}

        position_token(id_type id, std::size_t state
              , Iterator const& first, Iterator const& last)
          : base_type(id, state, first, last) {}

        token_value_type& value() { return this->base_type::matched(); }
        token_value_type const& value() const { return this->base_type::matched(); }
    };

    ///////////////////////////////////////////////////////////////////////////
    // These specializations for an attribute list of length one cause all token 
    // instances to expose the specified type as its attribute.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Attribute, typename HasState
      , typename Idtype>
    struct position_token<Iterator, mpl::vector<Attribute>, HasState, Idtype>
      : position_token<Iterator, lex::omit, HasState, Idtype>
    {
    private:
        typedef position_token<Iterator, lex::omit, HasState, Idtype> base_type;

    public:
        typedef typename base_type::id_type id_type;
        typedef typename base_type::iterator_type iterator_type;
        typedef typename base_type::iterpair_type iterpair_type;
        typedef HasState has_state;
        typedef boost::optional<Attribute> token_value_type;

        //  default constructed tokens correspond to EOI tokens
        position_token() {}

        //  construct an invalid token
        explicit position_token(int) 
          : base_type(0) {}

        position_token(id_type id, std::size_t state)
          : base_type(id, state) {}

        position_token(id_type id, std::size_t state, token_value_type const& v)
          : base_type(id, state, unused), value_(v) {}

        position_token(id_type id, std::size_t state
              , Iterator const& first, Iterator const& last)
          : base_type(id, state, first, last) {}

        token_value_type& value() { return value_; }
        token_value_type const& value() const { return value_; }

        bool has_value() const { return !!value_; }

#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
        // workaround for MSVC10 which has problems copying a default 
        // constructed iterator_range
        position_token& operator= (position_token const& rhs)
        {
            if (this != &rhs) 
            {
                this->base_type::operator=(static_cast<base_type const&>(rhs));
                if (this->is_valid()) 
                    value_ = rhs.value_;
            }
            return *this;
        }
#endif

    protected:
        token_value_type value_; // token value
    };

    template <typename Iterator, typename Attribute, typename HasState
      , typename Idtype>
    struct position_token<Iterator, mpl::vector1<Attribute>, HasState, Idtype>
      : position_token<Iterator, lex::omit, HasState, Idtype>
    {
    private:
        typedef position_token<Iterator, lex::omit, HasState, Idtype> base_type;

    public:
        typedef typename base_type::id_type id_type;
        typedef typename base_type::iterator_type iterator_type;
        typedef typename base_type::iterpair_type iterpair_type;
        typedef HasState has_state;
        typedef boost::optional<Attribute> token_value_type;

        //  default constructed tokens correspond to EOI tokens
        position_token() {}

        //  construct an invalid token
        explicit position_token(int) 
          : base_type(0) {}

        position_token(id_type id, std::size_t state)
          : base_type(id, state) {}

        position_token(id_type id, std::size_t state, token_value_type const& v)
          : base_type(id, state, unused), value_(v) {}

        position_token(id_type id, std::size_t state
              , Iterator const& first, Iterator const& last)
          : base_type(id, state, first, last) {}

        token_value_type& value() { return value_; }
        token_value_type const& value() const { return value_; }

        bool has_value() const { return value_; }

#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
        // workaround for MSVC10 which has problems copying a default 
        // constructed iterator_range
        position_token& operator= (position_token const& rhs)
        {
            if (this != &rhs) 
            {
                this->base_type::operator=(static_cast<base_type const&>(rhs));
                if (this->is_valid()) 
                    value_ = rhs.value_;
            }
            return *this;
        }
#endif

    protected:
        token_value_type value_; // token value
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The generic version of the position_token type derives from the 
    //  specialization above and adds a single data member holding the item 
    //  data carried by the token instance.
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        //  Meta-function to calculate the type of the variant data item to be 
        //  stored with each token instance.
        //
        //  Note: The iterator pair needs to be the first type in the list of 
        //        types supported by the generated variant type (this is being 
        //        used to identify whether the stored data item in a particular 
        //        token instance needs to be converted from the pair of 
        //        iterators (see the first of the assign_to_attribute_from_value 
        //        specializations below).
        ///////////////////////////////////////////////////////////////////////
        template <typename IteratorPair, typename AttributeTypes>
        struct position_token_value_typesequence
        {
            typedef typename mpl::insert<
                AttributeTypes
              , typename mpl::begin<AttributeTypes>::type
              , IteratorPair
            >::type sequence_type;
            typedef typename make_variant_over<sequence_type>::type type;
        };

        ///////////////////////////////////////////////////////////////////////
        //  The type of the data item stored with a token instance is defined 
        //  by the template parameter 'AttributeTypes' and may be:
        //  
        //     lex::omit:         no data item is stored with the token 
        //                        instance (this is handled by the 
        //                        specializations of the token class
        //                        below)
        //     mpl::vector0<>:    each token instance stores a pair of 
        //                        iterators pointing to the matched input 
        //                        sequence
        //     mpl::vector<...>:  each token instance stores a variant being 
        //                        able to store the pair of iterators pointing 
        //                        to the matched input sequence, or any of the 
        //                        types a specified in the mpl::vector<>
        //
        //  All this is done to ensure the token type is as small (in terms 
        //  of its byte-size) as possible.
        ///////////////////////////////////////////////////////////////////////
        template <typename IteratorPair, typename AttributeTypes>
        struct position_token_value
          : mpl::eval_if<
                mpl::or_<
                    is_same<AttributeTypes, mpl::vector0<> >
                  , is_same<AttributeTypes, mpl::vector<> > >
              , mpl::identity<IteratorPair>
              , position_token_value_typesequence<IteratorPair, AttributeTypes> >
        {};
    }

    template <typename Iterator, typename AttributeTypes, typename HasState
      , typename Idtype>
    struct position_token 
      : position_token<Iterator, lex::omit, HasState, Idtype>
    {
    private: // precondition assertions
        BOOST_STATIC_ASSERT((mpl::is_sequence<AttributeTypes>::value || 
                            is_same<AttributeTypes, lex::omit>::value));
        typedef position_token<Iterator, lex::omit, HasState, Idtype> 
            base_type;

    protected: 
        //  If no additional token value types are given, the token will 
        //  hold no token value at all as the base class already has the 
        //  iterator pair of the matched range in the underlying input sequence. 
        //  Otherwise the token value is stored as a variant and will 
        //  initially hold an unused_type but is able to hold any of 
        //  the given data types as well. The conversion from the iterator pair 
        //  to the required data type is done when it is accessed for the first 
        //  time.
        typedef iterator_range<Iterator> iterpair_type;

    public:
        typedef typename base_type::id_type id_type;
        typedef typename detail::position_token_value<
            iterpair_type, AttributeTypes>::type token_value_type;

        typedef Iterator iterator_type;

        //  default constructed tokens correspond to EOI tokens
        position_token() {}

        //  construct an invalid token
        explicit position_token(int)
          : base_type(0) {}

        position_token(id_type id, std::size_t state, token_value_type const& value)
          : base_type(id, state, value), value_(value) {}

        position_token(id_type id, std::size_t state, Iterator const& first
              , Iterator const& last)
          : base_type(id, state, first, last)
          , value_(iterpair_type(first, last)) 
        {}

        token_value_type& value() { return value_; }
        token_value_type const& value() const { return value_; }

#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
        // workaround for MSVC10 which has problems copying a default 
        // constructed iterator_range
        position_token& operator= (position_token const& rhs)
        {
            if (this != &rhs) 
            {
                this->base_type::operator=(static_cast<base_type const&>(rhs));
                if (this->is_valid()) 
                    value_ = rhs.value_;
            }
            return *this;
        }
#endif

    protected:
        token_value_type value_; // token value, by default a pair of iterators
    };

    ///////////////////////////////////////////////////////////////////////////
    //  tokens are considered equal, if their id's match (these are unique)
    template <typename Iterator, typename AttributeTypes, typename HasState
      , typename Idtype>
    inline bool 
    operator== (position_token<Iterator, AttributeTypes, HasState, Idtype> const& lhs, 
                position_token<Iterator, AttributeTypes, HasState, Idtype> const& rhs)
    {
        return lhs.id() == rhs.id();
    }

    ///////////////////////////////////////////////////////////////////////////
    //  This overload is needed by the multi_pass/functor_input_policy to 
    //  validate a token instance. It has to be defined in the same namespace 
    //  as the token class itself to allow ADL to find it.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename AttributeTypes, typename HasState
      , typename Idtype>
    inline bool 
    token_is_valid(position_token<Iterator, AttributeTypes, HasState, Idtype> const& t)
    {
        return t.is_valid();
    }
}}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  We have to provide specializations for the customization point
    //  assign_to_attribute_from_value allowing to extract the needed value 
    //  from the token. 
    ///////////////////////////////////////////////////////////////////////////

    //  This is called from the parse function of token_def if the token_def
    //  has been defined to carry a special attribute type
    template <typename Attribute, typename Iterator, typename AttributeTypes
      , typename HasState, typename Idtype>
    struct assign_to_attribute_from_value<Attribute
      , lex::lexertl::position_token<Iterator, AttributeTypes, HasState, Idtype> >
    {
        static void 
        call(lex::lexertl::position_token<
                Iterator, AttributeTypes, HasState, Idtype> const& t
          , Attribute& attr)
        {
        //  The goal of this function is to avoid the conversion of the pair of
        //  iterators (to the matched character sequence) into the token value 
        //  of the required type being done more than once. For this purpose it 
        //  checks whether the stored value type is still the default one (pair 
        //  of iterators) and if yes, replaces the pair of iterators with the 
        //  converted value to be returned from subsequent calls.

            if (0 == t.value().which()) {
            //  first access to the token value
                typedef iterator_range<Iterator> iterpair_type;
                iterpair_type const& ip = t.matched();

            // Interestingly enough we use the assign_to() framework defined in 
            // Spirit.Qi allowing to convert the pair of iterators to almost any 
            // required type (assign_to(), if available, uses the standard Spirit 
            // parsers to do the conversion).
                spirit::traits::assign_to(ip.begin(), ip.end(), attr);

            //  If you get an error during the compilation of the following 
            //  assignment expression, you probably forgot to list one or more 
            //  types used as token value types (in your token_def<...> 
            //  definitions) in your definition of the token class. I.e. any token 
            //  value type used for a token_def<...> definition has to be listed 
            //  during the declaration of the token type to use. For instance let's 
            //  assume we have two token_def's:
            //
            //      token_def<int> number; number = "...";
            //      token_def<std::string> identifier; identifier = "...";
            //
            //  Then you'll have to use the following token type definition 
            //  (assuming you are using the token class):
            //
            //      typedef mpl::vector<int, std::string> token_values;
            //      typedef token<base_iter_type, token_values> token_type;
            //
            //  where: base_iter_type is the iterator type used to expose the 
            //         underlying input stream.
            //
            //  This token_type has to be used as the second template parameter 
            //  to the lexer class:
            //
            //      typedef lexer<base_iter_type, token_type> lexer_type;
            //
            //  again, assuming you're using the lexer<> template for your 
            //  tokenization.

                typedef lex::lexertl::position_token<
                    Iterator, AttributeTypes, HasState, Idtype> token_type;
                spirit::traits::assign_to(
                    attr, const_cast<token_type&>(t).value());   // re-assign value
            }
            else {
            // reuse the already assigned value
                spirit::traits::assign_to(get<Attribute>(t.value()), attr);
            }
        }
    };

    template <typename Attribute, typename Iterator, typename AttributeTypes
      , typename HasState, typename Idtype>
    struct assign_to_container_from_value<Attribute
          , lex::lexertl::position_token<Iterator, AttributeTypes, HasState, Idtype> >
      : assign_to_attribute_from_value<Attribute
          , lex::lexertl::position_token<Iterator, AttributeTypes, HasState, Idtype> >
    {};

    ///////////////////////////////////////////////////////////////////////////
    //  These are called from the parse function of token_def if the token type
    //  has no special attribute type assigned 
    template <typename Attribute, typename Iterator, typename HasState
      , typename Idtype>
    struct assign_to_attribute_from_value<Attribute
      , lex::lexertl::position_token<Iterator, mpl::vector0<>, HasState, Idtype> >
    {
        static void 
        call(lex::lexertl::position_token<
                Iterator, mpl::vector0<>, HasState, Idtype> const& t
          , Attribute& attr)
        {
            //  The default type returned by the token_def parser component (if 
            //  it has no token value type assigned) is the pair of iterators 
            //  to the matched character sequence.
            spirit::traits::assign_to(t.begin(), t.end(), attr);
        }
    };

//     template <typename Attribute, typename Iterator, typename HasState
//       , typename Idtype>
//     struct assign_to_container_from_value<Attribute
//           , lex::lexertl::position_token<Iterator, mpl::vector0<>, HasState, Idtype> >
//       : assign_to_attribute_from_value<Attribute
//           , lex::lexertl::position_token<Iterator, mpl::vector0<>, HasState, Idtype> >
//     {};

    // same as above but using mpl::vector<> instead of mpl::vector0<>
    template <typename Attribute, typename Iterator, typename HasState
      , typename Idtype>
    struct assign_to_attribute_from_value<Attribute
      , lex::lexertl::position_token<Iterator, mpl::vector<>, HasState, Idtype> >
    {
        static void 
        call(lex::lexertl::position_token<
                Iterator, mpl::vector<>, HasState, Idtype> const& t
          , Attribute& attr)
        {
            //  The default type returned by the token_def parser component (if 
            //  it has no token value type assigned) is the pair of iterators 
            //  to the matched character sequence.
            spirit::traits::assign_to(t.begin(), t.end(), attr);
        }
    };

//     template <typename Attribute, typename Iterator, typename HasState
//       , typename Idtype>
//     struct assign_to_container_from_value<Attribute
//           , lex::lexertl::position_token<Iterator, mpl::vector<>, HasState, Idtype> >
//       : assign_to_attribute_from_value<Attribute
//           , lex::lexertl::position_token<Iterator, mpl::vector<>, HasState, Idtype> >
//     {};

    ///////////////////////////////////////////////////////////////////////////
    //  These are called from the parse function of token_def if the token type
    //  has no special attribute type assigned 
    template <typename Attribute, typename Iterator, typename Attr
      , typename HasState, typename Idtype>
    struct assign_to_attribute_from_value<Attribute
      , lex::lexertl::position_token<Iterator, mpl::vector1<Attr>, HasState, Idtype> >
    {
        static void 
        call(lex::lexertl::position_token<
                Iterator, mpl::vector1<Attr>, HasState, Idtype> const& t
          , Attribute& attr)
        {
        //  The goal of this function is to avoid the conversion of the pair of
        //  iterators (to the matched character sequence) into the token value 
        //  of the required type being done more than once.

            if (!t.has_value()) {
            //  first access to the token value
                typedef iterator_range<Iterator> iterpair_type;
                iterpair_type const& ip = t.matched();

            // Interestingly enough we use the assign_to() framework defined in 
            // Spirit.Qi allowing to convert the pair of iterators to almost any 
            // required type (assign_to(), if available, uses the standard Spirit 
            // parsers to do the conversion).
                spirit::traits::assign_to(ip.begin(), ip.end(), attr);

            // Re-assign the attribute to the stored value
                typedef lex::lexertl::position_token<
                    Iterator, mpl::vector1<Attr>, HasState, Idtype> token_type;
                spirit::traits::assign_to(
                    attr, const_cast<token_type&>(t).value());
            }
            else {
            // reuse the already assigned value
                spirit::traits::assign_to(t.value(), attr);
            }
        }
    };

//     template <typename Attribute, typename Iterator, typename Attr
//       , typename HasState, typename Idtype>
//     struct assign_to_container_from_value<Attribute
//           , lex::lexertl::position_token<Iterator, mpl::vector1<Attr>, HasState, Idtype> >
//       : assign_to_attribute_from_value<Attribute
//           , lex::lexertl::position_token<Iterator, mpl::vector1<Attr>, HasState, Idtype> >
//     {};

    // same as above but using mpl::vector<Attr> instead of mpl::vector1<Attr>
    template <typename Attribute, typename Iterator, typename Attr
      , typename HasState, typename Idtype>
    struct assign_to_attribute_from_value<Attribute
      , lex::lexertl::position_token<Iterator, mpl::vector<Attr>, HasState, Idtype> >
    {
        static void 
        call(lex::lexertl::position_token<
                Iterator, mpl::vector<Attr>, HasState, Idtype> const& t
          , Attribute& attr)
        {
        //  The goal of this function is to avoid the conversion of the pair of
        //  iterators (to the matched character sequence) into the token value 
        //  of the required type being done more than once.

            if (!t.has_value()) {
            //  first access to the token value
                typedef iterator_range<Iterator> iterpair_type;
                iterpair_type const& ip = t.matched();

            // Interestingly enough we use the assign_to() framework defined in 
            // Spirit.Qi allowing to convert the pair of iterators to almost any 
            // required type (assign_to(), if available, uses the standard Spirit 
            // parsers to do the conversion).
                spirit::traits::assign_to(ip.begin(), ip.end(), attr);

            // Re-assign the attribute to the stored value
                typedef lex::lexertl::position_token<
                    Iterator, mpl::vector<Attr>, HasState, Idtype> token_type;
                spirit::traits::assign_to(
                    attr, const_cast<token_type&>(t).value());
            }
            else {
            // reuse the already assigned value
                spirit::traits::assign_to(t.value(), attr);
            }
        }
    };

//     template <typename Attribute, typename Iterator, typename Attr
//       , typename HasState, typename Idtype>
//     struct assign_to_container_from_value<Attribute
//           , lex::lexertl::position_token<Iterator, mpl::vector<Attr>, HasState, Idtype> >
//       : assign_to_attribute_from_value<Attribute
//           , lex::lexertl::position_token<Iterator, mpl::vector<Attr>, HasState, Idtype> >
//     {};

    //  This is called from the parse function of token_def if the token type
    //  has been explicitly omitted (i.e. no attribute value is used), which
    //  essentially means that every attribute gets initialized using default 
    //  constructed values.
    template <typename Attribute, typename Iterator, typename HasState
      , typename Idtype>
    struct assign_to_attribute_from_value<Attribute
      , lex::lexertl::position_token<Iterator, lex::omit, HasState, Idtype> >
    {
        static void 
        call(lex::lexertl::position_token<Iterator, lex::omit, HasState, Idtype> const&
          , Attribute&)
        {
            // do nothing
        }
    };

    template <typename Attribute, typename Iterator, typename HasState
      , typename Idtype>
    struct assign_to_container_from_value<Attribute
          , lex::lexertl::position_token<Iterator, lex::omit, HasState, Idtype> >
      : assign_to_attribute_from_value<Attribute
          , lex::lexertl::position_token<Iterator, lex::omit, HasState, Idtype> >
    {};

    //  This is called from the parse function of lexer_def_
    template <typename Iterator, typename AttributeTypes, typename HasState
      , typename Idtype_, typename Idtype>
    struct assign_to_attribute_from_value<
        fusion::vector2<Idtype_, iterator_range<Iterator> >
      , lex::lexertl::position_token<Iterator, AttributeTypes, HasState, Idtype> >
    {
        static void 
        call(lex::lexertl::position_token<Iterator, AttributeTypes, HasState, Idtype> const& t
          , fusion::vector2<Idtype_, iterator_range<Iterator> >& attr)
        {
            //  The type returned by the lexer_def_ parser components is a 
            //  fusion::vector containing the token id of the matched token 
            //  and the pair of iterators to the matched character sequence.
            typedef iterator_range<Iterator> iterpair_type;
            typedef fusion::vector2<Idtype_, iterator_range<Iterator> > 
                attribute_type;

            iterpair_type const& ip = t.matched();
            attr = attribute_type(t.id(), ip);
        }
    };

    template <typename Iterator, typename AttributeTypes, typename HasState
      , typename Idtype_, typename Idtype>
    struct assign_to_container_from_value<
            fusion::vector2<Idtype_, iterator_range<Iterator> >
          , lex::lexertl::position_token<Iterator, AttributeTypes, HasState, Idtype> >
      : assign_to_attribute_from_value<
            fusion::vector2<Idtype_, iterator_range<Iterator> >
          , lex::lexertl::position_token<Iterator, AttributeTypes, HasState, Idtype> >
    {};

    ///////////////////////////////////////////////////////////////////////////
    // Overload debug output for a single token, this integrates lexer tokens 
    // with Qi's simple_trace debug facilities
    template <typename Iterator, typename Attribute, typename HasState
      , typename Idtype>
    struct token_printer_debug<
        lex::lexertl::position_token<Iterator, Attribute, HasState, Idtype> >
    {
        typedef lex::lexertl::position_token<Iterator, Attribute, HasState, Idtype> token_type;

        template <typename Out>
        static void print(Out& out, token_type const& val) 
        {
            out << '[';
            spirit::traits::print_token(out, val.value());
            out << ']';
        }
    };
}}}

#endif

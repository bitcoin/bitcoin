/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Martin Wille
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined BOOST_SPIRIT_GRAMMAR_IPP
#define BOOST_SPIRIT_GRAMMAR_IPP

#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
#include <boost/spirit/home/classic/core/non_terminal/impl/object_with_id.ipp>
#include <algorithm>
#include <functional>
#include <boost/move/unique_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#ifdef BOOST_SPIRIT_THREADSAFE
#include <boost/spirit/home/classic/core/non_terminal/impl/static.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

template <typename DerivedT, typename ContextT>
struct grammar;


//////////////////////////////////
template <typename GrammarT, typename ScannerT>
struct grammar_definition
{
    typedef typename GrammarT::template definition<ScannerT> type;
};


    namespace impl
    {

#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
    struct grammar_tag {};

    //////////////////////////////////
    template <typename GrammarT>
    struct grammar_helper_base
    {
        virtual int undefine(GrammarT *) = 0;
        virtual ~grammar_helper_base() {}
    };

    //////////////////////////////////
    template <typename GrammarT>
    struct grammar_helper_list
    {
        typedef GrammarT                      grammar_t;
        typedef grammar_helper_base<GrammarT> helper_t;
        typedef std::vector<helper_t*>        vector_t;

        grammar_helper_list() {}
        grammar_helper_list(grammar_helper_list const& /*x*/)
        {   // Does _not_ copy the helpers member !
        }

        grammar_helper_list& operator=(grammar_helper_list const& /*x*/)
        {   // Does _not_ copy the helpers member !
            return *this;
        }

        void push_back(helper_t *helper)
        { helpers.push_back(helper); }

        void pop_back()
        { helpers.pop_back(); }

        typename vector_t::size_type
        size() const
        { return helpers.size(); }

        typename vector_t::reverse_iterator
        rbegin()
        { return helpers.rbegin(); }

        typename vector_t::reverse_iterator
        rend()
        { return helpers.rend(); }

#ifdef BOOST_SPIRIT_THREADSAFE
        boost::mutex & mutex()
        { return m; }
#endif

    private:

        vector_t        helpers;
#ifdef BOOST_SPIRIT_THREADSAFE
        boost::mutex    m;
#endif
    };

    //////////////////////////////////
    struct grammartract_helper_list;

#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)

    struct grammartract_helper_list
    {
        template<typename GrammarT>
        static grammar_helper_list<GrammarT>&
        do_(GrammarT const* g)
        {
            return g->helpers;
        }
    };

#endif

    //////////////////////////////////
    template <typename GrammarT, typename DerivedT, typename ScannerT>
    struct grammar_helper : private grammar_helper_base<GrammarT>
    {
        typedef GrammarT grammar_t;
        typedef ScannerT scanner_t;
        typedef DerivedT derived_t;
        typedef typename grammar_definition<DerivedT, ScannerT>::type definition_t;

        typedef grammar_helper<grammar_t, derived_t, scanner_t> helper_t;
        typedef boost::shared_ptr<helper_t> helper_ptr_t;
        typedef boost::weak_ptr<helper_t>   helper_weak_ptr_t;

        grammar_helper*
        this_() { return this; }

        grammar_helper(helper_weak_ptr_t& p)
        : definitions_cnt(0)
        , self(this_())
        { p = self; }

        definition_t&
        define(grammar_t const* target_grammar)
        {
            grammar_helper_list<GrammarT> &helpers =
            grammartract_helper_list::do_(target_grammar);
            typename grammar_t::object_id id = target_grammar->get_object_id();

            if (definitions.size()<=id)
                definitions.resize(id*3/2+1);
            if (definitions[id]!=0)
                return *definitions[id];

            boost::movelib::unique_ptr<definition_t>
                result(new definition_t(target_grammar->derived()));

#ifdef BOOST_SPIRIT_THREADSAFE
            boost::unique_lock<boost::mutex> lock(helpers.mutex());
#endif
            helpers.push_back(this);

            ++definitions_cnt;
            definitions[id] = result.get();
            return *(result.release());
        }

        int
        undefine(grammar_t* target_grammar) BOOST_OVERRIDE
        {
            typename grammar_t::object_id id = target_grammar->get_object_id();

            if (definitions.size()<=id)
                return 0;
            delete definitions[id];
            definitions[id] = 0;
            if (--definitions_cnt==0)
                self.reset();
            return 0;
        }

    private:

        std::vector<definition_t*>  definitions;
        unsigned long               definitions_cnt;
        helper_ptr_t                self;
    };

#endif /* defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE) */

#ifdef BOOST_SPIRIT_THREADSAFE
    class get_definition_static_data_tag
    {
        template<typename DerivedT, typename ContextT, typename ScannerT>
        friend typename DerivedT::template definition<ScannerT> &
            get_definition(grammar<DerivedT, ContextT> const*  self);

        get_definition_static_data_tag() {}
    };
#endif

    template<typename DerivedT, typename ContextT, typename ScannerT>
    inline typename DerivedT::template definition<ScannerT> &
    get_definition(grammar<DerivedT, ContextT> const*  self)
    {
#if defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)

        typedef typename DerivedT::template definition<ScannerT> definition_t;
        static definition_t def(self->derived());
        return def;
#else
        typedef grammar<DerivedT, ContextT>                      self_t;
        typedef impl::grammar_helper<self_t, DerivedT, ScannerT> helper_t;
        typedef typename helper_t::helper_weak_ptr_t             ptr_t;

# ifdef BOOST_SPIRIT_THREADSAFE
        boost::thread_specific_ptr<ptr_t> & tld_helper
            = static_<boost::thread_specific_ptr<ptr_t>,
                get_definition_static_data_tag>(get_definition_static_data_tag());

        if (!tld_helper.get())
            tld_helper.reset(new ptr_t);
        ptr_t &helper = *tld_helper;
# else
        static ptr_t helper;
# endif
        if (helper.expired())
            new helper_t(helper);
        return helper.lock()->define(self);
#endif
    }

    template <int N>
    struct call_helper {

        template <typename RT, typename DefinitionT, typename ScannerT>
        static void
        do_ (RT &result, DefinitionT &def, ScannerT const &scan)
        {
            result = def.template get_start_parser<N>()->parse(scan);
        }
    };

    template <>
    struct call_helper<0> {

        template <typename RT, typename DefinitionT, typename ScannerT>
        static void
        do_ (RT &result, DefinitionT &def, ScannerT const &scan)
        {
            result = def.start().parse(scan);
        }
    };

    //////////////////////////////////
    template<int N, typename DerivedT, typename ContextT, typename ScannerT>
    inline typename parser_result<grammar<DerivedT, ContextT>, ScannerT>::type
    grammar_parser_parse(
        grammar<DerivedT, ContextT> const*  self,
        ScannerT const &scan)
    {
        typedef
            typename parser_result<grammar<DerivedT, ContextT>, ScannerT>::type
            result_t;
        typedef typename DerivedT::template definition<ScannerT> definition_t;

        result_t result;
        definition_t &def = get_definition<DerivedT, ContextT, ScannerT>(self);

        call_helper<N>::do_(result, def, scan);
        return result;
    }

    //////////////////////////////////
    template<typename GrammarT>
    inline void
    grammar_destruct(GrammarT* self)
    {
#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
        typedef grammar_helper_list<GrammarT> helper_list_t;

        helper_list_t&  helpers =
        grammartract_helper_list::do_(self);

        typedef typename helper_list_t::vector_t::reverse_iterator iterator_t;

        for (iterator_t i = helpers.rbegin(); i != helpers.rend(); ++i)
            (*i)->undefine(self);
#else
        (void)self;
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  entry_grammar class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename DerivedT, int N, typename ContextT>
    class entry_grammar
        : public parser<entry_grammar<DerivedT, N, ContextT> >
    {

    public:
        typedef entry_grammar<DerivedT, N, ContextT>    self_t;
        typedef self_t                                  embed_t;
        typedef typename ContextT::context_linker_t     context_t;
        typedef typename context_t::attr_t              attr_t;

        template <typename ScannerT>
        struct result
        {
            typedef typename match_result<ScannerT, attr_t>::type type;
        };

        entry_grammar(DerivedT const &p) : target_grammar(p) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse_main(ScannerT const& scan) const
        { return impl::grammar_parser_parse<N>(&target_grammar, scan); }

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            typedef parser_scanner_linker<ScannerT> scanner_t;
            BOOST_SPIRIT_CONTEXT_PARSE(scan, target_grammar, scanner_t,
                context_t, result_t)
        }

    private:
        DerivedT const &target_grammar;
    };

    } // namespace impl

///////////////////////////////////////
#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
#define BOOST_SPIRIT_GRAMMAR_ID , public impl::object_with_id<impl::grammar_tag>
#else
#define BOOST_SPIRIT_GRAMMAR_ID
#endif

///////////////////////////////////////
#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
#define BOOST_SPIRIT_GRAMMAR_STATE                            \
    private:                                                  \
    friend struct impl::grammartract_helper_list;    \
    mutable impl::grammar_helper_list<self_t> helpers;
#else
#define BOOST_SPIRIT_GRAMMAR_STATE
#endif

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif

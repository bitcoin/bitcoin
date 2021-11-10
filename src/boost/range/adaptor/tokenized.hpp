// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_TOKENIZED_HPP
#define BOOST_RANGE_ADAPTOR_TOKENIZED_HPP

#include <boost/regex.hpp>
#include <boost/range/iterator_range.hpp>

namespace boost
{
    namespace range_detail
    {

        template< class R >
        struct tokenized_range : 
            public boost::iterator_range< 
                      boost::regex_token_iterator< 
                          BOOST_DEDUCED_TYPENAME range_iterator<R>::type 
                                              >
                                         >
        {
        private:
            typedef           
                boost::regex_token_iterator< 
                          BOOST_DEDUCED_TYPENAME range_iterator<R>::type 
                                            >
                regex_iter;
            
            typedef BOOST_DEDUCED_TYPENAME regex_iter::regex_type 
                regex_type;
        
            typedef boost::iterator_range<regex_iter> 
                base;

        public:
            template< class Regex, class Submatch, class Flag >
            tokenized_range( R& r, const Regex& re, const Submatch& sub, Flag f )
              : base( regex_iter( boost::begin(r), boost::end(r), 
                                  regex_type(re), sub, f ),
                      regex_iter() )
            { }
        };

        template< class T, class U, class V >
        struct regex_holder
        {
            T  re;
            U  sub;
            V  f;

            regex_holder( const T& rex, const U& subm, V flag ) :
                re(rex), sub(subm), f(flag)
            { }
        private:
            // Not assignable
            void operator=(const regex_holder&);
        };

        struct regex_forwarder
        {           
            template< class Regex >
            regex_holder<Regex,int,regex_constants::match_flag_type>
            operator()( const Regex& re, 
                        int submatch = 0,    
                        regex_constants::match_flag_type f = 
                            regex_constants::match_default ) const
            {
                return regex_holder<Regex,int,
                           regex_constants::match_flag_type>( re, submatch, f );
            }
             
            template< class Regex, class Submatch >
            regex_holder<Regex,Submatch,regex_constants::match_flag_type> 
            operator()( const Regex& re, 
                        const Submatch& sub, 
                        regex_constants::match_flag_type f = 
                            regex_constants::match_default ) const
            {
                return regex_holder<Regex,Submatch,
                           regex_constants::match_flag_type>( re, sub, f ); 
            }
        };
        
        template< class BidirectionalRng, class R, class S, class F >
        inline tokenized_range<BidirectionalRng> 
        operator|( BidirectionalRng& r, 
                   const regex_holder<R,S,F>& f )
        {
            return tokenized_range<BidirectionalRng>( r, f.re, f.sub, f.f );   
        }

        template< class BidirectionalRng, class R, class S, class F  >
        inline tokenized_range<const BidirectionalRng> 
        operator|( const BidirectionalRng& r, 
                   const regex_holder<R,S,F>& f )
        {
            return tokenized_range<const BidirectionalRng>( r, f.re, f.sub, f.f );
        }
        
    } // 'range_detail'

    using range_detail::tokenized_range;

    namespace adaptors
    { 
        namespace
        {
            const range_detail::regex_forwarder tokenized = 
                    range_detail::regex_forwarder();
        }
        
        template<class BidirectionalRange, class Regex, class Submatch, class Flag>
        inline tokenized_range<BidirectionalRange>
        tokenize(BidirectionalRange& rng, const Regex& reg, const Submatch& sub, Flag f)
        {
            return tokenized_range<BidirectionalRange>(rng, reg, sub, f);
        }
        
        template<class BidirectionalRange, class Regex, class Submatch, class Flag>
        inline tokenized_range<const BidirectionalRange>
        tokenize(const BidirectionalRange& rng, const Regex& reg, const Submatch& sub, Flag f)
        {
            return tokenized_range<const BidirectionalRange>(rng, reg, sub, f);
        }
    } // 'adaptors'
    
}

#endif

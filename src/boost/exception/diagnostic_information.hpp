//Copyright (c) 2006-2010 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_0552D49838DD11DD90146B8956D89593
#define BOOST_EXCEPTION_0552D49838DD11DD90146B8956D89593

#include <boost/config.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/info.hpp>
#include <boost/utility/enable_if.hpp>
#ifndef BOOST_NO_RTTI
#include <boost/core/demangle.hpp>
#endif
#include <exception>
#include <sstream>
#include <string>
#ifndef BOOST_NO_EXCEPTIONS
#include <boost/exception/current_exception_cast.hpp>
#endif

#ifndef BOOST_EXCEPTION_ENABLE_WARNINGS
#if __GNUC__*100+__GNUC_MINOR__>301
#pragma GCC system_header
#endif
#ifdef __clang__
#pragma clang system_header
#endif
#ifdef _MSC_VER
#pragma warning(push,1)
#endif
#endif

#ifndef BOOST_NO_EXCEPTIONS
namespace
boost
    {
    namespace
    exception_detail
        {
        std::string diagnostic_information_impl( boost::exception const *, std::exception const *, bool, bool );
        }

    inline
    std::string
    current_exception_diagnostic_information( bool verbose=true)
        {
        boost::exception const * be=current_exception_cast<boost::exception const>();
        std::exception const * se=current_exception_cast<std::exception const>();
        if( be || se )
            return exception_detail::diagnostic_information_impl(be,se,true,verbose);
#if defined(__GLIBCXX__) && __cplusplus >= 201103L && !defined(BOOST_NO_RTTI)
        else if (auto* p=std::current_exception().__cxa_exception_type())
            return "Dynamic exception type: "+boost::core::demangle(p->name());
#endif
        else
            return "No diagnostic information available.";
        }
    }
#endif

namespace
boost
    {
    namespace
    exception_detail
        {
        inline
        exception const *
        get_boost_exception( exception const * e )
            {
            return e;
            }

        inline
        exception const *
        get_boost_exception( ... )
            {
            return 0;
            }

        inline
        std::exception const *
        get_std_exception( std::exception const * e )
            {
            return e;
            }

        inline
        std::exception const *
        get_std_exception( ... )
            {
            return 0;
            }

        inline
        char const *
        get_diagnostic_information( exception const & x, char const * header )
            {
#ifndef BOOST_NO_EXCEPTIONS
            try
                {
#endif
                error_info_container * c=x.data_.get();
                if( !c )
                    x.data_.adopt(c=new exception_detail::error_info_container_impl);
                char const * di=c->diagnostic_information(header);
                BOOST_ASSERT(di!=0);
                return di;
#ifndef BOOST_NO_EXCEPTIONS
                }
            catch(...)
                {
                return 0;
                }
#endif
            }

        inline
        std::string
        diagnostic_information_impl( boost::exception const * be, std::exception const * se, bool with_what, bool verbose )
            {
            if( !be && !se )
                return "Unknown exception.";
#ifndef BOOST_NO_RTTI
            if( !be )
                be=dynamic_cast<boost::exception const *>(se);
            if( !se )
                se=dynamic_cast<std::exception const *>(be);
#endif
            char const * wh=0;
            if( with_what && se )
                {
                wh=se->what();
                if( be && exception_detail::get_diagnostic_information(*be,0)==wh )
                    return wh;
                }
            std::ostringstream tmp;
            if( be && verbose )
                {
                char const * const * f=get_error_info<throw_file>(*be);
                int const * l=get_error_info<throw_line>(*be);
                char const * const * fn=get_error_info<throw_function>(*be);
                if( !f && !l && !fn )
                    tmp << "Throw location unknown (consider using BOOST_THROW_EXCEPTION)\n";
                else
                    {
                    if( f )
                        {
                        tmp << *f;
                        if( int const * l=get_error_info<throw_line>(*be) )
                            tmp << '(' << *l << "): ";
                        }
                    tmp << "Throw in function ";
                    if( char const * const * fn=get_error_info<throw_function>(*be) )
                        tmp << *fn;
                    else
                        tmp << "(unknown)";
                    tmp << '\n';
                    }
                }
#ifndef BOOST_NO_RTTI
            if ( verbose )
                tmp << std::string("Dynamic exception type: ") <<
                    core::demangle((be?(BOOST_EXCEPTION_DYNAMIC_TYPEID(*be)):(BOOST_EXCEPTION_DYNAMIC_TYPEID(*se))).type_->name()) << '\n';
#endif
            if( with_what && se && verbose )
                tmp << "std::exception::what: " << (wh ? wh : "(null)") << '\n';
            if( be )
                if( char const * s=exception_detail::get_diagnostic_information(*be,tmp.str().c_str()) )
                    if( *s )
                        return std::string(s);
            return tmp.str();
            }
        }

    template <class T>
    std::string
    diagnostic_information( T const & e, bool verbose=true )
        {
        return exception_detail::diagnostic_information_impl(exception_detail::get_boost_exception(&e),exception_detail::get_std_exception(&e),true,verbose);
        }

    inline
    char const *
    diagnostic_information_what( exception const & e, bool verbose=true ) BOOST_NOEXCEPT_OR_NOTHROW
        {
        char const * w=0;
#ifndef BOOST_NO_EXCEPTIONS
        try
            {
#endif
            (void) exception_detail::diagnostic_information_impl(&e,0,false,verbose);
            if( char const * di=exception_detail::get_diagnostic_information(e,0) )
                return di;
            else
                return "Failed to produce boost::diagnostic_information_what()";
#ifndef BOOST_NO_EXCEPTIONS
            }
        catch(
        ... )
            {
            }
#endif
        return w;
        }
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif

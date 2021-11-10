// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef EXCEPTION_HANDLER_DWA2002810_HPP
# define EXCEPTION_HANDLER_DWA2002810_HPP

# include <boost/python/detail/config.hpp>
# include <boost/function/function0.hpp>
# include <boost/function/function2.hpp>

namespace boost { namespace python { namespace detail {

struct exception_handler;

typedef function2<bool, exception_handler const&, function0<void> const&> handler_function;

struct BOOST_PYTHON_DECL exception_handler
{
 private: // types
    
 public:
    explicit exception_handler(handler_function const& impl);

    inline bool handle(function0<void> const& f) const;
    
    bool operator()(function0<void> const& f) const;
 
    static exception_handler* chain;
    
 private:
    static exception_handler* tail;
    
    handler_function m_impl;
    exception_handler* m_next;
};


inline bool exception_handler::handle(function0<void> const& f) const
{
    return this->m_impl(*this, f);
}

BOOST_PYTHON_DECL void register_exception_handler(handler_function const& f);

}}} // namespace boost::python::detail

#endif // EXCEPTION_HANDLER_DWA2002810_HPP

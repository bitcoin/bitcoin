// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef FORCE_INSTANTIATE_DWA200265_HPP
# define FORCE_INSTANTIATE_DWA200265_HPP

namespace boost { namespace python { namespace detail { 

// Allows us to force the argument to be instantiated without
// incurring unused variable warnings

template <class T>
inline void force_instantiate(T const&) {}

}}} // namespace boost::python::detail

#endif // FORCE_INSTANTIATE_DWA200265_HPP

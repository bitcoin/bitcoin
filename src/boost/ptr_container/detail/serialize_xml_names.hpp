//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2003-2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//

#ifndef BOOST_PTR_CONTAINER_DETAIL_SERIALIZE_XML_NAMES
#define BOOST_PTR_CONTAINER_DETAIL_SERIALIZE_XML_NAMES

namespace boost
{
    namespace ptr_container_detail
    {
        inline const char* count()  { return "count"; }
        inline const char* item()   { return "item"; }
        inline const char* first()  { return "first"; }
        inline const char* second() { return "second"; }

        template<class T>
        inline T const& serialize_as_const(T const& r)
        {
            return r;
        }
    }
}
#endif


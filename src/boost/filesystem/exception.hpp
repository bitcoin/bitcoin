//  boost/filesystem/exception.hpp  -----------------------------------------------------//

//  Copyright Beman Dawes 2003
//  Copyright Andrey Semashev 2019

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

#ifndef BOOST_FILESYSTEM_EXCEPTION_HPP
#define BOOST_FILESYSTEM_EXCEPTION_HPP

#include <boost/filesystem/config.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <boost/filesystem/detail/header.hpp> // must be the last #include

namespace boost {
namespace filesystem {

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                            class filesystem_error                                    //
//                                                                                      //
//--------------------------------------------------------------------------------------//

class BOOST_SYMBOL_VISIBLE filesystem_error :
    public system::system_error
{
    // see http://www.boost.org/more/error_handling.html for design rationale

public:
    BOOST_FILESYSTEM_DECL filesystem_error(const char* what_arg, system::error_code ec);
    BOOST_FILESYSTEM_DECL filesystem_error(std::string const& what_arg, system::error_code ec);
    BOOST_FILESYSTEM_DECL filesystem_error(const char* what_arg, path const& path1_arg, system::error_code ec);
    BOOST_FILESYSTEM_DECL filesystem_error(std::string const& what_arg, path const& path1_arg, system::error_code ec);
    BOOST_FILESYSTEM_DECL filesystem_error(const char* what_arg, path const& path1_arg, path const& path2_arg, system::error_code ec);
    BOOST_FILESYSTEM_DECL filesystem_error(std::string const& what_arg, path const& path1_arg, path const& path2_arg, system::error_code ec);

    BOOST_FILESYSTEM_DECL filesystem_error(filesystem_error const& that);
    BOOST_FILESYSTEM_DECL filesystem_error& operator=(filesystem_error const& that);

    BOOST_FILESYSTEM_DECL ~filesystem_error() BOOST_NOEXCEPT_OR_NOTHROW;

    path const& path1() const BOOST_NOEXCEPT
    {
        return m_imp_ptr.get() ? m_imp_ptr->m_path1 : get_empty_path();
    }
    path const& path2() const BOOST_NOEXCEPT
    {
        return m_imp_ptr.get() ? m_imp_ptr->m_path2 : get_empty_path();
    }

    BOOST_FILESYSTEM_DECL const char* what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE;

private:
    BOOST_FILESYSTEM_DECL static path const& get_empty_path() BOOST_NOEXCEPT;

private:
    struct impl :
        public boost::intrusive_ref_counter< impl >
    {
        path m_path1;       // may be empty()
        path m_path2;       // may be empty()
        std::string m_what; // not built until needed

        BOOST_DEFAULTED_FUNCTION(impl(), {})
        explicit impl(path const& path1) :
            m_path1(path1)
        {
        }
        impl(path const& path1, path const& path2) :
            m_path1(path1), m_path2(path2)
        {
        }
    };
    boost::intrusive_ptr< impl > m_imp_ptr;
};

} // namespace filesystem
} // namespace boost

#include <boost/filesystem/detail/footer.hpp>

#endif // BOOST_FILESYSTEM_EXCEPTION_HPP

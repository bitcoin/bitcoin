// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright 2015-2021 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_SHARED_LIBRARY_IMPL_HPP
#define BOOST_DLL_SHARED_LIBRARY_IMPL_HPP

#include <boost/dll/config.hpp>
#include <boost/dll/shared_library_load_mode.hpp>
#include <boost/dll/detail/aggressive_ptr_cast.hpp>
#include <boost/dll/detail/system_error.hpp>
#include <boost/dll/detail/windows/path_from_handle.hpp>

#include <boost/move/utility.hpp>
#include <boost/swap.hpp>

#include <boost/winapi/dll.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace dll { namespace detail {

class shared_library_impl {
    BOOST_MOVABLE_BUT_NOT_COPYABLE(shared_library_impl)

public:
    typedef boost::winapi::HMODULE_ native_handle_t;

    shared_library_impl() BOOST_NOEXCEPT
        : handle_(NULL)
    {}

    ~shared_library_impl() BOOST_NOEXCEPT {
        unload();
    }

    shared_library_impl(BOOST_RV_REF(shared_library_impl) sl) BOOST_NOEXCEPT
        : handle_(sl.handle_)
    {
        sl.handle_ = NULL;
    }

    shared_library_impl & operator=(BOOST_RV_REF(shared_library_impl) sl) BOOST_NOEXCEPT {
        swap(sl);
        return *this;
    }

    static boost::dll::fs::path decorate(const boost::dll::fs::path& sl) {
        boost::dll::fs::path actual_path = sl;
        actual_path += suffix();
        return actual_path;
    }

    void load(boost::dll::fs::path sl, load_mode::type portable_mode, boost::dll::fs::error_code &ec) {
        typedef boost::winapi::DWORD_ native_mode_t;
        native_mode_t native_mode = static_cast<native_mode_t>(portable_mode);
        unload();

        if (!sl.is_absolute() && !(native_mode & load_mode::search_system_folders)) {
            boost::dll::fs::error_code current_path_ec;
            boost::dll::fs::path prog_loc = boost::dll::fs::current_path(current_path_ec);

            if (!current_path_ec) {
                prog_loc /= sl;
                sl.swap(prog_loc);
            }
        }
        native_mode = static_cast<unsigned>(native_mode) & ~static_cast<unsigned>(load_mode::search_system_folders);

        // Trying to open with appended decorations
        if (!!(native_mode & load_mode::append_decorations)) {
            native_mode = static_cast<unsigned>(native_mode) & ~static_cast<unsigned>(load_mode::append_decorations);

            if (load_impl(decorate(sl), native_mode, ec)) {
                return;
            }

            // MinGW loves 'lib' prefix and puts it even on Windows platform.
            const boost::dll::fs::path mingw_load_path = (
                sl.has_parent_path()
                ? sl.parent_path() / L"lib"
                : L"lib"
            ).native() + sl.filename().native() + suffix().native();
            if (load_impl(mingw_load_path, native_mode, ec)) {
                return;
            }
        }

        // From MSDN: If the string specifies a module name without a path and the
        // file name extension is omitted, the function appends the default library
        // extension .dll to the module name.
        //
        // From experiments: Default library extension appended to the module name even if
        // we have some path. So we do not check for path, only for extension. We can not be sure that
        // such behavior remain across all platforms, so we add L"." by hand.
        if (sl.has_extension()) {
            handle_ = boost::winapi::LoadLibraryExW(sl.c_str(), 0, native_mode);
        } else {
            handle_ = boost::winapi::LoadLibraryExW((sl.native() + L".").c_str(), 0, native_mode);
        }

        // LoadLibraryExW method is capable of self loading from program_location() path. No special actions
        // must be taken to allow self loading.
        if (!handle_) {
            ec = boost::dll::detail::last_error_code();
        }
    }

    bool is_loaded() const BOOST_NOEXCEPT {
        return (handle_ != 0);
    }

    void unload() BOOST_NOEXCEPT {
        if (handle_) {
            boost::winapi::FreeLibrary(handle_);
            handle_ = 0;
        }
    }

    void swap(shared_library_impl& rhs) BOOST_NOEXCEPT {
        boost::swap(handle_, rhs.handle_);
    }

    boost::dll::fs::path full_module_path(boost::dll::fs::error_code &ec) const {
        return boost::dll::detail::path_from_handle(handle_, ec);
    }

    static boost::dll::fs::path suffix() {
        return L".dll";
    }

    void* symbol_addr(const char* sb, boost::dll::fs::error_code &ec) const BOOST_NOEXCEPT {
        if (is_resource()) {
            // `GetProcAddress` could not be called for libraries loaded with
            // `LOAD_LIBRARY_AS_DATAFILE`, `LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE`
            // or `LOAD_LIBRARY_AS_IMAGE_RESOURCE`.
            ec = boost::dll::fs::make_error_code(
                boost::dll::fs::errc::operation_not_supported
            );

            return NULL;
        }

        // Judging by the documentation of GetProcAddress
        // there is no version for UNICODE on desktop/server Windows, because
        // names of functions are stored in narrow characters.
        void* const symbol = boost::dll::detail::aggressive_ptr_cast<void*>(
            boost::winapi::get_proc_address(handle_, sb)
        );
        if (symbol == NULL) {
            ec = boost::dll::detail::last_error_code();
        }

        return symbol;
    }

    native_handle_t native() const BOOST_NOEXCEPT {
        return handle_;
    }

private:
    // Returns true if this load attempt should be the last one.
    bool load_impl(const boost::dll::fs::path &load_path, boost::winapi::DWORD_ mode, boost::dll::fs::error_code &ec) {
        handle_ = boost::winapi::LoadLibraryExW(load_path.c_str(), 0, mode);
        if (handle_) {
            return true;
        }

        ec = boost::dll::detail::last_error_code();
        if (boost::dll::fs::exists(load_path)) {
            // decorated path exists : current error is not a bad file descriptor
            return true;
        }

        ec.clear();
        return false;
    }

    bool is_resource() const BOOST_NOEXCEPT {
        return false; /*!!(
            reinterpret_cast<boost::winapi::ULONG_PTR_>(handle_) & static_cast<boost::winapi::ULONG_PTR_>(3)
        );*/
    }

    native_handle_t handle_;
};

}}} // boost::dll::detail

#endif // BOOST_DLL_SHARED_LIBRARY_IMPL_HPP

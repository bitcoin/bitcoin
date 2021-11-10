//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_UTILITY_PROGRAM_CACHE_HPP
#define BOOST_COMPUTE_UTILITY_PROGRAM_CACHE_HPP

#include <string>
#include <utility>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>

#include <boost/compute/context.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/detail/lru_cache.hpp>
#include <boost/compute/detail/global_static.hpp>

namespace boost {
namespace compute {

/// The program_cache class stores \ref program objects in a LRU cache.
///
/// This class can be used to help mitigate the overhead of OpenCL's run-time
/// kernel compilation model. Commonly used programs can be stored persistently
/// in the cache and only compiled once on their first use.
///
/// Program objects are stored and retreived based on a user-defined cache key
/// along with the options used to build the program (if any).
///
/// For example, to insert a program into the cache:
/// \code
/// cache.insert("foo", foo_program);
/// \endcode
///
/// And to retreive the program later:
/// \code
/// boost::optional<program> p = cache.get("foo");
/// if(p){
///     // program found in cache
/// }
/// \endcode
///
/// \see program
class program_cache : boost::noncopyable
{
public:
    /// Creates a new program cache with space for \p capacity number of
    /// program objects.
    program_cache(size_t capacity)
        : m_cache(capacity)
    {
    }

    /// Destroys the program cache.
    ~program_cache()
    {
    }

    /// Returns the number of program objects currently stored in the cache.
    size_t size() const
    {
        return m_cache.size();
    }

    /// Returns the total capacity of the cache.
    size_t capacity() const
    {
        return m_cache.capacity();
    }

    /// Clears the program cache.
    void clear()
    {
        m_cache.clear();
    }

    /// Returns the program object with \p key. Returns a null optional if no
    /// program with \p key exists in the cache.
    boost::optional<program> get(const std::string &key)
    {
        return m_cache.get(std::make_pair(key, std::string()));
    }

    /// Returns the program object with \p key and \p options. Returns a null
    /// optional if no program with \p key and \p options exists in the cache.
    boost::optional<program> get(const std::string &key, const std::string &options)
    {
        return m_cache.get(std::make_pair(key, options));
    }

    /// Inserts \p program into the cache with \p key.
    void insert(const std::string &key, const program &program)
    {
        insert(key, std::string(), program);
    }

    /// Inserts \p program into the cache with \p key and \p options.
    void insert(const std::string &key, const std::string &options, const program &program)
    {
        m_cache.insert(std::make_pair(key, options), program);
    }

    /// Loads the program with \p key from the cache if it exists. Otherwise
    /// builds a new program with \p source and \p options, stores it in the
    /// cache, and returns it.
    ///
    /// This is a convenience function to simplify the common pattern of
    /// attempting to load a program from the cache and, if not present,
    /// building the program from source and storing it in the cache.
    ///
    /// Equivalent to:
    /// \code
    /// boost::optional<program> p = get(key, options);
    /// if(!p){
    ///     p = program::create_with_source(source, context);
    ///     p->build(options);
    ///     insert(key, options, *p);
    /// }
    /// return *p;
    /// \endcode
    program get_or_build(const std::string &key,
                         const std::string &options,
                         const std::string &source,
                         const context &context)
    {
        boost::optional<program> p = get(key, options);
        if(!p){
            p = program::build_with_source(source, context, options);

            insert(key, options, *p);
        }
        return *p;
    }

    /// Returns the global program cache for \p context.
    ///
    /// This global cache is used internally by Boost.Compute to store compiled
    /// program objects used by its algorithms. All Boost.Compute programs are
    /// stored with a cache key beginning with \c "__boost". User programs
    /// should avoid using the same prefix in order to prevent collisions.
    static boost::shared_ptr<program_cache> get_global_cache(const context &context)
    {
        typedef detail::lru_cache<cl_context, boost::shared_ptr<program_cache> > cache_map;

        BOOST_COMPUTE_DETAIL_GLOBAL_STATIC(cache_map, caches, (8));

        boost::optional<boost::shared_ptr<program_cache> > cache = caches.get(context.get());
        if(!cache){
            cache = boost::make_shared<program_cache>(64);

            caches.insert(context.get(), *cache);
        }

        return *cache;
    }

private:
    detail::lru_cache<std::pair<std::string, std::string>, program> m_cache;
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_UTILITY_PROGRAM_CACHE_HPP

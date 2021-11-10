//---------------------------------------------------------------------------//
// Copyright (c) 2013-2015 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_PARAMETER_CACHE_HPP
#define BOOST_COMPUTE_DETAIL_PARAMETER_CACHE_HPP

#include <algorithm>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/detail/global_static.hpp>
#include <boost/compute/version.hpp>

#ifdef BOOST_COMPUTE_USE_OFFLINE_CACHE
#include <cstdio>  
#include <boost/algorithm/string/trim.hpp>
#include <boost/compute/detail/path.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#endif // BOOST_COMPUTE_USE_OFFLINE_CACHE

namespace boost {
namespace compute {
namespace detail {

class parameter_cache : boost::noncopyable
{
public:
    parameter_cache(const device &device)
        : m_dirty(false),
          m_device_name(device.name())
    {
    #ifdef BOOST_COMPUTE_USE_OFFLINE_CACHE
        // get offline cache file name (e.g. /home/user/.boost_compute/tune/device.json)
        m_file_name = make_file_name();

        // load parameters from offline cache file (if it exists)
        if(boost::filesystem::exists(m_file_name)){
            read_from_disk();
        }
    #endif // BOOST_COMPUTE_USE_OFFLINE_CACHE
    }

    ~parameter_cache()
    {
    #ifdef BOOST_COMPUTE_USE_OFFLINE_CACHE
        write_to_disk();
    #endif // BOOST_COMPUTE_USE_OFFLINE_CACHE
    }

    void set(const std::string &object, const std::string &parameter, uint_ value)
    {
        m_cache[std::make_pair(object, parameter)] = value;

        // set the dirty flag to true. this will cause the updated parameters
        // to be stored to disk.
        m_dirty = true;
    }

    uint_ get(const std::string &object, const std::string &parameter, uint_ default_value)
    {
        std::map<std::pair<std::string, std::string>, uint_>::iterator
            iter = m_cache.find(std::make_pair(object, parameter));
        if(iter != m_cache.end()){
            return iter->second;
        }
        else {
            return default_value;
        }
    }

    static boost::shared_ptr<parameter_cache> get_global_cache(const device &device)
    {
        // device name -> parameter cache
        typedef std::map<std::string, boost::shared_ptr<parameter_cache> > cache_map;

        BOOST_COMPUTE_DETAIL_GLOBAL_STATIC(cache_map, caches, ((std::less<std::string>())));

        cache_map::iterator iter = caches.find(device.name());
        if(iter == caches.end()){
            boost::shared_ptr<parameter_cache> cache =
                boost::make_shared<parameter_cache>(device);

            caches.insert(iter, std::make_pair(device.name(), cache));

            return cache;
        }
        else {
            return iter->second;
        }
    }

private:
#ifdef BOOST_COMPUTE_USE_OFFLINE_CACHE
    // returns a string containing a cannoical device name
    static std::string cannonical_device_name(std::string name)
    {
        boost::algorithm::trim(name);
        std::replace(name.begin(), name.end(), ' ', '_');
        std::replace(name.begin(), name.end(), '(', '_');
        std::replace(name.begin(), name.end(), ')', '_');
        return name;
    }

    // returns the boost.compute version string
    static std::string version_string()
    {
        char buf[32];
        // snprintf is in Visual Studio since Visual Studio 2015 (_MSC_VER == 1900)
        #if defined (_MSC_VER) && _MSC_VER < 1900
            #define DETAIL_SNPRINTF sprintf_s
        #else
            #define DETAIL_SNPRINTF std::snprintf
        #endif
        DETAIL_SNPRINTF(buf, sizeof(buf), "%d.%d.%d", BOOST_COMPUTE_VERSION_MAJOR,
                                                      BOOST_COMPUTE_VERSION_MINOR,
                                                      BOOST_COMPUTE_VERSION_PATCH);
        #undef DETAIL_SNPRINTF
        return buf;
    }

    // returns the file path for the cached parameters
    std::string make_file_name() const
    {
        return detail::parameter_cache_path(true) + cannonical_device_name(m_device_name) + ".json";
    }

    // store current parameters to disk
    void write_to_disk()
    {
        BOOST_ASSERT(!m_file_name.empty());

        if(m_dirty){
            // save current parameters to disk
            boost::property_tree::ptree pt;
            pt.put("header.device", m_device_name);
            pt.put("header.version", version_string());
            typedef std::map<std::pair<std::string, std::string>, uint_> map_type;
            for(map_type::const_iterator iter = m_cache.begin(); iter != m_cache.end(); ++iter){
                const std::pair<std::string, std::string> &key = iter->first;
                pt.add(key.first + "." + key.second, iter->second);
            }
            write_json(m_file_name, pt);

            m_dirty = false;
        }
    }

    // load stored parameters from disk
    void read_from_disk()
    {
        BOOST_ASSERT(!m_file_name.empty());

        m_cache.clear();

        boost::property_tree::ptree pt;
        try {
            read_json(m_file_name, pt);
        }
        catch(boost::property_tree::json_parser::json_parser_error&){
            // no saved cache file, ignore
            return;
        }

        std::string stored_device;
        try {
            stored_device = pt.get<std::string>("header.device");
        }
        catch(boost::property_tree::ptree_bad_path&){
            return;
        }

        std::string stored_version;
        try {
            stored_version = pt.get<std::string>("header.version");
        }
        catch(boost::property_tree::ptree_bad_path&){
            return;
        }

        if(stored_device == m_device_name && stored_version == version_string()){
            typedef boost::property_tree::ptree::const_iterator pt_iter;
            for(pt_iter iter = pt.begin(); iter != pt.end(); ++iter){
                if(iter->first == "header"){
                    // skip header
                    continue;
                }

                boost::property_tree::ptree child_pt = pt.get_child(iter->first);
                for(pt_iter child_iter = child_pt.begin(); child_iter != child_pt.end(); ++child_iter){
                    set(iter->first, child_iter->first, boost::lexical_cast<uint_>(child_iter->second.data()));
                }
            }
        }

        m_dirty = false;
    }
#endif // BOOST_COMPUTE_USE_OFFLINE_CACHE

private:
    bool m_dirty;
    std::string m_device_name;
    std::string m_file_name;
    std::map<std::pair<std::string, std::string>, uint_> m_cache;
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_PARAMETER_CACHE_HPP

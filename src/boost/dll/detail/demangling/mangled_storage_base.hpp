//  Copyright 2016 Klemens Morgenstern
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_DETAIL_MANGLE_STORAGE_BASE_HPP_
#define BOOST_DLL_DETAIL_MANGLE_STORAGE_BASE_HPP_

#include <vector>
#include <string>
#include <map>
#include <boost/dll/detail/demangling/demangle_symbol.hpp>
#include <boost/dll/library_info.hpp>
#include <boost/type_index/ctti_type_index.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost { namespace dll { namespace detail {

///stores the mangled names with the demangled name.
struct mangled_storage_base
{
    struct entry
    {
        std::string mangled;
        std::string demangled;
        entry() = default;
        entry(const std::string & m, const std::string &d) : mangled(m), demangled(d) {}
        entry(const entry&) = default;
        entry(entry&&)         = default;
        entry &operator= (const entry&) = default;
        entry &operator= (entry&&)         = default;
    };
protected:
    std::vector<entry> storage_;
    ///if a unknown class is imported it can be overloaded by this type
    std::map<boost::typeindex::ctti_type_index, std::string> aliases_;
public:
    void assign(const mangled_storage_base & storage)
    {
        aliases_  = storage.aliases_;
        storage_  = storage.storage_;
    }
    void swap( mangled_storage_base & storage)
    {
        aliases_.swap(storage.aliases_);
        storage_.swap(storage.storage_);
    }
    void clear()
    {
        storage_.clear();
        aliases_.clear();
    }
    std::vector<entry> & get_storage() {return storage_;};
    template<typename T>
    std::string get_name() const
    {
        using boost::typeindex::ctti_type_index;
        auto tx = ctti_type_index::type_id<T>();
        auto val = (aliases_.count(tx) > 0) ? aliases_.at(tx) : tx.pretty_name();
        return val;
    }

    mangled_storage_base() = default;
    mangled_storage_base(mangled_storage_base&&) = default;
    mangled_storage_base(const mangled_storage_base&) = default;

    mangled_storage_base(const std::vector<std::string> & symbols) { add_symbols(symbols);}

    explicit mangled_storage_base(library_info & li) : mangled_storage_base(li.symbols()) {}

    explicit mangled_storage_base(
            const boost::dll::fs::path& library_path,
            bool throw_if_not_native_format = true)
        : mangled_storage_base(library_info(library_path, throw_if_not_native_format).symbols())
    {

    }

    void load(library_info & li) { storage_.clear(); add_symbols(li.symbols()); };
    void load(const boost::dll::fs::path& library_path,
            bool throw_if_not_native_format = true)
    {
        storage_.clear();
        add_symbols(library_info(library_path, throw_if_not_native_format).symbols());
    };

    /*! Allows do add a class as alias, if the class imported is not known
     * in this binary.
     * @tparam Alias The Alias type
     *  @param The name to create the alias for.
     *
     *  @note There can be multiple aliases, this is on purpose.
     */
    template<typename Alias> void add_alias(const std::string& name)
    {
        aliases_.emplace(
            boost::typeindex::ctti_type_index::type_id<Alias>(),
            name
            );
    }
    void add_symbols(const std::vector<std::string> & symbols)
    {
        for (auto & sym : symbols)
        {
            auto dm = demangle_symbol(sym);
            if (!dm.empty())
                storage_.emplace_back(sym, dm);
            else
                storage_.emplace_back(sym, sym);
        }
    }


};


}}}

#endif /* BOOST_DLL_DETAIL_MANGLE_STORAGE_HPP_ */

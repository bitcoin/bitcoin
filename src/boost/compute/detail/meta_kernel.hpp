//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_META_KERNEL_HPP
#define BOOST_COMPUTE_DETAIL_META_KERNEL_HPP

#include <set>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <utility>

#include <boost/tuple/tuple.hpp>
#include <boost/type_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/static_assert.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/preprocessor/repetition.hpp>

#include <boost/compute/kernel.hpp>
#include <boost/compute/closure.hpp>
#include <boost/compute/function.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/type_traits.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/image/image2d.hpp>
#include <boost/compute/image/image_sampler.hpp>
#include <boost/compute/memory_object.hpp>
#include <boost/compute/memory/svm_ptr.hpp>
#include <boost/compute/detail/device_ptr.hpp>
#include <boost/compute/detail/sha1.hpp>
#include <boost/compute/utility/program_cache.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class T>
class meta_kernel_variable
{
public:
    typedef T result_type;

    meta_kernel_variable(const std::string &name)
        : m_name(name)
    {
    }

    meta_kernel_variable(const meta_kernel_variable &other)
        : m_name(other.m_name)
    {
    }

    meta_kernel_variable& operator=(const meta_kernel_variable &other)
    {
        if(this != &other){
            m_name = other.m_name;
        }

        return *this;
    }

    ~meta_kernel_variable()
    {
    }

    std::string name() const
    {
        return m_name;
    }

private:
    std::string m_name;
};

template<class T>
class meta_kernel_literal
{
public:
    typedef T result_type;

    meta_kernel_literal(const T &value)
        : m_value(value)
    {
    }

    meta_kernel_literal(const meta_kernel_literal &other)
        : m_value(other.m_value)
    {
    }

    meta_kernel_literal& operator=(const meta_kernel_literal &other)
    {
        if(this != &other){
            m_value = other.m_value;
        }

        return *this;
    }

    ~meta_kernel_literal()
    {
    }

    const T& value() const
    {
        return m_value;
    }

private:
    T m_value;
};

struct meta_kernel_stored_arg
{
    meta_kernel_stored_arg()
        : m_size(0),
          m_value(0)
    {
    }

    meta_kernel_stored_arg(const meta_kernel_stored_arg &other)
        : m_size(0),
          m_value(0)
    {
        set_value(other.m_size, other.m_value);
    }

    meta_kernel_stored_arg& operator=(const meta_kernel_stored_arg &other)
    {
        if(this != &other){
            set_value(other.m_size, other.m_value);
        }

        return *this;
    }

    template<class T>
    meta_kernel_stored_arg(const T &value)
        : m_size(0),
          m_value(0)
    {
        set_value(value);
    }

    ~meta_kernel_stored_arg()
    {
        if(m_value){
            std::free(m_value);
        }
    }

    void set_value(size_t size, const void *value)
    {
        if(m_value){
            std::free(m_value);
        }

        m_size = size;

        if(value){
            m_value = std::malloc(size);
            std::memcpy(m_value, value, size);
        }
        else {
            m_value = 0;
        }
    }

    template<class T>
    void set_value(const T &value)
    {
        set_value(sizeof(T), boost::addressof(value));
    }

    size_t m_size;
    void *m_value;
};

struct meta_kernel_buffer_info
{
    meta_kernel_buffer_info(const buffer &buffer,
                            const std::string &id,
                            memory_object::address_space addr_space,
                            size_t i)
      : m_mem(buffer.get()),
        identifier(id),
        address_space(addr_space),
        index(i)
    {
    }

    cl_mem m_mem;
    std::string identifier;
    memory_object::address_space address_space;
    size_t index;
};

struct meta_kernel_svm_info
{
    template <class T>
    meta_kernel_svm_info(const svm_ptr<T> ptr,
                         const std::string &id,
                         memory_object::address_space addr_space,
                         size_t i)
      : ptr(ptr.get()),
        identifier(id),
        address_space(addr_space),
        index(i)
    {

    }

    void* ptr;
    std::string identifier;
    memory_object::address_space address_space;
    size_t index;
};


class meta_kernel;

template<class Type>
struct inject_type_impl
{
    void operator()(meta_kernel &)
    {
        // default implementation does nothing
    }
};

#define BOOST_COMPUTE_META_KERNEL_DECLARE_SCALAR_TYPE_STREAM_OPERATOR(type) \
    meta_kernel& operator<<(const type &x) \
    { \
        m_source << x; \
        return *this; \
    }

#define BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(type) \
    meta_kernel& operator<<(const type &x) \
    { \
        m_source << "(" << type_name<type>() << ")"; \
        m_source << "("; \
        for(size_t i = 0; i < vector_size<type>::value; i++){ \
            *this << lit(x[i]); \
            \
            if(i != vector_size<type>::value - 1){ \
                m_source << ","; \
            } \
        } \
        m_source << ")"; \
        return *this; \
    }

#define BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(type) \
    BOOST_COMPUTE_META_KERNEL_DECLARE_SCALAR_TYPE_STREAM_OPERATOR(BOOST_PP_CAT(type, _)) \
    BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(BOOST_PP_CAT(BOOST_PP_CAT(type, 2), _)) \
    BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(BOOST_PP_CAT(BOOST_PP_CAT(type, 4), _)) \
    BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(BOOST_PP_CAT(BOOST_PP_CAT(type, 8), _)) \
    BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(BOOST_PP_CAT(BOOST_PP_CAT(type, 16), _))

class meta_kernel
{
public:
    template<class T>
    class argument
    {
    public:
        argument(const std::string &name, size_t index)
            : m_name(name),
              m_index(index)
        {
        }

        const std::string &name() const
        {
            return m_name;
        }

        size_t index() const
        {
            return m_index;
        }

    private:
        std::string m_name;
        size_t m_index;
    };

    explicit meta_kernel(const std::string &name)
        : m_name(name)
    {
    }

    meta_kernel(const meta_kernel &other)
    {
        m_source.str(other.m_source.str());
        m_options = other.m_options;
    }

    meta_kernel& operator=(const meta_kernel &other)
    {
        if(this != &other){
            m_source.str(other.m_source.str());
            m_options = other.m_options;
        }

        return *this;
    }

    ~meta_kernel()
    {
    }

    std::string name() const
    {
        return m_name;
    }

    std::string source() const
    {
        std::stringstream stream;

        // add pragmas
        if(!m_pragmas.empty()){
            stream << m_pragmas << "\n";
        }

        // add macros
        stream << "#define boost_pair_type(t1, t2) _pair_ ## t1 ## _ ## t2 ## _t\n";
        stream << "#define boost_pair_get(x, n) (n == 0 ? x.first ## x.second)\n";
        stream << "#define boost_make_pair(t1, x, t2, y) (boost_pair_type(t1, t2)) { x, y }\n";
        stream << "#define boost_tuple_get(x, n) (x.v ## n)\n";

        // add type declaration source
        stream << m_type_declaration_source.str() << "\n";

        // add external function source
        stream << m_external_function_source.str() << "\n";

        // add kernel source
        stream << "__kernel void " << m_name
               << "(" << boost::join(m_args, ", ") << ")\n"
               << "{\n" << m_source.str() << "\n}\n";

        return stream.str();
    }

    kernel compile(const context &context, const std::string &options = std::string())
    {
        // generate the program source
        std::string source = this->source();

        // generate cache key
        std::string cache_key = "__boost_meta_kernel_" +
            static_cast<std::string>(detail::sha1(source));

        // load program cache
        boost::shared_ptr<program_cache> cache =
            program_cache::get_global_cache(context);

        std::string compile_options = m_options + options;

        // load (or build) program from cache
        ::boost::compute::program program =
            cache->get_or_build(cache_key, compile_options, source, context);

        // create kernel
        ::boost::compute::kernel kernel = program.create_kernel(name());

        // bind stored args
        for(size_t i = 0; i < m_stored_args.size(); i++){
            const detail::meta_kernel_stored_arg &arg = m_stored_args[i];

            if(arg.m_size != 0){
                kernel.set_arg(i, arg.m_size, arg.m_value);
            }
        }

        // bind buffer args
        for(size_t i = 0; i < m_stored_buffers.size(); i++){
            const detail::meta_kernel_buffer_info &bi = m_stored_buffers[i];

            kernel.set_arg(bi.index, bi.m_mem);
        }

        // bind svm args
        for(size_t i = 0; i < m_stored_svm_ptrs.size(); i++){
            const detail::meta_kernel_svm_info &spi = m_stored_svm_ptrs[i];

            kernel.set_arg_svm_ptr(spi.index, spi.ptr);
        }

        return kernel;
    }

    template<class T>
    size_t add_arg(const std::string &name)
    {
        std::stringstream stream;
        stream << type<T>() << " " << name;

        // add argument to list
        m_args.push_back(stream.str());

        // return index
        return m_args.size() - 1;
    }

    template<class T>
    size_t add_arg(memory_object::address_space address_space,
                   const std::string &name)
    {
        return add_arg_with_qualifiers<T>(address_space_prefix(address_space), name);
    }

    template<class T>
    void set_arg(size_t index, const T &value)
    {
        if(index >= m_stored_args.size()){
            m_stored_args.resize(index + 1);
        }

        m_stored_args[index] = detail::meta_kernel_stored_arg(value);
    }

    void set_arg(size_t index, const memory_object &mem)
    {
        set_arg<cl_mem>(index, mem.get());
    }

    void set_arg(size_t index, const image_sampler &sampler)
    {
        set_arg<cl_sampler>(index, cl_sampler(sampler));
    }

    template<class T>
    size_t add_set_arg(const std::string &name, const T &value)
    {
        size_t index = add_arg<T>(name);
        set_arg<T>(index, value);
        return index;
    }

    void add_extension_pragma(const std::string &extension,
                              const std::string &value = "enable")
    {
        m_pragmas += "#pragma OPENCL EXTENSION " + extension + " : " + value + "\n";
    }

    void add_extension_pragma(const std::string &extension,
                              const std::string &value) const
    {
        return const_cast<meta_kernel *>(this)->add_extension_pragma(extension, value);
    }

    template<class T>
    std::string type() const
    {
        std::stringstream stream;

        // const qualifier
        if(boost::is_const<T>::value){
            stream << "const ";
        }

        // volatile qualifier
        if(boost::is_volatile<T>::value){
            stream << "volatile ";
        }

        // type
        typedef
            typename boost::remove_cv<
                typename boost::remove_pointer<T>::type
            >::type Type;
        stream << type_name<Type>();

        // pointer
        if(boost::is_pointer<T>::value){
            stream << "*";
        }

        // inject type pragmas and/or definitions
        inject_type<Type>();

        return stream.str();
    }

    template<class T>
    std::string decl(const std::string &name) const
    {
        return type<T>() + " " + name;
    }

    template<class T, class Expr>
    std::string decl(const std::string &name, const Expr &init) const
    {
        meta_kernel tmp((std::string()));
        tmp << tmp.decl<T>(name) << " = " << init;
        return tmp.m_source.str();
    }

    template<class T>
    detail::meta_kernel_variable<T> var(const std::string &name) const
    {
        type<T>();

        return make_var<T>(name);
    }

    template<class T>
    detail::meta_kernel_literal<T> lit(const T &value) const
    {
        type<T>();

        return detail::meta_kernel_literal<T>(value);
    }

    template<class T>
    detail::meta_kernel_variable<T> expr(const std::string &expr) const
    {
        type<T>();

        return detail::meta_kernel_variable<T>(expr);
    }

    // define stream operators for scalar and vector types
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(char)
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(uchar)
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(short)
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(ushort)
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(int)
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(uint)
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(long)
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(ulong)
    BOOST_COMPUTE_META_KERNEL_DECLARE_TYPE_STREAM_OPERATORS(double)

    // define stream operators for float scalar and vector types
    meta_kernel& operator<<(const float &x)
    {
        m_source << std::showpoint << x << 'f';
        return *this;
    }

    BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(float2_)
    BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(float4_)
    BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(float8_)
    BOOST_COMPUTE_META_KERNEL_DECLARE_VECTOR_TYPE_STREAM_OPERATOR(float16_)

    // define stream operators for variable types
    template<class T>
    meta_kernel& operator<<(const meta_kernel_variable<T> &variable)
    {
        return *this << variable.name();
    }

    // define stream operators for literal types
    template<class T>
    meta_kernel& operator<<(const meta_kernel_literal<T> &literal)
    {
        return *this << literal.value();
    }

    meta_kernel& operator<<(const meta_kernel_literal<bool> &literal)
    {
        return *this << (literal.value() ? "true" : "false");
    }

    meta_kernel& operator<<(const meta_kernel_literal<char> &literal)
    {
        const char c = literal.value();

        switch(c){
        // control characters
        case '\0':
            return *this << "'\\0'";
        case '\a':
            return *this << "'\\a'";
        case '\b':
            return *this << "'\\b'";
        case '\t':
            return *this << "'\\t'";
        case '\n':
            return *this << "'\\n'";
        case '\v':
            return *this << "'\\v'";
        case '\f':
            return *this << "'\\f'";
        case '\r':
            return *this << "'\\r'";

        // characters which need escaping
        case '\"':
        case '\'':
        case '\?':
        case '\\':
            return *this << "'\\" << c << "'";

        // all other characters
        default:
            return *this << "'" << c << "'";
        }
    }

    meta_kernel& operator<<(const meta_kernel_literal<signed char> &literal)
    {
        return *this << lit<char>(literal.value());
    }

    meta_kernel& operator<<(const meta_kernel_literal<unsigned char> &literal)
    {
        return *this << uint_(literal.value());
    }

    // define stream operators for strings
    meta_kernel& operator<<(char ch)
    {
        m_source << ch;
        return *this;
    }

    meta_kernel& operator<<(const char *string)
    {
        m_source << string;
        return *this;
    }

    meta_kernel& operator<<(const std::string &string)
    {
        m_source << string;
        return *this;
    }

    template<class T>
    static detail::meta_kernel_variable<T> make_var(const std::string &name)
    {
        return detail::meta_kernel_variable<T>(name);
    }

    template<class T>
    static detail::meta_kernel_literal<T> make_lit(const T &value)
    {
        return detail::meta_kernel_literal<T>(value);
    }

    template<class T>
    static detail::meta_kernel_variable<T> make_expr(const std::string &expr)
    {
        return detail::meta_kernel_variable<T>(expr);
    }

    event exec(command_queue &queue)
    {
        return exec_1d(queue, 0, 1);
    }

    event exec_1d(command_queue &queue,
                  size_t global_work_offset,
                  size_t global_work_size,
                  const wait_list &events = wait_list())
    {
        const context &context = queue.get_context();

        ::boost::compute::kernel kernel = compile(context);

        return queue.enqueue_1d_range_kernel(
                   kernel,
                   global_work_offset,
                   global_work_size,
                   0,
                   events
               );
    }

    event exec_1d(command_queue &queue,
                 size_t global_work_offset,
                 size_t global_work_size,
                 size_t local_work_size,
                 const wait_list &events = wait_list())
    {
        const context &context = queue.get_context();

        ::boost::compute::kernel kernel = compile(context);

        return queue.enqueue_1d_range_kernel(
                   kernel,
                   global_work_offset,
                   global_work_size,
                   local_work_size,
                   events
               );
    }

    template<class T>
    std::string get_buffer_identifier(const buffer &buffer,
                                      const memory_object::address_space address_space =
                                          memory_object::global_memory)
    {
        // check if we've already seen buffer
        for(size_t i = 0; i < m_stored_buffers.size(); i++){
            const detail::meta_kernel_buffer_info &bi = m_stored_buffers[i];

            if(bi.m_mem == buffer.get() &&
               bi.address_space == address_space){
                return bi.identifier;
            }
        }

        // create a new binding
        std::string identifier =
            "_buf" + lexical_cast<std::string>(m_stored_buffers.size());
        size_t index = add_arg<T *>(address_space, identifier);

        // store new buffer info
        m_stored_buffers.push_back(
            detail::meta_kernel_buffer_info(buffer, identifier, address_space, index));

        return identifier;
    }

    template<class T>
    std::string get_svm_identifier(const svm_ptr<T> &svm_ptr,
                                   const memory_object::address_space address_space =
                                       memory_object::global_memory)
    {
        BOOST_ASSERT(
            (address_space == memory_object::global_memory)
                || (address_space == memory_object::constant_memory)
        );

        // check if we've already seen this pointer
        for(size_t i = 0; i < m_stored_svm_ptrs.size(); i++){
            const detail::meta_kernel_svm_info &spi = m_stored_svm_ptrs[i];

            if(spi.ptr == svm_ptr.get() &&
               spi.address_space == address_space){
                return spi.identifier;
            }
        }

        // create a new binding
        std::string identifier =
            "_svm_ptr" + lexical_cast<std::string>(m_stored_svm_ptrs.size());
        size_t index = add_arg<T *>(address_space, identifier);

        if(m_stored_svm_ptrs.empty()) {
            m_options += std::string(" -cl-std=CL2.0");
        }

        // store new svm pointer info
        m_stored_svm_ptrs.push_back(
            detail::meta_kernel_svm_info(
                svm_ptr, identifier, address_space, index
            )
        );

        return identifier;
    }

    std::string get_image_identifier(const char *qualifiers, const image2d &image)
    {
        size_t index = add_arg_with_qualifiers<image2d>(qualifiers, "image");

        set_arg(index, image);

        return "image";
    }

    std::string get_sampler_identifier(bool normalized_coords,
                                       cl_addressing_mode addressing_mode,
                                       cl_filter_mode filter_mode)
    {
        (void) normalized_coords;
        (void) addressing_mode;
        (void) filter_mode;

        m_pragmas += "const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |\n"
                     "                          CLK_ADDRESS_NONE |\n"
                     "                          CLK_FILTER_NEAREST;\n";

        return "sampler";
    }

    template<class Expr>
    static std::string expr_to_string(const Expr &expr)
    {
        meta_kernel tmp((std::string()));
        tmp << expr;
        return tmp.m_source.str();
    }

    template<class Predicate>
    detail::invoked_function<bool, boost::tuple<Predicate> > if_(Predicate pred) const
    {
        return detail::invoked_function<bool, boost::tuple<Predicate> >(
            "if", std::string(), boost::make_tuple(pred)
        );
    }

    template<class Predicate>
    detail::invoked_function<bool, boost::tuple<Predicate> > else_if_(Predicate pred) const
    {
        return detail::invoked_function<bool, boost::tuple<Predicate> >(
            "else if", std::string(), boost::make_tuple(pred)
        );
    }

    detail::meta_kernel_variable<cl_uint> get_global_id(size_t dim) const
    {
        return expr<cl_uint>("get_global_id(" + lexical_cast<std::string>(dim) + ")");
    }

    void add_function(const std::string &name, const std::string &source)
    {
        if(m_external_function_names.count(name)){
            return;
        }

        m_external_function_names.insert(name);
        m_external_function_source << source << "\n";
    }

    void add_function(const std::string &name,
                      const std::string &source,
                      const std::map<std::string, std::string> &definitions)
    {
        typedef std::map<std::string, std::string>::const_iterator iter;

        std::stringstream s;

        // add #define's
        for(iter i = definitions.begin(); i != definitions.end(); i++){
            s << "#define " << i->first;
            if(!i->second.empty()){
                s << " " << i->second;
            }
            s << "\n";
        }

        s << source << "\n";

        // add #undef's
        for(iter i = definitions.begin(); i != definitions.end(); i++){
            s << "#undef " << i->first << "\n";
        }

        add_function(name, s.str());
    }

    template<class Type>
    void add_type_declaration(const std::string &declaration)
    {
        const char *name = type_name<Type>();

        // check if the type has already been declared
        std::string source = m_type_declaration_source.str();
        if(source.find(name) != std::string::npos){
            return;
        }

        m_type_declaration_source << declaration;
    }

    template<class Type>
    void inject_type() const
    {
        inject_type_impl<Type>()(const_cast<meta_kernel &>(*this));
    }

    // the insert_function_call() method inserts a call to a function with
    // the given name tuple of argument values.
    template<class ArgTuple>
    void insert_function_call(const std::string &name, const ArgTuple &args)
    {
        *this << name << '(';
        insert_function_call_args(args);
        *this << ')';
    }

    // the insert_function_call_args() method takes a tuple of argument values
    // and inserts them into the source string with a comma in-between each.
    // this is useful for creating function calls given a tuple of values.
    void insert_function_call_args(const boost::tuple<>&)
    {
    }

    #define BOOST_COMPUTE_META_KERNEL_INSERT_FUNCTION_ARG_TYPE(z, n, unused) \
        inject_type<BOOST_PP_CAT(T, n)>();

    #define BOOST_COMPUTE_META_KERNEL_STREAM_FUNCTION_ARG(z, n, unused) \
        << boost::get<BOOST_PP_DEC(n)>(args) << ", "

    #define BOOST_COMPUTE_META_KERNEL_INSERT_FUNCTION_ARGS(z, n, unused) \
        template<BOOST_PP_ENUM_PARAMS(n, class T)> \
        void insert_function_call_args( \
            const boost::tuple<BOOST_PP_ENUM_PARAMS(n, T)> &args \
        ) \
        { \
            BOOST_PP_REPEAT_FROM_TO( \
                0, n, BOOST_COMPUTE_META_KERNEL_INSERT_FUNCTION_ARG_TYPE, ~ \
            ) \
            *this \
                BOOST_PP_REPEAT_FROM_TO( \
                    1, n, BOOST_COMPUTE_META_KERNEL_STREAM_FUNCTION_ARG, ~ \
                ) \
                << boost::get<BOOST_PP_DEC(n)>(args); \
        }

    BOOST_PP_REPEAT_FROM_TO(
        1, BOOST_COMPUTE_MAX_ARITY, BOOST_COMPUTE_META_KERNEL_INSERT_FUNCTION_ARGS, ~
    )

    #undef BOOST_COMPUTE_META_KERNEL_INSERT_FUNCTION_ARG_TYPE
    #undef BOOST_COMPUTE_META_KERNEL_STREAM_FUNCTION_ARG
    #undef BOOST_COMPUTE_META_KERNEL_INSERT_FUNCTION_ARGS

    static const char* address_space_prefix(const memory_object::address_space value)
    {
        switch(value){
            case memory_object::global_memory: return "__global";
            case memory_object::local_memory: return "__local";
            case memory_object::private_memory: return "__private";
            case memory_object::constant_memory: return "__constant";
        };

        return 0; // unreachable
    }

private:
    template<class T>
    size_t add_arg_with_qualifiers(const char *qualifiers, const std::string &name)
    {
        size_t index = add_arg<T>(name);

        // update argument type declaration with qualifiers
        std::stringstream s;
        s << qualifiers << " " << m_args[index];
        m_args[index] = s.str();

        return index;
    }

private:
    std::string m_name;
    std::stringstream m_source;
    std::stringstream m_external_function_source;
    std::stringstream m_type_declaration_source;
    std::set<std::string> m_external_function_names;
    std::vector<std::string> m_args;
    std::string m_pragmas;
    std::string m_options;
    std::vector<detail::meta_kernel_stored_arg> m_stored_args;
    std::vector<detail::meta_kernel_buffer_info> m_stored_buffers;
    std::vector<detail::meta_kernel_svm_info> m_stored_svm_ptrs;
};

template<class ResultType, class ArgTuple>
inline meta_kernel&
operator<<(meta_kernel &kernel, const invoked_function<ResultType, ArgTuple> &expr)
{
    if(!expr.source().empty()){
        kernel.add_function(expr.name(), expr.source(), expr.definitions());
    }

    kernel.insert_function_call(expr.name(), expr.args());

    return kernel;
}

template<class ResultType, class ArgTuple, class CaptureTuple>
inline meta_kernel&
operator<<(meta_kernel &kernel,
           const invoked_closure<ResultType, ArgTuple, CaptureTuple> &expr)
{
    if(!expr.source().empty()){
        kernel.add_function(expr.name(), expr.source(), expr.definitions());
    }

    kernel << expr.name() << '(';
    kernel.insert_function_call_args(expr.args());
    kernel << ", ";
    kernel.insert_function_call_args(expr.capture());
    kernel << ')';

    return kernel;
}

template<class Arg1, class Arg2, class Result>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const invoked_binary_operator<Arg1,
                                                             Arg2,
                                                             Result> &expr)
{
    return kernel << "((" << expr.arg1() << ")"
                  << expr.op()
                  << "(" << expr.arg2() << "))";
}

template<class T, class IndexExpr>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const detail::device_ptr_index_expr<T, IndexExpr> &expr)
{
    if(expr.m_index == 0){
        return kernel <<
                   kernel.get_buffer_identifier<T>(expr.m_buffer) <<
                   '[' << expr.m_expr << ']';
    }
    else {
        return kernel <<
                   kernel.get_buffer_identifier<T>(expr.m_buffer) <<
                   '[' << expr.m_index << "+(" << expr.m_expr << ")]";
    }
}

template<class T1, class T2, class IndexExpr>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const detail::device_ptr_index_expr<std::pair<T1, T2>, IndexExpr> &expr)
{
    typedef std::pair<T1, T2> T;

    if(expr.m_index == 0){
        return kernel <<
                   kernel.get_buffer_identifier<T>(expr.m_buffer) <<
                   '[' << expr.m_expr << ']';
    }
    else {
        return kernel <<
                   kernel.get_buffer_identifier<T>(expr.m_buffer) <<
                   '[' << expr.m_index << "+(" << expr.m_expr << ")]";
    }
}

// SVM requires OpenCL 2.0
#if defined(BOOST_COMPUTE_CL_VERSION_2_0) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
template<class T, class IndexExpr>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const svm_ptr_index_expr<T, IndexExpr> &expr)
{
    return kernel <<
        kernel.get_svm_identifier<T>(expr.m_svm_ptr) <<
        '[' << expr.m_expr << ']';
}
#endif

template<class Predicate, class Arg>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const invoked_unary_negate_function<Predicate,
                                                                   Arg> &expr)
{
    return kernel << "!(" << expr.pred()(expr.expr()) << ')';
}

template<class Predicate, class Arg1, class Arg2>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const invoked_binary_negate_function<Predicate,
                                                                    Arg1,
                                                                    Arg2> &expr)
{
    return kernel << "!(" << expr.pred()(expr.expr1(), expr.expr2()) << ')';
}

// get<N>() for vector types
template<size_t N, class Arg, class T>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const invoked_get<N, Arg, T> &expr)
{
    BOOST_STATIC_ASSERT(N < 16);

    if(N < 10){
        return kernel << expr.m_arg << ".s" << int_(N);
    }
    else if(N < 16){
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4307)
#endif
        return kernel << expr.m_arg << ".s" << char('a' + (N - 10));
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
    }

    return kernel;
}

template<class T, class Arg>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const invoked_field<T, Arg> &expr)
{
    return kernel << expr.m_arg << "." << expr.m_field;
}

template<class T, class Arg>
inline meta_kernel& operator<<(meta_kernel &k,
                               const invoked_as<T, Arg> &expr)
{
    return k << "as_" << type_name<T>() << "(" << expr.m_arg << ")";
}

template<class T, class Arg>
inline meta_kernel& operator<<(meta_kernel &k,
                               const invoked_convert<T, Arg> &expr)
{
    return k << "convert_" << type_name<T>() << "(" << expr.m_arg << ")";
}

template<class T, class Arg>
inline meta_kernel& operator<<(meta_kernel &k,
                               const invoked_identity<T, Arg> &expr)
{
    return k << expr.m_arg;
}

template<>
struct inject_type_impl<double_>
{
    void operator()(meta_kernel &kernel)
    {
        kernel.add_extension_pragma("cl_khr_fp64", "enable");
    }
};

template<class Scalar, size_t N>
struct inject_type_impl<vector_type<Scalar, N> >
{
    void operator()(meta_kernel &kernel)
    {
        kernel.inject_type<Scalar>();
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_META_KERNEL_HPP

// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/config.h>
#include <mp/util.h>

#include <algorithm>
#include <capnp/schema.h>
#include <capnp/schema-parser.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <kj/array.h>
#include <kj/common.h>
#include <kj/filesystem.h>
#include <kj/memory.h>
#include <kj/string.h>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

#define PROXY_BIN "mpgen"
#define PROXY_DECL "mp/proxy.h"
#define PROXY_TYPES "mp/proxy-types.h"

constexpr uint64_t NAMESPACE_ANNOTATION_ID = 0xb9c6f99ebf805f2cull; // From c++.capnp
constexpr uint64_t INCLUDE_ANNOTATION_ID = 0xb899f3c154fdb458ull;   // From proxy.capnp
constexpr uint64_t INCLUDE_TYPES_ANNOTATION_ID = 0xbcec15648e8a0cf1ull; // From proxy.capnp
constexpr uint64_t WRAP_ANNOTATION_ID = 0xe6f46079b7b1405eull;      // From proxy.capnp
constexpr uint64_t COUNT_ANNOTATION_ID = 0xd02682b319f69b38ull;     // From proxy.capnp
constexpr uint64_t EXCEPTION_ANNOTATION_ID = 0x996a183200992f88ull; // From proxy.capnp
constexpr uint64_t NAME_ANNOTATION_ID = 0xb594888f63f4dbb9ull;      // From proxy.capnp
constexpr uint64_t SKIP_ANNOTATION_ID = 0x824c08b82695d8ddull;      // From proxy.capnp

template <typename Reader>
static bool AnnotationExists(const Reader& reader, uint64_t id)
{
    for (const auto annotation : reader.getAnnotations()) {
        if (annotation.getId() == id) {
            return true;
        }
    }
    return false;
}

template <typename Reader>
static bool GetAnnotationText(const Reader& reader, uint64_t id, kj::StringPtr* result)
{
    for (const auto annotation : reader.getAnnotations()) {
        if (annotation.getId() == id) {
            *result = annotation.getValue().getText();
            return true;
        }
    }
    return false;
}

template <typename Reader>
static bool GetAnnotationInt32(const Reader& reader, uint64_t id, int32_t* result)
{
    for (const auto annotation : reader.getAnnotations()) {
        if (annotation.getId() == id) {
            *result = annotation.getValue().getInt32();
            return true;
        }
    }
    return false;
}

static void ForEachMethod(const capnp::InterfaceSchema& interface, const std::function<void(const capnp::InterfaceSchema& interface, const capnp::InterfaceSchema::Method)>& callback) // NOLINT(misc-no-recursion)
{
    for (const auto super : interface.getSuperclasses()) {
        ForEachMethod(super, callback);
    }
    for (const auto method : interface.getMethods()) {
        callback(interface, method);
    }
}

using CharSlice = kj::ArrayPtr<const char>;

// Overload for any type with a string .begin(), like kj::StringPtr and kj::ArrayPtr<char>.
template <class OutputStream, class Array, const char* Enable = decltype(std::declval<Array>().begin())()>
static OutputStream& operator<<(OutputStream& os, const Array& array)
{
    os.write(array.begin(), array.size());
    return os;
}

struct Format
{
    template <typename Value>
    Format& operator<<(Value&& value)
    {
        m_os << value;
        return *this;
    }
    operator std::string() const { return m_os.str(); }
    std::ostringstream m_os;
};

static std::string Cap(kj::StringPtr str)
{
    std::string result = str;
    if (!result.empty() && 'a' <= result[0] && result[0] <= 'z') result[0] -= 'a' - 'A';
    return result;
}

static bool BoxedType(const ::capnp::Type& type)
{
    return !(type.isVoid() || type.isBool() || type.isInt8() || type.isInt16() || type.isInt32() || type.isInt64() ||
             type.isUInt8() || type.isUInt16() || type.isUInt32() || type.isUInt64() || type.isFloat32() ||
             type.isFloat64() || type.isEnum());
}

// src_file is path to .capnp file to generate stub code from.
//
// src_prefix can be used to generate outputs in a different directory than the
// source directory. For example if src_file is "/a/b/c/d/file.canp", and
// src_prefix is "/a/b", then output files will be "c/d/file.capnp.h"
// "c/d/file.capnp.cxx" "c/d/file.capnp.proxy.h", etc. This is equivalent to
// the capnp "--src-prefix" option (see "capnp help compile").
//
// include_prefix can be used to control relative include paths used in
// generated files. For example if src_file is "/a/b/c/d/file.canp" and
// include_prefix is "/a/b/c" include lines like
// "#include <d/file.capnp.proxy.h>" "#include <d/file.capnp.proxy-types.h>"i
// will be generated.
static void Generate(kj::StringPtr src_prefix,
    kj::StringPtr include_prefix,
    kj::StringPtr src_file,
    const std::vector<kj::StringPtr>& import_paths,
    const kj::ReadableDirectory& src_dir,
    const std::vector<kj::Own<const kj::ReadableDirectory>>& import_dirs)
{
    std::string output_path;
    if (src_prefix == kj::StringPtr{"."}) {
        output_path = src_file;
    } else if (!src_file.startsWith(src_prefix) || src_file.size() <= src_prefix.size() ||
               src_file[src_prefix.size()] != '/') {
        throw std::runtime_error("src_prefix is not src_file prefix");
    } else {
        output_path = src_file.slice(src_prefix.size() + 1);
    }

    std::string include_path;
    if (include_prefix == kj::StringPtr{"."}) {
        include_path = src_file;
    } else if (!src_file.startsWith(include_prefix) || src_file.size() <= include_prefix.size() ||
               src_file[include_prefix.size()] != '/') {
        throw std::runtime_error("include_prefix is not src_file prefix");
    } else {
        include_path = src_file.slice(include_prefix.size() + 1);
    }

    std::string include_base = include_path;
    const std::string::size_type p = include_base.rfind('.');
    if (p != std::string::npos) include_base.erase(p);

    std::vector<std::string> args;
    args.emplace_back(capnp_PREFIX "/bin/capnp");
    args.emplace_back("compile");
    args.emplace_back("--src-prefix=");
    args.back().append(src_prefix.cStr(), src_prefix.size());
    for (const auto& import_path : import_paths) {
        args.emplace_back("--import-path=");
        args.back().append(import_path.cStr(), import_path.size());
    }
    args.emplace_back("--output=" capnp_PREFIX "/bin/capnpc-c++");
    args.emplace_back(src_file);
    const int pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork");
    }
    if (!pid) {
        mp::ExecProcess(args);
    }
    const int status = mp::WaitProcess(pid);
    if (status) {
        throw std::runtime_error("Invoking " capnp_PREFIX "/bin/capnp failed");
    }

    const capnp::SchemaParser parser;
    auto directory_pointers = kj::heapArray<const kj::ReadableDirectory*>(import_dirs.size());
    for (size_t i = 0; i < import_dirs.size(); ++i) {
        directory_pointers[i] = import_dirs[i].get();
    }
    auto file_schema = parser.parseFromDirectory(src_dir, kj::Path::parse(output_path), directory_pointers);

    std::ofstream cpp_server(output_path + ".proxy-server.c++");
    cpp_server << "// Generated by " PROXY_BIN " from " << src_file << "\n\n";
    cpp_server << "// IWYU pragma: no_include <kj/memory.h>\n";
    cpp_server << "// IWYU pragma: no_include <memory>\n";
    cpp_server << "// IWYU pragma: begin_keep\n";
    cpp_server << "#include <" << include_path << ".proxy.h>\n";
    cpp_server << "#include <" << include_path << ".proxy-types.h>\n";
    cpp_server << "#include <capnp/generated-header-support.h>\n";
    cpp_server << "#include <cstring>\n";
    cpp_server << "#include <kj/async.h>\n";
    cpp_server << "#include <kj/common.h>\n";
    cpp_server << "#include <kj/exception.h>\n";
    cpp_server << "#include <mp/proxy.h>\n";
    cpp_server << "#include <mp/util.h>\n";
    cpp_server << "#include <" << PROXY_TYPES << ">\n";
    cpp_server << "// IWYU pragma: end_keep\n\n";
    cpp_server << "namespace mp {\n";

    std::ofstream cpp_client(output_path + ".proxy-client.c++");
    cpp_client << "// Generated by " PROXY_BIN " from " << src_file << "\n\n";
    cpp_client << "// IWYU pragma: no_include <kj/memory.h>\n";
    cpp_client << "// IWYU pragma: no_include <memory>\n";
    cpp_client << "// IWYU pragma: begin_keep\n";
    cpp_client << "#include <" << include_path << ".h>\n";
    cpp_client << "#include <" << include_path << ".proxy.h>\n";
    cpp_client << "#include <" << include_path << ".proxy-types.h>\n";
    cpp_client << "#include <capnp/generated-header-support.h>\n";
    cpp_client << "#include <cstring>\n";
    cpp_client << "#include <kj/common.h>\n";
    cpp_client << "#include <mp/proxy.h>\n";
    cpp_client << "#include <mp/util.h>\n";
    cpp_client << "#include <" << PROXY_TYPES << ">\n";
    cpp_client << "// IWYU pragma: end_keep\n\n";
    cpp_client << "namespace mp {\n";

    std::ofstream cpp_types(output_path + ".proxy-types.c++");
    cpp_types << "// Generated by " PROXY_BIN " from " << src_file << "\n\n";
    cpp_types << "// IWYU pragma: no_include \"mp/proxy.h\"\n";
    cpp_types << "// IWYU pragma: no_include \"mp/proxy-io.h\"\n";
    cpp_types << "#include <" << include_path << ".proxy.h>\n";
    cpp_types << "#include <" << include_path << ".proxy-types.h> // IWYU pragma: keep\n";
    cpp_types << "#include <" << PROXY_TYPES << ">\n\n";
    cpp_types << "namespace mp {\n";

    std::string guard = output_path;
    std::ranges::transform(guard, guard.begin(), [](unsigned char c) -> unsigned char {
        if ('0' <= c && c <= '9') return c;
        if ('A' <= c && c <= 'Z') return c;
        if ('a' <= c && c <= 'z') return c - 'a' + 'A';
        return '_';
    });

    std::ofstream inl(output_path + ".proxy-types.h");
    inl << "// Generated by " PROXY_BIN " from " << src_file << "\n\n";
    inl << "#ifndef " << guard << "_PROXY_TYPES_H\n";
    inl << "#define " << guard << "_PROXY_TYPES_H\n\n";
    inl << "// IWYU pragma: no_include \"mp/proxy.h\"\n";
    inl << "#include <mp/proxy.h> // IWYU pragma: keep\n";
    inl << "#include <" << include_path << ".proxy.h> // IWYU pragma: keep\n";
    for (const auto annotation : file_schema.getProto().getAnnotations()) {
        if (annotation.getId() == INCLUDE_TYPES_ANNOTATION_ID) {
            inl << "#include \"" << annotation.getValue().getText() << "\" // IWYU pragma: export\n";
        }
    }
    inl << "namespace mp {\n";

    std::ofstream h(output_path + ".proxy.h");
    h << "// Generated by " PROXY_BIN " from " << src_file << "\n\n";
    h << "#ifndef " << guard << "_PROXY_H\n";
    h << "#define " << guard << "_PROXY_H\n\n";
    h << "#include <" << include_path << ".h> // IWYU pragma: keep\n";
    for (const auto annotation : file_schema.getProto().getAnnotations()) {
        if (annotation.getId() == INCLUDE_ANNOTATION_ID) {
            h << "#include \"" << annotation.getValue().getText() << "\" // IWYU pragma: export\n";
        }
    }
    h << "#include <" << PROXY_DECL << ">\n\n";
    h << "#if defined(__GNUC__)\n";
    h << "#pragma GCC diagnostic push\n";
    h << "#if !defined(__has_warning)\n";
    h << "#pragma GCC diagnostic ignored \"-Wsuggest-override\"\n";
    h << "#elif __has_warning(\"-Wsuggest-override\")\n";
    h << "#pragma GCC diagnostic ignored \"-Wsuggest-override\"\n";
    h << "#endif\n";
    h << "#endif\n";
    h << "namespace mp {\n";

    kj::StringPtr message_namespace;
    GetAnnotationText(file_schema.getProto(), NAMESPACE_ANNOTATION_ID, &message_namespace);

    std::string base_name = include_base;
    const size_t output_slash = base_name.rfind('/');
    if (output_slash != std::string::npos) {
        base_name.erase(0, output_slash + 1);
    }

    std::ostringstream methods;
    std::set<kj::StringPtr> accessors_done;
    std::ostringstream accessors;
    std::ostringstream dec;
    std::ostringstream def_server;
    std::ostringstream def_client;
    std::ostringstream int_client;
    std::ostringstream def_types;

    auto add_accessor = [&](kj::StringPtr name) {
        if (!accessors_done.insert(name).second) return;
        const std::string cap = Cap(name);
        accessors << "struct " << cap << "\n";
        accessors << "{\n";
        accessors << "    template<typename S> static auto get(S&& s) -> decltype(s.get" << cap << "()) { return s.get" << cap << "(); }\n";
        accessors << "    template<typename S> static bool has(S&& s) { return s.has" << cap << "(); }\n";
        accessors << "    template<typename S, typename A> static void set(S&& s, A&& a) { s.set" << cap
                  << "(std::forward<A>(a)); }\n";
        accessors << "    template<typename S, typename... A> static decltype(auto) init(S&& s, A&&... a) { return s.init"
                  << cap << "(std::forward<A>(a)...); }\n";
        accessors << "    template<typename S> static bool getWant(S&& s) { return s.getWant" << cap << "(); }\n";
        accessors << "    template<typename S> static void setWant(S&& s) { s.setWant" << cap << "(true); }\n";
        accessors << "    template<typename S> static bool getHas(S&& s) { return s.getHas" << cap << "(); }\n";
        accessors << "    template<typename S> static void setHas(S&& s) { s.setHas" << cap << "(true); }\n";
        accessors << "};\n";
    };

    for (const auto node_nested : file_schema.getProto().getNestedNodes()) {
        kj::StringPtr node_name = node_nested.getName();
        const auto& node = file_schema.getNested(node_name);
        kj::StringPtr proxied_class_type;
        GetAnnotationText(node.getProto(), WRAP_ANNOTATION_ID, &proxied_class_type);

        if (node.getProto().isStruct()) {
            const auto& struc = node.asStruct();
            std::ostringstream generic_name;
            generic_name << node_name;
            dec << "template<";
            bool first_param = true;
            for (const auto param : node.getProto().getParameters()) {
                if (first_param) {
                    first_param = false;
                    generic_name << "<";
                } else {
                    dec << ", ";
                    generic_name << ", ";
                }
                dec << "typename " << param.getName();
                generic_name << "" << param.getName();
            }
            if (!first_param) generic_name << ">";
            dec << ">\n";
            dec << "struct ProxyStruct<" << message_namespace << "::" << generic_name.str() << ">\n";
            dec << "{\n";
            dec << "    using Struct = " << message_namespace << "::" << generic_name.str() << ";\n";
            for (const auto field : struc.getFields()) {
                auto field_name = field.getProto().getName();
                add_accessor(field_name);
                dec << "    using " << Cap(field_name) << "Accessor = Accessor<" << base_name
                    << "_fields::" << Cap(field_name) << ", FIELD_IN | FIELD_OUT";
                if (BoxedType(field.getType())) dec << " | FIELD_BOXED";
                dec << ">;\n";
            }
            dec << "    using Accessors = std::tuple<";
            size_t i = 0;
            for (const auto field : struc.getFields()) {
                if (AnnotationExists(field.getProto(), SKIP_ANNOTATION_ID)) {
                    continue;
                }
                if (i) dec << ", ";
                dec << Cap(field.getProto().getName()) << "Accessor";
                ++i;
            }
            dec << ">;\n";
            dec << "    static constexpr size_t fields = " << i << ";\n";
            dec << "};\n";

            if (proxied_class_type.size()) {
                inl << "template<>\n";
                inl << "struct ProxyType<" << proxied_class_type << ">\n";
                inl << "{\n";
                inl << "public:\n";
                inl << "    using Struct = " << message_namespace << "::" << node_name << ";\n";
                size_t i = 0;
                for (const auto field : struc.getFields()) {
                    if (AnnotationExists(field.getProto(), SKIP_ANNOTATION_ID)) {
                        continue;
                    }
                    auto field_name = field.getProto().getName();
                    auto member_name = field_name;
                    GetAnnotationText(field.getProto(), NAME_ANNOTATION_ID, &member_name);
                    inl << "    static decltype(auto) get(std::integral_constant<size_t, " << i << ">) { return "
                        << "&" << proxied_class_type << "::" << member_name << "; }\n";
                    ++i;
                }
                inl << "    static constexpr size_t fields = " << i << ";\n";
                inl << "};\n";
            }
        }

        if (proxied_class_type.size() && node.getProto().isInterface()) {
            const auto& interface = node.asInterface();

            std::ostringstream client;
            client << "template<>\nstruct ProxyClient<" << message_namespace << "::" << node_name << "> final : ";
            client << "public ProxyClientCustom<" << message_namespace << "::" << node_name << ", "
                   << proxied_class_type << ">\n{\n";
            client << "public:\n";
            client << "    using ProxyClientCustom::ProxyClientCustom;\n";
            client << "    ~ProxyClient();\n";

            std::ostringstream server;
            server << "template<>\nstruct ProxyServer<" << message_namespace << "::" << node_name << "> : public "
                   << "ProxyServerCustom<" << message_namespace << "::" << node_name << ", " << proxied_class_type
                   << ">\n{\n";
            server << "public:\n";
            server << "    using ProxyServerCustom::ProxyServerCustom;\n";
            server << "    ~ProxyServer();\n";

            const std::ostringstream client_construct;
            const std::ostringstream client_destroy;

            int method_ordinal = 0;
            ForEachMethod(interface, [&] (const capnp::InterfaceSchema& method_interface, const capnp::InterfaceSchema::Method& method) {
                const kj::StringPtr method_name = method.getProto().getName();
                kj::StringPtr proxied_method_name = method_name;
                GetAnnotationText(method.getProto(), NAME_ANNOTATION_ID, &proxied_method_name);

                const std::string method_prefix = Format() << message_namespace << "::" << method_interface.getShortDisplayName()
                                                           << "::" << Cap(method_name);
                const bool is_construct = method_name == kj::StringPtr{"construct"};
                const bool is_destroy = method_name == kj::StringPtr{"destroy"};

                struct Field
                {
                    ::capnp::StructSchema::Field param;
                    bool param_is_set = false;
                    ::capnp::StructSchema::Field result;
                    bool result_is_set = false;
                    int args = 0;
                    bool retval = false;
                    bool optional = false;
                    bool requested = false;
                    bool skip = false;
                    kj::StringPtr exception;
                };

                std::vector<Field> fields;
                std::map<kj::StringPtr, int> field_idx; // name -> args index
                bool has_result = false;

                auto add_field = [&](const ::capnp::StructSchema::Field& schema_field, bool param) {
                    if (AnnotationExists(schema_field.getProto(), SKIP_ANNOTATION_ID)) {
                        return;
                    }

                    auto field_name = schema_field.getProto().getName();
                    auto inserted = field_idx.emplace(field_name, fields.size());
                    if (inserted.second) {
                        fields.emplace_back();
                    }
                    auto& field = fields[inserted.first->second];
                    if (param) {
                        field.param = schema_field;
                        field.param_is_set = true;
                    } else {
                        field.result = schema_field;
                        field.result_is_set = true;
                    }

                    if (!param && field_name == kj::StringPtr{"result"}) {
                        field.retval = true;
                        has_result = true;
                    }

                    GetAnnotationText(schema_field.getProto(), EXCEPTION_ANNOTATION_ID, &field.exception);

                    int32_t count = 1;
                    if (!GetAnnotationInt32(schema_field.getProto(), COUNT_ANNOTATION_ID, &count)) {
                        if (schema_field.getType().isStruct()) {
                            GetAnnotationInt32(schema_field.getType().asStruct().getProto(),
                                    COUNT_ANNOTATION_ID, &count);
                        } else if (schema_field.getType().isInterface()) {
                            GetAnnotationInt32(schema_field.getType().asInterface().getProto(),
                                    COUNT_ANNOTATION_ID, &count);
                        }
                    }


                    if (inserted.second && !field.retval && !field.exception.size()) {
                        field.args = count;
                    }
                };

                for (const auto schema_field : method.getParamType().getFields()) {
                    add_field(schema_field, true);
                }
                for (const auto schema_field : method.getResultType().getFields()) {
                    add_field(schema_field, false);
                }
                for (auto& field : field_idx) {
                    auto has_field = field_idx.find("has" + Cap(field.first));
                    if (has_field != field_idx.end()) {
                        fields[has_field->second].skip = true;
                        fields[field.second].optional = true;
                    }
                    auto want_field = field_idx.find("want" + Cap(field.first));
                    if (want_field != field_idx.end() && fields[want_field->second].param_is_set) {
                        fields[want_field->second].skip = true;
                        fields[field.second].requested = true;
                    }
                }

                if (!is_construct && !is_destroy && (&method_interface == &interface)) {
                    methods << "template<>\n";
                    methods << "struct ProxyMethod<" << method_prefix << "Params>\n";
                    methods << "{\n";
                    methods << "    static constexpr auto impl = &" << proxied_class_type
                            << "::" << proxied_method_name << ";\n";
                    methods << "};\n\n";
                }

                std::ostringstream client_args;
                std::ostringstream client_invoke;
                std::ostringstream server_invoke_start;
                std::ostringstream server_invoke_end;
                int argc = 0;
                for (const auto& field : fields) {
                    if (field.skip) continue;

                    const auto& f = field.param_is_set ? field.param : field.result;
                    auto field_name = f.getProto().getName();
                    auto field_type = f.getType();

                    std::ostringstream field_flags;
                    if (!field.param_is_set) {
                        field_flags << "FIELD_OUT";
                    } else if (field.result_is_set) {
                        field_flags << "FIELD_IN | FIELD_OUT";
                    } else {
                        field_flags << "FIELD_IN";
                    }
                    if (field.optional) field_flags << " | FIELD_OPTIONAL";
                    if (field.requested) field_flags << " | FIELD_REQUESTED";
                    if (BoxedType(field_type)) field_flags << " | FIELD_BOXED";

                    add_accessor(field_name);

                    std::ostringstream fwd_args;
                    for (int i = 0; i < field.args; ++i) {
                        if (argc > 0) client_args << ",";

                        // Add to client method parameter list.
                        client_args << "M" << method_ordinal << "::Param<" << argc << "> " << field_name;
                        if (field.args > 1) client_args << i;

                        // Add to MakeClientParam argument list using Fwd helper for perfect forwarding.
                        if (i > 0) fwd_args << ", ";
                        fwd_args << "M" << method_ordinal << "::Fwd<" << argc << ">(" << field_name;
                        if (field.args > 1) fwd_args << i;
                        fwd_args << ")";

                        ++argc;
                    }
                    client_invoke << ", ";

                    if (field.exception.size()) {
                        client_invoke << "ClientException<" << field.exception << ", ";
                    } else {
                        client_invoke << "MakeClientParam<";
                    }

                    client_invoke << "Accessor<" << base_name << "_fields::" << Cap(field_name) << ", "
                                  << field_flags.str() << ">>(";

                    if (field.retval) {
                        client_invoke << field_name;
                    } else {
                        client_invoke << fwd_args.str();
                    }
                    client_invoke << ")";

                    if (field.exception.size()) {
                        server_invoke_start << "Make<ServerExcept, " << field.exception;
                    } else if (field.retval) {
                        server_invoke_start << "Make<ServerRet";
                    } else {
                        server_invoke_start << "MakeServerField<" << field.args;
                    }
                    server_invoke_start << ", Accessor<" << base_name << "_fields::" << Cap(field_name) << ", "
                                        << field_flags.str() << ">>(";
                    server_invoke_end << ")";
                }

                const std::string static_str{is_construct || is_destroy ? "static " : ""};
                const std::string super_str{is_construct || is_destroy ? "Super& super" : ""};
                const std::string self_str{is_construct || is_destroy ? "super" : "*this"};

                client << "    using M" << method_ordinal << " = ProxyClientMethodTraits<" << method_prefix
                       << "Params>;\n";
                client << "    " << static_str << "typename M" << method_ordinal << "::Result " << method_name << "("
                       << super_str << client_args.str() << ")";
                client << ";\n";
                def_client << "ProxyClient<" << message_namespace << "::" << node_name << ">::M" << method_ordinal
                           << "::Result ProxyClient<" << message_namespace << "::" << node_name << ">::" << method_name
                           << "(" << super_str << client_args.str() << ") {\n";
                if (has_result) {
                    def_client << "    typename M" << method_ordinal << "::Result result;\n";
                }
                def_client << "    clientInvoke(" << self_str << ", &" << message_namespace << "::" << node_name
                           << "::Client::" << method_name << "Request" << client_invoke.str() << ");\n";
                if (has_result) def_client << "    return result;\n";
                def_client << "}\n";

                server << "    kj::Promise<void> " << method_name << "(" << Cap(method_name)
                       << "Context call_context) override;\n";

                def_server << "kj::Promise<void> ProxyServer<" << message_namespace << "::" << node_name
                           << ">::" << method_name << "(" << Cap(method_name)
                           << "Context call_context) {\n"
                              "    return serverInvoke(*this, call_context, "
                           << server_invoke_start.str();
                if (is_destroy) {
                    def_server << "ServerDestroy()";
                } else {
                    def_server << "ServerCall()";
                }
                def_server << server_invoke_end.str() << ");\n}\n";
                ++method_ordinal;
            });

            client << "};\n";
            server << "};\n";
            dec << "\n" << client.str() << "\n" << server.str() << "\n";
            KJ_IF_MAYBE(bracket, proxied_class_type.findFirst('<')) {
              // Skip ProxyType definition for complex type expressions which
              // could lead to duplicate definitions. They can be defined
              // manually if actually needed.
            } else {
              dec << "template<>\nstruct ProxyType<" << proxied_class_type << ">\n{\n";
              dec << "    using Type = " << proxied_class_type << ";\n";
              dec << "    using Message = " << message_namespace << "::" << node_name << ";\n";
              dec << "    using Client = ProxyClient<Message>;\n";
              dec << "    using Server = ProxyServer<Message>;\n";
              dec << "};\n";
              int_client << "ProxyTypeRegister t" << node_nested.getId() << "{TypeList<" << proxied_class_type << ">{}};\n";
            }
            def_types << "ProxyClient<" << message_namespace << "::" << node_name
                      << ">::~ProxyClient() { clientDestroy(*this); " << client_destroy.str() << " }\n";
            def_types << "ProxyServer<" << message_namespace << "::" << node_name
                      << ">::~ProxyServer() { serverDestroy(*this); }\n";
        }
    }

    h << methods.str() << "namespace " << base_name << "_fields {\n"
      << accessors.str() << "} // namespace " << base_name << "_fields\n"
      << dec.str();

    cpp_server << def_server.str();
    cpp_server << "} // namespace mp\n";

    cpp_client << def_client.str();
    cpp_client << "namespace {\n" << int_client.str() << "} // namespace\n";
    cpp_client << "} // namespace mp\n";

    cpp_types << def_types.str();
    cpp_types << "} // namespace mp\n";

    inl << "} // namespace mp\n";
    inl << "#endif\n";

    h << "} // namespace mp\n";
    h << "#if defined(__GNUC__)\n";
    h << "#pragma GCC diagnostic pop\n";
    h << "#endif\n";
    h << "#endif\n";
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: " << PROXY_BIN << " SRC_PREFIX INCLUDE_PREFIX SRC_FILE [IMPORT_PATH...]\n";
        exit(1);
    }
    std::vector<kj::StringPtr> import_paths;
    std::vector<kj::Own<const kj::ReadableDirectory>> import_dirs;
    auto fs = kj::newDiskFilesystem();
    auto cwd = fs->getCurrentPath();
    kj::Own<const kj::ReadableDirectory> src_dir;
    KJ_IF_MAYBE(dir, fs->getRoot().tryOpenSubdir(cwd.evalNative(argv[1]))) {
        src_dir = kj::mv(*dir);
    } else {
        throw std::runtime_error(std::string("Failed to open src_prefix prefix directory: ") + argv[1]);
    }
    for (int i = 4; i < argc; ++i) {
        KJ_IF_MAYBE(dir, fs->getRoot().tryOpenSubdir(cwd.evalNative(argv[i]))) {
            import_paths.emplace_back(argv[i]);
            import_dirs.emplace_back(kj::mv(*dir));
        } else {
            throw std::runtime_error(std::string("Failed to open import directory: ") + argv[i]);
        }
    }
    for (const char* path : {CMAKE_INSTALL_PREFIX "/include", capnp_PREFIX "/include"}) {
        KJ_IF_MAYBE(dir, fs->getRoot().tryOpenSubdir(cwd.evalNative(path))) {
            import_paths.emplace_back(path);
            import_dirs.emplace_back(kj::mv(*dir));
        }
        // No exception thrown if _PREFIX directories do not exist
    }
    Generate(argv[1], argv[2], argv[3], import_paths, *src_dir, import_dirs);
    return 0;
}

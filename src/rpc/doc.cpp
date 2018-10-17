// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <rpc/doc.h>

RPCDocExample::RPCDocExample(const std::string& description, const std::string& code)
    : m_description(description), m_code(code)
{
}

RPCDocExample::RPCDocExample(const std::string& code)
    : m_code(code)
{
}

std::string RPCDocExample::AsText() const
{
    std::string result;
    if (!m_description.empty()) {
        result += m_description;
        result += "\n";
    }

    result += "> ";
    result += m_code;
    result += "\n";
    return result;
}

RPCDocTableRow::RPCDocTableRow(const std::string& code)
    : m_code(code)
{
}

RPCDocTableRow::RPCDocTableRow(const std::string& code, const std::string& description)
    : m_code(code), m_description(description)
{
}

RPCDocTableRow::RPCDocTableRow(const std::string& code, const std::initializer_list<std::string>& types, const std::string& description)
    : m_code(code), m_types(types), m_description(description)
{
}

std::string const& RPCDocTableRow::Code() const
{
    return m_code;
}

std::vector<std::string> const& RPCDocTableRow::Types() const
{
    return m_types;
}

std::vector<std::string> RPCDocTableRow::DescriptionLines() const
{
    std::vector<std::string> res;
    boost::split(res, m_description, boost::is_any_of("\n"));
    return res;
}

size_t RPCDocTable::PrefixLength() const
{
    size_t max = 0;
    for (auto const& row : m_rows) {
        size_t prefix = row.Code().length() + 2;
        if (prefix > max) {
            max = prefix;
        }
    }
    return max;
}


RPCDocTable::RPCDocTable(const std::string& name)
    : m_name(name)
{
}

void RPCDocTable::AddRow(const RPCDocTableRow& row)
{
    m_rows.push_back(row);
}

std::string RPCDocTable::AsText() const
{
    std::string res = "";
    res += m_name;
    res += ":\n";

    size_t prefixLen = PrefixLength();
    for (auto const& row : m_rows) {
        std::string code = row.Code();
        res += row.Code();
        auto lines = row.DescriptionLines();
        bool firstLine = true;
        for (auto const& line : lines) {
            size_t spaces;
            if (firstLine) {
                spaces = prefixLen - code.length();
            } else {
                spaces = prefixLen;
            }
            res += std::string(spaces, ' ');

            if (firstLine) {
                auto types = row.Types();
                if (!types.empty()) {
                    bool firstType = true;
                    res += "(";
                    for (auto const& type : types) {
                        if (!firstType) {
                            res += ", ";
                        }
                        res += type;
                        firstType = false;
                    }
                    res += ") ";
                }
            }
            res += line;
            res += "\n";
            firstLine = false;
        }
    }
    return res;
}

RPCDoc::RPCDoc(std::string methodName, std::string firstArguments)
    : m_methodName(methodName), m_firstArguments(firstArguments)
{
}

RPCDoc::RPCDoc(std::string methodName)
    : m_methodName(methodName), m_firstArguments("")
{
}

RPCDoc& RPCDoc::Desc(const std::string& description)
{
    m_description = description;
    return *this;
}

RPCDoc& RPCDoc::Table(const std::string& name)
{
    m_tables.push_back(RPCDocTable(name));
    return *this;
}

RPCDoc& RPCDoc::Row(const std::string& code)
{
    if (m_tables.empty()) {
        throw std::out_of_range("No table in RPC doc.");
    }
    m_tables.back().AddRow(RPCDocTableRow(code));
    return *this;
}

RPCDoc& RPCDoc::Row(const std::string& code, const std::string& description)
{
    if (m_tables.empty()) {
        throw std::out_of_range("No table in RPC doc.");
    }
    m_tables.back().AddRow(RPCDocTableRow(code, description));
    return *this;
}

RPCDoc& RPCDoc::Row(const std::string& code, const std::initializer_list<std::string>& types, const std::string& description)
{
    if (m_tables.empty()) {
        throw std::out_of_range("No table in RPC doc.");
    }
    m_tables.back().AddRow(RPCDocTableRow(code, types, description));
    return *this;
}

RPCDoc& RPCDoc::Row(const std::string& code, const std::initializer_list<std::string>& types)
{
    if (m_tables.empty()) {
        throw std::out_of_range("No table in RPC doc.");
    }
    m_tables.back().AddRow(RPCDocTableRow(code, types, ""));
    return *this;
}

RPCDoc& RPCDoc::Rows(const std::vector<RPCDocTableRow>& rows)
{
    for (auto const& row : rows) {
        m_tables.back().AddRow(row);
    }
    return *this;
}

RPCDoc& RPCDoc::Example(const std::string& code)
{
    m_examples.push_back(RPCDocExample(code));
    return *this;
}

RPCDoc& RPCDoc::Example(const std::string& code, const std::string& example)
{
    m_examples.push_back(RPCDocExample(code, example));
    return *this;
}

RPCDoc& RPCDoc::ExampleCli(const std::string& description, const std::string& methodName, const std::string& args)
{
    m_examples.push_back(
        RPCDocExample(description, "bitcoin-cli " + methodName + " " + args));
    return *this;
}

RPCDoc& RPCDoc::ExampleRpc(const std::string& description, const std::string& methodName, const std::string& args)
{
    m_examples.push_back(RPCDocExample(
        description,
        "curl --user myusername --data-binary '{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", "
        "\"method\": \"" +
            methodName + "\", \"params\": [" + args + "] }' -H 'content-type: text/plain;' http://127.0.0.1:8332/"));
    return *this;
}

RPCDoc& RPCDoc::ExampleCli(const std::string& description, const std::string& args)
{
    return ExampleCli(description, m_methodName, args);
}

RPCDoc& RPCDoc::ExampleRpc(const std::string& description, const std::string& args)
{
    return ExampleRpc(description, m_methodName, args);
}

RPCDoc& RPCDoc::ExampleCli(const std::string& args)
{
    return ExampleCli("", args);
}

RPCDoc& RPCDoc::ExampleRpc(const std::string& args)
{
    return ExampleRpc("", args);
}

std::string RPCDoc::AsText() const
{
    std::string result;

    result += m_methodName;
    if (!m_firstArguments.empty()) {
        result += " ";
        result += m_firstArguments;
    }

    result += "\n";

    if (!m_description.empty()) {
        result += "\n";
        result += m_description;
        result += "\n";
    }

    for (const auto& table : m_tables) {
        result += "\n";
        result += table.AsText();
    }

    if (!m_examples.empty()) {
        result += "\nExamples:\n";
        for (const auto& example : m_examples) {
            result += "\n";
            result += example.AsText();
        }
    }

    return result;
}

std::runtime_error RPCDoc::AsError() const
{
    return std::runtime_error(AsText());
}

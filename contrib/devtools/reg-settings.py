from __future__ import annotations
import argparse
import collections
import enum
import os
import re
import subprocess
from dataclasses import dataclass, field

@dataclass
class Call:
    file: str
    position: int
    call_text: str
    obj_name: str
    arg_name: str
    context: str
    namespace: str

class DataType(int, enum.Enum):
    STRING_LIST = 1
    STRING = 2
    PATH = 3
    INT = 4
    BOOL = 5
    DISABLED = 6
    UNSET = 7

# What default value is returned by a particular GetArg call when the setting is
# unset. Either a string containing a c++ expression, or True to indicate it
# returns whatever the default-constructed value for the setting type is (e.g.
# the empty vector, or string, or path) or False to indicate it is a
# single-argument GetArg, GetIntArg, or GetBoolArg call returning std::nullopt.
DefaultValue = str | bool

@dataclass
class SettingType:
    """Information about one C++ setting type (one "using FooSetting =
    Setting<...>" declaration in the output). There is usually just a single
    SettingType for each setting, but if a setting is registered multiple times
    there will be multiple SettingsType instances. Also if is a setting is
    retrieved with inconsistent types there will be different SettingType
    instances for each type."""
    name: str
    primary: bool = False
    defaults: set[DefaultValue] = field(default_factory=set)
    """Set of default arguments passed to GetArg() calls for this setting."""
    default_value: DefaultValue = False
    includes: set[str] = field(default_factory=set)
    """Include paths needed in setting file for default expressions"""
    default_fn: bool = False
    """Whether needs to use DefaultFn<> instead of Default<> because default value is not constexpr"""

@dataclass
class AddArg:
    call: Call
    summary: str
    help_text: str
    help_args: tuple[str, ...]
    flags: str
    category: str
    include_path: str | None = None
    """Path to _settings.h file defining setting"""
    rel_include_path: str | None = None
    data_types: dict[DataType, SettingType] = field(default_factory=dict)
    includes: set[str] = field(default_factory=set)
    """Include paths needed in setting file for help format variables"""
    help_fn: bool = False
    """Whether needs to use HelpFn<> instead of HelpArgs<> because help arguments are not not constexpr"""
    extern_args: list[str] = field(default_factory=list)
    """List of external arguments needed to forwarded from Setting::Register to HelpFn to format the help string."""
    hidden_args: list[AddArg] | None = None
    """One AddArg object is produced for each argument listed in a single
    AddHiddenArgs({...}) call, but all the AddArg objects from the same call a
    hidden_args member pointing to a shared list of related AddArgs."""
    nonhidden_arg: AddArg | None = None
    """Reference to duplicate nonhidden argument, if this is a hidden argument duplicating a non-hidden one."""
    fake: bool = False
    """If true, AddArg object does not correspond to real AddArg call, and is
    used to retrieve test settings that are never registered."""

@dataclass
class GetArg:
    call: Call
    function_name: str
    data_type: DataType
    default_value: DefaultValue
    nots: int
    add: AddArg | None = None

@dataclass
class Setting:
    arg_name: str
    adds: list[AddArg] = field(default_factory=list)
    gets: list[GetArg] = field(default_factory=list)

@dataclass
class Range:
    start: int
    end: int | None

def get_files_with_args(src_dir: str) -> list[str]:
    # Run git grep to find files containing AddArg/GetArg/GetIntArg/GetBoolArg/GetArgs
    result = subprocess.run(
        [
            "git", "grep", "-l", r"AddArg(\|AddHiddenArgs(\|GetArg(\|GetPathArg(\|GetIntArg(\|GetBoolArg(\|GetArgs(\|IsArgSet(\|IsArgNegated(", "--", src_dir
        ],
        capture_output=True,
        text=True
    )
    return result.stdout.splitlines()

# Converts something like:
#   '("-foo", strprintf("helptext %i", DEFAULT), ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS)'
# into:
#   (125, ['"-foo"', ' strprintf("helptext %i", DEFAULT', ' ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION', ' OptionsCategory::OPTIONS'])
def parse_function_args(arg_str: str) -> tuple[int, list[str]]:
    args: list[str] = []
    stack: list[str] = []
    quot = False
    for pos, c in enumerate(arg_str):
        if c == '"':
            quot = not quot
        if quot:
            pass
        elif c == "(":
            stack.append(")")
            if len(stack) == 1: continue
        elif c == "{":
            stack.append("}")
            if len(stack) == 1: continue
        elif c == stack[-1]:
            stack.pop()
            if not stack: break
        elif c == "," and len(stack) == 1:
            args.append("")
            continue
        if not args: args.append("")
        args[-1] += c
    return pos, args

def parse_calls(file_path: str) -> tuple[list[AddArg], list[GetArg]]:
    adds: list[AddArg] = []
    gets: list[GetArg] = []
    context = get_file_context(file_path)
    namespace = get_file_namespace(file_path)
    with open(file_path, 'r') as f:
        content = f.read()
    for match in re.finditer(r'\b(\w+)\.AddArg\((")', content):
        call_len, (summary, help_text, flags, category) = parse_function_args(content[match.start(2)-1:])
        arg_name_match = re.match(r'"([^"=(]+).*', summary)
        assert arg_name_match
        call = Call(
            file=file_path,
            position=match.start(),
            call_text=content[match.start():match.start(2)+call_len],
            obj_name=match.group(1),
            arg_name=arg_name_match.group(1),
            context=context,
            namespace=namespace,
        )
        help_text=help_text.strip()
        help_args: list[str] = []
        if m := re.match(r"strprintf\(", help_text):
            _, help_args = parse_function_args(help_text[m.end()-1:])
            help_text = help_args[0].strip()
            help_args = [a.strip() for a in help_args[1:]]
        adds.append(AddArg(
            call=call,
            summary=summary.strip(),
            help_text=help_text,
            help_args=tuple(help_args),
            flags=flags.strip(),
            category=category.strip(),
        ))
    for match in re.finditer(r'( *)\b(\w+)\.AddHiddenArgs\((\{)', content):
        call_len, call_args = parse_function_args(content[match.start(3):])
        hidden_args: list[AddArg] = []
        for summary in call_args:
            summary = summary.strip()
            if not summary: continue
            arg_name_match = re.match(r'"([^"=(]+).*', summary)
            assert arg_name_match
            call = Call(
                file=file_path,
                position=match.start(),
                call_text=content[match.start():match.start(3)+call_len+2],
                obj_name=match.group(2),
                arg_name=arg_name_match.group(1),
                context=context,
                namespace=namespace,
            )
            hidden_args.append(AddArg(
                call=call,
                summary=summary,
                help_text="",
                help_args=(),
                flags="ArgsManager::ALLOW_ANY",
                category="OptionsCategory::HIDDEN",
                hidden_args=hidden_args,
            ))
            adds.append(hidden_args[-1])

    for match in re.finditer(r'(!*)\b(?:((?:\w|\.|->|\(\))+)(\.|->))?(GetArg|GetPathArg|GetIntArg|GetBoolArg|GetArgs|IsArgSet|IsArgNegated)\((.)', content):
        call_len, call_args = parse_function_args(content[match.start(5)-1:])
        if not call_args: continue
        nots, obj_name, op, function_name, _ = match.groups()
        if op == "->":
            obj_name = f"*{obj_name}"
        elif obj_name is None:
            obj_name = "*this"
        arg_name = ""
        if m := re.match('^"(.*)"$', call_args[0].strip()):
            arg_name = m.group(1)
        call = Call(
            file=file_path,
            position=match.start(),
            call_text=content[match.start():match.start(5)+call_len],
            obj_name=obj_name,
            arg_name=arg_name,
            context=context,
            namespace=namespace,
        )
        data_type = (DataType.STRING_LIST if function_name == "GetArgs" else
                     DataType.STRING if function_name == "GetArg" else
                     DataType.PATH if function_name == "GetPathArg" else
                     DataType.INT if function_name == "GetIntArg" else
                     DataType.BOOL if function_name == "GetBoolArg" else
                     DataType.DISABLED if function_name == "IsArgNegated" else
                     DataType.UNSET if function_name == "IsArgSet" else
                     None)
        assert data_type
        default_arg = call_args[1].strip() if len(call_args) > 1 else None
        default_value = (
            True if data_type == DataType.STRING and default_arg == '""' else
            True if data_type == DataType.INT and default_arg == "0" else
            True if data_type == DataType.BOOL and default_arg == "false" else
            default_arg if default_arg is not None else
            False if data_type in (DataType.STRING, DataType.INT, DataType.BOOL) else
            True if data_type in (DataType.PATH, DataType.STRING_LIST, DataType.DISABLED, DataType.UNSET) else
            None)
        assert default_value is not None
        gets.append(GetArg(call=call, function_name=function_name, data_type=data_type, default_value=default_value, nots=len(match.group(1))))
    return adds, gets

def make_setting(settings: dict[str, Setting], call: Call) -> Setting:
    name = call.arg_name.lstrip("-")
    if name in settings:
        setting = settings[name]
    else:
        setting = settings[name] = Setting(name)
    return setting

def flags_to_options(flag_str: str) -> list[str]:
    flags = set()
    for flag in flag_str.split("|"):
        flag = flag.strip()
        if flag: flags.add(flag)

    def pop(flag):
        if flag in flags:
            flags.remove(flag)
            return True
        return False

    options = [".legacy = true"]
    if pop("ArgsManager::DEBUG_ONLY"):
        options.append(".debug_only = true")
    if pop("ArgsManager::NETWORK_ONLY"):
        options.append(".network_only = true")
    if pop("ArgsManager::SENSITIVE"):
        options.append(".sensitive = true")
    if pop("ArgsManager::DISALLOW_NEGATION"):
      options.append(".disallow_negation = true")
    if pop("ArgsManager::DISALLOW_ELISION"):
        options.append(".disallow_elision = true")
    pop("ArgsManager::ALLOW_ANY")
    if flags:
        raise Exception(f"Unknown flags {flags!r}")
    return options

def collect_argument_information(src_dir: str) -> dict[str, Setting]:
    files = get_files_with_args(src_dir)
    settings: dict[str, Setting] = {}
    for file in files:
        adds, gets = parse_calls(file)
        for add in adds:
            setting = make_setting(settings, add.call)
            setting.adds.append(add)
        for get in gets:
            setting = make_setting(settings, get.call)
            setting.gets.append(get)

    for arg_name, setting in settings.items():
        # Find duplicate hidden/nonhidden settings
        for hadd in setting.adds:
            if hadd.hidden_args is not None:
                for add in setting.adds:
                    if add.hidden_args is None and add.call.context == hadd.call.context and add.call.arg_name == hadd.call.arg_name:
                        hadd.nonhidden_arg = add
                        break

        # Link GetArg/AddArg calls. If any GetArg calls don't have associated
        # AddArg calls (only happens in tests) create fake AddArg objects.
        for add in setting.adds:
            for get in setting.gets:
                if get.add is None and same_context(add, get):
                    get.add = add
        fake_adds: dict[tuple[str, str], AddArg] = {} # map of (file path, arg name) -> AddArg
        for get in setting.gets:
            if get.add is None and get.call.arg_name:
                key = get.call.file, get.call.arg_name
                if key in fake_adds:
                    get.add = fake_adds[key]
                else:
                    add = AddArg(call=get.call, summary=f'"{get.call.arg_name}"', help_text="", help_args=(), flags="", category="", fake=True)
                    get.add = fake_adds[key] = add
                    setting.adds.append(add)

        # Figure out setting types and default values.
        setting_name = ''.join(NAME_MAP.get(word) or word.capitalize() for word in arg_name.split('-')) + "Setting"
        counter: collections.Counter = collections.Counter()
        for add in setting.adds:
            add.include_path = add.call.file.replace(".cpp", "_settings.h")
            add.rel_include_path = re.sub("^src/", "", add.include_path)
            key = add.call.context, arg_name
            add_setting_name = setting_name
            if add.hidden_args is None:
                counter[key] += 1
                if counter[key] > 1: add_setting_name += str(counter[key])
            elif add.nonhidden_arg is not None:
                continue
            else:
                add_setting_name += "Hidden"
            for get in setting.gets:
                if not same_context(add, get):
                    continue
                if get.data_type in add.data_types:
                    setting_type = add.data_types[get.data_type]
                else:
                    setting_type = add.data_types[get.data_type] = SettingType(add_setting_name)
                setting_type.defaults.add(get.default_value)

            # Clean up data types. Add if empty, remove if redundant, set primary
            if len(add.data_types) == 0:
                add.data_types[DataType.UNSET] = SettingType(add_setting_name)
            add.data_types[min(add.data_types.keys())].primary = True
            for data_type in (DataType.DISABLED, DataType.UNSET):
                if data_type in add.data_types and not add.data_types[data_type].primary:
                    del add.data_types[data_type]

            for data_type, setting_type in add.data_types.items():
                # If same setting is retrieved as different types, add suffixes to distinguish setting names
                if not setting_type.primary:
                    setting_type.name += (
                        "List" if data_type == DataType.STRING_LIST else
                        "Str" if data_type == DataType.STRING else
                        "Path" if data_type == DataType.PATH else
                        "Int" if data_type == DataType.INT else
                        "Bool" if data_type == DataType.BOOL else
                        "Disabled" if data_type == DataType.DISABLED else
                        "Unset" if data_type == DataType.UNSET else
                        None
                    )
                # Only set ::Default<> if there are no GetArg calls returning
                # std::optional and there is a single consistent default.
                if False not in setting_type.defaults:
                    defaults = set()
                    for default_value in setting_type.defaults:
                        if isinstance(default_value, str):
                            for pattern, options in ARG_PATTERNS.items():
                                if re.search(pattern, default_value) and options.extern:
                                    # Treat a GetArg() default argument with
                                    # extern=True the same as not specifying any
                                    # default. extern=True indicates argument is
                                    # non-constant and can't be embedded into
                                    # the SettingType with ::Default or
                                    # ::DefaultFn.
                                    default_value = False
                                    break
                                if pattern == default_value and options.namespace:
                                    default_value = f"{options.namespace}::{default_value}"
                                    break
                        defaults.add(default_value)
                    if len(defaults) == 1:
                        default_value = next(iter(defaults))
                        assert default_value is not None
                        setting_type.default_value = default_value

                # Check if default setting value requires any includes or DefaultFn lambda
                for pattern, options in ARG_PATTERNS.items():
                    if isinstance(setting_type.default_value, str) and re.search(pattern, setting_type.default_value):
                        if options.include_path:
                            setting_type.includes.add(options.include_path)
                        if options.runtime:
                            setting_type.default_fn = True
                        assert not options.extern

            # Check if help string formatting requires any includes or external variables.
            for pattern, options in ARG_PATTERNS.items():
                if re.search(pattern, add.help_text) or any(re.search(pattern, a) for a in add.help_args):
                    if options.include_path:
                        add.includes.add(options.include_path)
                    if options.runtime:
                        add.help_fn = True
                    if options.extern:
                        add.extern_args.append(pattern)

    return settings

def setting_definitions(add: AddArg) -> tuple[list[str], set[str]]:
    defs = []
    includes = set()
    # If this is hidden argument and there is non-hidden argument with the same
    # name and type information no need to output a separate definition.
    if add.nonhidden_arg is None:
        includes.update(add.includes)
        # Generate setting definition for a given setting name and type.
        for data_type, setting_type in sorted(add.data_types.items(), key=lambda p: p[0]):
            includes.update(setting_type.includes)
            ctype = ("std::vector<std::string>" if data_type == DataType.STRING_LIST else
                      "std::string" if data_type == DataType.STRING else
                      "fs::path" if data_type == DataType.PATH else
                      "int64_t" if data_type == DataType.INT else
                      "bool" if data_type == DataType.BOOL else
                      "common::Disabled" if data_type == DataType.DISABLED else
                      "common::Unset" if data_type == DataType.UNSET else
                      None)
            assert ctype
            if False in setting_type.defaults:
                ctype = f"std::optional<{ctype}>"
            help_str = ""
            if setting_type.primary and add.help_text:
                help_str = f",\n    {add.help_text}"
            extra = ""
            help_args = ', '.join(a for a in add.help_args)
            default_arg = (setting_type.default_value if setting_type.default_value is not True else
                            '""' if data_type == DataType.STRING else
                            "0" if data_type == DataType.INT else
                            "false" if data_type == DataType.BOOL else
                            f"{ctype}{{}}")
            if setting_type.default_value is True and (not help_args or help_args != default_arg):
                default_arg = False
            if default_arg:
                namespace = get_file_namespace(add.include_path)
                prefix = f"{namespace}::"
                if default_arg.startswith(prefix):
                    default_arg = default_arg[len(prefix):]
                if setting_type.default_fn:
                    extra += f"\n    ::DefaultFn<[] {{ return {default_arg}; }}>"
                else:
                    extra += f"\n    ::Default<{default_arg}>"
            if ((help_args and setting_type.primary) or default_arg) and help_args != default_arg:
                if add.help_fn or add.extern_args:
                    lambda_args = ", ".join(f"const auto& {a}" for a in ["help"] + add.extern_args)
                    extra += f"\n    ::HelpFn<[]({lambda_args}) {{ return help.Format({help_args}); }}>"
                else:
                    extra += f"\n    ::HelpArgs<{help_args}>"
            if add.category and add.category != "OptionsCategory::OPTIONS" and setting_type.primary:
                    extra += f"\n    ::Category<{add.category}>"
            options = flags_to_options(add.flags)
            # Writing common::SettingOptions{...} instead of just {...}
            # should be unneccesary because latter is valid initialization
            # syntax in C++20, but unfortunately it is only supported as of
            # clang 18. clang 17 and early versions do not seem to allow
            # using designated initializers to initialize template
            # parameters.
            options_str = f"common::SettingOptions{{{', '.join(options)}}}" if options else ""
            defs.append(f"\nusing {setting_type.name} = common::Setting<\n    {add.summary}, {ctype}, {options_str}{help_str}>{extra};\n")
    return defs, includes

def addarg_replacement(add: AddArg) -> str:
    new_call = ""
    if add.hidden_args is None:
        new_call = register_str(add)
    else:
        for hadd in add.hidden_args:
            if new_call: new_call += ";\n"
            hidden_match = re.match(" *", hadd.call.call_text)
            assert hidden_match
            spaces=hidden_match.group()
            new_call += f"{spaces}{register_str(hadd.nonhidden_arg or hadd, hidden=hadd.nonhidden_arg is not None)}"
    return new_call

def arg_name(add: AddArg) -> str:
    default_data_type = min(add.data_types.keys())
    return add.data_types[default_data_type].name

def register_str(add: AddArg, hidden: bool=False) -> str:
    register_args = ", ".join([add.call.obj_name] + add.extern_args)
    return f"{arg_name(add)}{'::Hidden' if hidden else ''}::Register({register_args})"

def getarg_replacement(get: GetArg) -> str:
    assert get.add
    setting_type = get.add.data_types.get(get.data_type) or get.add.data_types[min(get.add.data_types.keys())]
    default_arg = suffix = ""
    if get.data_type == DataType.UNSET:
        method = "Value"
        suffix = ".isNull()"
    elif get.data_type == DataType.DISABLED:
        method = "Value"
        suffix = ".isFalse()"
    else:
        method = "Get"
        if get.default_value and not setting_type.default_value:
            default_arg = (get.default_value if get.default_value is not True else
                           '""' if get.data_type == DataType.STRING else
                           "0" if get.data_type == DataType.INT else
                           "false" if get.data_type == DataType.BOOL else "{}")
            default_arg = f", {default_arg}"
    new_call = ((get.nots + (get.data_type == DataType.UNSET)) % 2) * "!"
    if get.add.call.namespace and get.call.namespace != get.add.call.namespace:
        new_call += f"{get.add.call.namespace}::"
    new_call += f"{setting_type.name}::{method}({get.call.obj_name}{default_arg}){suffix}"
    return new_call

def add_to_file(file_path: str, local_include: list[str], system_include: list[str]=[], defs: list[str]=[]):
    if os.path.exists(file_path):
        with open(file_path, 'r') as f:
            lines = f.readlines()
    else:
        guard = "BITCOIN_" + re.sub("^src/", "", file_path).replace('/', '_').replace('.', '_').replace('-', '_').upper()
        lines = [
            f"#ifndef {guard}\n"
            f"#define {guard}\n"
            "\n"
        ]
        namespace = get_file_namespace(file_path)
        if namespace:
            lines.extend([
                f"namespace {namespace} {{\n",
                f"}} // namespace {namespace}\n"
            ])
        lines.extend(["\n", f"#endif // {guard}\n"])

    # Identify include blocks and their positions
    blocks: list[Range] = []
    self_include = f"#include <{file_path.replace('src/', '').replace('.cpp', '.h')}>"
    first = last = None
    for i, line in enumerate(lines):
        if line.startswith('#include') and "IWYU pragma: keep" not in line and not line.startswith(self_include):
            if not blocks or blocks[-1].end is not None:
                blocks.append(Range(i, None))
        elif blocks and blocks[-1].end is None:
            blocks[-1].end = i

        if first is None and not line.startswith("//") and not line.startswith("#ifndef") and not line.startswith("#define") and line != "\n":
            first = i
        if line != "\n" and not line.startswith("#endif") and not line.startswith("} // namespace "):
            last = i + 1

    if len(blocks) == 0:
        # If there are no include blocks, add an empty one where includes should go.
        assert first
        assert last
        m = min(first, last)
        blocks.append(Range(m, m))
    if len(blocks) == 1:
        # If there's only one include block, hard to tell if it is contains
        # local or system includes, but reasonably heuristic is to assume it
        # contains local includes in .cpp files and system includes in .h files.
        if file_path.endswith(".cpp"):
            assert blocks[0].end
            blocks.append(Range(blocks[0].end, blocks[0].end))
        else:
            blocks.insert(0, Range(blocks[0].start, blocks[0].start))

    lines[last:last] = defs

    system_includes = set(f"#include <{i}>\n" for i in system_include)
    if system_includes:
        existing_includes = lines[blocks[1].start:blocks[1].end]
        # Add extra newline after system includes if they are new
        tail = ["\n"] if blocks[1].start == blocks[1].end == first else []
        lines[blocks[1].start:blocks[1].end] = sorted(system_includes | set(existing_includes)) + tail

    local_includes = set(f"#include <{i}>\n" for i in local_include)
    if local_includes:
        existing_includes = lines[blocks[0].start:blocks[0].end]
        # Add extra newline after local includes if they are new and system includes are present
        tail = ["\n"] if blocks[0].start == blocks[0].end and (system_includes or blocks[1].start != blocks[1].end) else []
        lines[blocks[0].start:blocks[0].end] = sorted(local_includes | set(existing_includes)) + tail

    with open(file_path, 'w') as f:
        f.writelines(lines)

def replace_in_file(file_path: str, old: str, new: str, replacements: list[tuple[str, str]] | None=None):
    with open(file_path, 'r') as f:
        content = f.read()
    if replacements is not None:
        # Update list of past replacments, needed if replacements within a
        # file can be contained within each other
        for o, n in replacements:
            old = old.replace(o, n)
            new = new.replace(o, n)
        replacements.append((old, new))
    new_content = content.replace(old, new)
    with open(file_path, 'w') as f:
        f.write(new_content)

def modify_source_files(settings: dict[str, Setting], git_commit: bool=False):
    # map file path->list of (old, new) tuples with GetArg->Get replacements made so far
    replacements: dict[str, list[tuple[str, str]]] = collections.defaultdict(list)

    for setting in settings.values():
        for add in setting.adds:
            if not add.fake:
                replace_in_file(add.call.file, add.call.call_text, addarg_replacement(add))
            assert add.rel_include_path
            add_to_file(add.call.file, [add.rel_include_path])

            for get in setting.gets:
                if get.add is add:
                    replace_in_file(get.call.file, get.call.call_text, getarg_replacement(get),
                                    replacements[get.call.file])
                    assert get.add.rel_include_path
                    add_to_file(get.call.file, [get.add.rel_include_path])

            defs, def_includes = setting_definitions(add)
            if defs:
                assert add.include_path
                add_to_file(add.include_path,
                            [include for include in def_includes | {"common/setting.h"}],
                            ["string", "vector"], defs)

            if git_commit and subprocess.run(["git", "status", "--porcelain", "--untracked-files=no"], stdout=subprocess.PIPE, check=True).stdout:
                assert add.include_path
                subprocess.run(["git", "add", "-N", add.include_path], check=True)
                msg = f"{add.rel_include_path}: Add {arg_name(add.nonhidden_arg or add)}"
                subprocess.run(["git", "commit", "-am", msg], check=True)

def same_context(add, get):
    if add.call.context == get.call.context:
        return True
    if add.call.context == "main" and get.call.context == "gui" and not add.hidden_args:
        return True
    return False

def get_file_context(path):
    if path in ["src/bitcoin-cli.cpp"]: return "cli"
    if path in ["src/bitcoin-tx.cpp"]: return "tx"
    if path in ["src/bitcoin-util.cpp"]: return "util"
    if path in ["src/bitcoin-wallet.cpp", "src/wallet/wallettool.cpp", "src/wallet/dump.cpp"]: return "wallet-tool"
    if path in ["src/dummywallet.cpp"]: return "dummywallet"
    if path.startswith("src/qt/test/"): return "test"
    if path.startswith("src/qt/"): return "gui"
    if path in ["src/test/argsman_tests.cpp", "src/test/logging_tests.cpp,", "src/test/fuzz/system.cpp", "src/test/getarg_tests.cpp"]: return path
    if path in ["src/zmq/zmqnotificationinterface.cpp"]: return "test" # FIX
    return "main"

def get_file_namespace(path):
    if path.startswith("src/wallet/"): return "wallet"
    return ""

@dataclass
class PatternOptions:
    include_path: str | None = None
    runtime: bool = False
    extern: bool = False
    """extern=True for help arguments indiciates argument is non-constant
    constant, so will be formatted with ::HelpFn rather than ::HelpArgs.

    extern=True for default arguments indicates that GetArg() default argument is
    non-constant, so it can't be added to the SettingType with ::Default or
    ::DefaultFn and needs to be passed as a runtime argument to Get()"""
    namespace: str | None = None

# Expression patterns to look for in AddArg / GetArg calls, and options
# controlling what to do when patterns are matched.
ARG_PATTERNS = {
    # Constants
    "BITCOIN_CONF_FILENAME": PatternOptions(include_path="common/args.h", runtime=True),
    "BITCOIN_PID_FILENAME": PatternOptions(include_path="init.h", runtime=True),
    "BITCOIN_SETTINGS_FILENAME": PatternOptions(include_path="common/args.h", runtime=True),
    "COOKIEAUTH_FILE": PatternOptions(extern=True),
    "CURRENCY_UNIT": PatternOptions(include_path="policy/feerate.h", runtime=True),
    "DEFAULT_ACCEPT_STALE_FEE_ESTIMATES": PatternOptions(include_path="policy/fees/block_policy_estimator.h"),
    "DEFAULT_ADDRESS_TYPE": PatternOptions(include_path="wallet/wallet.h"),
    "DEFAULT_ADDRMAN_CONSISTENCY_CHECKS": PatternOptions(include_path="addrman.h"),
    "DEFAULT_ASMAP_FILENAME": PatternOptions(include_path="init.h", runtime=True),
    "DEFAULT_AVOIDPARTIALSPENDS": PatternOptions(include_path="wallet/coincontrol.h", runtime=True),
    "DEFAULT_BENCH_FILTER": PatternOptions(runtime=True),
    "DEFAULT_BLOCKFILTERINDEX": PatternOptions(include_path="index/blockfilterindex.h", runtime=True),
    "DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN": PatternOptions(include_path="net_processing.h"),
    "DEFAULT_CHOOSE_DATADIR": PatternOptions(include_path="qt/intro.h"),
    "DEFAULT_COINSTATSINDEX": PatternOptions(include_path="index/coinstatsindex.h"),
    "DEFAULT_COLOR_SETTING": PatternOptions(runtime=True),
    "DEFAULT_DAEMON": PatternOptions(include_path="init.h"),
    "DEFAULT_DB_CACHE": PatternOptions(include_path="node/caches.h"),
    "DEFAULT_DEBUGLOGFILE": PatternOptions(include_path="logging.h", runtime=True),
    "DEFAULT_DISABLE_WALLET": PatternOptions(include_path="wallet/wallet.h", namespace="wallet"),
    "DEFAULT_HTTP_SERVER_TIMEOUT": PatternOptions(include_path="httpserver.h"),
    "DEFAULT_LISTEN": PatternOptions(include_path="net.h"),
    "DEFAULT_MAX_MEMPOOL_SIZE_MB": PatternOptions(include_path="kernel/mempool_options.h"),
    "DEFAULT_MAX_TRIES": PatternOptions(include_path="rpc/mining.h"),
    "DEFAULT_MAX_UPLOAD_TARGET": PatternOptions(include_path="net.h", runtime=True),
    "DEFAULT_MISBEHAVING_BANTIME": PatternOptions(include_path="banman.h"),
    "DEFAULT_NATPMP": PatternOptions(include_path="mapport.h"),
    "DEFAULT_NBLOCKS": PatternOptions(runtime=True),
    "DEFAULT_PERSIST_MEMPOOL": PatternOptions(include_path="node/mempool_persist_args.h", namespace="node"),
    "DEFAULT_PRINT_MODIFIED_FEE": PatternOptions(include_path="node/miner.h"),
    "DEFAULT_PRIORITY": PatternOptions(runtime=True),
    "DEFAULT_SPLASHSCREEN": PatternOptions(include_path="qt/guiconstants.h"),
    "DEFAULT_STOPATHEIGHT": PatternOptions(include_path="node/kernel_notifications.h"),
    "DEFAULT_TOR_CONTROL": PatternOptions(include_path="torcontrol.h", runtime=True),
    "DEFAULT_TOR_CONTROL_PORT": PatternOptions(include_path="torcontrol.h"),
    "DEFAULT_TXINDEX": PatternOptions(include_path="index/txindex.h"),
    "DEFAULT_UIPLATFORM": PatternOptions(include_path="qt/bitcoingui.h", runtime=True),
    "DEFAULT_VALIDATION_CACHE_BYTES": PatternOptions(include_path="script/sigcache.h"),
    "DEFAULT_XOR_BLOCKSDIR": PatternOptions(include_path="kernel/blockmanager_opts.h"),
    "DEFAULT_ZMQ_SNDHWM": PatternOptions(include_path="zmq/zmqabstractnotifier.h"),
    "LIST_CHAIN_NAMES": PatternOptions(include_path="chainparamsbase.h"),
    "MAX_SCRIPTCHECK_THREADS": PatternOptions(include_path="node/chainstatemanager_args.h"),
    "UNIX_EPOCH_TIME": PatternOptions(include_path="rpc/util.h", runtime=True),

    # Chain parameters
    "defaultChainParams": PatternOptions(extern=True),
    "testnetChainParams": PatternOptions(extern=True),
    "testnet4ChainParams": PatternOptions(extern=True),
    "signetChainParams": PatternOptions(extern=True),
    "regtestChainParams": PatternOptions(extern=True),
    "defaultBaseParams": PatternOptions(extern=True),
    "testnetBaseParams": PatternOptions(extern=True),
    "testnet4BaseParams": PatternOptions(extern=True),
    "signetBaseParams": PatternOptions(extern=True),
    "regtestBaseParams": PatternOptions(extern=True),

    # Misc expressions used as defaults and help arguments
    "args.": PatternOptions(extern=True),
    "BaseParams\\(\\)": PatternOptions(include_path="chainparamsbase.h", runtime=True),
    "DatabaseOptions": PatternOptions(include_path="wallet/db.h", runtime=True),
    "^def$": PatternOptions(extern=True),
    "!expected": PatternOptions(extern=True),
    "FormatMoney\\(": PatternOptions(include_path="util/moneystr.h", runtime=True),
    "FormatOutputType\\(": PatternOptions(include_path="outputtype.h", runtime=True),
    "FormatAllOutputTypes\\(\\)": PatternOptions(include_path="outputtype.h", runtime=True),
    "format_value": PatternOptions(extern=True),
    "gArgs": PatternOptions(extern=True),
    "Join\\(": PatternOptions(include_path="util/string.h", runtime=True),
    "lang_territory": PatternOptions(extern=True),
    "ListBlockFilterTypes\\(\\)": PatternOptions(include_path="blockfilter.h", runtime=True),
    "LogInstance\\(\\)": PatternOptions(include_path="logging.h", runtime=True),
    "mempool_limits.": PatternOptions(extern=True),
    "mempool_opts.": PatternOptions(extern=True),
    "nBytesPerSigOp": PatternOptions(include_path="policy/settings.h", runtime=True),
    "nDefaultDbBatchSize": PatternOptions(include_path="txdb.h"),
    "options\\.": PatternOptions(extern=True),
    "PathToString\\(": PatternOptions(include_path="util/fs.h", runtime=True),
    "pblock->nVersion": PatternOptions(extern=True),
    "proxy_doc": PatternOptions(extern=True),
    '"default"': PatternOptions(extern=True),
    '"regtest only; "': PatternOptions(runtime=True),
    '"xxx"': PatternOptions(extern=True),
    '"zzzzz"': PatternOptions(extern=True),
}

# Name mapping to give settings CamelCase names. Using the map is optional, if a
# setting name is not found in here it will just be capitalized.
NAME_MAP = {
  "?": "Q",
  "acceptnonstdtxn": "AcceptNonstdTxn",
  "acceptstalefeeestimates": "AcceptStaleFeeEstimates",
  "addnode": "AddNode",
  "addresstype": "AddressType",
  "addrinfo": "AddrInfo",
  "alertnotify": "AlertNotify",
  "allowignoredconf": "AllowIgnoredConf",
  "asmap": "AsMap",
  "assumevalid": "AssumeValid",
  "avoidpartialspends": "AvoidPartialSpends",
  "bantime": "BanTime",
  "blockfilterindex": "BlockFilterIndex",
  "blockmaxweight": "BlockMaxWeight",
  "blockmintxfee": "BlockMinTxFee",
  "blocknotify": "BlockNotify",
  "blockreconstructionextratxn": "BlockReconstructionExtraTxn",
  "blocksdir": "BlocksDir",
  "blocksonly": "BlocksOnly",
  "blocksxor": "BlocksXor",
  "blockversion": "BlockVersion",
  "booltest1": "BoolTest1",
  "booltest2": "BoolTest2",
  "booltest3": "BoolTest3",
  "booltest4": "BoolTest4",
  "bytespersigop": "BytesPerSigOp",
  "capturemessages": "CaptureMessages",
  "changetype": "ChangeType",
  "checkaddrman": "CheckAddrMan",
  "checkblockindex": "CheckBlockIndex",
  "checkblocks": "CheckBlocks",
  "checklevel": "CheckLevel",
  "checkmempool": "CheckMempool",
  "checkpoints": "CheckPoints",
  "choosedatadir": "ChooseDataDir",
  "cjdnsreachable": "CJdnsReachable",
  "coinstatsindex": "CoinStatsIndex",
  "consolidatefeerate": "ConsolidateFeeRate",
  "daemonwait": "DaemonWait",
  "datacarrier": "DataCarrier",
  "datacarriersize": "DataCarrierSize",
  "datadir": "DataDir",
  "dbbatchsize": "DbBatchSize",
  "dbcache": "DbCache",
  "dbcrashratio": "DbCrashRatio",
  "dblogsize": "DbLogSize",
  "debugexclude": "DebugExclude",
  "debuglogfile": "DebugLogFile",
  "delin": "DelIn",
  "delout": "DelOut",
  "deprecatedrpc": "DeprecatedRpc",
  "disablewallet": "DisableWallet",
  "discardfee": "DiscardFee",
  "dnsseed": "DnsSeed",
  "dumpfile": "DumpFile",
  "dustrelayfee": "DustRelayFee",
  "externalip": "ExternalIp",
  "fallbackfee": "FallbackFee",
  "fastprune": "FastPrune",
  "fixedseeds": "FixedSeeds",
  "flushwallet": "FlushWallet",
  "fooo": "FooO",
  "forcecompactdb": "ForceCompactDb",
  "forcednsseed": "ForceDnsSeed",
  "getinfo": "GetInfo",
  "i2pacceptincoming": "I2pAcceptIncoming",
  "i2psam": "I2pSam",
  "includeconf": "IncludeConf",
  "incrementalrelayfee": "IncrementalRelayFee",
  "inttest1": "IntTest1",
  "inttest2": "IntTest2",
  "inttest3": "IntTest3",
  "ipcbind": "IpcBind",
  "keypool": "KeyPool",
  "limitancestorcount": "LimitAncestorCount",
  "limitancestorsize": "LimitAncestorSize",
  "limitdescendantcount": "LimitDescendantCount",
  "limitdescendantsize": "LimitDescendantSize",
  "listenonion": "ListenOnion",
  "loadblock": "LoadBlock",
  "locktime": "LockTime",
  "logips": "LogIps",
  "loglevel": "LogLevel",
  "loglevelalways": "LogLevelAlways",
  "logsourcelocations": "LogSourceLocations",
  "logthreadnames": "LogThreadNames",
  "logtimemicros": "LogTimeMicros",
  "logtimestamps": "LogTimestamps",
  "maxapsfee": "MaxApsFee",
  "maxconnections": "MaxConnections",
  "maxmempool": "MaxMemPool",
  "maxorphantx": "MaxOrphanTx",
  "maxreceivebuffer": "MaxReceiveBuffer",
  "maxsendbuffer": "MaxSendBuffer",
  "maxsigcachesize": "MaxSigCacheSize",
  "maxtipage": "MaxTipAge",
  "maxtxfee": "MaxTxFee",
  "maxuploadtarget": "MaxUploadTarget",
  "mempoolexpiry": "MempoolExpiry",
  "minimumchainwork": "MinimumChainWork",
  "minrelaytxfee": "MinRelayTxFee",
  "mintxfee": "MinTxFee",
  "mocktime": "MockTime",
  "natpmp": "NatPmp",
  "netinfo": "NetInfo",
  "networkactive": "NetworkActive",
  "nversion": "NVersion",
  "onlynet": "OnlyNet",
  "outaddr": "OutAddr",
  "outdata": "OutData",
  "outmultisig": "OutMultiSig",
  "outpubkey": "OutPubKey",
  "outscript": "OutScript",
  "paytxfee": "PayTxFee",
  "peerblockfilters": "PeerBlockFilters",
  "peerbloomfilters": "PeerBloomFilters",
  "peertimeout": "PeerTimeout",
  "permitbaremultisig": "PermitBareMultiSig",
  "persistmempool": "PersistMempool",
  "persistmempoolv1": "PersistMempoolV1",
  "printpriority": "PrintPriority",
  "printtoconsole": "PrintToConsole",
  "pritest1": "PriTest1",
  "pritest2": "PriTest2",
  "pritest3": "PriTest3",
  "pritest4": "PriTest4",
  "privdb": "PrivDb",
  "proxyrandomize": "ProxyRandomize",
  "regtest": "RegTest",
  "reindex": "ReIndex",
  "reindex-chainstate": "ReIndexChainState",
  "resetguisettings": "ResetGuiSettings",
  "rpcallowip": "RpcAllowIp",
  "rpcauth": "RpcAuth",
  "rpcbind": "RpcBind",
  "rpcclienttimeout": "RpcClientTimeout",
  "rpcconnect": "RpcConnect",
  "rpccookiefile": "RpcCookieFile",
  "rpccookieperms": "RpcCookiePerms",
  "rpcdoccheck": "RpcDocCheck",
  "rpcpassword": "RpcPassword",
  "rpcport": "RpcPort",
  "rpcservertimeout": "RpcServerTimeout",
  "rpcthreads": "RpcThreads",
  "rpcuser": "RpcUser",
  "rpcwait": "RpcWait",
  "rpcwaittimeout": "RpcWaitTimeout",
  "rpcwallet": "RpcWallet",
  "rpcwhitelist": "RpcWhitelist",
  "rpcwhitelistdefault": "RpcWhitelistDefault",
  "rpcworkqueue": "RpcWorkQueue",
  "seednode": "SeedNode",
  "shrinkdebugfile": "ShrinkDebugFile",
  "shutdownnotify": "ShutdownNotify",
  "signetchallenge": "SignetChallenge",
  "signetseednode": "SignetSeedNode",
  "spendzeroconfchange": "SpendZeroConfChange",
  "startupnotify": "StartupNotify",
  "stdinrpcpass": "StdinRpcPass",
  "stdinwalletpassphrase": "StdinWalletPassphrase",
  "stopafterblockimport": "StopAfterBlockImport",
  "stopatheight": "StopAtHeight",
  "strtest1": "StrTest1",
  "strtest2": "StrTest2",
  "swapbdbendian": "SwapBdbEndian",
  "testactivationheight": "TestActivationHeight",
  "testdatadir": "TestDataDir",
  "testnet": "TestNet",
  "testnet4": "TestNet4",
  "torcontrol": "TorControl",
  "torpassword": "TorPassword",
  "txconfirmtarget": "TxConfirmTarget",
  "txid": "TxId",
  "txindex": "TxIndex",
  "txreconciliation": "TxReconciliation",
  "uacomment": "UaComment",
  "uiplatform": "UiPlatform",
  "unsafesqlitesync": "UnsafeSqliteSync",
  "v2transport": "V2Transport",
  "vbparams": "VbParams",
  "walletbroadcast": "WalletBroadcast",
  "walletcrosschain": "WalletCrossChain",
  "walletdir": "WalletDir",
  "walletnotify": "WalletNotify",
  "walletrbf": "WalletRbf",
  "walletrejectlongchains": "WalletRejectLongChains",
  "whitebind": "WhiteBind",
  "whitelist": "WhiteList",
  "whitelistforcerelay": "WhiteListForceRelay",
  "whitelistrelay": "WhiteListRelay",
  "withinternalbdb": "WithInternalBdb",
  "zmqpubhashblock": "ZmqPubHashBlock",
  "zmqpubhashblockhwm": "ZmqPubHashBlockHwm",
  "zmqpubhashtx": "ZmqPubHashTx",
  "zmqpubhashtxhwm": "ZmqPubHashTxHwm",
  "zmqpubrawblock": "ZmqPubRawBlock",
  "zmqpubrawblockhwm": "ZmqPubRawBlockHwm",
  "zmqpubrawtx": "ZmqPubRawTx",
  "zmqpubrawtxhwm": "ZmqPubRawTxHwm",
  "zmqpubsequence": "ZmqPubSequence",
  "zmqpubsequencehwm": "ZmqPubSequenceHwm",
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--git-commit', action='store_true')
    args = parser.parse_args()
    src_dir = "src/"
    settings = collect_argument_information(src_dir)
    modify_source_files(settings, git_commit=args.git_commit)

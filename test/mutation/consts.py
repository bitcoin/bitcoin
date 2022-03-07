FILES = [
    "src/wallet/coinselection.cpp",
    # "src/wallet/spend.cpp",
    # "src/net.cpp",
    # "src/wallet/feebumper.cpp",
    # "src/script/interpreter.cpp"
]

REPLACE_OPERATORS = [
             [" > ", " < "], [" < ", " > "], [" >= ", " > "],
             [" >= ", " <= "], [" <= ", " > "], [" == ", " != "],
             [" != ", " == "], [" + ", " - "],  [" - ", " + "],
             [" && ", " || "], [" || ", " && "], [" / ", " * "],
             [" += ", " -= "], [" -= ", " += "], [" % ", " * "],
             [" /= ", " *= "], [" *= ", " /= "], [" * ", " / "],
             [" break", " continue"], [" continue", " break"],
             ["std::all_of", "std::any_of"], ["std::any_of", "std::all_of"],
             ["std::min", "std::max"], ["std::max", "std::min"], ["std::begin", "std::end"],
             ["std::end", "std::begin"], ["true", "false"], ["false", "true"]]

REPLACEMENT_IN_REGEX = ["LESS_THAN", "RANDOM_INT", "GREATER_THAN"]

REGEX_OPERATORS = [[r"at(\(.*\))", r"at(0)"], [r"CAmount\s+(\w+)\s*=\s*([0-9]+)", r"CAmount \1 = RANDOM_INT"],
                   [r"CAmount\s+(\w+)\s*=\s*([0-9]+)", r"CAmount \1 = LESS_THAN"],
                   [r"CAmount{+(\w+)}", r"CAmount{RANDOM_INT}"], [r"int\s+(\w+)\s*=\s*([0-9]+)", r"int \1 = RANDOM_INT"],
                   [r"int\s+(\w+)\s*=\s*([0-9]+)", r"int \1 = LESS_THAN"], [r"int\s+(\w+)\s*=\s*([0-9]+)", r"int \1 = GREATER_THAN"],
                   [r"int32_t\s+(\w+)\s*=\s*([0-9]+)", r"int32_t \1 = LESS_THAN"], [r"int32_t\s+(\w+)\s*=\s*([0-9]+)", r"int32_t \1 = GREATER_THAN"],
                   [r"int64_t\s+(\w+)\s*=\s*([0-9]+)", r"int64_t \1 = RANDOM_INT"], [r"int64_t\s+(\w+)\s*=\s*([0-9]+)", r"int64_t \1 = GREATER_THAN"],
                   [r"bool\s+(\w+)\s*=\s+(.*)", r"bool \1 = true;"], [r"int64_t\s+(\w+)\s*=\s*([0-9]+)", r"int64_t \1 = LESS_THAN"],
                   [r"bool\s+(\w+)\s*=\s+(.*)", r"bool \1 = false;"], [r"std::chrono::seconds (\w+){+(\w+)}", r"std::chrono::seconds \1{RANDOM_INT}"],
                   [r"int32_t\s+(\w+)\s*=\s*([0-9]+)", r"int32_t \1 = RANDOM_INT"], [r"const\s+size_t\s+(\w+)\s*=\s*([0-9]+)", r"const size_t \1 = RANDOM_INT"],
                   [r"(^\s*)(\S+.*)\n", r"\1\2\n\1break;\n"], [r"(^\s*)(\S+.*)\n", r"\1\2\n\1continue;\n"],
                   [r"std::chrono::seconds (\w+){+(\w+)}", r"std::chrono::seconds \1{RANDOM_INT}"],
                   [r"std::chrono::seconds (\w+)\s*=\s*(.*)", r"std::chrono::seconds \1 = RANDOM_INT;"]]

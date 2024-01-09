# Disable files from being included in completions by default
complete --command bitcoind --no-files

# Extract options
function __fish_bitcoind_get_options
    argparse 'nofiles' -- $argv
    set --local cmd (commandline -opc)[1]
    set --local options

    if set -q _flag_nofiles
        set --append options ($cmd -help-debug | string match -r '^  -.*' | string replace -r '  -' '-' | string replace -r '=.*' '=' | string match --invert -r '^.*=$')
    else
        set --append options ($cmd -help-debug | string match -r '^  -.*' | string replace -r '  -' '-' | string replace -r '=.*' '=' | string match -r '^.*=$')
    end

    for option in $options
        echo $option
    end
end


# Add options with file completion
complete \
    --command bitcoind \
    --arguments "(__fish_bitcoind_get_options)"
# Enable file completions only if the commandline now contains a `*.=` style option
complete --command bitcoind \
    --condition 'string match --regex -- ".*=" (commandline -pt)' \
    --force-files

# Add options without file completion
complete \
    --command bitcoind \
    --arguments "(__fish_bitcoind_get_options --nofiles)"


# Disable files from being included in completions by default
complete --command syscoin-util --no-files

# Extract options
function __fish_syscoin_util_get_options
    set --local cmd (commandline -opc)[1]
    set --local options

    set --append options ($cmd -help 2>&1 | string match -r '^  -.*' | string replace -r '  -' '-' | string replace -r '=.*' '=')

    for option in $options
        echo $option
    end
end

# Extract commands
function __fish_syscoin_util_get_commands
    set --local cmd (commandline -opc)[1]
    set --local commands

    set --append commands ($cmd -help | sed -e '1,/Commands:/d' -e 's/=/=\t/' -e 's/(=/=/' -e '/^  [a-z]/ p' -e d | string replace -r '\ \ ' '')
    for command in $commands
        echo $command
    end
end

# Add options
complete \
    --command syscoin-util \
    --condition "not __fish_seen_subcommand_from (__fish_syscoin_util_get_commands)" \
    --arguments "(__fish_syscoin_util_get_options)"

# Add commands
complete \
    --command syscoin-util \
    --condition "not __fish_seen_subcommand_from (__fish_syscoin_util_get_commands)" \
    --arguments "(__fish_syscoin_util_get_commands)"


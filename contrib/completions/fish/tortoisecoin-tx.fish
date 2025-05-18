# Disable files from being included in completions by default
complete --command tortoisecoin-tx --no-files

# Modified version of __fish_seen_subcommand_from
# Uses regex to detect cmd= syntax
function __fish_tortoisecoin_seen_cmd
    set -l cmd (commandline -oc)
    set -e cmd[1]
    for i in $cmd
        for j in $argv
            if string match --quiet --regex -- "^$j.*" $i
                return 0
            end
        end
    end
    return 1
end

# Extract options
function __fish_tortoisecoin_tx_get_options
    set --local cmd (commandline -oc)[1]
    if string match --quiet --regex -- '^-help$|-\?$' $cmd
        return
    end

    for option in ($cmd -help 2>&1 | string match -r '^  -.*' | string replace -r '  -' '-' | string replace -r '=.*' '=')
        echo $option
    end
end

# Extract commands
function __fish_tortoisecoin_tx_get_commands
    argparse 'commandsonly' -- $argv
    set --local cmd (commandline -oc)[1]
    set --local commands

    if set -q _flag_commandsonly
        set --append commands ($cmd -help | sed -e '1,/Commands:/d' -e 's/=/=\t/' -e 's/(=/=/' -e '/^  [a-z]/ p' -e d | string replace -r '\ \ ' '' | string replace -r '=.*' '')
    else
        set --append commands ($cmd -help | sed -e '1,/Commands:/d' -e 's/=/=\t/' -e 's/(=/=/' -e '/^  [a-z]/ p' -e d | string replace -r '\ \ ' '')
    end

    for command in $commands
        echo $command
    end
end

# Add options
complete \
    --command tortoisecoin-tx \
    --condition "not __fish_tortoisecoin_seen_cmd (__fish_tortoisecoin_tx_get_commands --commandsonly)" \
    --arguments "(__fish_tortoisecoin_tx_get_options)" \
    --no-files

# Add commands
complete \
    --command tortoisecoin-tx \
    --arguments "(__fish_tortoisecoin_tx_get_commands)" \
    --no-files

# Add file completions for load and set commands
complete \
    --command tortoisecoin-tx \
    --condition 'string match --regex -- "(load|set)=" (commandline -pt)' \
    --force-files

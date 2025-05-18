# Disable files from being included in completions by default
complete --command tortoisecoin-wallet --no-files

# Extract options
function __fish_tortoisecoin_wallet_get_options
    set --local cmd (commandline -opc)[1]
    for option in ($cmd -help 2>&1 | string match -r '^  -.*' | string replace -r '  -' '-' | string replace -r '=.*' '=')
        echo $option
    end
end

# Extract commands
function __fish_tortoisecoin_wallet_get_commands
    set --local cmd (commandline -opc)[1]
    for command in ($cmd -help | sed -e '1,/Commands:/d' -e 's/=/=\t/' -e 's/(=/=/' -e '/^  [a-z]/ p' -e d | string replace -r '\ \ ' '')
        echo $command
    end
end

# Add options
complete \
    --command tortoisecoin-wallet \
    --condition "not __fish_seen_subcommand_from (__fish_tortoisecoin_wallet_get_commands)" \
    --arguments "(__fish_tortoisecoin_wallet_get_options)"

# Add commands
complete \
    --command tortoisecoin-wallet \
    --condition "not __fish_seen_subcommand_from (__fish_tortoisecoin_wallet_get_commands)" \
    --arguments "(__fish_tortoisecoin_wallet_get_commands)"

# Add file completions for load and set commands
complete --command tortoisecoin-wallet \
    --condition "string match -r -- '(dumpfile|datadir)*=' (commandline -pt)" \
    --force-files

Bash Completion
---------------------

The following script provide bash completion functionality for `sycoind` and `syscoin-cli`.

* `contrib/syscoind.bash-completion`
* `contrib/syscoin-cli.bash-completion`
* `contrib/_osx.bash-completion`

### OSX ###
Use `brew` to install `bash-completion` then update `~/.bashrc` to include the completion scripts and helper functions to provide `have()` and `_have()` on OSX.

The example assumes Syscoin was compiled in `~/syscoin` and the scripts will be availabe in `~/syscoin/contrib`, however they can be moved to a different location.

```
brew install bash-completion

BASHRC=$(cat <<EOF
if [ -f $(brew --prefix)/etc/bash_completion ]; then
  . $(brew --prefix)/etc/bash_completion
fi
. ~/syscoin/contrib/_osx.bash-completion
. ~/syscoin/contrib/syscoind.bash-completion
. ~/syscoin/contrib/syscoin-cli.bash-completion
EOF
)

grep -q "/etc/bash_completion" ~/.bashrc || echo "$BASHRC" >> ~/.bashrc
. ~/.bashrc

```
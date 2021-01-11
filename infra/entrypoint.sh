#!/usr/bin/env bash
#
# ItCoin
#
# Entrypoint for the docker container of itcoin-core.
# Allows starting an arbitrary linux command inside the container.
#
# If the command is a itcoin-specific one (for example "bitcoind" or
# "bitcoin-cli"), the startup procedure is different: before launching the
# command, a configuration file is created via environment variable
# interpolation and placed at /opt/itcoin-core/configdir/bitcoin.conf. The
# requested itcoin binary is then launched pointing at that file, and at a fixed
# datadir (/opt/itcoin-core/datadir). Any additional command line parameters are
# then passed to the binary.
#
# This allows configuring itcoin components via enviromnent variables, even if
# the bitcoin binaries do not support it.
#
# USAGE:
#     entrypoint.sh --help
#     entrypoint.sh <bitcoind, bitcoin-cli> [ parameters... ]
#     entrypoint.sh somecommand [ parameters... ]
#
# EXAMPLE:
#    docker run --rm arthub.azurecr.io/itcoin-core:git-2a43646f76e4 --help
#    docker run --rm --env BLOCKSCRIPT=XXX <more variables...> arthub.azurecr.io/itcoin-core:git-2a43646f76e4 bitcoin-cli -help
#    docker run --rm arthub.azurecr.io/itcoin-core:git-2a43646f76e4 cat /etc/passwd
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

# This is the set of environment variables that will be supported by the itcoin
# container when running some bitcoin-specific commands.
SUPPORTED_VARIABLES=(
	BITCOIN_PORT
	BLOCKSCRIPT
	RPC_HOST
	RPC_PORT
	ZMQ_PUBHASHTX_PORT
	ZMQ_PUBRAWBLOCK_PORT
)

INTERNAL_CONFIG_DIR="/opt/itcoin-core/configdir"
INTERNAL_DATADIR="/opt/itcoin-core/datadir"

usage() {
	cat <<-USAGE
	USAGE:

	entrypoint.sh < [--help|-H] | command [parameters...] >

	You have to invoke this container passing the name of a command to execute.
	The command can be any binary existing in the container, including the ones
	specific for itcoin, which are:
	$(printf "    - %s\n" $(ls -1 /usr/local/bin))

	Additionally, for the following commands:
	    - bitcoind
	    - bitcoin-cli
	    - miner

	A dynamic runtime configuration will be performed. This REQUIRES that the
	following environment variables are set:
	$(printf "    - %s\n" "${SUPPORTED_VARIABLES[@]}")

	Please note that, inside the container, the configuration file will be
	separated form the data directory (this is different from default bitcoin
	behaviour):
	    - datadir:       ${INTERNAL_DATADIR}
	    - configuration: ${INTERNAL_CONFIG_DIR}/bitcoin.conf

	These directories ARE NOT PRESENT in the container: you will have to provide
	them at execution time.
	    - datadir must be a bind mount on a physical directory on the host
	    - ${INTERNAL_CONFIG_DIR} must be a tmpfs mount that will host an
	      ephemeral configuration file, generated at runtime
	USAGE

	exit 0
} # usage

generateConfigFile() {
	# fail gracefully if the configuration directory is not writable.
	# Remember to our users that they need to bind mount a tmpfs there, in order
	# to store the dynamically generated config file.
	if [ ! -d "${INTERNAL_CONFIG_DIR}" ]; then
		>&2 cat <<-ERROR_MSG
			The directory ${INTERNAL_CONFIG_DIR} does not exist or is not a directory.
			Please ensure to run this container with --tmps ${INTERNAL_CONFIG_DIR}
		ERROR_MSG

		exit 1
	fi
	if [ ! -w "${INTERNAL_CONFIG_DIR}" ]; then
		>&2 cat <<-ERROR_MSG
			The directory ${INTERNAL_CONFIG_DIR} is not writable.
			Please ensure to run this container with --tmps ${INTERNAL_CONFIG_DIR}
		ERROR_MSG

		exit 1
	fi

	# generate the configuration file, based on the env var values
	render-template.sh "${SUPPORTED_VARIABLES[@]}" < /opt/itcoin-core/bitcoin.conf.tmpl > "${INTERNAL_CONFIG_DIR}/bitcoin.conf"
} # generateConfigFile

if [ "${1-}" = '' ] || [ "${1-}" = '--help' ] || [ "${1-}" = '-H' ] ; then
	usage
fi

if [ "$1" = 'bitcoind' ] || [ "$1" = 'bitcoin-cli' ] || [ "$1" = 'miner' ]; then
	# if the command is a itcoin-specific one, we need to create bitcoin.conf
	# interpolating the templates with environment variables, and execute the
	# command, forcing it to use the configuration file we just generated, and a
	# fixed datadir.
	COMMAND="$1"

	# Any additional command line parameters will be passed untouched to the
	# binary.
	shift

	generateConfigFile
fi

# The idiom "${COMMAND-}" expands to the empty string if COMMAND is not defined.
# This allows us to keep happy the nounset (set -u) shell option, otherwise it
# would complain.
#
# see: https://unix.stackexchange.com/questions/56837/how-to-test-if-a-variable-is-defined-at-all-in-bash-prior-to-version-4-2-with-th#56846
if [ "${COMMAND-}" = 'bitcoind' ]; then
	exec bitcoind \
		-conf="${INTERNAL_CONFIG_DIR}/bitcoin.conf" \
		-datadir="${INTERNAL_DATADIR}" \
		"$@"
	# The control flow will never reach here, because we exec-ed.
fi

if [ "${COMMAND-}" = 'bitcoin-cli' ]; then
	exec bitcoin-cli \
		-conf="${INTERNAL_CONFIG_DIR}/bitcoin.conf" \
		-datadir="${INTERNAL_DATADIR}" \
		-rpcconnect="${RPC_HOST}" \
		"$@"
	# The control flow will never reach here, because we exec-ed.
fi

if [ "${COMMAND-}" = 'miner' ]; then
	exec miner \
		--cli="bitcoin-cli -conf=${INTERNAL_CONFIG_DIR}/bitcoin.conf -datadir=${INTERNAL_DATADIR} -rpcconnect=${RPC_HOST}" \
		"$@"
	# The control flow will never reach here, because we exec-ed.
fi

# We reach here if the container was called with an arbitrary command (let's
# say, "cat /etc/passwd" instead of "bitcoind"). Just launch it.
exec "$@"

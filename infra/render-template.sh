#!/usr/bin/env bash
#
# ItCoin
#
# Reads a bash-like templated file from stdin, and accepts a space separated
# list of variable names to interpolate. The template file is written on stdout,
# after replacing only the indicated template variables with the corresponding
# values taken from the environment.
#
# The variables indicated in the command line must be defined and must not be an
# empty string.
#
# REQUIREMENTS:
# - envsubst (apt get install gettext-base)
#
# USAGE:
#     render-template.sh VAR_NAME_1 VAR_NAME_2 ... < inputfile.tmpl
#
# EXAMPLE:
#     $ cat inputfile.tmpl:
#     keyname=${VAR_NAME}
#
#     $ export VAR_NAME=somevalue
#
#     $ render-template.sh VAR_NAME < inputfile.tmpl
#     keyname=somevalue
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

errecho() {
	# prints to stderr
	>&2 echo "${@}"
}

checkPrerequisites() {
	if ! command -v envsubst &> /dev/null; then
		errecho "Please install envsubst (apt install gettext-base)"
		exit 1
	fi
}

# Do not run if the required packages are not installed
checkPrerequisites

for VAR_NAME in "${@}"; do
	if [[ ! -v "${VAR_NAME}" ]]; then
		errecho "ERROR: variable ${VAR_NAME} is not set"
		exit 1
	elif [[ -z "${!VAR_NAME}" ]]; then
		errecho "ERROR: variable ${VAR_NAME} is set to the empty string"
		exit 2
    fi

	# MEMO:
	# if needed for debugging purposes, we could print the variable name and
	# value using:
	#     errecho "${VAR_NAME} has value ${!VAR_NAME}"
	# but that would end up being very noisy in production (and not every
	# variable's value can be printed wisely).
done

# https://unix.stackexchange.com/questions/294378/replacing-only-specific-variables-with-envsubst/294400#294400
VARIABLES_TO_BE_RENDERED=$(printf '${%s} \n' "${@}")

envsubst "${VARIABLES_TO_BE_RENDERED}" #< bitcoin.conf.tmpl

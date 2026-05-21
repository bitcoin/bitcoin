#!/usr/bin/env bash

export LC_ALL=C

set -o errexit -o nounset -o pipefail -o xtrace

write_env_var() {
  local key="$1"
  local value="$2"

  if [[ -n "${GITHUB_ENV:-}" ]]; then
    echo "${key}=${value}" >> "${GITHUB_ENV}"
  else
    export "${key}=${value}"
  fi
}

write_output_var() {
  local key="$1"
  local value="$2"

  if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    echo "${key}=${value}" >> "${GITHUB_OUTPUT}"
  else
    echo "${key}=${value}"
  fi
}

available_nproc() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  else
    sysctl -n hw.logicalcpu
  fi
}

determine_parallelism() {
  local multiplier="$1"
  local available

  available="$(available_nproc)"
  write_env_var BUILD_PARALLEL "${available}"
  write_env_var PARALLEL "$((available * multiplier))"
}

reset_ccache_stats() {
  which ccache
  ccache --version
  ccache --zero-stats
}

show_ccache_stats() {
  ccache --show-stats
}

install_apt_packages() {
  sudo apt-get install --no-install-recommends -y "$@"
}

install_homebrew_packages() {
  HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1 brew install --quiet "$@"
}

install_pip_packages() {
  pip3 install "$@"
}

determine_host() {
  local config_guess="${1:-./depends/config.guess}"
  local output_key="${2:-host}"

  write_output_var "${output_key}" "$("${config_guess}")"
}

ci_helpers_main() {
  local command="${1:?missing command}"
  shift

  [[ "${command}" =~ ^[a-z_][a-z0-9_]*$ ]] || {
    echo "Invalid command: ${command}" >&2
    exit 1
  }

  if declare -F "${command}" >/dev/null; then
    "${command}" "$@"
  else
    echo "Unknown command: ${command}" >&2
    exit 1
  fi
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  ci_helpers_main "$@"
fi

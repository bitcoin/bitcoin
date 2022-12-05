#!/usr/bin/env bash

echo "${@:2} | tee $1"

${@:2} | tee $1

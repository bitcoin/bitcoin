# syntax = edrevo/dockerfile-plus

FROM ubuntu:focal

INCLUDE+ ci/Dockerfile

# The inherited Dockerfile switches to non-privileged context and we've
# just started configuring this image, give us root access
USER root

# Discourage root access, this is an interactive instance
#
# Sometimes these commands are run repetitively _after_ the non-sudo
# user was introduced and therefore these commands would fail
# To mitigate the build halting, we've added "|| true" so that it
# unconditionally returns 0
#
RUN apt-get update && apt-get install $APT_ARGS sudo && rm -rf /var/lib/apt/lists/*
RUN usermod -aG sudo dash
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Make development environment more standalone
RUN apt-get update && apt-get install $APT_ARGS nano openssh-client zsh gdb && rm -rf /var/lib/apt/lists/*

# Disable noninteractive mode
ENV DEBIAN_FRONTEND="dialog"

# Expose Dash P2P and RPC ports for main network and test networks
EXPOSE 9998 9999 19998 19999

# We're done, switch back to non-privileged user
USER dash

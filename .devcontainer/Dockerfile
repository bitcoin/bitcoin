FROM ubuntu:focal
ARG APT_PACKAGES_DEPS="automake autotools-dev bsdmainutils build-essential libboost-filesystem-dev libboost-system-dev libboost-test-dev libboost-thread-dev libdb-dev libdb++-dev libevent-dev libminiupnpc-dev libqrencode-dev libqt5core5a libqt5dbus5 libqt5gui5 libtool libzmq3-dev pkg-config python3 qttools5-dev qttools5-dev-tools"
ARG APT_PACKAGES_DEVENV="apt-utils bear black clang clang-tidy cppcheck curl dialog emacs flake8 gdb git htop isort less links lsb-release lsof lynx man manpages manpages-dev manpages-posix mc mypy net-tools netcat procps pydocstyle pyflakes3 pylama pylint python3-autopep8 python3-jedi python3-pip shellcheck sudo vim wget yapf3"
ARG DEBIAN_FRONTEND="noninteractive"
ARG LOGIN_GID=1000
ARG LOGIN_UID=1000
ARG LOGIN_USERNAME="vscode"
ARG PYPI_PACKAGES_DEVENV="prospector==1.3.0 rope==0.18.0"
RUN apt-get update \
    && apt-get install --no-install-recommends -y ${APT_PACKAGES_DEPS} ${APT_PACKAGES_DEVENV} 2>&1 \
    && pip3 install ${PYPI_PACKAGES_DEVENV} \
    && groupadd --gid ${LOGIN_GID} ${LOGIN_USERNAME} \
    && useradd -s /bin/bash --uid ${LOGIN_UID} --gid ${LOGIN_GID} -m ${LOGIN_USERNAME} \
    && echo "${LOGIN_USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${LOGIN_USERNAME} \
    && chmod 0440 /etc/sudoers.d/${LOGIN_USERNAME}

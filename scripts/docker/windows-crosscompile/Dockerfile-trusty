FROM ubuntu:trusty

#Default branch name develop
ARG BRANCH_NAME=develop
#Default
ARG REPO_SLUG=peercoin/peercoin
ENV REPO_SLUG=${REPO_SLUG}
ENV REPO_URL=https://github.com/${REPO_SLUG}

RUN apt-get -qq update && \
    apt-get -qqy install \
    git \
    sudo

#RUN git clone ${REPO_URL} --branch $BRANCH_NAME --single-branch --depth 1

COPY peercoin.tar.gz /peercoin.tar.gz
RUN tar -xvf /peercoin.tar.gz

#xenial
#Missing requirement: libtool
#RUN apt install -yqq libtool-bin
RUN cd /peercoin/scripts/windows-crosscompile && ./dependencies.sh
RUN cd /peercoin && scripts/windows-crosscompile/compile-peercoin.sh

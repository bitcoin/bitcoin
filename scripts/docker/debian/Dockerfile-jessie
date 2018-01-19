FROM debian:jessie

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

RUN cd /peercoin && ./scripts/dependencies-ubuntu.sh && ./scripts/install-ubuntu.sh && \
	apt-get purge git build-essential -qy && \
	apt-get autoremove -qy && \
	apt-get clean

FROM phusion/baseimage:bionic-1.0.0
LABEL maintainer="Dash Developers <dev@dash.org>"
LABEL description="Dockerised DashCore, built from Travis"

ARG USER_ID
ARG GROUP_ID

ENV HOME /dash

# add user with specified (or default) user/group ids
ENV USER_ID ${USER_ID:-1000}
ENV GROUP_ID ${GROUP_ID:-1000}
RUN groupadd -g ${GROUP_ID} dash
RUN useradd -u ${USER_ID} -g dash -s /bin/bash -m -d /dash dash
RUN mkdir /dash/.dashcore
RUN chown dash:dash -R /dash

COPY bin/* /usr/local/bin/

RUN chmod a+x /usr/local/bin/*

USER dash

VOLUME ["/dash"]

EXPOSE 9998 9999 19998 19999

WORKDIR /dash

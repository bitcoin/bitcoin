FROM phusion/baseimage:focal-1.0.0
LABEL maintainer="Dash Developers <dev@dash.org>"
LABEL description="Dockerised DashCore, built from CI"

ARG USER_ID
ARG GROUP_ID

ENV HOME /home/dash

# add user with specified (or default) user/group ids
ENV USER_ID ${USER_ID:-1000}
ENV GROUP_ID ${GROUP_ID:-1000}
RUN groupadd -g ${GROUP_ID} dash && \
    useradd -u ${USER_ID} -g dash -s /bin/bash -m -d /home/dash dash && \
    mkdir /home/dash/.dashcore && \
    chown ${USER_ID}:${GROUP_ID} -R /home/dash

COPY bin/* /usr/local/bin/

RUN chmod a+x /usr/local/bin/*

USER dash

VOLUME ["/home/dash"]

EXPOSE 9998 9999 19998 19999

WORKDIR /home/dash

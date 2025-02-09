FROM phusion/baseimage:noble-1.0.0
LABEL maintainer="Dash Developers <dev@dash.org>"
LABEL description="Dockerised DashCore, built from CI"

# Setup unprivileged user
ARG USER_ID=1000 \
    GROUP_ID=1000
RUN groupmod -g ${GROUP_ID} -n dash ubuntu; \
    usermod -u ${USER_ID} -md /home/dash -l dash ubuntu; \
    mkdir -p /home/dash/.dashcore && \
    chown ${USER_ID}:${GROUP_ID} -R /home/dash

COPY bin/* /usr/local/bin/

RUN chmod a+x /usr/local/bin/*

USER dash

VOLUME ["/home/dash"]

EXPOSE 9998 9999 19998 19999

WORKDIR /home/dash

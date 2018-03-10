#!/bin/bash -ev

#Travis pre installs many unnecessary packages https://docs.travis-ci.com/user/reference/trusty/

sudo apt-get -y --purge remove mongodb* mysql* postgresql* rabbitmq* redis* 


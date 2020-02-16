#!/usr/bin/env bash

coverage run --append --include='blockchain_parser/*' --omit='*/tests/*' setup.py test
coverage report
coverage erase

#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import os
import re
import json
import codecs
from urllib.request import Request, urlopen
from urllib.error import HTTPError

def get_response(req_url, ghtoken):
    req = Request(req_url)
    if ghtoken is not None:
        req.add_header('Authorization', 'token ' + ghtoken)
    return urlopen(req)

def retrieve_json(req_url, ghtoken):
    '''
    Retrieve json from github.
    Return None if an error happens.
    '''
    try:
        reader = codecs.getreader('utf-8')
        return json.load(reader(get_response(req_url, ghtoken)))
    except HTTPError as e:
        error_message = e.read()
        print('Warning: unable to retrieve pull information from github: %s' % e)
        print('Detailed error: %s' % error_message)
        return None
    except Exception as e:
        print('Warning: unable to retrieve pull information from github: %s' % e)
        return None

def retrieve_pr_info(pull, ghtoken):
    req_url = "https://api.github.com/repos/bitcoin/bitcoin/pulls/"+pull
    return retrieve_json(req_url, ghtoken)

def main():
    # Get pull request number from Travis environment
    if 'TRAVIS_PULL_REQUEST' in os.environ:
        pull = os.environ['TRAVIS_PULL_REQUEST']
    else:
        assert False, "Error: No pull request number found, PR description could not be checked"

    # This Github token is in the .travis.yml as a secure (encrypted)
    # variable. See Travis CI documentation on how it is created:
    # https://docs.travis-ci.com/user/environment-variables/#encrypting-environment-variables
    if 'GITHUB_TOKEN' in os.environ:
        ghtoken = os.environ['GITHUB_TOKEN']
    else:
        assert False, "Error: No Github token found, PR description could not be checked"

    # Receive pull information from github
    info = retrieve_pr_info(pull, ghtoken)
    if info is None:
        assert False, "Error: Did not receive a valid response from Github API"

    body = info['body'].strip()

    # Simple, good enough Github username regex that tries to replicate
    # the logic by which Github tags users in messages and notifies them:
    # - Newline/start of string or any special character except hyphen
    # - Followed by @
    # - Followed by one alphanumeric character
    # - Followed by up to 37 alphanumeric characters or hyphens
    # - End with end of line/string or any special character except hyphen
    # - Ignore case
    gh_username = "(^|[^a-z0-9-])@([a-z0-9])([a-z0-9-]){0,37}($|[^a-z0-9-])"
    assert not bool(re.search(gh_username, body, re.IGNORECASE)), "Please remove any GitHub @-prefixed usernames from your PR description"

    # Match a start or an end tag because there may just be a fraction
    # of the pull request template left over.
    html_comment_start = "<!--"
    assert not bool(re.search(html_comment_start, body)), "Please remove the pull request template from your PR description"
    html_comment_end = "-->"
    assert not bool(re.search(html_comment_end, body)), "Please remove the pull request template from your PR description"


if __name__ == '__main__':
    main()

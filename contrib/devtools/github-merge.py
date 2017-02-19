#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# This script will locally construct a merge commit for a pull request on a
# github repository, inspect it, sign it and optionally push it.

# The following temporary branches are created/overwritten and deleted:
# * pull/$PULL/base (the current master we're merging onto)
# * pull/$PULL/head (the current state of the remote pull request)
# * pull/$PULL/merge (github's merge)
# * pull/$PULL/local-merge (our merge)

# In case of a clean merge that is accepted by the user, the local branch with
# name $BRANCH is overwritten with the merged result, and optionally pushed.
from __future__ import division,print_function,unicode_literals
import os
from sys import stdin,stdout,stderr
import argparse
import subprocess
import json,codecs
try:
    from urllib.request import Request,urlopen
except:
    from urllib2 import Request,urlopen

# External tools (can be overridden using environment)
GIT = os.getenv('GIT','git')
BASH = os.getenv('BASH','bash')

# OS specific configuration for terminal attributes
ATTR_RESET = ''
ATTR_PR = ''
COMMIT_FORMAT = '%h %s (%an)%d'
if os.name == 'posix': # if posix, assume we can use basic terminal escapes
    ATTR_RESET = '\033[0m'
    ATTR_PR = '\033[1;36m'
    COMMIT_FORMAT = '%C(bold blue)%h%Creset %s %C(cyan)(%an)%Creset%C(green)%d%Creset'

def git_config_get(option, default=None):
    '''
    Get named configuration option from git repository.
    '''
    try:
        return subprocess.check_output([GIT,'config','--get',option]).rstrip().decode('utf-8')
    except subprocess.CalledProcessError as e:
        return default

def retrieve_pr_info(repo,pull):
    '''
    Retrieve pull request information from github.
    Return None if no title can be found, or an error happens.
    '''
    try:
        req = Request("https://api.github.com/repos/"+repo+"/pulls/"+pull)
        result = urlopen(req)
        reader = codecs.getreader('utf-8')
        obj = json.load(reader(result))
        return obj
    except Exception as e:
        print('Warning: unable to retrieve pull information from github: %s' % e)
        return None

def ask_prompt(text):
    print(text,end=" ",file=stderr)
    stderr.flush()
    reply = stdin.readline().rstrip()
    print("",file=stderr)
    return reply

def parse_arguments():
    epilog = '''
        In addition, you can set the following git configuration variables:
        githubmerge.repository (mandatory),
        user.signingkey (mandatory),
        githubmerge.host (default: git@github.com),
        githubmerge.branch (no default),
        githubmerge.testcmd (default: none).
    '''
    parser = argparse.ArgumentParser(description='Utility to merge, sign and push github pull requests',
            epilog=epilog)
    parser.add_argument('pull', metavar='PULL', type=int, nargs=1,
        help='Pull request ID to merge')
    parser.add_argument('branch', metavar='BRANCH', type=str, nargs='?',
        default=None, help='Branch to merge against (default: githubmerge.branch setting, or base branch for pull, or \'master\')')
    return parser.parse_args()

def main():
    # Extract settings from git repo
    repo = git_config_get('githubmerge.repository')
    host = git_config_get('githubmerge.host','git@github.com')
    opt_branch = git_config_get('githubmerge.branch',None)
    testcmd = git_config_get('githubmerge.testcmd')
    signingkey = git_config_get('user.signingkey')
    if repo is None:
        print("ERROR: No repository configured. Use this command to set:", file=stderr)
        print("git config githubmerge.repository <owner>/<repo>", file=stderr)
        exit(1)
    if signingkey is None:
        print("ERROR: No GPG signing key set. Set one using:",file=stderr)
        print("git config --global user.signingkey <key>",file=stderr)
        exit(1)

    host_repo = host+":"+repo # shortcut for push/pull target

    # Extract settings from command line
    args = parse_arguments()
    pull = str(args.pull[0])

    # Receive pull information from github
    info = retrieve_pr_info(repo,pull)
    if info is None:
        exit(1)
    title = info['title']
    # precedence order for destination branch argument:
    #   - command line argument
    #   - githubmerge.branch setting
    #   - base branch for pull (as retrieved from github)
    #   - 'master'
    branch = args.branch or opt_branch or info['base']['ref'] or 'master'

    # Initialize source branches
    head_branch = 'pull/'+pull+'/head'
    base_branch = 'pull/'+pull+'/base'
    merge_branch = 'pull/'+pull+'/merge'
    local_merge_branch = 'pull/'+pull+'/local-merge'

    devnull = open(os.devnull,'w')
    try:
        subprocess.check_call([GIT,'checkout','-q',branch])
    except subprocess.CalledProcessError as e:
        print("ERROR: Cannot check out branch %s." % (branch), file=stderr)
        exit(3)
    try:
        subprocess.check_call([GIT,'fetch','-q',host_repo,'+refs/pull/'+pull+'/*:refs/heads/pull/'+pull+'/*'])
    except subprocess.CalledProcessError as e:
        print("ERROR: Cannot find pull request #%s on %s." % (pull,host_repo), file=stderr)
        exit(3)
    try:
        subprocess.check_call([GIT,'log','-q','-1','refs/heads/'+head_branch], stdout=devnull, stderr=stdout)
    except subprocess.CalledProcessError as e:
        print("ERROR: Cannot find head of pull request #%s on %s." % (pull,host_repo), file=stderr)
        exit(3)
    try:
        subprocess.check_call([GIT,'log','-q','-1','refs/heads/'+merge_branch], stdout=devnull, stderr=stdout)
    except subprocess.CalledProcessError as e:
        print("ERROR: Cannot find merge of pull request #%s on %s." % (pull,host_repo), file=stderr)
        exit(3)
    try:
        subprocess.check_call([GIT,'fetch','-q',host_repo,'+refs/heads/'+branch+':refs/heads/'+base_branch])
    except subprocess.CalledProcessError as e:
        print("ERROR: Cannot find branch %s on %s." % (branch,host_repo), file=stderr)
        exit(3)
    subprocess.check_call([GIT,'checkout','-q',base_branch])
    subprocess.call([GIT,'branch','-q','-D',local_merge_branch], stderr=devnull)
    subprocess.check_call([GIT,'checkout','-q','-b',local_merge_branch])

    try:
        # Create unsigned merge commit.
        if title:
            firstline = 'Merge #%s: %s' % (pull,title)
        else:
            firstline = 'Merge #%s' % (pull,)
        message = firstline + '\n\n'
        message += subprocess.check_output([GIT,'log','--no-merges','--topo-order','--pretty=format:%h %s (%an)',base_branch+'..'+head_branch]).decode('utf-8')
        try:
            subprocess.check_call([GIT,'merge','-q','--commit','--no-edit','--no-ff','-m',message.encode('utf-8'),head_branch])
        except subprocess.CalledProcessError as e:
            print("ERROR: Cannot be merged cleanly.",file=stderr)
            subprocess.check_call([GIT,'merge','--abort'])
            exit(4)
        logmsg = subprocess.check_output([GIT,'log','--pretty=format:%s','-n','1']).decode('utf-8')
        if logmsg.rstrip() != firstline.rstrip():
            print("ERROR: Creating merge failed (already merged?).",file=stderr)
            exit(4)

        print('%s#%s%s %s %sinto %s%s' % (ATTR_RESET+ATTR_PR,pull,ATTR_RESET,title,ATTR_RESET+ATTR_PR,branch,ATTR_RESET))
        subprocess.check_call([GIT,'log','--graph','--topo-order','--pretty=format:'+COMMIT_FORMAT,base_branch+'..'+head_branch])
        print()
        # Run test command if configured.
        if testcmd:
            # Go up to the repository's root.
            toplevel = subprocess.check_output([GIT,'rev-parse','--show-toplevel']).strip()
            os.chdir(toplevel)
            if subprocess.call(testcmd,shell=True):
                print("ERROR: Running %s failed." % testcmd,file=stderr)
                exit(5)

            # Show the created merge.
            diff = subprocess.check_output([GIT,'diff',merge_branch+'..'+local_merge_branch])
            subprocess.check_call([GIT,'diff',base_branch+'..'+local_merge_branch])
            if diff:
                print("WARNING: merge differs from github!",file=stderr)
                reply = ask_prompt("Type 'ignore' to continue.")
                if reply.lower() == 'ignore':
                    print("Difference with github ignored.",file=stderr)
                else:
                    exit(6)
            reply = ask_prompt("Press 'd' to accept the diff.")
            if reply.lower() == 'd':
                print("Diff accepted.",file=stderr)
            else:
                print("ERROR: Diff rejected.",file=stderr)
                exit(6)
        else:
            # Verify the result manually.
            print("Dropping you on a shell so you can try building/testing the merged source.",file=stderr)
            print("Run 'git diff HEAD~' to show the changes being merged.",file=stderr)
            print("Type 'exit' when done.",file=stderr)
            if os.path.isfile('/etc/debian_version'): # Show pull number on Debian default prompt
                os.putenv('debian_chroot',pull)
            subprocess.call([BASH,'-i'])
            reply = ask_prompt("Type 'm' to accept the merge.")
            if reply.lower() == 'm':
                print("Merge accepted.",file=stderr)
            else:
                print("ERROR: Merge rejected.",file=stderr)
                exit(7)

        # Sign the merge commit.
        reply = ask_prompt("Type 's' to sign off on the merge.")
        if reply == 's':
            try:
                subprocess.check_call([GIT,'commit','-q','--gpg-sign','--amend','--no-edit'])
            except subprocess.CalledProcessError as e:
                print("Error signing, exiting.",file=stderr)
                exit(1)
        else:
            print("Not signing off on merge, exiting.",file=stderr)
            exit(1)

        # Put the result in branch.
        subprocess.check_call([GIT,'checkout','-q',branch])
        subprocess.check_call([GIT,'reset','-q','--hard',local_merge_branch])
    finally:
        # Clean up temporary branches.
        subprocess.call([GIT,'checkout','-q',branch])
        subprocess.call([GIT,'branch','-q','-D',head_branch],stderr=devnull)
        subprocess.call([GIT,'branch','-q','-D',base_branch],stderr=devnull)
        subprocess.call([GIT,'branch','-q','-D',merge_branch],stderr=devnull)
        subprocess.call([GIT,'branch','-q','-D',local_merge_branch],stderr=devnull)

    # Push the result.
    reply = ask_prompt("Type 'push' to push the result to %s, branch %s." % (host_repo,branch))
    if reply.lower() == 'push':
        subprocess.check_call([GIT,'push',host_repo,'refs/heads/'+branch])

if __name__ == '__main__':
    main()


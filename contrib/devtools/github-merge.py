#!/usr/bin/env python3
# Copyright (c) 2016-2017 Bitcoin Core Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# This script will locally construct a merge commit for a pull request on a
# github repository, inspect it, sign it and optionally push it.
# Supports also gitlab API v4 - see git config "githubmerge.gitlabbaseurl".

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
import hashlib
import subprocess
import sys
import json
import codecs
try:
    from urllib.request import Request,urlopen
except:
    from urllib2 import Request,urlopen

try:
    from urllib import quote
except:
    from urllib.parse import quote

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

api_mode_supported = ['github','gitlab'] # first element [0] is the default API mode

def api_mode_print_help():
    '''
    Examples of using supported git-server APIs
    '''
    delim="--------------"
    print("", file=stderr)
    print(delim, file=stderr)
    print("Example configuration", file=stderr)
    print(delim, file=stderr)
    print("To use github, usually you should configure:", file=stderr)
    print("git config githubmerge.apimode github   # default, but better set it to clear --global", file=stderr)
    print("git config githubmerge.repository bitcoin/bitcoin     # <owner>/<repo> ", file=stderr)
    print("git config githubmerge.host git@github.com  # default, but better set it to clear --global", file=stderr)
    print("", file=stderr)
    print("To use gitlab (works with own gitlab server too), usually you should configure:", file=stderr)
    print("git config githubmerge.apimode gitlab", file=stderr)
    print("git config githubmerge.repository bitcoin/bitcoin     # <owner>/<repo>, or e.g. some_ns/you/bitcoin ", file=stderr)
    print("git config githubmerge.host git@gitlab.com", file=stderr)
    print("git config githubmerge.gitlabbaseurl https://gitlab.com/", file=stderr)
    print(delim, file=stderr)


def git_config_get(option, default=None):
    '''
    Get named configuration option from git repository.
    '''
    try:
        return subprocess.check_output([GIT,'config','--get',option]).rstrip().decode('utf-8')
    except subprocess.CalledProcessError:
        return default

def retrieve_pr_info_github(repo,pull,enable_debug):
    '''
    Retrieve pull request information from github.
    Return None if no title can be found, or an error happens.
    '''
    try:
        if enable_debug:
            print("Using github API", file=stderr)
        req = Request("https://api.github.com/repos/"+repo+"/pulls/"+pull)
        result = urlopen(req)
        reader = codecs.getreader('utf-8')
        obj = json.load(reader(result))
        if enable_debug:
            print("Got JSON object", file=stderr)
        return obj
    except Exception as e:
        print('Warning: unable to retrieve pull information from github: %s' % e)
        return None


def retrieve_pr_info_gitlab(repo_raw,pull,baseurl,enable_debug):
    '''
    Retrieve pull request information from gitlab.
    Return None if no title can be found, or an error happens.
    '''
    try:
        if enable_debug:
            print('Using gitlab API, with baseurl=%s' % baseurl, file=stderr)
        # API cmd is eg.: https://gitlab.com/api/v4/projects/orgname%2Fsubname%2Fgproject/merge_requests?iids[]=1
        repo_encoded = quote(repo_raw,"") # namespace/project must encode slashes
        if not baseurl.endswith('/'):
            baseurl = baseurl + '/'
        url = baseurl + "api/v4/projects/" + repo_encoded + "/merge_requests?iids[]=" + pull
        if enable_debug:
            print ("gitlab API command url: " + url, file=stderr)
        req = Request(url)
        result = urlopen(req)
        reader = codecs.getreader('utf-8')
        obj_all = json.load(reader(result))
        for obj in obj_all:
            return obj # returns 1st object, this should be the only iid from iids
    except Exception as e:
        print('Warning: unable to retrieve pull information from gitlab: %s for API command "%s"' % (e,url) )
        return None

    print('Warning: no PR / merge-request data received from server')
    return None

def ask_prompt(text):
    print(text,end=" ",file=stderr)
    stderr.flush()
    reply = stdin.readline().rstrip()
    print("",file=stderr)
    return reply

def get_symlink_files():
    files = sorted(subprocess.check_output([GIT, 'ls-tree', '--full-tree', '-r', 'HEAD']).splitlines())
    ret = []
    for f in files:
        if (int(f.decode('utf-8').split(" ")[0], 8) & 0o170000) == 0o120000:
            ret.append(f.decode('utf-8').split("\t")[1])
    return ret

def tree_sha512sum(commit='HEAD'):
    # request metadata for entire tree, recursively
    files = []
    blob_by_name = {}
    for line in subprocess.check_output([GIT, 'ls-tree', '--full-tree', '-r', commit]).splitlines():
        name_sep = line.index(b'\t')
        metadata = line[:name_sep].split() # perms, 'blob', blobid
        assert(metadata[1] == b'blob')
        name = line[name_sep+1:]
        files.append(name)
        blob_by_name[name] = metadata[2]

    files.sort()
    # open connection to git-cat-file in batch mode to request data for all blobs
    # this is much faster than launching it per file
    p = subprocess.Popen([GIT, 'cat-file', '--batch'], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    overall = hashlib.sha512()
    for f in files:
        blob = blob_by_name[f]
        # request blob
        p.stdin.write(blob + b'\n')
        p.stdin.flush()
        # read header: blob, "blob", size
        reply = p.stdout.readline().split()
        assert(reply[0] == blob and reply[1] == b'blob')
        size = int(reply[2])
        # hash the blob data
        intern = hashlib.sha512()
        ptr = 0
        while ptr < size:
            bs = min(65536, size - ptr)
            piece = p.stdout.read(bs)
            if len(piece) == bs:
                intern.update(piece)
            else:
                raise IOError('Premature EOF reading git cat-file output')
            ptr += bs
        dig = intern.hexdigest()
        assert(p.stdout.read(1) == b'\n') # ignore LF that follows blob data
        # update overall hash with file hash
        overall.update(dig.encode("utf-8"))
        overall.update("  ".encode("utf-8"))
        overall.update(f)
        overall.update("\n".encode("utf-8"))
    p.stdin.close()
    if p.wait():
        raise IOError('Non-zero return value executing git cat-file')
    return overall.hexdigest()

def print_merge_details(pull, title, branch, base_branch, head_branch):
    print('%s#%s%s %s %sinto %s%s' % (ATTR_RESET+ATTR_PR,pull,ATTR_RESET,title,ATTR_RESET+ATTR_PR,branch,ATTR_RESET))
    subprocess.check_call([GIT,'log','--graph','--topo-order','--pretty=format:'+COMMIT_FORMAT,base_branch+'..'+head_branch])

def parse_arguments():
    api_info = 'default: ' + api_mode_supported[0] + ', possible values: ' + ','.join(api_mode_supported)
    epilog = '''
        In addition, you can set the following git configuration variables:
        githubmerge.repository (mandatory),
        user.signingkey (mandatory),
        githubmerge.host (default: git@github.com),
        githubmerge.branch (no default),
        githubmerge.testcmd (default: none),
        githubmerge.gitlabbaseurl (default: none, put here string like 'https://gitlab.com/', or word 'none' to disable it).
        githubmerge.apimode (''' + api_info + ''')
    '''
    parser = argparse.ArgumentParser(description='Utility to merge, sign and push github pull requests',
            epilog=epilog)
    parser.add_argument('pull', metavar='PULL', type=int, nargs=1,
        help='Pull request (merge-request) ID to merge')
    parser.add_argument('branch', metavar='BRANCH', type=str, nargs='?',
        default=None, help='Branch to merge against (default: githubmerge.branch setting, or base branch for pull, or \'master\')')
    parser.add_argument('--debug', dest='enable_debug', action='store_true', help='Use this to enable debug (disabled by default)')
    parser.add_argument('--no-debug', dest='enable_debug', action='store_false', help='Use this to disable debug (disabled by default)')
    parser.set_defaults(enable_debug=False)
    return parser.parse_args()

def main():
    # Extract settings from git repo
    host_default_github='git@github.com'
    repo = git_config_get('githubmerge.repository')
    host = git_config_get('githubmerge.host', host_default_github)
    opt_branch = git_config_get('githubmerge.branch',None)
    testcmd = git_config_get('githubmerge.testcmd')
    signingkey = git_config_get('user.signingkey')

    baseurl_gitlab = git_config_get('githubmerge.gitlabbaseurl')
    if baseurl_gitlab == 'none':
        baseurl_gitlab = None

    if repo is None:
        print("ERROR: No repository configured. Use this command to set:", file=stderr)
        print("git config githubmerge.repository <owner>/<repo>", file=stderr)
        print("if you want to use gitlab instead github, then run this script again after setting option:", file=stderr)
        print("git config githubmerge.apimode gitlab", file=stderr)
        api_mode_print_help()
        sys.exit(1)

    if signingkey is None:
        print("ERROR: No GPG signing key set. Set one using:",file=stderr)
        print("git config --global user.signingkey <key>",file=stderr)
        sys.exit(1)

    api_mode = git_config_get('githubmerge.apimode','github')
    if api_mode not in api_mode_supported:
        print("Unknown API mode was configured (%s)" % api_mode, file=stderr)
        api_mode_print_help()
        sys.exit(1)

    if (baseurl_gitlab is not None) and (api_mode == 'github') :
        print("You configured use of github protocol, but you also configured a gitlab url. Unset that url, or set it to 'none', or change api mode, for example:", file=stderr)
        print("git config githubmerge.gitlabbaseurl none", file=stderr)
        api_mode_print_help()
        sys.exit(1)

    if (api_mode == 'gitlab') and (host == host_default_github) :
        print("You configured use of gitlab protocol (instead github), so you must also configure option githubmerge.host", file=stderr)
        print("to provide the ssh login of your own gitlab (or of the default gitlab)", file=stderr)
        print("for example with following command:", file=stderr)
        print("git config githubmerge.host git@gitlab.com", file=stderr)
        api_mode_print_help()
        sys.exit(1)

    host_repo = host+":"+repo # shortcut for push/pull target

    # Extract settings from command line
    args = parse_arguments()

    if args.enable_debug:
        print("using host_repo='%s' configured with git options (--local or --global) githubmerge.host and githubmerge.repository" % (host_repo), file=stderr)

    pull = str(args.pull[0])

    # Receive pull information from github/gitlab server
    if api_mode == 'github':
        info = retrieve_pr_info_github(repo,pull,args.enable_debug)
    elif api_mode == 'gitlab':
        info = retrieve_pr_info_gitlab(repo,pull,baseurl_gitlab,args.enable_debug)

    if info is None:
        sys.exit(1)

    if api_mode == 'github':
        description = info['body'].strip()
    elif api_mode == 'gitlab':
        description = info['description'].strip()
        info['base'] = dict()
        info['base']['ref'] = info['target_branch'] # target_branch

    title = info['title'].strip()
    if args.enable_debug:
        print("PR/MR title: '%s'" % title, file=stderr)

    # precedence order for destination branch argument:
    #   - command line argument
    #   - githubmerge.branch setting
    #   - base branch for pull (as retrieved from github)
    #   - 'master'
    branch = args.branch or opt_branch or info['base']['ref'] or 'master'
    if args.enable_debug:
        print("branch (target): " + branch, file=stderr)

    # Initialize source branches
    if api_mode == 'github':
        refs_pull_name='pull' # this is how github.com names the refs
    elif api_mode == 'gitlab':
        refs_pull_name='merge-requests' # and this is how gitlab does name refs

    # ref names e.g.: "refs/merge-requests/1/head" on gitlab; "refs/pull/1/head" on github
    head_branch = refs_pull_name + '/' + pull + '/head'
    base_branch = refs_pull_name + '/' + pull + '/base'
    merge_branch = refs_pull_name + '/' + pull + '/merge'
    local_merge_branch = refs_pull_name + '/' + pull + '/local-merge'

    devnull = open(os.devnull, 'w', encoding="utf8")
    try:
        if args.enable_debug:
            print("checkout branch '%s'" % branch, file=stderr)
        subprocess.check_call([GIT,'checkout','-q',branch])
    except subprocess.CalledProcessError:
        print("ERROR: Cannot check out branch %s." % (branch), file=stderr)
        sys.exit(3)

    try:
        refs1='+refs/'+refs_pull_name+'/'+pull+'/*:refs/heads/'+refs_pull_name+'/'+pull+'/*'
        refs2='+refs/heads/'+branch+':refs/heads/'+base_branch
        if args.enable_debug:
            print("from host_repo='%s' fetch refs: '%s' and '%s'" % (host_repo,refs1,refs2), file=stderr)
        subprocess.check_call([GIT,'fetch','-q',host_repo,
            refs1,
            refs2])
    except subprocess.CalledProcessError:
        print("ERROR: Cannot find pull request #%s or branch %s on %s using refs '%s' '%s'." % (pull,branch,host_repo,refs1,refs2), file=stderr)
        sys.exit(3)

    try:
        subprocess.check_call([GIT,'log','-q','-1','refs/heads/'+head_branch], stdout=devnull, stderr=stdout)
    except subprocess.CalledProcessError:
        print("ERROR: Cannot find head of pull request #%s on %s." % (pull,host_repo), file=stderr)
        sys.exit(3)

    if api_mode == "github":
        try:
            if args.enable_debug:
                print("checking whether refs merge exist", file=stderr)
            subprocess.check_call([GIT,'log','-q','-1','refs/heads/'+merge_branch], stdout=devnull, stderr=stdout)
        except subprocess.CalledProcessError:
            print("ERROR: Cannot find merge of pull request #%s on %s." % (pull,host_repo), file=stderr)
            sys.exit(3)
    # gitlab does not (by default at least) create the /merge refs for pulls, so skip this test

    subprocess.check_call([GIT,'checkout','-q',base_branch])
    subprocess.call([GIT,'branch','-q','-D',local_merge_branch], stderr=devnull)
    subprocess.check_call([GIT,'checkout','-q','-b',local_merge_branch])

    try:
        # Go up to the repository's root.
        toplevel = subprocess.check_output([GIT,'rev-parse','--show-toplevel']).strip()
        os.chdir(toplevel)
        # Create unsigned merge commit.
        if title:
            firstline = 'Merge #%s: %s' % (pull,title)
        else:
            firstline = 'Merge #%s' % (pull,)
        message = firstline + '\n\n'
        message += subprocess.check_output([GIT,'log','--no-merges','--topo-order','--pretty=format:%h %s (%an)',base_branch+'..'+head_branch]).decode('utf-8')
        message += '\n\nPull request description:\n\n  ' + description.replace('\n', '\n  ') + '\n'
        try:
            subprocess.check_call([GIT,'merge','-q','--commit','--no-edit','--no-ff','-m',message.encode('utf-8'),head_branch])
        except subprocess.CalledProcessError:
            print("ERROR: Cannot be merged cleanly.",file=stderr)
            subprocess.check_call([GIT,'merge','--abort'])
            sys.exit(4)
        logmsg = subprocess.check_output([GIT,'log','--pretty=format:%s','-n','1']).decode('utf-8')
        if logmsg.rstrip() != firstline.rstrip():
            print("ERROR: Creating merge failed (already merged?).",file=stderr)
            sys.exit(4)

        symlink_files = get_symlink_files()
        for f in symlink_files:
            print("ERROR: File %s was a symlink" % f)
        if len(symlink_files) > 0:
            sys.exit(4)

        # Put tree SHA512 into the message
        try:
            first_sha512 = tree_sha512sum()
            message += '\n\nTree-SHA512: ' + first_sha512
        except subprocess.CalledProcessError:
            print("ERROR: Unable to compute tree hash")
            sys.exit(4)
        try:
            subprocess.check_call([GIT,'commit','--amend','-m',message.encode('utf-8')])
        except subprocess.CalledProcessError:
            print("ERROR: Cannot update message.", file=stderr)
            sys.exit(4)

        print_merge_details(pull, title, branch, base_branch, head_branch)
        print()

        # Run test command if configured.
        if testcmd:
            if subprocess.call(testcmd,shell=True):
                print("ERROR: Running %s failed." % testcmd,file=stderr)
                sys.exit(5)

            # Show the created merge.
            diff = subprocess.check_output([GIT,'diff',merge_branch+'..'+local_merge_branch])
            subprocess.check_call([GIT,'diff',base_branch+'..'+local_merge_branch])
            if diff:
                print("WARNING: merge differs from github!",file=stderr)
                reply = ask_prompt("Type 'ignore' to continue.")
                if reply.lower() == 'ignore':
                    print("Difference with github ignored.",file=stderr)
                else:
                    sys.exit(6)
        else:
            # Verify the result manually.
            print("Dropping you on a shell so you can try building/testing the merged source.",file=stderr)
            print("Run 'git diff HEAD~' to show the changes being merged.",file=stderr)
            print("Type 'exit' when done.",file=stderr)
            if os.path.isfile('/etc/debian_version'): # Show pull number on Debian default prompt
                os.putenv('debian_chroot',pull)
            subprocess.call([BASH,'-i'])

        second_sha512 = tree_sha512sum()
        if first_sha512 != second_sha512:
            print("ERROR: Tree hash changed unexpectedly",file=stderr)
            sys.exit(8)

        # Sign the merge commit.
        print_merge_details(pull, title, branch, base_branch, head_branch)
        while True:
            reply = ask_prompt("Type 's' to sign off on the above merge, or 'x' to reject and exit.").lower()
            if reply == 's':
                try:
                    subprocess.check_call([GIT,'commit','-q','--gpg-sign','--amend','--no-edit'])
                    break
                except subprocess.CalledProcessError:
                    print("Error while signing, asking again.",file=stderr)
            elif reply == 'x':
                print("Not signing off on merge, exiting.",file=stderr)
                sys.exit(1)

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
    while True:
        reply = ask_prompt("Type 'push' to push the result to %s, branch %s, or 'x' to exit without pushing." % (host_repo,branch)).lower()
        if reply == 'push':
            subprocess.check_call([GIT,'push',host_repo,'refs/heads/'+branch])
            break
        elif reply == 'x':
            sys.exit(1)

if __name__ == '__main__':
    main()


#!/usr/bin/python
import json
from urllib import urlopen
import requests
import getpass
from string import Template
import sys
import os
import subprocess

class RunError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

def run(command, **kwargs):
    fail_hard = kwargs.pop("fail_hard", True)
    # output to /dev/null by default:
    kwargs.setdefault("stdout", open('/dev/null', 'w'))
    kwargs.setdefault("stderr", open('/dev/null', 'w'))
    command = Template(command).substitute(os.environ)
    if "TRACE" in os.environ:
        if 'cwd' in kwargs:
            print("[cwd=%s] %s"%(kwargs['cwd'], command))
        else: print(command)
    try:
        process = subprocess.Popen(command.split(' '), **kwargs)
        process.wait()
    except KeyboardInterrupt:
        process.terminate()
        raise
    if process.returncode != 0 and fail_hard:
        raise RunError("Failed: "+command)
    return process.returncode

def checkout_pull(clone_url, commit, out):
    # Init
    build_dir=os.environ["BUILD_DIR"]
    run("umount ${CHROOT_COPY}/proc", fail_hard=False)
    run("rsync --delete -apv ${CHROOT_MASTER}/ ${CHROOT_COPY}")
    run("rm -rf ${CHROOT_COPY}${SCRIPTS_DIR}")
    run("cp -a ${SCRIPTS_DIR} ${CHROOT_COPY}${SCRIPTS_DIR}")
    # Merge onto upstream/master
    run("rm -rf ${BUILD_DIR}")
    run("mkdir -p ${BUILD_DIR}")
    run("git clone ${CLONE_URL} ${BUILD_DIR}")
    run("git remote add pull "+clone_url, cwd=build_dir, stdout=out, stderr=out)
    run("git fetch pull", cwd=build_dir, stdout=out, stderr=out)
    if run("git merge "+ commit, fail_hard=False, cwd=build_dir, stdout=out, stderr=out) != 0:
        return False
    run("chown -R ${BUILD_USER}:${BUILD_GROUP} ${BUILD_DIR}", stdout=out, stderr=out)
    run("mount --bind /proc ${CHROOT_COPY}/proc")
    return True

def commentOn(commentUrl, success, inMerge, needTests, linkUrl):
    common_message = """
This test script verifies pulls every time they are updated. It, however, dies sometimes and fails to test properly.  If you are waiting on a test, please check timestamps to verify that the test.log is moving at http://jenkins.bluematt.me/pull-tester/current/
Contact BlueMatt on freenode if something looks broken."""

    # Remove old BitcoinPullTester comments (I'm being lazy and not paginating here)
    recentcomments = requests.get(commentUrl+"?sort=created&direction=desc",
                                  auth=(os.environ['GITHUB_USER'], os.environ["GITHUB_AUTH_TOKEN"])).json
    for comment in recentcomments:
        if comment["user"]["login"] == os.environ["GITHUB_USER"] and common_message in comment["body"]:
            requests.delete(comment["url"],
                                  auth=(os.environ['GITHUB_USER'], os.environ["GITHUB_AUTH_TOKEN"]))

    if success == True:
        if needTests:
            message = "Automatic sanity-testing: PLEASE ADD TEST-CASES, though technically passed. See " + linkUrl + " for binaries and test log."
        else:
            message = "Automatic sanity-testing: PASSED, see " + linkUrl + " for binaries and test log."

        post_data = { "body" : message + common_message}
    elif inMerge:
        post_data = { "body" : "Automatic sanity-testing: FAILED MERGE, see " + linkUrl + " for test log." + """

This pull does not merge cleanly onto current master""" + common_message}
    else:
        post_data = { "body" : "Automatic sanity-testing: FAILED BUILD/TEST, see " + linkUrl + " for binaries and test log." + """

This could happen for one of several reasons:
1. It chanages changes build scripts in a way that made them incompatible with the automated testing scripts (please tweak those patches in qa/pull-tester)
2. It adds/modifies tests which test network rules (thanks for doing that), which conflicts with a patch applied at test time
3. It does not build on either Linux i386 or Win32 (via MinGW cross compile)
4. The test suite fails on either Linux i386 or Win32
5. The block test-cases failed (lookup the first bNN identifier which failed in https://github.com/TheBlueMatt/test-scripts/blob/master/FullBlockTestGenerator.java)

If you believe this to be in error, please ping BlueMatt on freenode or TheBlueMatt here.
""" + common_message}

    resp = requests.post(commentUrl, json.dumps(post_data), auth=(os.environ['GITHUB_USER'], os.environ["GITHUB_AUTH_TOKEN"]))

def testpull(number, comment_url, clone_url, commit):
    print("Testing pull %d: %s : %s"%(number, clone_url,commit))

    dir = os.environ["RESULTS_DIR"] + "/" + commit + "/"
    print(" ouput to %s"%dir)
    if os.path.exists(dir):
        os.system("rm -r " + dir)
    os.makedirs(dir)
    currentdir = os.environ["RESULTS_DIR"] + "/current"
    os.system("rm -r "+currentdir)
    os.system("ln -s " + dir + " " + currentdir)
    out = open(dir + "test.log", 'w+')

    resultsurl = os.environ["RESULTS_URL"] + commit
    checkedout = checkout_pull(clone_url, commit, out)
    if checkedout != True:
        print("Failed to test pull - sending comment to: " + comment_url)
        commentOn(comment_url, False, True, False, resultsurl)
        open(os.environ["TESTED_DB"], "a").write(commit + "\n")
        return

    run("rm -rf ${CHROOT_COPY}/${OUT_DIR}", fail_hard=False);
    run("mkdir -p ${CHROOT_COPY}/${OUT_DIR}", fail_hard=False);
    run("chown -R ${BUILD_USER}:${BUILD_GROUP} ${CHROOT_COPY}/${OUT_DIR}", fail_hard=False)

    script = os.environ["BUILD_PATH"]+"/qa/pull-tester/pull-tester.sh"
    script += " ${BUILD_PATH} ${MINGW_DEPS_DIR} ${SCRIPTS_DIR}/BitcoindComparisonTool_jar/BitcoindComparisonTool.jar 0 6 ${OUT_DIR}"
    returncode = run("chroot ${CHROOT_COPY} sudo -u ${BUILD_USER} -H timeout ${TEST_TIMEOUT} "+script,
                     fail_hard=False, stdout=out, stderr=out)

    run("mv ${CHROOT_COPY}/${OUT_DIR} " + dir)
    run("mv ${BUILD_DIR} " + dir)

    if returncode == 42:
        print("Successfully tested pull (needs tests) - sending comment to: " + comment_url)
        commentOn(comment_url, True, False, True, resultsurl)
    elif returncode != 0:
        print("Failed to test pull - sending comment to: " + comment_url)
        commentOn(comment_url, False, False, False, resultsurl)
    else:
        print("Successfully tested pull - sending comment to: " + comment_url)
        commentOn(comment_url, True, False, False, resultsurl)
    open(os.environ["TESTED_DB"], "a").write(commit + "\n")

def environ_default(setting, value):
    if not setting in os.environ:
        os.environ[setting] = value

if getpass.getuser() != "root":
	print("Run me as root!")
	sys.exit(1)

if "GITHUB_USER" not in os.environ or "GITHUB_AUTH_TOKEN" not in os.environ:
    print("GITHUB_USER and/or GITHUB_AUTH_TOKEN environment variables not set")
    sys.exit(1)

environ_default("CLONE_URL", "https://github.com/bitcoin/bitcoin.git")
environ_default("MINGW_DEPS_DIR", "/mnt/w32deps")
environ_default("SCRIPTS_DIR", "/mnt/test-scripts")
environ_default("CHROOT_COPY", "/mnt/chroot-tmp")
environ_default("CHROOT_MASTER", "/mnt/chroot")
environ_default("OUT_DIR", "/mnt/out")
environ_default("BUILD_PATH", "/mnt/bitcoin")
os.environ["BUILD_DIR"] = os.environ["CHROOT_COPY"] + os.environ["BUILD_PATH"]
environ_default("RESULTS_DIR", "/mnt/www/pull-tester")
environ_default("RESULTS_URL", "http://jenkins.bluematt.me/pull-tester/")
environ_default("GITHUB_REPO", "bitcoin/bitcoin")
environ_default("TESTED_DB", "/mnt/commits-tested.txt")
environ_default("BUILD_USER", "matt")
environ_default("BUILD_GROUP", "matt")
environ_default("TEST_TIMEOUT", str(60*60*2))

print("Optional usage: pull-tester.py 2112")

f = open(os.environ["TESTED_DB"])
tested = set( line.rstrip() for line in f.readlines() )
f.close()

if len(sys.argv) > 1:
    pull = requests.get("https://api.github.com/repos/"+os.environ["GITHUB_REPO"]+"/pulls/"+sys.argv[1],
                        auth=(os.environ['GITHUB_USER'], os.environ["GITHUB_AUTH_TOKEN"])).json
    testpull(pull["number"], pull["_links"]["comments"]["href"],
             pull["head"]["repo"]["clone_url"], pull["head"]["sha"])

else:
    for page in range(1,100):
        result = requests.get("https://api.github.com/repos/"+os.environ["GITHUB_REPO"]+"/pulls?state=open&page=%d"%(page,),
                              auth=(os.environ['GITHUB_USER'], os.environ["GITHUB_AUTH_TOKEN"])).json
        if len(result) == 0: break;
        for pull in result:
            if pull["head"]["sha"] in tested:
                print("Pull %d already tested"%(pull["number"],))
                continue
            testpull(pull["number"], pull["_links"]["comments"]["href"],
                     pull["head"]["repo"]["clone_url"], pull["head"]["sha"])

BEGIN
GLOW4
!#/User/bin/Bash/
.github/workflows/Rust.yml
Revert "Update and rename .github/workflows/auto-assign.yml to ci :C:… #44
Jobs
Run details
Workflow file for this run
.github/workflows/Rust.yml at 28ba123
#Name :slate.yml
mod.mtml :
On ::starts''
 Check failure on line 3 in .github/workflows/Rust.yml

GitHub Actions
/ .github/workflows/Rust.yml
Invalid workflow file
You have an error in your yaml syntax on line 3
starts'' :GLOW4 ::BEGIN :'::'Run:'#Test'@ci'' :run-on :'Name ::'Run'' :
-on:
  push:
  workflow_dispatch:
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: anishathalye/proof-html@v1.1.0
        with:
          directory: ./
 [![Auto Assign](https://github.com/https-www-bitore-net/demo-repository/actions/workflows/auto-assign.yml/badge.svg?branch=Main.yml)](https://github.com/https-www-bitore-net/demo-repository/actions/workflows/auto-assign.yml)
Search logs
1s
##[debug]Starting: Set up job
Current runner version: '2.301.1'
Operating System
Runner Image
Runner Image Provisioner
GITHUB_TOKEN Permissions
Secret source: Actions
##[debug]Primary repository: https-www-bitore-net/demo-repository
Prepare workflow directory
##[debug]Creating pipeline directory: '/home/runner/work/demo-repository'
##[debug]Creating workspace directory: '/home/runner/work/demo-repository/demo-repository'
##[debug]Update context data
##[debug]Evaluating job-level environment variables
##[debug]Evaluating job container
##[debug]Evaluating job service containers
##[debug]Evaluating job defaults
Prepare all required actions
Getting action download info
Download action repository 'wow-actions/auto-assign@v1' (SHA:707ec486377e28a595619469e26b375d8746b330)
##[debug]Download 'https://api.github.com/repos/wow-actions/auto-assign/tarball/707ec486377e28a595619469e26b375d8746b330' to '/home/runner/work/_actions/_temp_b511ebcf-63c1-4305-8674-0253f9fbcf13/01abee82-ec57-4330-9447-86d5db93791e.tar.gz'
##[debug]Unwrap 'wow-actions-auto-assign-707ec48' to '/home/runner/work/_actions/wow-actions/auto-assign/v1'
##[debug]Archive '/home/runner/work/_actions/_temp_b511ebcf-63c1-4305-8674-0253f9fbcf13/01abee82-ec57-4330-9447-86d5db93791e.tar.gz' has been unzipped into '/home/runner/work/_actions/wow-actions/auto-assign/v1'.
##[debug]action.yml for action: '/home/runner/work/_actions/wow-actions/auto-assign/v1/action.yml'.
##[debug]Set step '__wow-actions_auto-assign' display name to: 'Run wow-actions/auto-assign@v1'
Complete job name: run
##[debug]Collect running processes for tracking orphan processes.
##[debug]Finishing: Set up job
1s
##[debug]Evaluating condition for step: 'Run wow-actions/auto-assign@v1'
##[debug]Evaluating: success()
##[debug]Evaluating success:
##[debug]=> true
##[debug]Result: true
##[debug]Starting: Run wow-actions/auto-assign@v1
##[debug]Loading inputs
##[debug]Evaluating: secrets.GITHUB_TOKEN
##[debug]Evaluating Index:
##[debug]..Evaluating secrets:
##[debug]..=> Object
##[debug]..Evaluating String:
##[debug]..=> 'GITHUB_TOKEN'
##[debug]=> '***'
##[debug]Result: '***'
##[debug]Loading env
Run wow-actions/auto-assign@v1
##[debug]event: issues
##[debug]action: opened
(node:1620) [DEP0005] DeprecationWarning: Buffer() is deprecated due to security and usability issues. Please use the Buffer.alloc(), Buffer.allocUnsafe(), or Buffer.from() methods instead.
(Use `node --trace-deprecation ...` to show where the warning was created)
##[debug]Load config from ".github/auto-assign.yml": 
##[debug]{
##[debug]  "addReviewers": true,
##[debug]  "addAssignees": "author",
##[debug]  "numberOfAssignees": 0,
##[debug]  "numberOfReviewers": 1,
##[debug]  "reviewers": [
##[debug]    "mowjoejoejoejoe"
##[debug]  ],
##[debug]  "assignees": [
##[debug]    "mowjoejoejoejoe"
##[debug]  ],
##[debug]  "skipKeywords": [
##[debug]    "wip"
##[debug]  ]
##[debug]}
##[debug]Assignees: [
##[debug]  "mowjoejoejoejoe"
##[debug]]
Error: HttpError: Resource not accessible by integration
Error: Resource not accessible by integration
##[debug]Node Action run completed with exit code 1
##[debug]Finishing: Run wow-actions/auto-assign@v1
.github/workflows/proof-html.yml → .github/workflows/slate.yml
@@ -1,5 +1,8 @@
name: Proof HTML
on:
#Name :slate.yml
mod.mtml :
On ::starts''
starts'' :GLOW4 ::BEGIN :'::'Run:'#Test'@ci'' :run-on :'Name ::'Run'' :
-on:
  push:
  workflow_dispatch:
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: anishathalye/proof-html@v1.1.0
        with:
          directory: ./
# See test/eslintrc.search for usage.
# {{{{'$'' '{[(((c)(r))'.[12753750'.[00]m]BITORE_34173'.1337)'' ')]'' '}}}'"'':
# This container basically has to live in this directory in order to pull in the CI
# install scripts. If it lived in the root directory, it would have to pull in the
# entire repo as docker context during build; if it lived elsewhere, it wouldn't be
# able to make back-references to pull in the install scripts. So here it lives.

FROM python:3.7-buster

ENV DEBIAN_FRONTEND=noninteractive
ENV LC_ALL=C.UTF-8

# This is used by the 04_install.sh script; we can't read the Python version from
# .python-version for the same reasons as above, and it's more efficient to pull a
# preexisting Python image than it is to build from source.
ENV SKIP_PYTHON_INSTALL=1

# Must be built from ./ci/lint/ for these paths to work.
COPY ./docker-entrypoint.sh /entrypoint.sh
COPY ./04_install.sh /install.sh

RUN /install.sh && \
  echo 'alias lint="./ci/lint/06_script.sh"' >> ~/.bashrc && \
  chmod 755 /entrypoint.sh && \
  rm -rf /var/lib/apt/lists/*


WORKDIR /bitcoin
ENTRYPOINT ["/entrypoint.sh"]
Skip to content
Search or jump to…
Pull requests
Issues
Codespaces
Marketplace
Explore
 
@mowjoejoejoejoe 
Your account has been flagged.
Because of that, your profile is hidden from the public. If you believe this is a mistake, contact support to have your account status reviewed.
https-www-bitore-net
/
demo-repository
Private
Fork your own copy of https-www-bitore-net/demo-repository
Code
Issues
1
Pull requests
4
Actions
Projects
Security
Insights
Settings
.github/workflows/Rust.yml
Revert "Update and rename .github/workflows/auto-assign.yml to ci :C:… #44
Jobs
Run details
Workflow file for this run
.github/workflows/Rust.yml at 28ba123
#Name :slate.yml
mod.mtml :
On ::starts''
 Check failure on line 3 in .github/workflows/Rust.yml

GitHub Actions
/ .github/workflows/Rust.yml
Invalid workflow file
You have an error in your yaml syntax on line 3
starts'' :GLOW4 ::BEGIN :'::'Run:'#Test'@ci'' :run-on :'Name ::'Run'' :
-on:
  push:
  workflow_dispatch:
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: anishathalye/proof-html@v1.1.0
        with:
          directory: ./
 [![Auto Assign](https://github.com/https-www-bitore-net/demo-repository/actions/workflows/auto-assign.yml/badge.svg?branch=Main.yml)](https://github.com/https-www-bitore-net/demo-repository/actions/workflows/auto-assign.yml)
Search logs
1s
##[debug]Starting: Set up job
Current runner version: '2.301.1'
Operating System
Runner Image
Runner Image Provisioner
GITHUB_TOKEN Permissions
Secret source: Actions
##[debug]Primary repository: https-www-bitore-net/demo-repository
Prepare workflow directory
##[debug]Creating pipeline directory: '/home/runner/work/demo-repository'
##[debug]Creating workspace directory: '/home/runner/work/demo-repository/demo-repository'
##[debug]Update context data
##[debug]Evaluating job-level environment variables
##[debug]Evaluating job container
##[debug]Evaluating job service containers
##[debug]Evaluating job defaults
Prepare all required actions
Getting action download info
Download action repository 'wow-actions/auto-assign@v1' (SHA:707ec486377e28a595619469e26b375d8746b330)
##[debug]Download 'https://api.github.com/repos/wow-actions/auto-assign/tarball/707ec486377e28a595619469e26b375d8746b330' to '/home/runner/work/_actions/_temp_b511ebcf-63c1-4305-8674-0253f9fbcf13/01abee82-ec57-4330-9447-86d5db93791e.tar.gz'
##[debug]Unwrap 'wow-actions-auto-assign-707ec48' to '/home/runner/work/_actions/wow-actions/auto-assign/v1'
##[debug]Archive '/home/runner/work/_actions/_temp_b511ebcf-63c1-4305-8674-0253f9fbcf13/01abee82-ec57-4330-9447-86d5db93791e.tar.gz' has been unzipped into '/home/runner/work/_actions/wow-actions/auto-assign/v1'.
##[debug]action.yml for action: '/home/runner/work/_actions/wow-actions/auto-assign/v1/action.yml'.
##[debug]Set step '__wow-actions_auto-assign' display name to: 'Run wow-actions/auto-assign@v1'
Complete job name: run
##[debug]Collect running processes for tracking orphan processes.
##[debug]Finishing: Set up job
1s
##[debug]Evaluating condition for step: 'Run wow-actions/auto-assign@v1'
##[debug]Evaluating: success()
##[debug]Evaluating success:
##[debug]=> true
##[debug]Result: true
##[debug]Starting: Run wow-actions/auto-assign@v1
##[debug]Loading inputs
##[debug]Evaluating: secrets.GITHUB_TOKEN
##[debug]Evaluating Index:
##[debug]..Evaluating secrets:
##[debug]..=> Object
##[debug]..Evaluating String:
##[debug]..=> 'GITHUB_TOKEN'
##[debug]=> '***'
##[debug]Result: '***'
##[debug]Loading env
Run wow-actions/auto-assign@v1
##[debug]event: issues
##[debug]action: opened
(node:1620) [DEP0005] DeprecationWarning: Buffer() is deprecated due to security and usability issues. Please use the Buffer.alloc(), Buffer.allocUnsafe(), or Buffer.from() methods instead.
(Use `node --trace-deprecation ...` to show where the warning was created)
##[debug]Load config from ".github/auto-assign.yml": 
##[debug]{
##[debug]  "addReviewers": true,
##[debug]  "addAssignees": "author",
##[debug]  "numberOfAssignees": 0,
##[debug]  "numberOfReviewers": 1,
##[debug]  "reviewers": [
##[debug]    "mowjoejoejoejoe"
##[debug]  ],
##[debug]  "assignees": [
##[debug]    "mowjoejoejoejoe"
##[debug]  ],
##[debug]  "skipKeywords": [
##[debug]    "wip"
##[debug]  ]
##[debug]}
##[debug]Assignees: [
##[debug]  "mowjoejoejoejoe"
##[debug]]
Error: HttpError: Resource not accessible by integration
Error: Resource not accessible by integration
##[debug]Node Action run completed with exit code 1
##[debug]Finishing: Run wow-actions/auto-assign@v1
.github/workflows/proof-html.yml → .github/workflows/slate.yml
@@ -1,5 +1,8 @@
name: Proof HTML
on:
#Name :slate.yml
mod.mtml :
On ::starts''
starts'' :GLOW4 ::BEGIN :'::'Run:'#Test'@ci'' :run-on :'Name ::'Run'' :
-on:
  push:
  workflow_dispatch:
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: anishathalye/proof-html@v1.1.0
        with:
          directory: ./

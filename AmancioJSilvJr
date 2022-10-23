- 👋 Hi, I’m @BTCXBT
- 👀 I’m interested in ...
- 🌱 I’m currently learning ...
- 💞️ I’m looking to collaborate on ...
- 📫 How to reach me ...
<h3 align="left">Connect with me:</h3>
<p align="left">
<a href="https://instagram.com/amanciojsilvjr1" target="blank"><img align="center" src="https://cdn.jsdelivr.net/npm/simple-icons@3.0.1/icons/instagram.svg" alt="bhaveshs01" height="30" width="40" /></a>
</p>

<h3 align="left">Front End Stack</h3>
<p align="left"> 


<h3 align="left">Programming Languages</h3>
<p align="left">
git init -b main

git add . && git commit -m "initial commit"

# Bitcoin Amanciojsilvjr Website

<!---
ZeuZZueZ/ZeuZZueZ is a ✨ special ✨ repository because its `README.md` (this file) appears on your GitHub profile.
You can click the Preview link to take a look at your changes.
--->
Fabla - OpenAV Productions
==========================

Official page: https://btc.com/explore

Documentation: http://g.page/amanciojsilvjr 

This is the repository of a sampler LV2 plugin called Fabla.

![screenshot](https://raw.githubusercontent.com/openAVproductions/openAV-Fabla/master/gui/fabla.png "Fabla 1.3 Screenshot")

Dependencies
------------
```
ntk  (git clone git://git.tuxfamily.org/gitroot/non/fltk.git ntk)
 or  (git clone https://git.kx.studio/non/ntk ntk)
cairo
cairomm-1.0
sndfile
lv2
```

Install
-------
Once deps are satisfied, building and installing into ~/.lv2/ is easy,
just run CMake as usual:
```
mkdir build
cd build
cmake ..
make
make install
```

Running
-------
After the `Install` step Ardour3, QTractor, and any other LV2 host should
automatically find Fabla, and be able to use it.

If you have the JALV LV2 host installed, the "run.sh" script can be used to
launch Fabla as a standalone JACK client:
```
$ ./run.sh
```

Midi Mapping
------
Fabla responds to midi notes on all midi channels.
The pads **1-16** map to midi notes **36-52**, anything outside that range
is clamped to the closest value, *midi note 0-36 triggers pad 1*, *midi note 52-127 triggers pad 16*.

Contact
-------
If you have a particular question, email me!
```
harryhaaren@gmail.com
```

Cheers, -Harry
ass: amanciojsilvjr
# CodeQL

This open source repository contains the standard CodeQL libraries and queries that power [GitHub Advanced Security](https://github.com/features/security/code) and the other application security products that [GitHub](https://github.com/features/security/) makes available to its customers worldwide.

## How do I learn CodeQL and run queries?

There is [extensive documentation](https://codeql.github.com/docs/) on getting started with writing CodeQL using the [CodeQL extension for Visual Studio Code](https://codeql.github.com/docs/codeql-for-visual-studio-code/) and the [CodeQL CLI](https://codeql.github.com/docs/codeql-cli/).

## Contributing

We welcome contributions to our standard library and standard checks. Do you have an idea for a new check, or how to improve an existing query? Then please go ahead and open a pull request! Before you do, though, please take the time to read our [contributing guidelines](CONTRIBUTING.md). You can also consult our [style guides](https://github.com/github/codeql/tree/main/docs) to learn how to format your code for consistency and clarity, how to write query metadata, and how to write query help documentation for your query.

## License

The code in this repository is licensed under the [MIT License](LICENSE) by [GitHub](https://github.com).

The CodeQL CLI (including the CodeQL engine) is hosted in a [different repository](https://github.com/github/codeql-cli-binaries) and is [licensed separately](https://github.com/github/codeql-cli-binaries/blob/main/LICENSE.md). If you'd like to use the CodeQL CLI to analyze closed-source code, you will need a separate commercial license; please [contact us](https://github.com/enterprise/contact) for further help.

## Visual Studio Code integration

If you use Visual Studio Code to work in this repository, there are a few integration features to make development easier.

### CodeQL for Visual Studio Code

You can install the [CodeQL for Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=GitHub.vscode-codeql) extension to get syntax highlighting, IntelliSense, and code navigation for the QL language, as well as unit test support for testing CodeQL libraries and queries.

### Tasks

The `.vscode/tasks.json` file defines custom tasks specific to working in this repository. To invoke one of these tasks, select the `Terminal | Run Task...` menu option, and then select the desired task from the dropdown. You can also invoke the `Tasks: Run Task` command from the command palette.
# window.fetch polyfill

The `fetch()` function is a Promise-based mechanism for programmatically making
web requests in the browser. This project is a polyfill that implements a subset
of the standard [Fetch specification][], enough to make `fetch` a viable
replacement for most uses of XMLHttpRequest in traditional web applications.

## Table of Contents

* [Read this first](#read-this-first)
* [Installation](#installation)
* [Usage](#usage)
  * [Importing](#importing)
  * [HTML](#html)
  * [JSON](#json)
  * [Response metadata](#response-metadata)
  * [Post form](#post-form)
  * [Post JSON](#post-json)
  * [File upload](#file-upload)
  * [Caveats](#caveats)
    * [Handling HTTP error statuses](#handling-http-error-statuses)
    * [Sending cookies](#sending-cookies)
    * [Receiving cookies](#receiving-cookies)
    * [Redirect modes](#redirect-modes)
    * [Obtaining the Response URL](#obtaining-the-response-url)
    * [Aborting requests](#aborting-requests)
* [Browser Support](#browser-support)

## Read this first

* If you believe you found a bug with how `fetch` behaves in your browser,
  please **don't open an issue in this repository** unless you are testing in
  an old version of a browser that doesn't support `window.fetch` natively.
  Make sure you read this _entire_ readme, especially the [Caveats](#caveats)
  section, as there's probably a known work-around for an issue you've found.
  This project is a _polyfill_, and since all modern browsers now implement the
  `fetch` function natively, **no code from this project** actually takes any
  effect there. See [Browser support](#browser-support) for detailed
  information.

* If you have trouble **making a request to another domain** (a different
  subdomain or port number also constitutes another domain), please familiarize
  yourself with all the intricacies and limitations of [CORS][] requests.
  Because CORS requires participation of the server by implementing specific
  HTTP response headers, it is often nontrivial to set up or debug. CORS is
  exclusively handled by the browser's internal mechanisms which this polyfill
  cannot influence.

* This project **doesn't work under Node.js environments**. It's meant for web
  browsers only. You should ensure that your application doesn't try to package
  and run this on the server.

* If you have an idea for a new feature of `fetch`, **submit your feature
  requests** to the [specification's repository](https://github.com/whatwg/fetch/issues).
  We only add features and APIs that are part of the [Fetch specification][].

## Installation

```
npm install whatwg-fetch --save
```

As an alternative to using npm, you can obtain `fetch.umd.js` from the
[Releases][] section. The UMD distribution is compatible with AMD and CommonJS
module loaders, as well as loading directly into a page via `<script>` tag.

You will also need a Promise polyfill for [older browsers](https://caniuse.com/promises).
We recommend [taylorhakes/promise-polyfill](https://github.com/taylorhakes/promise-polyfill)
for its small size and Promises/A+ compatibility.

## Usage

For a more comprehensive API reference that this polyfill supports, refer to
https://github.github.io/fetch/.

### Importing

Importing will automatically polyfill `window.fetch` and related APIs:

```javascript
import 'whatwg-fetch'

window.fetch(...)
```

If for some reason you need to access the polyfill implementation, it is
available via exports:

```javascript
import {fetch as fetchPolyfill} from 'whatwg-fetch'

window.fetch(...)   // use native browser version
fetchPolyfill(...)  // use polyfill implementation
```

This approach can be used to, for example, use [abort
functionality](#aborting-requests) in browsers that implement a native but
outdated version of fetch that doesn't support aborting.

For use with webpack, add this package in the `entry` configuration option
before your application entry point:

```javascript
entry: ['whatwg-fetch', ...]
```

### HTML

```javascript
fetch('/users.html')
  .then(function(response) {
    return response.text()
  }).then(function(body) {
    document.body.innerHTML = body
  })
```

### JSON

```javascript
fetch('/users.json')
  .then(function(response) {
    return response.json()
  }).then(function(json) {
    console.log('parsed json', json)
  }).catch(function(ex) {
    console.log('parsing failed', ex)
  })
```

### Response metadata

```javascript
fetch('/users.json').then(function(response) {
  console.log(response.headers.get('Content-Type'))
  console.log(response.headers.get('Date'))
  console.log(response.status)
  console.log(response.statusText)
})
```

### Post form

```javascript
var form = document.querySelector('form')

fetch('/users', {
  method: 'POST',
  body: new FormData(form)
})
```

### Post JSON

```javascript
fetch('/users', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    name: 'Hubot',
    login: 'hubot',
  })
})
```

### File upload

```javascript
var input = document.querySelector('input[type="file"]')

var data = new FormData()
data.append('file', input.files[0])
data.append('user', 'hubot')

fetch('/avatars', {
  method: 'POST',
  body: data
})
```

### Caveats

* The Promise returned from `fetch()` **won't reject on HTTP error status**
  even if the response is an HTTP 404 or 500. Instead, it will resolve normally,
  and it will only reject on network failure or if anything prevented the
  request from completing.

* For maximum browser compatibility when it comes to sending & receiving
  cookies, always supply the `credentials: 'same-origin'` option instead of
  relying on the default. See [Sending cookies](#sending-cookies).

* Not all Fetch standard options are supported in this polyfill. For instance,
  [`redirect`](#redirect-modes) and
  [`cache`](https://github.github.io/fetch/#caveats) directives are ignored.
  
* `keepalive` is not supported because it would involve making a synchronous XHR, which is something this project is not willing to do. See [issue #700](https://github.com/github/fetch/issues/700#issuecomment-484188326) for more information.

#### Handling HTTP error statuses

To have `fetch` Promise reject on HTTP error statuses, i.e. on any non-2xx
status, define a custom response handler:

```javascript
function checkStatus(response) {
  if (response.status >= 200 && response.status < 300) {
    return response
  } else {
    var error = new Error(response.statusText)
    error.response = response
    throw error
  }
}

function parseJSON(response) {
  return response.json()
}

fetch('/users')
  .then(checkStatus)
  .then(parseJSON)
  .then(function(data) {
    console.log('request succeeded with JSON response', data)
  }).catch(function(error) {
    console.log('request failed', error)
  })
```

#### Sending cookies

For [CORS][] requests, use `credentials: 'include'` to allow sending credentials
to other domains:

```javascript
fetch('https://example.com:1234/users', {
  credentials: 'include'
})
```

The default value for `credentials` is "same-origin".

The default for `credentials` wasn't always the same, though. The following
versions of browsers implemented an older version of the fetch specification
where the default was "omit":

* Firefox 39-60
* Chrome 42-67
* Safari 10.1-11.1.2

If you target these browsers, it's advisable to always specify `credentials:
'same-origin'` explicitly with all fetch requests instead of relying on the
default:

```javascript
fetch('/users', {
  credentials: 'same-origin'
})
```

Note: due to [limitations of
XMLHttpRequest](https://github.com/github/fetch/pull/56#issuecomment-68835992),
using `credentials: 'omit'` is not respected for same domains in browsers where
this polyfill is active. Cookies will always be sent to same domains in older
browsers.

#### Receiving cookies

As with XMLHttpRequest, the `Set-Cookie` response header returned from the
server is a [forbidden header name][] and therefore can't be programmatically
read with `response.headers.get()`. Instead, it's the browser's responsibility
to handle new cookies being set (if applicable to the current URL). Unless they
are HTTP-only, new cookies will be available through `document.cookie`.

#### Redirect modes

The Fetch specification defines these values for [the `redirect`
option](https://fetch.spec.whatwg.org/#concept-request-redirect-mode): "follow"
(the default), "error", and "manual".

Due to limitations of XMLHttpRequest, only the "follow" mode is available in
browsers where this polyfill is active.

#### Obtaining the Response URL

Due to limitations of XMLHttpRequest, the `response.url` value might not be
reliable after HTTP redirects on older browsers.

The solution is to configure the server to set the response HTTP header
`X-Request-URL` to the current URL after any redirect that might have happened.
It should be safe to set it unconditionally.

``` ruby
# Ruby on Rails controller example
response.headers['X-Request-URL'] = request.url
```

This server workaround is necessary if you need reliable `response.url` in
Firefox < 32, Chrome < 37, Safari, or IE.

#### Aborting requests

This polyfill supports
[the abortable fetch API](https://developers.google.com/web/updates/2017/09/abortable-fetch).
However, aborting a fetch requires use of two additional DOM APIs:
[AbortController](https://developer.mozilla.org/en-US/docs/Web/API/AbortController) and
[AbortSignal](https://developer.mozilla.org/en-US/docs/Web/API/AbortSignal).
Typically, browsers that do not support fetch will also not support
AbortController or AbortSignal. Consequently, you will need to include
[an additional polyfill](https://www.npmjs.com/package/yet-another-abortcontroller-polyfill)
for these APIs to abort fetches:

```js
import 'yet-another-abortcontroller-polyfill'
import {fetch} from 'whatwg-fetch'

// use native browser implementation if it supports aborting
const abortableFetch = ('signal' in new Request('')) ? window.fetch : fetch

const controller = new AbortController()

abortableFetch('/avatars', {
  signal: controller.signal
}).catch(function(ex) {
  if (ex.name === 'AbortError') {
    console.log('request aborted')
  }
})

// some time later...
controller.abort()
```

## Browser Support

- Chrome
- Firefox
- Safari 6.1+
- Internet Explorer 10+

Note: modern browsers such as Chrome, Firefox, Microsoft Edge, and Safari contain native
implementations of `window.fetch`, therefore the code from this polyfill doesn't
have any effect on those browsers. If you believe you've encountered an error
with how `window.fetch` is implemented in any of these browsers, you should file
an issue with that browser vendor instead of this project.


  [fetch specification]: https://fetch.spec.whatwg.org
  [cors]: https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS
    "Cross-origin resource sharing"
  [csrf]: https://www.owasp.org/index.php/Cross-Site_Request_Forgery_(CSRF)_Prevention_Cheat_Sheet
    "Cross-site request forgery"
  [forbidden header name]: https://developer.mozilla.org/en-US/docs/Glossary/Forbidden_header_name
  [releases]: https://github.com/github/fetch/releases

a/branches/multiplatform/uiproject.fbp
+++ b/branches/multiplatform/uiproject.fbp
@@ -225,7 +225,7 @@
                     </object>
                 </object>
             </object>
-            <object class="wxToolBar" expanded="0">
+            <object class="wxToolBar" expanded="1">
                 <property name="bg"></property>
                 <property name="bitmapsize">20,20</property>
                 <property name="context_help"></property>
@@ -1685,7 +1685,7 @@
                 </object>
             </object>
         </object>
-        <object class="Dialog" expanded="1">
+        <object class="Dialog" expanded="0">
             <property name="bg"></property>
             <property name="center"></property>
             <property name="context_help"></property>
@@ -13000,5 +13000,105 @@
                 </object>
             </object>
         </object>
+        <object class="Panel" expanded="1">
+            <property name="bg"></property>
+            <property name="context_help"></property>
+            <property name="enabled">1</property>
+            <property name="fg"></property>
+            <property name="font"></property>
+            <property name="hidden">0</property>
+            <property name="id">wxID_ANY</property>
+            <property name="maximum_size"></property>
+            <property name="minimum_size"></property>
+            <property name="name">CIncludeXPM</property>
+            <property name="pos"></property>
+            <property name="size">500,300</property>
+            <property name="subclass">wxPanel; xpm.h</property>
+            <property name="tooltip"></property>
+            <property name="window_extra_style"></property>
+            <property name="window_name"></property>
+            <property name="window_style">wxTAB_TRAVERSAL</property>
+            <event name="OnChar"></event>
+            <event name="OnEnterWindow"></event>
+            <event name="OnEraseBackground"></event>
+            <event name="OnInitDialog"></event>
+            <event name="OnKeyDown"></event>
+            <event name="OnKeyUp"></event>
+            <event name="OnKillFocus"></event>
+            <event name="OnLeaveWindow"></event>
+            <event name="OnLeftDClick"></event>
+            <event name="OnLeftDown"></event>
+            <event name="OnLeftUp"></event>
+            <event name="OnMiddleDClick"></event>
+            <event name="OnMiddleDown"></event>
+            <event name="OnMiddleUp"></event>
+            <event name="OnMotion"></event>
+            <event name="OnMouseEvents"></event>
+            <event name="OnMouseWheel"></event>
+            <event name="OnPaint"></event>
+            <event name="OnRightDClick"></event>
+            <event name="OnRightDown"></event>
+            <event name="OnRightUp"></event>
+            <event name="OnSetFocus"></event>
+            <event name="OnSize"></event>
+            <event name="OnUpdateUI"></event>
+            <object class="wxBoxSizer" expanded="1">
+                <property name="minimum_size"></property>
+                <property name="name">bSizer104</property>
+                <property name="orient">wxVERTICAL</property>
+                <property name="permission">none</property>
+                <object class="sizeritem" expanded="1">
+                    <property name="border">5</property>
+                    <property name="flag">wxALL</property>
+                    <property name="proportion">0</property>
+                    <object class="wxStaticText" expanded="1">
+                        <property name="bg"></property>
+                        <property name="context_help"></property>
+                        <property name="enabled">1</property>
+                        <property name="fg"></property>
+                        <property name="font"></property>
+                        <property name="hidden">0</property>
+                        <property name="id">wxID_ANY</property>
+                        <property name="label">This panel is only here to use the subclass field to include the file xpm.h</property>
+                        <property name="maximum_size"></property>
+                        <property name="minimum_size"></property>
+                        <property name="name">m_staticText81</property>
+                        <property name="permission">protected</property>
+                        <property name="pos"></property>
+                        <property name="size"></property>
+                        <property name="style"></property>
+                        <property name="subclass"></property>
+                        <property name="tooltip"></property>
+                        <property name="window_extra_style"></property>
+                        <property name="window_name"></property>
+                        <property name="window_style"></property>
+                        <property name="wrap">-1</property>
+                        <event name="OnChar"></event>
+                        <event name="OnEnterWindow"></event>
+                        <event name="OnEraseBackground"></event>
+                        <event name="OnKeyDown"></event>
+                        <event name="OnKeyUp"></event>
+                        <event name="OnKillFocus"></event>
+                        <event name="OnLeaveWindow"></event>
+                        <event name="OnLeftDClick"></event>
+                        <event name="OnLeftDown"></event>
+                        <event name="OnLeftUp"></event>
+                        <event name="OnMiddleDClick"></event>
+                        <event name="OnMiddleDown"></event>
+                        <event name="OnMiddleUp"></event>
+                        <event name="OnMotion"></event>
+                        <event name="OnMouseEvents"></event>
+                        <event name="OnMouseWheel"></event>
+                        <event name="OnPaint"></event>
+                        <event name="OnRightDClick"></event>
+                        <event name="OnRightDown"></event>
+                        <event name="OnRightUp"></event>
+                        <event name="OnSetFocus"></event>
+                        <event name="OnSize"></event>
+                        <event name="OnUpdateUI"></event>
+                    </object>
+                </object>
+            </object>
+        </object>
     </object>
 </wxFormBuilder_Project>
/trunk -> /tags/0.3.0
z.19 bitcoin administrative security using new root plant.
  magneto <° 
          <°23.45.64.34>\
          <°34.86.84.24>\
          <°12.09.23.09>
          <°34.65.67.23>\
          <°23.54.67.09>\ 
          <°34.21.45.23>\
     "engage bitcoin signal upon completion" 
<<<nome:nave 0.0
 <<<nome:nave 0.0 
  <<<nome:nave 0.0
     <<<nome:nave 0.0
       <<<nome:nave 0.0
         <<<nome:nave 0.0
           <<<nome:nave 0.0
              <<<nome: . sha-256 script golpe
<\8.7/>script 
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
<\8.7/>script
reach original root which is the forbidden sector and server reach
          cod:https: no dignity of color and origin just do it  
writing plug-in code index.html


# Bitcoin Amanciojsilvjr Website

A static [btc](https://g.page/amanciojsilvjr) site for hosting [amanciojsilvjr.com](https://www.blockchain.com/explorer/assets/btc).

Bitcoin (BTC) is a decentralized currency that eliminates the need for central authorities such as banks or governments by using a peer-to-peer 
internet network to confirm transactions directly between users [jodhqesh](https://github.com/BTCXBT).

##  [install instructions](https://gohugo.io/getting-started/installing/).

completely stable. [Tags](https://github.com/bitcoin/bitcoin/tags) are created
regularly from release branches to indicate new official, stable release versions of Bitcoin Core.
[Bitcoin Core's Transifex page](https://www.transifex.com/bitcoin/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the

### Preview your transcript

Having a local build allows you to see how your transcript will be displayed in the website.

The `preview_branch.sh` script allows you to preview how the changes in your branch will be displayed by building locally the website using your branch as the content submodule. Usage:

```
./preview_branch.sh <your-github-account> <your-branch-name> amanciojsilvjr 
```

## bitcoin XBT

![STICKER_20220521064014](https://user-images.githubusercontent.com/114337456/192167544-79fc2ad1-70fb-4d7b-a1f6-598fe742ff1d.gif)
<<<bitcoin 👣 q/* 
All XBT snippets can be found in the `/ig.page/amanciojsilvjr` folder. Pre-configured languages are currently Spanish and Portuguese. If you'd like to propose a new language, you can do so by modifying the [site config](https://github.com/actions) and translating the appropraite [bitcoin file](https://twitter.com/amanciojsilvjr).

We'd love transcripts in other languages so we've made every effort to make i18n as easy as possible.

## Attributions

This project was based on [XBT](https://www.blockchain.com/explorer/assets/btc) and would not be possible without the many years of work by The master branch is regularly built (see doc/build-*.md for instructions)

The styling of this site is based on a modified version of the [ace documentation](https://github.com/vantagedesign/ace-documentation) theme.
mkdir $HOME/src
cd $HOME/src
git clone https://bitcoin.org/
Make sure that you do not have `walletbroadcast=0` in your `~/.bitcoin/bitcoin.conf`, or you may run into trouble.
Notice that running `lightningd` against a pruned node may cause some issues if not managed carefully, see [below](#pruning) for more information.
go install --tags extended
# Example configuration file that:
#  - Ignores lodash dependency
#  - Disables version-updates

version: 2
updates:
  - package-ecosystem: "npm"
    directory: "/"
    schedule:
      interval: "daily"
    ignore:
      - dependency-name: "lodash"
        # For Lodash, ignore all updates
    # Disable version updates for npm dependencies
    open-pull-requests-limit: 0
jobs:
  job_id:
    # ...

    # Add "id-token" with the intended permissions.
    permissions:
      contents: 'read'
      id-token: 'write'

    steps:
    # actions/checkout MUST come before auth
    - uses: 'actions/checkout@v3'

    - id: 'auth'
      name: 'Authenticate to Google Cloud'
      uses: 'google-github-actions/auth@v0'
      with:
        workload_identity_provider: 'projects/123456789/locations/global/workloadIdentityPools/my-pool/providers/my-provider'
        service_account: 'my-service-account@my-project.iam.gserviceaccount.com'

    # ... further steps are automatically authenticated
# Amanciojsilvjr to your organization's bitcoin respository
This code repository (or "repo") is designed to demonstrate the best GitHub has to offer with the least amount of noise.

The repo includes an `index.html` file (so it can render a web page), two GitHub Actions workflows, and a CSS stylesheet dependency.

# Set to true to add reviewers to PRs
addReviewers: true

# Set to 'author' to add PR's author as a assignee
addAssignees: author

# A list of reviewers to be added to PRs (GitHub user  name)
reviewers:
  - SecurityBTC
  - octocat

# A number of reviewers added to the PR
# Set 0 to add all the reviewers (default: 0)
numberOfReviewers: 1

# A list of assignees, overrides reviewers if set
assignees:
  - SecurityBTC
  - octocat

# A number of assignees to add to the PRs
# Set to 0 to add all of the assignees.
# Uses numberOfReviewers if unset.
numberOfAssignees: 0

# A list of keywords to be skipped the process if PR's title include it
skipKeywords:
  - wip
## Backers

Thank you to all our backers! 🙏 [[Become a backer](https://opencollective.com/curl#backer)]

[![Open Collective Backers](https://opencollective.com/curl/backers.svg?width=890)](https://opencollective.com/curl#backers)

## Sponsors

Support this project by becoming a sponsor. Your logo will show up here with a
link to your website. [[Become a sponsor](https://opencollective.com/curl#sponsor)]
<!-- markdown-link-check-disable -->
[![Open Collective Sponsor 0](https://opencollective.com/curl/sponsor/0/avatar.svg)](https://opencollective.com/curl/sponsor/0/website)
[![Open Collective Sponsor 1](https://opencollective.com/curl/sponsor/1/avatar.svg)](https://opencollective.com/curl/sponsor/1/website)
[![Build Status](https://github.com/tree-sitter/tree-sitter/workflows/CI/badge.svg)](https://github.com/tree-sitter/tree-sitter/actions)
[![Build status](https://ci.appveyor.com/api/projects/status/vtmbd6i92e97l55w/branch/master?svg=true)](https://ci.appveyor.com/project/maxbrunsfeld/tree-sitter/branch/master)
[![DOI](https://zenodo.org/badge/14164618.svg)](https://zenodo.org/badge/latestdoi/14164618)


[![My Skills](https://skillicons.dev/icons?i=js,html,css,wasm)](https://skillicons.dev)



[![My Skills](https://skillicons.dev/icons?i=java,kotlin,nodejs,figma&theme=light)](https://skillicons.dev)




[![My Skills](https://skillicons.dev/icons?i=aws,gcp,azure,react,vue,flutter&perline=3)](https://skillicons.dev)



<p align="center">
 <a href="https://skillicons.dev">
 <img src="https://skillicons.dev/icons?i=git,kubernetes,docker,c,vim" />
 </a>
</p>

Zz_cryptography advances new bitcoin #thank you 

<p align="center">

  <a require 'rugged'
require 'linguist'

repo = Rugged::Repository.new('.')
project = Linguist::Repository.new(repo, repo.head.target_id)
project.language       #=> "Ruby"
project.languages      #=> { "Ruby" => 119387 } <p align="center">
  <img src="https://avatars0.githubusercontent.com/u/44036562?s=100&v=4"/> "> <img src="./images/guia.png" 
  </a>
  <h1 align="center">Guia de Cyber Security</h1>
</p>

## :dart: O guia para alavancar a sua carreira

Abaixo você encontrará conteúdos para te guiar e ajudar a se tornar um profissional na área de segurança da informação ou se especializar caso você já atue na área, confira o repositório para descobrir novas ferramentas para o seu dia-a-dia, tecnologias para incorporar na sua stack com foco em se tornar um profissional atualizado e diferenciado em segurança da informação, alguns sites ou artigos podem estar em um idioma diferente do seu, porém isso não impede que você consiga realizar a leitura do artigo ou site em questão, você pode utilizar a ferramenta de tradução do Google para traduzir: sites, arquivos, textos.

<sub> <strong>Siga nas redes sociais para acompanhar mais conteúdos: </strong> <br>
[<img src = "https://img.shields.io/badge/GitHub-100000?style=for-the-badge&logo=github&logoColor=white">](https://github.com/BTCXBT)
[<img src = "https://img.shields.io/badge/Facebook-1877F2?style=for-the-badge&logo=facebook&logoColor=white">](https://www.facebook.com/amanciojunior/)
[<img src="https://img.shields.io/badge/linkedin-%230077B5.svg?&style=for-the-badge&logo=linkedin&logoColor=white" />](https://br.linkedin.com/in/amancio-j-238b97111/en?trk=people-guest_people_search-card)
[<img src = "https://img.shields.io/badge/Twitter-1DA1F2?style=for-the-badge&logo=twitter&logoColor=white">](https://twitter.com/amanciojsilvjr)
[<img src = "https://img.shields.io/badge/instagram-%23E4405F.svg?&style=for-the-badge&logo=instagram&logoColor=white">](https://www.instagram.com/amanciojsilvjr/)
</sub>

## 

> Antes de tudo você pode me ajudar e colaborar, deu bastante trabalho fazer esse repositório thanks.

- Me siga no [Github](https://github.com/BTCXBT)
- Acesse as redes sociais do [Guia Dev Brasil](https://br.linkedin.com/in/amancio-j-238b97111/en?trk=people-guest_people_search-card)
- Mande feedbacks no [Linkedin](https://br.linkedin.com/in/amancio-j-238b97111/en?trk=people-guest_people_search-card/)

> Se você deseja acompanhar esse repositório em outro idioma que não seja o Português , você pode optar pelas escolhas de idiomas abaixo, você também pode colaborar com a tradução para outros idiomas e a correções de possíveis erros ortográficos, a comunidade agradece.

<img src = "https://i.imgur.com/lpP9V2p.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>English — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/GprSvJe.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>Spanish — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/4DX1q8l.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>Chinese — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/6MnAOMg.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>Hindi — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/8t4zBFd.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>Arabic — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/iOdzTmD.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>French — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/PILSgAO.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>Italian — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/0lZOSiy.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>Korean — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/3S5pFlQ.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>Russian — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/i6DQjZa.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>German — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>
<img src = "https://i.imgur.com/wWRZMNK.png" alt="Guia Extenso de Programação" width="16" height="15">・<b>Japanese — </b> [Click Here](https://github.com/arthurspk/guiadevbrasil)<br>

## 📚 ÍNDICE

[🗺️ Cyber Security roadmap](#%EF%B8%8F-cyber-security-roadmap) <br>
[🔧 Ferramentas para tradução de conteúdo](#-ferramentas-para-tradução-de-conteúdo) <br>
[🧥 Introdução a área de Cyber Security](#-introdução-a-área-de-cyber-security) <br>
[💼 Carreiras na área de Cyber Security](#-carreiras-na-área-de-cyber-security) <br>
[🕵️‍♂️ Sites para estudar Cyber Security](#%EF%B8%8F%EF%B8%8F-sites-para-estudar-cyber-security) <br>
[📰 Sites de noticias](#-sites-de-noticias-de-cyber-security) <br>
[📃 Newsletters](#-newsletters-de-cyber-security) <br>
[🗃️ Awesome Hacking](#%EF%B8%8F-awesome-hacking) <br>
[🔗 Testes de segurança de API](#-testes-de-segurança-de-api) <br>
[🎥 Canais do Youtube](#-canais-do-youtube) <br>
[🔎 Ferramentas de busca](#-ferramentas-de-busca) <br>
[📱 Ferramentas de Mobile](#-ferramentas-de-mobile) <br>
[🎤 Podcasts](#-podcasts-de-cyber-security) <br>
[📽️ Palestras](#%EF%B8%8F-palestras) <br>
[🃏 CheatSheets](#-cheatsheets) <br>
[♟️ Exploitation](#%EF%B8%8F-exploitation) <br>
[🎬 Documentários](#-documentários) <br>
[🚩 Capture the Flag](#-capture-the-flag) <br>
[🐧 Distros de Linux](#-distros-de-linux) <br>
[💻 Máquinas Virtuais](#-máquinas-virtuais) <br>
[💰 Sites de Bug Bounty](#-sites-de-bug-bounty) <br>
[📮 Perfis no Twitter](#-perfis-no-twitter) <br>
[✨ Perfis no Instagram](#-perfis-no-instagram) <br>
[🎇 Comunidades no Reddit](#-comunidades-no-reddit) <br>
[🌌 Comunidades no Discord](#-comunidades-no-discord) <br>
[📚 Recomendações de livros](#-recomendações-de-livros) <br>
[🛠️ Frameworks e ferramentas de Hacking Web](#%EF%B8%8F-frameworks-e-ferramentas-de-hacking-web) <br>
[🪓 Ferramentas para obter informações](#-ferramentas-para-obter-informações) <br>
[🔧 Ferramentas para Pentesting](#-ferramentas-para-pentesting) <br>
[🔨 Ferramentas para Hardware Hacking](#-ferramentas-para-hardware-hacking) <br>
[🦉 Sites e cursos para aprender C](#-sites-e-cursos-para-aprender-c) <br>
[🐬 Sites e cursos para aprender Go](#-sites-e-cursos-para-aprender-go) <br>
[🦚 Sites e cursos para aprender C#](#-sites-e-cursos-para-aprender-c-1) <br>
[🐸 Sites e cursos para aprender C++](#-sites-e-cursos-para-aprender-c-2) <br>
[🐘 Sites e cursos para aprender PHP](#-sites-e-cursos-para-aprender-php) <br>
[🦓 Sites e cursos para aprender Java](#-sites-e-cursos-para-aprender-java) <br>
[🐦 Sites e cursos para aprender Ruby](#-sites-e-cursos-para-aprender-ruby) <br>
[🐪 Sites e cursos para aprender Perl](#-sites-e-cursos-para-aprender-perl) <br>
[🐷 Sites e cursos para aprender Bash](#-sites-e-cursos-para-aprender-bash) <br>
[🐴 Sites e cursos para aprender MySQL](#-sites-e-cursos-para-aprender-mysql) <br>
[🐧 Sites e cursos para aprender Linux](#-sites-e-cursos-para-aprender-linux) <br>
[🦂 Sites e cursos para aprender Swift](#-sites-e-cursos-para-aprender-swift) <br>
[🐍 Sites e cursos para aprender Python](#-sites-e-cursos-para-aprender-python) <br>
[🐋 Sites e cursos para aprender Docker](#-sites-e-cursos-para-aprender-docker) <br>
[🐼 Sites e cursos para aprender Assembly](#-sites-e-cursos-para-aprender-assembly) <br>
[🦞 Sites e cursos para aprender Powershell](#-sites-e-cursos-para-aprender-powershell) <br>
[🖥️ Sites e cursos para aprender Hardware Hacking](#%EF%B8%8F-sites-e-cursos-para-aprender-hardware-hacking) <br>
[📡 Sites e cursos para aprender Redes de Computadores](#-sites-e-cursos-para-aprender-redes-de-computadores) <br>
[🎓 Certificações para Cyber Security](#-certificações-para-cyber-security) <br>

## 🗺️ Cyber Security roadmap

![Cyber Security roadmap](https://i.imgur.com/eq4uu7P.jpg)

## 🔧 Ferramentas para tradução de conteúdo
> Muito do conteúdo desse repositório pode se encontrar em um idioma diferente do Português , desta maneira, fornecemos algumas ferramentas para que você consiga realizar a tradução do conteúdo, lembrando que o intuito desse guia é fornecer todo o conteúdo possível para que você possa se capacitar na área de Cyber Security independente do idioma a qual o material é fornecido, visto que se você possuí interesse em consumir esse material isso não será um empecilho para você continue seus estudos.

- [Google Translate](https://translate.google.com.br/?hl=pt-BR)
- [Linguee](https://www.linguee.com.br/ingles-portugues/traducao/translate.html)
- [DeepL](https://www.deepl.com/pt-BR/translator)
- [Reverso](https://context.reverso.net/traducao/ingles-portugues/translate)

## 🧥 Introdução a área de Cyber Security
> Também chamada de segurança de computadores ou segurança da tecnologia da informação, a cybersecurity é a prática de proteção de hardwares e softwares contra roubo ou danos, como servidores, dispositivos móveis, redes e aplicativos, as pessoas que atuam na área de Cyber Security de uma empresa são responsáveis por identificar todos os pontos vulneráveis do negócio no ambiente digital e em variados sistemas, o trabalho consiste em mapear todos os pontos fracos, que podem ser usados como porta de acesso para ataques virtuais. Além disso, é importante simular todos os possíveis ataques que poderiam ser realizados e criar proteções contra eles, antevendo os fatos para poder reforçar a segurança das informações e a redundância dos processos e sistemas de bancos de dados, a fim de evitar que haja interrupção de serviços, de uma forma geral, é esperado que as pessoas que trabalham com Cyber Security realizem uma série de atividades, tais como:

- Prever os riscos de sistemas, lojas virtuais e ambientes virtuais de empresas e diminuir possibilidades de ataques;
- Detectar todas as intrusões e elaborar sistemas de proteção;
- Criar políticas e planos de acesso a dados e informações;
- Implementar e atualizar parâmetros de segurança;
- Treinar e supervisionar o trabalho do time de Cyber Security;
- Organizar um sistema eficiente e seguro para colaboradores/as e terceirizados/as;
- Verificar todas as vulnerabilidades e as falhas responsáveis por elas;
- Fazer auditorias periódicas nos sistemas;
- Realizar avaliações de risco em redes, apps e sistemas;
- Fazer testes de suscetibilidade;
- Garantir plena segurança ao armazenamento de dados de empresas, lojas virtuais e outros.

## 💼 Carreiras na área de Cyber Security
> Nesse tópico você irá conhecer mais sobre as carreiras que você pode seguir dentro da área de Cyber Security, você encontrará as profissões em conjunto com um artigo ou video explicativo sobre como funciona.

- [Forensics](https://imasters.com.br/carreira-dev/profissao-analista-forense-computacional)
- [Biometrics](https://www.thoughtworks.com/pt-br/insights/decoder/b/biometrics)
- [IA Security](https://successfulstudent.org/jobs-in-information-assurance-and-security/)
- [IoT Security](https://www.quora.com/How-do-I-start-my-career-in-IoT-Security)
- [Cryptography](https://www.wgu.edu/career-guide/information-technology/cryptographer-career.html#:~:text=What%20Does%20a%20Cryptographer%20Do,%2C%20business%2C%20or%20military%20data.)
- [Cloud Security](https://www.cybersecurityjobsite.com/staticpages/10291/careers-in-cloud-security/#:~:text=Cloud%20security%20careers%20are%20set,these%20critical%20systems%20are%20safe.)
- [Fraud Prevention](https://www.zippia.com/fraud-prevention-specialist-jobs/)
- [Malware Analysis](https://onlinedegrees.sandiego.edu/malware-analyst-career-guide/#:~:text=A%20malware%20analyst%20works%20in,%2C%E2%80%9D%20explains%20the%20Infosec%20Institute.)
- [Hardware Hacking](https://www.helpnetsecurity.com/2019/07/15/hardware-hacker/)
- [Big Data Security](https://www.simplilearn.com/cyber-security-vs-data-science-best-career-option-article)
- [Physical Security](https://www.zippia.com/physical-security-specialist-jobs/what-does-a-physical-security-specialist-do/)
- [Security Awareness](https://resources.infosecinstitute.com/topic/security-awareness-manager-is-it-the-career-for-you/)
- [Threat Intelligence](https://www.eccouncil.org/cybersecurity-exchange/threat-intelligence/why-pursue-career-cyber-threat-intelligence/#:~:text=Put%20simply%2C%20threat%20intelligence%20professionals,combat%20existing%20and%20emerging%20threats.)
- [Business Continuity](https://www.zippia.com/business-continuity-planner-jobs/)
- [Operations Security](https://www.zippia.com/operational-security-specialist-jobs/)
- [Application Security](https://www.appsecengineer.com/blog/guide-to-becoming-an-application-security-engineer)
- [Legal and Regulations](https://onlinemasteroflegalstudies.com/career-guides/)
- [Communications Security](https://www.ukcybersecuritycouncil.org.uk/qualifications-and-careers/careers-route-map/cryptography-communications-security/)
- [Cyber Security Engineer](https://onlinedegrees.sandiego.edu/should-you-become-a-cyber-security-engineer/#:~:text=Cybersecurity%20engineers%2C%20sometimes%20called%20information,and%20all%20types%20of%20cybercrime.)
- [Advanced Cyber Analytics](https://www.coursera.org/articles/cybersecurity-analyst-job-guide)
- [Vulnerability Management](https://www.ukcybersecuritycouncil.org.uk/qualifications-and-careers/careers-route-map/vulnerability-management/)
- [Industrial Control Systems](https://ianmartin.com/careers/177380-industrial-control-system-ics-engineer/)
- [Privacy and Data Protection](https://resources.infosecinstitute.com/topic/data-privacy-careers-which-path-is-right-for-you/)
- [Data Protection Officer (DPO)](https://www.michaelpage.com.hk/advice/job-description/technology/data-protection-officer)
- [Penetration Testing Engineer](https://onlinedegrees.sandiego.edu/vulnerability-and-penetration-testing/)
- [Security and Risk Assessment](https://learn.org/articles/What_are_Some_Career_Options_in_Security_Risk_Assessment.html)
- [Identity and Acess Management](https://identitymanagementinstitute.org/identity-and-access-management-career-and-jobs/#:~:text=Identity%20and%20access%20management%20career%20and%20jobs%20involve%20protecting%20systems,mechanism%20and%20have%20the%20appropriate)
- [Software Development Security](https://cybersecurityguide.org/careers/security-software-developer/#:~:text=A%20security%20software%20developer%20is,written%20and%20verbal%20communication%20skills.)
- [Offensive Security (Red Team)](https://careers.mitre.org/us/en/job/R107322/Offensive-Security-Red-Team-Developer)
- [Defensive Security (Blue Team)](https://www.csnp.org/post/a-career-in-defensive-security-blue-team#:~:text=What%20is%20the%20Blue%20team,all%20the%20security%20measures%20applied.)
- [Incident Handling And Analysis](https://www.ziprecruiter.com/Career/Incident-Response-Analyst/What-Is-How-to-Become)
- [Introsuion Detection and Prevention](https://www.spiceworks.com/it-security/vulnerability-management/articles/what-is-idps/#:~:text=An%20intrusion%20detection%20and%20prevention,administrator%20and%20prevent%20potential%20attacks.)
- [Information Security Governance](https://www.ukcybersecuritycouncil.org.uk/qualifications-and-careers/careers-route-map/cyber-security-governance-risk-management/)
- [Security Frameworks and Standards](https://www.linkedin.com/pulse/overview-cyber-security-frameworks-standards-tommy/)
- [Security Architecture and Design](https://www.infosectrain.com/blog/roles-and-responsibilities-of-a-security-architect/#:~:text=A%20Security%20Architect%20creates%2C%20plans,%2C%20cybersecurity%2C%20and%20risk%20management.)

## 🕵️‍♂️ Sites para estudar Cyber Security

- [HackXpert](https://hackxpert.com/) - Laboratórios e treinamentos gratuitos.
- [TryHackMe](https://tryhackme.com/) - Exercícios práticos e laboratórios.
- [CyberSecLabs](https://www.cyberseclabs.co.uk/) - Laboratórios de treinamento de alta qualidade.
- [Cybrary](https://www.cybrary.it/) - Vídeos, laboratórios e exames práticos.
- [LetsDefend](https://letsdefend.io/) - Plataforma de treinamento da blue team.
- [Root Me](https://www.root-me.org/) - Mais de 400 desafios de cyber security.
- [RangeForce](https://www.rangeforce.com/) - Plataforma interativa e prática. 
- [Certified Secure](https://www.certifiedsecure.com/frontpage) - Muitos desafios diferentes.
- [Vuln Machines](https://www.vulnmachines.com/) - Cenários do mundo real para praticar.
- [Try2Hack](https://try2hack.me/) - Jogue um jogo baseado nos ataques reais.
- [TCM Security](https://academy.tcm-sec.com/) - Cursos de nível básico para cyber security.
- [EchoCTF](https://echoctf.red/) - Treine suas habilidades ofensivas e defensivas.
- [Hack The Box](https://www.hackthebox.com/) - Plataforma online de treinamento em cyber security.
- [Vuln Hub](https://www.vulnhub.com/) - Material para experiência prática.
- [OverTheWire](https://overthewire.org/wargames/) - Aprenda conceitos de segurança por meio de desafios.
- [PentesterLab](https://pentesterlab.com/) - Aprenda testes de penetração de aplicativos da web.
- [PortSwigger Web Security Academy](https://portswigger.net/web-security) - Amplo material didático.

## 📰 Sites de noticias de Cyber Security

- [IT Security Guru](https://www.itsecurityguru.org/)
- [Security Weekly](https://securityweekly.com/)
- [The Hacker News](https://thehackernews.com/)
- [Infosecurity Magazine](https://www.infosecurity-magazine.com/)
- [CSO Online](https://www.csoonline.com/)
- [The State of Security - Tripwire](https://www.tripwire.com/state-of-security/)
- [The Last Watchdog](https://www.lastwatchdog.com/)
- [Naked Security](https://nakedsecurity.sophos.com/)
- [Graham Cluley](https://grahamcluley.com/)
- [Cyber Magazine](https://cybermagazine.com/)
- [WeLiveSecurity](https://www.welivesecurity.com/br/)
- [Dark Reading](https://www.darkreading.com/)
- [Threatpost](https://threatpost.com/)
- [Krebs on Security](https://krebsonsecurity.com/)
- [Help Net Security](https://www.helpnetsecurity.com/)
- [HackRead](https://www.hackread.com/)
- [SearchSecurity](https://www.techtarget.com/searchsecurity/)
- [TechWorm](https://www.techworm.net/category/security-news)
- [GBHackers On Security](https://gbhackers.com/)
- [The CyberWire](https://thecyberwire.com/)
- [Cyber Defense Magazine](https://www.cyberdefensemagazine.com/)
- [Hacker Combat](https://hackercombat.com/)
- [Cybers Guards](https://cybersguards.com/)
- [Cybersecurity Insiders](https://www.cybersecurity-insiders.com/)
- [Information Security Buzz](https://informationsecuritybuzz.com/)
- [The Security Ledger](https://securityledger.com/)
- [Security Gladiators](https://securitygladiators.com/)
- [Infosec Land](https://pentester.land/)
- [Cyber Security Review](https://www.cybersecurity-review.com/)
- [Comodo News](https://blog.comodo.com/)
- [Internet Storm Center | SANS](https://isc.sans.edu/)
- [Daniel Miessler](https://danielmiessler.com/)
- [TaoSecurity](https://www.taosecurity.com/)
- [Reddit](https://www.reddit.com/search/?q=Security%20news)
- [All InfoSec News](https://allinfosecnews.com/)
- [CVE Trends](https://cvetrends.com/)
- [Securibee](https://securib.ee/)
- [threatABLE](https://www.threatable.io/)
- [Troy Hunt Blog](https://www.troyhunt.com/)
- [Errata Security](https://blog.erratasec.com/)

## 📃 Newsletters de Cyber Security

- [API Security Newsletter](https://apisecurity.io/) - Notícias e vulnerabilidades de segurança da API.
- [Blockchain Threat Intelligence](https://newsletter.blockthreat.io/) - Ferramentas, eventos, ameaças.
- [We Live Security](https://www.welivesecurity.com/br/) - Notícias, visualizações e insights premiados.
- [SecPro](https://www.thesec.pro/) - Análise de ameaças, ataques e tutoriais.
- [Gov Info Security](https://www.govinfosecurity.com/) - Notícias governamentais de segurança cibernética nacionais e internacionais.
- [Threatpost](https://threatpost.com/newsletter-sign/) - Exploits, vulnerabilidades, malware e segurança cibernética.
- [AWS Security Digest](https://awssecuritydigest.com/) - Atualizações de segurança da AWS.
- [Krebs On Security](https://krebsonsecurity.com/subscribe/) - Jornalismo investigativo de segurança cibernética que é interessante.
- [Risky Biz](https://risky.biz/subscribe/) - Análise de grandes histórias cibernéticas.
- [Unsupervised Learning Community](https://danielmiessler.com/newsletter/) - Histórias importantes de segurança cibernética.
- [Schneier on Security](https://www.schneier.com/) - Notícias e opiniões sobre segurança cibernética.
- [CyberSecNewsWeekly](https://buttondown.email/CybersecNewsWeekly) - Coleção de notícias, artigos e ferramentas.
- [RTCSec](https://www.rtcsec.com/newsletter/) - Notícias sobre segurança VOIP e WebRTC.
- [This Week in 4n6](https://thisweekin4n6.com/) - Atualizações do DFIR.
- [Securibee Newsletter](https://securib.ee/newsletter/) - Notícias de segurança cibernética com curadoria.
- [Shift Security Left](https://shift-security-left.curated.co/) - Segurança, arquitetura e incidentes de aplicativos.
- [TripWire’s State of Security](https://www.tripwire.com/state-of-security/) - Notícias de segurança cibernética corporativa.
- [Graham Cluley](https://grahamcluley.com/gchq-newsletter/) - Notícias e opiniões sobre segurança cibernética.
- [Zero Day](https://zetter.substack.com/) - Histórias sobre hackers, espiões e crimes cibernéticos.
- [The Hacker News](https://thehackernews.com/#email-outer) - Notícias de cibersegurança.
- [CSO Online](https://www.csoonline.com/newsletters/signup.html) - Notícias, análises e pesquisas sobre segurança e gerenciamento de riscos.
- [Naked Security](https://nakedsecurity.sophos.com/) - Como se proteger de ataques.
- [AdvisoryWeek](https://advisoryweek.com/) - Resumos de consultoria de segurança dos principais fornecedores.
- [tl;dr sec Newsletter](https://tldrsec.com/) - Ferramentas, posts em blogs, conferências e pesquisas.

## 🗃️ Awesome Hacking

- [Awesome Hacking](https://github.com/Hack-with-Github/Awesome-Hacking)
- [Awesome Honeypots](https://github.com/paralax/awesome-honeypots)
- [Awesome Incident Response](https://github.com/meirwah/awesome-incident-response)
- [Awesome Malware Analysis](https://github.com/rshipp/awesome-malware-analysis)
- [Awesome Red Teaming](https://github.com/yeyintminthuhtut/Awesome-Red-Teaming)
- [Awesome Security](https://github.com/sbilly/awesome-security)
- [Awesome Privacy](https://github.com/Lissy93/awesome-privacy)
- [Awesome Darknet](https://github.com/DarknetList/awesome-darknet)
- [Awesome Tor](https://github.com/ajvb/awesome-tor)
- [Awesome Mobile Security](https://github.com/vaib25vicky/awesome-mobile-security)
- [Awesome Penetration Testing](https://github.com/enaqx/awesome-pentest)
- [Awesome Pentesting Bible](https://github.com/blaCCkHatHacEEkr/PENTESTING-BIBLE)
- [Awesome Hacking Amazing Project](https://github.com/carpedm20/awesome-hacking)
- [Awesome Pentest Tools](https://github.com/gwen001/pentest-tools)
- [Awesome Forensic Tools](https://github.com/ivbeg/awesome-forensicstools)
- [Awesome Android Security](https://github.com/ashishb/android-security-awesome)
- [Awesome AppSec](https://github.com/paragonie/awesome-appsec)
- [Awesome Asset Discovery](https://github.com/redhuntlabs/Awesome-Asset-Discovery)
- [Awesome Bug Bounty](https://github.com/djadmin/awesome-bug-bounty)
- [Awesome CTF](https://github.com/apsdehal/awesome-ctf)
- [Awesome Cyber Skills](https://github.com/joe-shenouda/awesome-cyber-skills)
- [Awesome DevSecOps](https://github.com/devsecops/awesome-devsecops)
- [Awesome Embedded and IoT Security](https://github.com/fkie-cad/awesome-embedded-and-iot-security)
- [Awesome Exploit Development](https://github.com/FabioBaroni/awesome-exploit-development)
- [Awesome Fuzzing](https://github.com/secfigo/Awesome-Fuzzing)
- [Awesome Hacking Resources](https://github.com/vitalysim/Awesome-Hacking-Resources)
- [Awesome Industrial Control System](https://github.com/hslatman/awesome-industrial-control-system-security)
- [Awesome Infosec](https://github.com/onlurking/awesome-infosec)
- [Awesome IoT Hacks](https://github.com/nebgnahz/awesome-iot-hacks)
- [Awesome Mainframe Hacking](https://github.com/samanL33T/Awesome-Mainframe-Hacking)
- [Awesome OSINT](https://github.com/jivoi/awesome-osint)
- [Awesome macOS and iOS Security Related Tools](https://github.com/ashishb/osx-and-ios-security-awesome)
- [Awesome PCAP Tools](https://github.com/caesar0301/awesome-pcaptools)
- [Awesome PHP](https://github.com/ziadoz/awesome-php)
- [Awesome Reversing](https://github.com/tylerha97/awesome-reversing)
- [Awesome Reinforcement Learning for Cyber Security](https://github.com/Limmen/awesome-rl-for-cybersecurity)
- [Awesome Security Talks](https://github.com/PaulSec/awesome-sec-talks)
- [Awesome Serverless Security](https://github.com/puresec/awesome-serverless-security/)
- [Awesome Social Engineering](https://github.com/v2-dev/awesome-social-engineering)
- [Awesome Threat Intelligence](https://github.com/hslatman/awesome-threat-intelligence)
- [Awesome Vehicle Security](https://github.com/jaredthecoder/awesome-vehicle-security)
- [Awesome Vulnerability Research](https://github.com/sergey-pronin/Awesome-Vulnerability-Research)
- [Awesome Web Hacking](https://github.com/infoslack/awesome-web-hacking)
- [Awesome Advanced Windows Exploitation References](https://github.com/yeyintminthuhtut/Awesome-Advanced-Windows-Exploitation-References)
- [Awesome Wifi Arsenal](https://github.com/0x90/wifi-arsenal)
- [Awesome YARA](https://github.com/InQuest/awesome-yara)
- [Awesome Browser Exploit](https://github.com/Escapingbug/awesome-browser-exploit)
- [Awesome Linux Rootkits](https://github.com/milabs/awesome-linux-rootkits)
- [Awesome API Security](https://github.com/arainho/awesome-api-security/)

## 🔗 Testes de segurança de API

> Cursos, videos, artigos, blogs, podcast sobre testes de segurança de API em Português 
- [Segurança em APIs REST](https://blog.mandic.com.br/artigos/seguranca-em-apis-rest-parte-1/)
- [Segurança de APIs - Red Hat](https://www.redhat.com/pt-br/topics/security/api-security)
- [Segurança de API: 5 melhores práticas para controlar riscos](https://www.bry.com.br/blog/seguranca-de-api/)
- [O que é Segurança de API](https://minutodaseguranca.blog.br/o-que-e-seguranca-de-api/)

> Cursos, videos, artigos, blogs, podcast sobre testes de segurança de API em Inglês
- [Traceable AI, API Hacking 101](https://www.youtube.com/watch?v=qC8NQFwVOR0&ab_channel=Traceable)
- [Katie Paxton-Fear, API Hacking](https://www.youtube.com/watch?v=qC8NQFwVOR0&ab_channel=Traceable)
- [Bad API, hAPI Hackers! by jr0ch17](https://www.youtube.com/watch?v=UT7-ZVawdzA&ab_channel=Bugcrowd)
- [OWASP API Security Top 10 Webinar](https://www.youtube.com/watch?v=zTkv_9ChVPY&ab_channel=42Crunch)
- [How to Hack APIs in 2021](https://labs.detectify.com/2021/08/10/how-to-hack-apis-in-2021/)
- [Let's build an API to hack](https://hackxpert.com/blog/API-Hacking-Excercises/Excercises%207e5f4779cfe34295a0d477a12c05ecbd/Let's%20build%20an%20API%20to%20hack%20-%20Part%201%20The%20basics%2007599097837a4f539104b20376346b7e.html)
- [Bugcrowd, API Security 101 - Sadako](https://www.youtube.com/watch?v=ijalD2NkRFg&ab_channel=Bugcrowd)
- [David Bombal, Free API Hacking Course](https://www.youtube.com/watch?v=CkVvB5woQRM&ab_channel=DavidBombal)
- [How To Hack API In 60 Minutes With Open Source Tools](https://www.wallarm.com/what/how-to-hack-api-in-60-minutes-with-open-source)
- [APIsecurity IO, API Security Articles](https://apisecurity.io/)
- [The API Security Maturity Model](https://curity.io/resources/learn/the-api-security-maturity-model/)
- [API Security Best Practices MegaGuide](https://expeditedsecurity.com/api-security-best-practices-megaguide/)
- [API Security Testing Workshop - Grant Ongers](https://www.youtube.com/watch?v=l0ISDMUpm68&ab_channel=StackHawk)
- [The XSS Rat, API Testing And Securing Guide](https://www.youtube.com/playlist?list=PLd92v1QxPOprsg5fTjGBApq4rpb0G-N8L)
- [APIsec OWASP API Security Top 10: A Deep Dive](https://www.apisec.ai/blog/what-is-owasp-api-security-top-10)
- [We Hack Purple, API Security Best Practices](https://www.youtube.com/watch?v=F9CN0NE93Qc&ab_channel=WeHackPurple)
- [Kontra Application Security, Owasp Top 10 for API](https://application.security/free/owasp-top-10-API)
- [OWASP API Top 10 CTF Walk-through](https://securedelivery.io/articles/api-top-ten-walkthrough/)
- [How To Hack An API And Get Away With It](https://smartbear.com/blog/api-security-testing-how-to-hack-an-api-part-1/)
- [Ping Identity, API Security: The Complete Guide 2022](https://www.pingidentity.com/en/resources/blog/post/complete-guide-to-api-security.html)
- [Analyzing The OWASP API Security Top 10 For Pen Testers](https://www.youtube.com/watch?v=5UTHUZ3NGfw&ab_channel=SANSOffensiveOperations)
- [Finding and Exploiting Unintended Functionality in Main Web App APIs](https://bendtheory.medium.com/finding-and-exploiting-unintended-functionality-in-main-web-app-apis-6eca3ef000af)
- [API Security: The Complete Guide to Threats, Methods & Tools](https://brightsec.com/blog/api-security/)

## 🎥 Canais do Youtube

- [Mente binária](https://www.youtube.com/c/PapoBin%C3%A1rio) - Contéudo geral sobre Cyber Security
- [Guia Anônima](https://www.youtube.com/user/adsecf) - Contéudo geral sobre Cyber Security
- [Hak5](https://www.youtube.com/c/hak5) - Contéudo geral sobre Cyber Security
- [The XSS rat](https://www.youtube.com/c/TheXSSrat) - Tudo sobre Bug Bounty
- [ITProTV](https://www.youtube.com/c/ItproTv) - Contéudo geral sobre Cyber Security
- [Infosec](https://www.youtube.com/c/InfoSecInstitute) - Conscientização sobre Cyber Security
- [Cyrill Gössi](https://www.youtube.com/channel/UCp1rLlh9AQN9Pejzbg9dcAg) - Vídeos de criptografia.
- [DC CyberSec](https://www.youtube.com/c/DCcybersec) - Contéudo geral sobre Cyber Security
- [Black Hat](https://www.youtube.com/c/BlackHatOfficialYT) - Conferências técnicas de cibersegurança.
- [David Bombal](https://www.youtube.com/c/DavidBombal) - Tudo relacionado à segurança cibernética.
- [Outpost Gray](https://www.youtube.com/c/OutpostGray) - Desenvolvimento de carreira em segurança cibernética.
- [Bugcrowd](https://www.youtube.com/c/Bugcrowd) - Metodologias de Bug Bounty e entrevistas.
- [Network Chuck](https://www.youtube.com/c/NetworkChuck) - Tudo relacionado à segurança cibernética.
- [Professor Messer](https://www.youtube.com/c/professormesser) - Guias cobrindo certificações.
- [Cyberspatial](https://www.youtube.com/c/Cyberspatial) - Educação e treinamento em segurança cibernética.
- [OWASP Foundation](https://www.youtube.com/c/OWASPGLOBAL) - Conteúdo de segurança de aplicativos da Web.
- [Nahamsec](https://www.youtube.com/c/Nahamsec) - Vídeos educativos sobre hackers e bug bounty.
- [Computerphile](https://www.youtube.com/user/Computerphile) - Abrange conceitos e técnicas básicas.
- [InfoSec Live](https://www.youtube.com/c/infoseclive) - Tudo, desde tutoriais a entrevistas.
- [InsiderPHD](https://www.youtube.com/c/InsiderPhD) - Como começar a caçar bugs.
- [Security Weekly](https://www.youtube.com/c/SecurityWeekly) - Entrevistas com figuras de segurança cibernética.
- [Hack eXPlorer](https://www.youtube.com/c/HackeXPlorer) - Tutoriais gerais, dicas e técnicas.
- [Cyber CDH](https://www.youtube.com/c/cybercdh) - Ferramentas, táticas e técnicas de segurança cibernética.
- [John Hammond](https://www.youtube.com/c/JohnHammond010) - Análise de malware, programação e carreiras.
- [SANS Offensive Operations](https://www.youtube.com/c/SANSOffensiveOperations) - Vídeos técnicos de segurança cibernética.
- [13Cubed](https://www.youtube.com/c/13cubed) - Vídeos sobre ferramentas, análise forense e resposta a incidentes.
- [HackerSploit](https://www.youtube.com/c/HackerSploit) - Teste de penetração, hacking de aplicativos da web.
- [Z-winK University](https://www.youtube.com/channel/UCDl4jpAVAezUdzsDBDDTGsQ) - Educação e demonstrações de bug bountys.
- [Peter Yaworski](https://www.youtube.com/c/yaworsk1) - Dicas e entrevistas de hacking de aplicativos da Web.
- [IppSec](https://www.youtube.com/c/ippsec) - Laboratórios e tutoriais de capture the flag, HackTheBox etc.
- [Pentester Academy TV](https://www.youtube.com/c/PentesterAcademyTV) - Discussões e ataques demonstrativos.
- [BlackPerl](https://www.youtube.com/c/BlackPerl) - Análise de malware, análise forense e resposta a incidentes.
- [Offensive Security](https://www.youtube.com/c/OffensiveSecurityTraining) - Conteúdo educacional e orientações de laboratório.
- [Day Cyberwox](https://www.youtube.com/c/DayCyberwox) - Conteúdo útil de segurança na nuvem e orientações.
- [DEFCONConference](https://www.youtube.com/user/DEFCONConference) - Tudo do evento de segurança cibernética DEF CON.
- [STÖK](https://www.youtube.com/c/STOKfredrik) - Vídeos sobre ferramentas, análise de vulnerabilidades e metodologia.
- [MalwareTechBlog](https://www.youtube.com/c/MalwareTechBlog)- Conteúdo de segurança cibernética e engenharia reversa.
- [The Hated One](https://www.youtube.com/c/TheHatedOne) - Pesquisa que explica as concepções de segurança cibernética.
- [Simply Cyber](https://www.youtube.com/c/GeraldAuger) - Ajuda as pessoas com o desenvolvimento de carreira de segurança cibernética.
- [Black Hills Information Security](https://www.youtube.com/c/BlackHillsInformationSecurity) - Contéudo geral sobre Cyber Security
- [Security Now](https://www.youtube.com/c/securitynow) - Notícias de crimes cibernéticos, hackers e segurança de aplicativos da web.
- [The Cyber Mentor](https://www.youtube.com/c/TheCyberMentor) - Hacking ético, hacking de aplicativos da web e ferramentas.
- [Joe Collins](https://www.youtube.com/user/BadEditPro) - Tudo relacionado ao Linux, incluindo tutoriais e guias.
- [Null Byte](https://www.youtube.com/c/NullByteWHT) - Segurança cibernética para hackers éticos e cientistas da computação.
- [LiveOverflow](https://www.youtube.com/c/LiveOverflow) - Envolve hacking, vídeos de gravação e capture the flags.
- [The PC Security Channel](https://www.youtube.com/c/thepcsecuritychannel) - Segurança do Windows, notícias sobre malware e tutoriais.

## 🔎 Ferramentas de busca

- [Dehashed](https://www.dehashed.com/) - Veja as credenciais vazadas.
- [SecurityTrails](https://securitytrails.com/) - Extensos dados de DNS.
- [DorkSearch](https://dorksearch.com/) - Google dorking muito rápido.
- [ExploitDB](https://www.exploit-db.com/) - Arquivo de vários exploits.
- [ZoomEye](https://www.zoomeye.org/) - Reúna informações sobre alvos.
- [Pulsedive](https://pulsedive.com/) - Procure por inteligência de ameaças.
- [GrayHatWarfare](https://grayhatwarfare.com/) - Pesquise buckets S3 públicos.
- [PolySwarm](https://polyswarm.io/) - Verifique arquivos e URLs em busca de ameaças.
- [Fofa](http://fofa.so/) - Procure por várias inteligências de ameaças.
- [LeakIX](https://leakix.net/) - Pesquise informações indexadas publicamente.
- [DNSDumpster](https://dnsdumpster.com/) - Pesquise registros DNS rapidamente.
- [FullHunt](https://fullhunt.io/) - Superfícies de ataque de pesquisa e descoberta.
- [AlienVault](https://otx.alienvault.com/) - Pesquise e descubra sobre ataques de surfaces.
- [ONYPHE](https://www.onyphe.io/) - Amplo feed de inteligência de ameaças.
- [Grep App](https://grep.app/) - Coleta dados de inteligência de ameaças cibernéticas.
- [URL Scan](https://urlscan.io/) - Pesquise em meio milhão de repositórios git.
- [Vulners](https://vulners.com/) - Serviço gratuito para digitalizar e analisar sites.
- [WayBackMachine](https://archive.org/web/) - Visualize o conteúdo de sites excluídos.
- [Shodan](https://www.shodan.io/) - Procure dispositivos conectados à internet.
- [Netlas](https://netlas.io/) - Pesquise e monitore ativos conectados à Internet.
- [CRT sh](https://crt.sh/) - Procure por certificados que foram registrados pelo CT.
- [Wigle](https://www.wigle.net/) - Banco de dados de redes sem fio, com estatísticas.
- [PublicWWW](https://publicwww.com/) - Pesquisa de marketing e marketing de afiliados.
- [Binary Edge](https://www.binaryedge.io/) - Verifica a Internet em busca de inteligência de ameaças.
- [GreyNoise](https://www.greynoise.io/) - Procure dispositivos conectados à internet.
- [Hunter](https://hunter.io/) - Pesquise endereços de e-mail pertencentes a um site.
- [Censys](https://censys.io/) - Avaliando a superfície de ataque para dispositivos conectados à Internet.
- [IntelligenceX](https://intelx.io/) - Pesquise Tor, I2P, vazamentos de dados, domínios e e-mails.
- [Packet Storm Security](https://packetstormsecurity.com/) - Navegue pelas vulnerabilidades e explorações mais recentes.
- [SearchCode](https://searchcode.com/) - Pesquise 75 bilhões de linhas de código de 40 milhões de projetos.

## 📱 Ferramentas de Mobile

- [Mobile Security Framework](https://github.com/MobSF/Mobile-Security-Framework-MobSF)
- [Hacker 101](https://github.com/Hacker0x01/hacker101)
- [Objection Runtime Mobile Exploration](https://github.com/sensepost/objection)
- [Wire iOS](https://github.com/wireapp/wire-ios)
- [Drozer](https://github.com/WithSecureLabs/drozer)
- [Needle](https://github.com/WithSecureLabs/needle)

## 🎤 Podcasts de Cyber Security

- [Cyber Work](https://www.infosecinstitute.com/podcast/)
- [Click Here](https://therecord.media/podcast/)
- [Defrag This](https://open.spotify.com/show/6AIuefXVoa6XXriNo4ZAuF)
- [Security Now](https://twit.tv/shows/security-now)
- [InfoSec Real](https://www.youtube.com/channel/UC2flvup7giBpysO-4wdynMg/featured)
- [InfoSec Live](https://www.youtube.com/c/infoseclive)
- [Simply Cyber](https://www.simplycyber.io/podcast)
- [OWASP Podcast](https://owasp.org/www-project-podcast/)
- [We Talk Cyber](https://monicatalkscyber.com/)
- [Risky Business](https://open.spotify.com/show/0BdExoUZqbGsBYjt6QZl4Q)
- [Malicious Life](https://malicious.life/)
- [Hacking Humans](https://thecyberwire.com/podcasts/hacking-humans)
- [What The Shell](https://open.spotify.com/show/3QcBl6Yf1E3rLdz3UJEzOM)
- [Life of a CISO](https://open.spotify.com/show/3rn3xiUMELnMtAPHMOebx2)
- [H4unt3d Hacker](https://thehauntedhacker.com/podcasts)
- [2 Cyber Chicks](https://www.itspmagazine.com/2-cyber-chicks-podcast)
- [The Hacker Mind](https://thehackermind.com/)
- [Security Weekly](https://securityweekly.com/)
- [Cyberside Chats](https://open.spotify.com/show/6kqTXF20QV3gphPsikl1Uo)
- [Darknet Diaries](https://darknetdiaries.com/)
- [CyberWire Daily](https://thecyberwire.com/podcasts/daily-podcast)
- [Absolute AppSec](https://absoluteappsec.com/)
- [Security in Five](https://securityinfive.libsyn.com/)
- [The Cyber Queens](https://www.cyberqueenspodcast.com/)
- [Smashing Security](https://www.smashingsecurity.com/)
- [401 Access Denied](https://delinea.com/events/podcasts)
- [7 Minute Security](https://7minsec.com/)
- [8th Layer Insights](https://thecyberwire.com/podcasts/8th-layer-insights)
- [Adopting Zero Trust](https://open.spotify.com/show/5hrfiDWuthYUQwj7wyIMzI)
- [Cyber Crime Junkies](https://cybercrimejunkies.com/)
- [Dr Dark Web Podcast](https://www.cybersixgill.com/resources/podcast/)
- [Cyber Security Sauna](https://www.withsecure.com/en/whats-new/podcasts)
- [The Cyberlaw Podcast](https://www.lawfareblog.com/topic/cyberlaw-podcast)
- [Unsupervised Learning](https://open.spotify.com/show/0cIzWAEYacLz7Ag1n1YhUJ)
- [Naked Security Podcast](https://nakedsecurity.sophos.com/podcast/)
- [Identity at the Center](https://www.identityatthecenter.com/)
- [Breaking Down Security](https://www.brakeingsecurity.com/)
- [The Shellsharks Podcast](https://shellsharks.com/podcast)
- [The Virtual CISO Moment](https://virtual-ciso.us/)
- [The Cyber Ranch Podcast](https://hackervalley.com/cyberranch/)
- [The Cyber Tap (cyberTAP)](https://cyber.tap.purdue.edu/cybertap-podcast/)
- [The Shared Security Show](https://sharedsecurity.net/)
- [The Social-Engineer Podcast](https://open.spotify.com/show/6Pmp3DQKUDW6DXBlnGpxkH)
- [The 443 Security Simplified](https://www.secplicity.org/category/the-443/)
- [Adventures of Alice and Bob](https://www.beyondtrust.com/podcast)
- [Cybersecurity Today by ITWC](https://open.spotify.com/show/2YiPcnkJLIcxtQ04nCfaSu)
- [Crypto-Gram Security Podcast](https://crypto-gram.libsyn.com/)
- [Open Source Security Podcast](https://opensourcesecurity.io/category/podcast/)
- [Hacker Valley Studio Podcast](https://hackervalley.com/)
- [The Hacker Chronicles Podcast](https://www.tenable.com/podcast/hacker-chronicles)
- [BarCode Cybersecurity Podcast](https://thebarcodepodcast.com/)
- [Task Force 7 Cyber Security Radio](https://www.tf7radio.com/)
- [The Privacy, Security, & OSINT Show](https://inteltechniques.com/podcast.html)
- [Cyber Security Headlines by the CISO Series](https://cisoseries.com/category/podcast/cyber-security-headlines/)
- [SANS Internet Stormcenter Daily Cyber Podcast](https://podcasts.apple.com/us/podcast/sans-internet-stormcenter-daily-cyber-security-podcast/id304863991)

## 📽️ Palestras

- [Hardware Hacking e Bad USB - Leonardo La Rosa](https://www.youtube.com/watch?v=s25Fw69u9tM&ab_channel=MeninadeCybersec)
- [Atribuições de Ataques na Visão de Cyber Threat Intelligence - Robson Silva](https://www.youtube.com/watch?v=JallvQuZXZA&ab_channel=MeninadeCybersec)
- [Defesa Cibernética - Milena Barboza](https://youtu.be/Sc1VQkN3xiw)
- [DevSecOps Desenvolvimento Seguro - Michelle Mesquita](https://youtu.be/_ngBWBkq6wA)
- [Linguagem de Baixo Nível, Assembly e binários - Carolina Trigo](https://youtu.be/CL51I8xzzf8)
- [Segurança Ofensiva, Red Team e GRC - João Góes](https://youtu.be/q_moH0u9cFE)
- [Como se manter hacker num mundo de segurança - Thauan Santos](https://youtu.be/uo3STUx5mMk)
- [Hardware Hacking, Vulnerabilidades em RFID e NFC - Davi Mikael](https://youtu.be/zTv7JZpO-IA)
- [Como se tornar um Hacker em um mundo de script kiddies - Rafael Sousa](https://youtu.be/veFyCTv5i3g)
- [Black Hat Python - Hacking, Programação e Red Team - Joas Antonio](https://youtu.be/EOulWqLHmjo)
- [Python 101 - André Castro](https://youtu.be/AGxleHdhY8Q)
- [Engenharia Social e Humand Hacking - Marina Ciavatta](https://youtu.be/7mj2i2E5QMI)
- [Certificações em Cibersegurança - Fábio Augusto](https://youtu.be/b7Pwl3RGo9E)
- [Mobile Security - Oryon Farias](https://youtu.be/oMmzSbaj3Gk)

## 🃏 CheatSheets

- [Kali Linux Cheatsheets](https://www.comparitech.com/net-admin/kali-linux-cheat-sheet/)
- [Python Cheatsheets](https://www.pythoncheatsheet.org/)
- [Linux Command Line Cheatsheets](https://cheatography.com/davechild/cheat-sheets/linux-command-line/)
- [Nmap Cheatsheets](https://www.stationx.net/nmap-cheat-sheet/)
- [Red Team Cheatsheets](https://0xsp.com/offensive/red-team-cheatsheet/)
- [Blue Team Cheatsheets](https://guidance.ctag.org.uk/blue-team-cheatsheet)
- [Pentesting Cheatsheets](https://www.ired.team/offensive-security-experiments/offensive-security-cheetsheets)

## ♟️ Exploitation

- [Exploitation Tools](https://github.com/nullsecuritynet/tools)
- [SSRFmap](https://github.com/swisskyrepo/SSRFmap)
- [Fuxploider](https://github.com/almandin/fuxploider)
- [Explotation Windows](https://github.com/Hack-with-Github/Windows)

## 🎬 Documentários

- [We Are Legion – The Story Of The Hacktivists](https://lnkd.in/dEihGfAg)
- [The Internet’s Own Boy: The Story Of Aaron Swartz](https://lnkd.in/d3hQVxqp)
- [Hackers Wanted](https://lnkd.in/du-pMY2R)
- [Secret History Of Hacking](https://lnkd.in/dnCWU-hp)
- [Def Con: The Documentary](https://lnkd.in/dPE4jVVA)
- [Web Warriors](https://lnkd.in/dip22djp)
- [Risk (2016)](https://lnkd.in/dMgWT-TN)
- [Zero Days (2016)](https://lnkd.in/dq_gZA8z)
- [Guardians Of The New World (Hacking Documentary) | Real Stories](https://lnkd.in/dUPybtFd)
- [A Origem dos Hackers](https://lnkd.in/dUJgG-6J)
- [The Great Hack](https://lnkd.in/dp-MsrQJ)
- [The Networks Dilemma](https://lnkd.in/dB6rC2RD)
- [21st Century Hackers](https://lnkd.in/dvdnZkg5)
- [Cyber War - Dot of Documentary](https://lnkd.in/dhNTBbbx)
- [CyberWar Threat - Inside Worlds Deadliest Cyberattack](https://lnkd.in/drmzKJDu)
- [The Future of Cyberwarfare](https://lnkd.in/dE6_rD5x)
- [Dark Web Fighting Cybercrime Full Hacking](https://lnkd.in/dByEzTE9)
- [Cyber Defense: Military Training for Cyber Warfare](https://lnkd.in/dhA8c52h)
- [Hacker Hunter: WannaCry The History Marcus Hutchin](https://lnkd.in/dnPcnvSv)
- [The Life Hacker Documentary](https://lnkd.in/djAqBhbw)
- [Hacker The Realm and Electron - Hacker Group](https://lnkd.in/dx_uyTuT)

## 🚩 Capture the Flag

- [Hacker 101](https://www.hackerone.com/hackers/hacker101)
- [PicoCTF](https://picoctf.org/)
- [TryHackMe](https://tryhackme.com)
- [HackTheBox](https://www.hackthebox.com/)
- [VulnHub](https://www.vulnhub.com/)
- [HackThisSite](https://hackthissite.org/)
- [CTFChallenge](https://ctfchallenge.co.uk/)
- [Attack-Defense](https://attackdefense.com/)
- [Alert to win](https://alf.nu/alert1)
- [Bancocn](https://bancocn.com/)
- [CTF Komodo Security](https://ctf.komodosec.com/)
- [CryptoHack](https://cryptohack.org/)
- [CMD Challenge](https://cmdchallenge.com/http://overthewire.org/)
- [Explotation Education](https://exploit.education/)
- [Google CTF](https://lnkd.in/e46drbz8)
- [Hackthis](https://www.hackthis.co.uk/)
- [Hacksplaining](https://lnkd.in/eAB5CSTA)
- [Hacker Security](https://lnkd.in/ex7R-C-e)
- [Hacking-Lab](https://hacking-lab.com/)
- [HSTRIKE](https://hstrike.com/)
- [ImmersiveLabs](https://immersivelabs.com/)
- [NewbieContest](https://lnkd.in/ewBk6fU5)
- [OverTheWire](http://overthewire.org/)
- [Practical Pentest Labs](https://lnkd.in/esq9Yuv5)
- [Pentestlab](https://pentesterlab.com/)
- [Hackaflag BR](https://hackaflag.com.br/)
- [Penetration Testing Practice Labs](https://lnkd.in/e6wVANYd)
- [PWNABLE](https://lnkd.in/eMEwBJzn)
- [Root-Me](https://www.root-me.org/)
- [Root in Jail](http://rootinjail.com/)
- [SANS Challenger](https://lnkd.in/e5TAMawK)
- [SmashTheStack](https://lnkd.in/eVn9rP9p)
- [The Cryptopals Crypto Challenges](https://cryptopals.com/)
- [W3Challs](https://w3challs.com/)
- [WeChall](http://www.wechall.net/)
- [Zenk-Security](https://lnkd.in/ewJ5rNx2)
- [Cyber Defenders](https://lnkd.in/dVcmjEw8)
- [LetsDefend](https://letsdefend.io/)
- [Vulnmachines](https://vulnmachines.com/)
- [Rangeforce](https://www.rangeforce.com/)
- [Ctftime](https://ctftime.org/)
- [Pwn college](https://dojo.pwn.college/)
- [Free Money CTF](https://bugbase.in/)
- [PortSwigger Web Security Academy](https://portswigger.net/web-security)
- [OWASP Juice Shop](https://owasp.org/www-project-juice-shop/)
- [XSSGame](https://xss-game.appspot.com/)
- [BugBountyHunter](https://www.bugbountyhunter.com/)
- [DVWA](https://dvwa.co.uk/)
- [bWAPP](http://www.itsecgames.com/)
- [Metasploitable2](https://sourceforge.net/projects/metasploitable/files/Metasploitable2/)

## 🐧 Distros de Linux

- [Parrot Security](https://www.parrotsec.org/)  - Distribuição Parrot SecurityOS 
- [Kali Linux](https://www.kali.org) - Distribuição Linux Kali Linux
- [Black Arch Linux](https://blackarch.org/) - Distribuição Black Arch
- [Arch Linux](https://archlinux.org/) - Distribuição Linux Arch Linux
- [Pop!\_Os](https://pop.system76.com/) - Distribuição Linux Pop!\_Os
- [Debian](https://www.debian.org/) - Distribuição Linux Debian
- [Ubuntu](https://ubuntu.com/) - Distribuição Linux Ubuntu
- [Fedora](https://getfedora.org/pt_BR/) - Distribuição Linux Fedora
- [Linux Mint](https://linuxmint.com/) - Distribuição Linux Mint
- [OpenSUSE](https://www.opensuse.org) - Distribuição Linux OpenSUS
- [KDE Neon](https://www.neon.kde.org) - Distribuição Linux KDE Neon
- [Solus](https://www.getsol.us) - Distribuição Linux Solus
- [Tails](https://www.tails.boum.org) - Distribuição Linux Tails
- [Zorin OS](https://zorin.com/os/) - Distribuição Linux Zorin
- [Kubuntu](https://kubuntu.org/) - Distribuição Linux Kubuntu

## 💻 Máquinas Virtuais

- [Oracle VM VirtualBox](https://www.virtualbox.org/)
- [VMware Workstation](https://www.vmware.com/br/products/workstation-player/workstation-player-evaluation.html)
- [VMware Workstation Player](https://www.vmware.com/products/workstation-player.html)
- [VMware Fusion](https://www.vmware.com/br/products/fusion.html)
- [Vagrant](https://www.vagrantup.com/)

## 💰 Sites de Bug Bounty

- [Bug Crowd - Bug Bounty List](https://www.bugcrowd.com/bug-bounty-list/)

## 🦤 Perfis no Twitter

- [Ben Sadeghipour](https://twitter.com/NahamSec)
- [STÖK](https://twitter.com/stokfredrik)
- [TomNomNom](https://twitter.com/TomNomNom)
- [Shubs](https://twitter.com/infosec_au)
- [Emad Shanab](https://twitter.com/Alra3ees)
- [Payloadartist](https://twitter.com/payloadartist)
- [Remon](https://twitter.com/remonsec)
- [Aditya Shende](https://twitter.com/ADITYASHENDE17)
- [Hussein Daher](https://twitter.com/HusseiN98D)
- [The XSS Rat](https://twitter.com/theXSSrat)
- [zseano](https://twitter.com/zseano)
- [based god](https://twitter.com/hacker)
- [Vickie Li](https://twitter.com/vickieli7)
- [GodFather Orwa](https://twitter.com/GodfatherOrwa)
- [Ashish Kunwar](https://twitter.com/D0rkerDevil)
- [Farah Hawa](https://twitter.com/Farah_Hawaa)
- [Jason Haddix](https://twitter.com/Jhaddix)
- [Brute Logic](https://twitter.com/brutelogic)
- [Bug Bounty Reports Explained](https://twitter.com/gregxsunday)

## ✨ Perfis no Instagram

- [Hacking na Web (Rafael Sousa)](https://www.instagram.com/hackingnaweboficial/)
- [Acker Code | Tech & Ethical Hacking](https://www.instagram.com/ackercode/)
- [Hacking Club by Crowsec EdTech](https://www.instagram.com/hackingclub.io/)
- [Hacking Esports](https://www.instagram.com/hackingesports/)
- [XPSec | Pentest & Hacking](https://www.instagram.com/xpsecsecurity/)
- [Hérika Ströngreen | Hacking](https://www.instagram.com/herikastrongreen/)
- [Linux e Hacking](https://www.instagram.com/linux.gnu/)
- [Njay | Ethical Hacking](https://www.instagram.com/bountyhawk/)
- [Cyber Security/Ethical Hacking](https://www.instagram.com/thecyberw0rld/)
- [Load The Code](https://www.instagram.com/load_thecode/)
- [Learn ethical hacking](https://www.instagram.com/learn_hacking4/)
- [Cyber TechQ](https://www.instagram.com/cyber.techq/)
- [The Cyber Security Hub™](https://www.instagram.com/thecybersecurityhub/)
- [Darshit Pandey | Cyber Security](https://www.instagram.com/cyberrabitx/)
- [Harsha | Cyber Security](https://www.instagram.com/cyberrpreneur/)

## 🎇 Comunidades no Reddit

- [Cyber Security](https://www.reddit.com/r/cybersecurity/)
- [Hacking: security in practice](https://www.reddit.com/r/hacking/)
- [A forum for the security professionals and white hat hackers.](https://www.reddit.com/r/Hacking_Tutorials/)
- [Cybersecurity Career Discussion](https://www.reddit.com/r/CyberSecurityJobs/)
- [AskNetsec](https://www.reddit.com/r/AskNetsec/)
- [Subreddit for students studying Network Security](https://www.reddit.com/r/netsecstudents/)
- [Cyber Security Fórum](https://www.reddit.com/r/cyber_security/)
- [Reverse Engineering](https://www.reddit.com/r/ReverseEngineering/)
- [Red Team Security](https://www.reddit.com/r/redteamsec/)
- [Blue Team Security](https://www.reddit.com/r/blueteamsec/)
- [Purple Team Security](https://www.reddit.com/r/purpleteamsec/)

## 🌌 Comunidades no Discord

- [Boitatech](https://discord.gg/6bqBdyJ9PA)
- [Mente Binaria](https://menteb.in/discord)
- [Guia Anônima ](https://discord.gg/GzrMtXvuAM)
- [Comunidade Conecta](https://discord.gg/3hWYewJemP)
- [Central Help CTF](https://discord.gg/5xWJBXSaJe)
- [Menina do CyberSec](https://discord.gg/aCSxhGK6hK)
- [Hack4u](https://discord.gg/U84pHspusM)
- [Spyboy Cybersec](https://discord.gg/3mt6H67jjQ)
- [Ninjhacks Community](https://discord.gg/KkTxuWb4VX)
- [Code Society](https://discord.gg/pGHFyMZa46)
- [WhiteHat Hacking](https://discord.gg/XpmjtEGYYk)
- [Hacking & Coding](https://discord.gg/KawfcEnbXX)
- [Red Team Village](https://discord.gg/redteamvillage)
- [TryHackMe](https://discord.gg/JqAn7NNTtF)
- [DC Cyber Sec](https://discord.gg/dccybersec)
- [Try Hard Security](https://discord.gg/tryhardsecurity)
- [Linux Chat](https://discord.gg/linuxchat)
- [Cyber Jobs Hunting](https://discord.gg/SsBzsuQGBh)
- [NSL - Community](https://discord.gg/jhMuTYTNZv)
- [HackTheBox](https://discord.gg/hackthebox)
- [eLeanSecurity](https://discord.gg/F88c9XWQvM)
- [3DLock](https://discord.gg/rqsTBxxuGw)
- [Code Red](https://discord.gg/yYCRAApxwf)
- [The Cyber Council](https://discord.gg/CjYTbQTjQH)
- [Certification Station](https://discord.gg/certstation)
- [Bounty Hunters](https://discord.gg/AUFTZ5EkPZ)
- [Tech Raven](https://discord.gg/TFPuaXkweR)
- [The Cybersecurity Club](https://discord.gg/B4Av7acqXp)
- [ImaginaryCTF](https://discord.gg/M9J6GdqrE4)
- [Hack This Site](https://discord.gg/hts)
- [Cyber Badgers](https://discord.gg/wkXF9Gc44R)
- [TheBlackSide](https://discord.gg/pUeuzxvft7)

## 📚 Recomendações de livros

> Recomendação de livros para aprimoramento do conhecimento em Cyber Security em Português 
- [Introdução ao Pentest](https://www.amazon.com.br/Introdu%C3%A7%C3%A3o-ao-Pentest-Daniel-Moreno/dp/8575228072/ref=asc_df_8575228072/?tag=googleshopp00-20&linkCode=df0&hvadid=379773616949&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-850530960141&psc=1)
- [Pentest em Aplicações web](https://www.amazon.com.br/Pentest-Aplica%C3%A7%C3%B5es-Web-Daniel-Moreno/dp/8575226134/ref=pd_bxgy_img_sccl_1/145-1578869-2329559?pd_rd_w=2dTdj&content-id=amzn1.sym.57f5b0c5-8f2e-45a4-8595-2eb0fcbe85cd&pf_rd_p=57f5b0c5-8f2e-45a4-8595-2eb0fcbe85cd&pf_rd_r=DSSS27BQRN1MT8XSNT55&pd_rd_wg=smd9N&pd_rd_r=cc5197e3-0659-4e91-98fd-07a7b7b3c6aa&pd_rd_i=8575226134&psc=1)
- [Pentest em Redes sem fio](https://www.amazon.com.br/Pentest-em-Redes-sem-Fio/dp/8575224832/ref=pd_bxgy_img_sccl_2/145-1578869-2329559?pd_rd_w=2dTdj&content-id=amzn1.sym.57f5b0c5-8f2e-45a4-8595-2eb0fcbe85cd&pf_rd_p=57f5b0c5-8f2e-45a4-8595-2eb0fcbe85cd&pf_rd_r=DSSS27BQRN1MT8XSNT55&pd_rd_wg=smd9N&pd_rd_r=cc5197e3-0659-4e91-98fd-07a7b7b3c6aa&pd_rd_i=8575224832&psc=1)
- [Exploração de vulnerabilidades em redes TCP/IP](https://www.amazon.com.br/Explora%C3%A7%C3%A3o-vulnerabilidade-Rede-TCP-IP/dp/8550800708/ref=asc_df_8550800708/?tag=googleshopp00-20&linkCode=df0&hvadid=379765802390&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-423299859071&psc=1)
- [Algoritmos de Destruição em Massa](https://www.amazon.com.br/Algoritmos-Destrui%C3%A7%C3%A3o-Massa-Cathy-ONeil/dp/6586460026/ref=asc_df_6586460026/?tag=googleshopp00-20&linkCode=df0&hvadid=379792431986&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-1007895878384&psc=1)
- [Kali Linux. Introdução ao Penetration Testing](https://www.amazon.com.br/Kali-Linux-Introdu%C3%A7%C3%A3o-Penetration-Testing/dp/8539906236/ref=asc_df_8539906236/?tag=googleshopp00-20&linkCode=df0&hvadid=379787347388&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-421604521830&psc=1)
- [Criptografia e Segurança de Redes: Princípios e Práticas](https://www.amazon.com.br/Criptografia-seguran%C3%A7a-redes-princ%C3%ADpios-pr%C3%A1ticas/dp/8543005892/ref=asc_df_8543005892/?tag=googleshopp00-20&linkCode=df0&hvadid=379792581512&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-810094894442&psc=1)
- [Análise de Tráfego em Redes TCP/IP: Utilize Tcpdump na Análise de Tráfegos em Qualquer Sistema Operacional](https://www.amazon.com.br/An%C3%A1lise-Tr%C3%A1fego-Redes-TCP-IP/dp/8575223755/ref=asc_df_8575223755/?tag=googleshopp00-20&linkCode=df0&hvadid=379818494621&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-396355445891&psc=1)
- [Segurança de computadores e teste de invasão](https://www.amazon.com.br/Seguran%C3%A7a-computadores-teste-invas%C3%A3o-Alfred/dp/8522117993/ref=asc_df_8522117993/?tag=googleshopp00-20&linkCode=df0&hvadid=379765802390&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-394932359707&psc=1)
- [Ransomware: Defendendo-se da Extorsão Digital](https://www.amazon.com.br/Ransomware-Defendendo-Se-Extors%C3%A3o-Allan-Liska/dp/8575225510/ref=asc_df_8575225510/?tag=googleshopp00-20&linkCode=df0&hvadid=379818494621&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-812784633318&psc=1)
- [Fundamentos de Segurança da Informação: com Base na ISO 27001 e na ISO 27002](https://www.amazon.com.br/Fundamentos-Seguran%C3%A7a-Informa%C3%A7%C3%A3o-27001-27002/dp/8574528609/ref=asc_df_8574528609/?tag=googleshopp00-20&linkCode=df0&hvadid=379787347388&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-809202559856&psc=1)
- [Testes de Invasão: uma Introdução Prática ao Hacking](https://www.amazon.com.br/Testes-Invas%C3%A3o-Georgia-Weidman/dp/8575224077/ref=asc_df_8575224077/?tag=googleshopp00-20&linkCode=df0&hvadid=379739109739&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-332577553663&psc=1)
- [CISEF - Segurança Cibernética: Uma Questão de Sobrevivência](https://www.amazon.com.br/CISEF-Seguran%C3%A7a-Cibern%C3%A9tica-Quest%C3%A3o-Sobreviv%C3%AAncia/dp/B097TPYCGG/ref=asc_df_B097TPYCGG/?tag=googleshopp00-20&linkCode=df0&hvadid=379715964603&hvpos=&hvnetw=g&hvrand=3870620309104752989&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-1430488033379&psc=1)
- [Black Hat Python: Programação Python Para Hackers e Pentesters](https://www.amazon.com.br/Black-Hat-Python-Justin-Seitz/dp/8575224204)

> Recomendação de livros para aprimoramento do conhecimento em Cyber Security em Inglês
- [Hacking: The Art of Exploitation](https://www.amazon.com.br/Hacking-Exploitation-CDROM-Jon-Erickson/dp/1593271441)
- [Penetration Testing: A Hands-On Introduction to Hacking](https://www.amazon.com.br/Penetration-Testing-Hands-Introduction-Hacking/dp/1593275641)
- [The Hacker Playbook 2: Practical Guide to Penetration Testing](https://www.amazon.com.br/Hacker-Playbook-Practical-Penetration-Testing/dp/1512214566)
- [The Basics of Hacking and Penetration Testing: Ethical Hacking and Penetration Testing Made Easy](https://www.amazon.com.br/Basics-Hacking-Penetration-Testing-Ethical/dp/0124116442)
- [The Hacker Playbook 3: Practical Guide To Penetration Testing](https://www.amazon.com.br/Hacker-Playbook-Practical-Penetration-Testing-ebook/dp/B07CSPFYZ2)
- [The Web Application Hacker's Handbook: Finding and Exploiting Security Flaws](https://www.amazon.com.br/Web-Application-Hackers-Handbook-Exploiting/dp/1118026470)
- [Web Hacking 101](https://www.goodreads.com/book/show/33596532-web-hacking-101)
- [Mastering Modern Web Penetration Testing](https://www.amazon.com.br/Mastering-Modern-Penetration-Testing-English-ebook/dp/B01GVMSTEO)
- [Bug Bounty Playbook](https://payhip.com/b/wAoh)
- [Real-World Bug Hunting: A Field Guide to Web Hacking](https://www.amazon.com.br/Real-World-Bug-Hunting-Field-Hacking/dp/1593278616)
- [OWASP Testing Guide V10](https://owasp.org/www-project-web-security-testing-guide/assets/archive/OWASP_Testing_Guide_v4.pdf)
- [Black Hat Python: Python Programming for Hackers and Pentesters](https://www.amazon.com.br/Black-Hat-Python-Programming-Pentesters/dp/1593275900/ref=asc_df_1593275900/?tag=googleshopp00-20&linkCode=df0&hvadid=379726160779&hvpos=&hvnetw=g&hvrand=12817915842755546773&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-406163956473&psc=1)
- [Black Hat Python, 2nd Edition: Python Programming for Hackers and Pentesters](https://www.amazon.com.br/Black-Hat-Python-2nd-Programming/dp/1718501129/ref=asc_df_1718501129/?tag=googleshopp00-20&linkCode=df0&hvadid=379787788238&hvpos=&hvnetw=g&hvrand=12817915842755546773&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-1129943149832&psc=1)
- [Black Hat Go: Go Programming for Hackers and Pentesters](https://www.amazon.com.br/Black-Hat-Go-Programming-Pentesters/dp/1593278659/ref=asc_df_1593278659/?tag=googleshopp00-20&linkCode=df0&hvadid=379787788238&hvpos=&hvnetw=g&hvrand=12817915842755546773&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-872661430541&psc=1)
- [Advanced Penetration Testing: Hacking the World's Most Secure Networks](https://www.amazon.com.br/Advanced-Penetration-Testing-Hacking-Networks/dp/1119367689)
- [Gray Hat Hacking: The Ethical Hacker's Handbook ](https://www.amazon.com.br/Gray-Hat-Hacking-Ethical-Handbook/dp/0072257091)
- [Social Engineering: The Art of Human Hacking](https://www.amazon.com.br/Social-Engineering-Art-Human-Hacking/dp/0470639539)
- [Social Engineering: The Science of Human Hacking](https://www.amazon.com.br/Social-Engineering-Science-Human-Hacking/dp/111943338X/ref=asc_df_111943338X/?tag=googleshopp00-20&linkCode=df0&hvadid=379726160779&hvpos=&hvnetw=g&hvrand=10534013289063384157&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-490758470823&psc=1)
- [Practical Social Engineering: A Primer for the Ethical Hacker](https://www.amazon.com.br/Practical-Social-Engineering-Joe-Gray/dp/171850098X/ref=asc_df_171850098X/?tag=googleshopp00-20&linkCode=df0&hvadid=379735814613&hvpos=&hvnetw=g&hvrand=10534013289063384157&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-934732928526&psc=1)
- [Practical Malware Analysis: The Hands-On Guide to Dissecting Malicious Software](https://www.amazon.com.br/Practical-Malware-Analysis-Hands-Dissecting/dp/1593272901/ref=asc_df_1593272901/?tag=googleshopp00-20&linkCode=df0&hvadid=379735814613&hvpos=&hvnetw=g&hvrand=18239998534715401467&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-406163956073&psc=1)
- [Practical Binary Analysis: Build Your Own Linux Tools for Binary Instrumentation, Analysis, and Disassembly](https://www.amazon.com.br/Practical-Binary-Analysis-Instrumentation-Disassembly/dp/1593279124/ref=asc_df_1593279124/?tag=googleshopp00-20&linkCode=df0&hvadid=379726160779&hvpos=&hvnetw=g&hvrand=18239998534715401467&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-525099683939&psc=1)
- [Rootkits and Bootkits: Reversing Modern Malware and Next Generation Threats](https://www.amazon.com.br/Rootkits-Bootkits-Reversing-Malware-Generation/dp/1593277164/ref=asc_df_1593277164/?tag=googleshopp00-20&linkCode=df0&hvadid=379735814613&hvpos=&hvnetw=g&hvrand=18239998534715401467&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-326856398373&psc=1)
- [Malware Data Science: Attack Detection and Attribution](https://www.amazon.com.br/Malware-Data-Science-Detection-Attribution/dp/1593278594/ref=asc_df_1593278594/?tag=googleshopp00-20&linkCode=df0&hvadid=379726160779&hvpos=&hvnetw=g&hvrand=18239998534715401467&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-526160276073&psc=1)
- [The Art of Mac Malware: The Guide to Analyzing Malicious Software](https://www.amazon.com.br/Art-Mac-Malware-Analyzing-Malicious/dp/1718501943/ref=asc_df_1718501943/?tag=googleshopp00-20&linkCode=df0&hvadid=379726160779&hvpos=&hvnetw=g&hvrand=18239998534715401467&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-1435226984335&psc=1)
- [Android Hacker's Handbook](https://www.amazon.com.br/Android-Hackers-Handbook-Joshua-Drake/dp/111860864X/ref=asc_df_111860864X/?tag=googleshopp00-20&linkCode=df0&hvadid=379735814613&hvpos=&hvnetw=g&hvrand=18239998534715401467&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=1001506&hvtargid=pla-459716102046&psc=1)
- [Metasploit: The Penetration Tester's Guide](https://www.amazon.com.br/Metasploit-Penetration-Testers-David-Kennedy/dp/159327288X)
- [Rtfm: Red Team Field Manual](https://www.amazon.com.br/Rtfm-Red-Team-Field-Manual/dp/1494295504)
- [Blue Team Field Manual (BTFM)](https://www.amazon.com.br/Blue-Team-Field-Manual-Btfm/dp/154101636X)

## 🛠️ Frameworks e ferramentas de Hacking Web

- [Burp Suite](https://portswigger.net/burp) - Framework.
- [ZAP Proxy](https://www.zaproxy.org/) - Framework.
- [Dirsearch](https://github.com/maurosoria/dirsearch) - HTTP bruteforcing.
- [Nmap](https://nmap.org/) - Port scanning.
- [Sublist3r](https://github.com/aboul3la/Sublist3r) - Subdomain discovery.
- [Amass](https://github.com/OWASP/Amass) - Subdomain discovery.
- [SQLmap](https://sqlmap.org/) - SQLi exploitation.
- [Metasploit](https://www.metasploit.com/) - Framework
- [WPscan](https://wpscan.com/wordpress-security-scanner) - WordPress exploitation.
- [Nikto](https://github.com/sullo/nikto) - Webserver scanning.
- [HTTPX](https://github.com/projectdiscovery/httpx) - HTTP probing.
- [Nuclei](https://github.com/projectdiscovery/nuclei) - YAML based template scanning.
- [FFUF](https://github.com/ffuf/ffuf) - HTTP probing.
- [Subfinder](https://github.com/projectdiscovery/subfinder) - Subdomain discovery.
- [Masscan](https://github.com/robertdavidgraham/masscan) - Mass IP and port scanner.
- [Lazy Recon](https://github.com/nahamsec/lazyrecon) - Subdomain discovery.
- [XSS Hunter](https://xsshunter.com/) - Blind XSS discovery.
- [Aquatone](https://github.com/michenriksen/aquatone) - HTTP based recon.
- [LinkFinder](https://github.com/GerbenJavado/LinkFinder) - Endpoint discovery through JS files.
- [JS-Scan](https://github.com/0x240x23elu/JSScanner) - Endpoint discovery through JS files.
- [Parameth](https://github.com/maK-/parameth) - Bruteforce GET and POST parameters.
- [truffleHog](https://github.com/trufflesecurity/trufflehog) - Encontrar credenciais em commits do GitHub.

## 🪓 Ferramentas para obter informações 

- [theHarvester](https://github.com/laramies/theHarvester) - E-mails, subdomínios e nomes Harvester.
- [CTFR](https://github.com/UnaPibaGeek/ctfr) - Abusando de logs de transparência de certificado para obter subdomínios de sites HTTPS.
- [Sn1per](https://github.com/1N3/Sn1per) - Scanner automatizado de reconhecimento de pentest.
- [RED Hawk](https://github.com/Tuhinshubhra/RED_HAWK) - Tudo em uma ferramenta para coleta de informações, verificação de vulnerabilidades e rastreamento. Uma ferramenta obrigatória para todos os testadores de penetração.
- [Infoga](https://github.com/m4ll0k/Infoga) - Coleta de informações de e-mail.
- [KnockMail](https://github.com/4w4k3/KnockMail) - Verifique se o endereço de e-mail existe.
- [a2sv](https://github.com/hahwul/a2sv) - Verificação automática para vulnerabilidade SSL.
- [Wfuzz](https://github.com/xmendez/wfuzz) - Fuzzer de aplicativos da web.
- [Nmap](https://github.com/nmap/nmap) - Uma ferramenta muito comum. Host de rede, vuln e detector de porta.
- [PhoneInfoga](https://github.com/sundowndev/PhoneInfoga) - 	Uma estrutura OSINT para números de telefone.

## 🔧 Ferramentas para Pentesting

- [Pentest Tools](https://github.com/gwen001/pentest-tools)
- [Hacktronian Tools](https://github.com/thehackingsage/hacktronian)
- [Linux Smart Enumeration](https://github.com/diego-treitos/linux-smart-enumeration)
- [Infection Monkey](https://github.com/guardicore/monkey)
- [Xerror](https://github.com/Chudry/Xerror)
- [Mongoaudit](https://github.com/stampery/mongoaudit)
- [Pentesting Scripts](https://github.com/killswitch-GUI/PenTesting-Scripts)
- [TxTool](https://github.com/kuburan/txtool)
- [All Pentesting Tools](https://github.com/nullsecuritynet/tools)

## 🔨 Ferramentas para Hardware Hacking

- [Multímetro Digital](http://s.click.aliexpress.com/e/_d8he3mb)
- [Módulo conversor FT232RL Usb para TTL](http://s.click.aliexpress.com/e/_dSIAjWL)
- [Ch341A](http://s.click.aliexpress.com/e/_d62fRI3)
- [Bus Pirate](http://s.click.aliexpress.com/e/_dUPIrJ9)
- [SOP8 Clip](http://s.click.aliexpress.com/e/_dVZ9XFN)
- [Arduino Uno R3](http://s.click.aliexpress.com/e/_dW85MoT)
- [Osciloscópio Instrustar](http://s.click.aliexpress.com/e/_d80YjJl)
- [Arduino Nano](http://s.click.aliexpress.com/e/_dZj36oL)
- [Arduino Uno R3](http://s.click.aliexpress.com/e/_dXsrRxz)
- [Arduino Pro Micro](http://s.click.aliexpress.com/e/_dSSuhuX)
- [Esp8266](http://s.click.aliexpress.com/e/_dVzK5qj)
- [Esp32](http://s.click.aliexpress.com/e/_d7orFfH)
- [Arduino Micro SS](http://s.click.aliexpress.com/e/_d8Vrda3)
- [Digispark](http://s.click.aliexpress.com/e/_dZfgtbl)
- [Proxmark3](http://s.click.aliexpress.com/e/_dUTFHmL)
- [Gravador de RFID](http://s.click.aliexpress.com/e/_dTFhbsX)
- [Esp8266](http://s.click.aliexpress.com/e/_d8lGkzd)
- [Analisador Lógico](http://s.click.aliexpress.com/e/_d9e9PDD)
- [Raspberry Pi 0 W](http://s.click.aliexpress.com/e/_Bf7UqZxN)
- [Pickit 3]( http://s.click.aliexpress.com/e/_dYwoTqL)
- [Ft232h](http://s.click.aliexpress.com/e/_dUpL9XN)
- [Ft232h](http://s.click.aliexpress.com/e/_dVVWLrH)
- [M5stickC](http://s.click.aliexpress.com/e/_dVbh4T1)
- [M5 Atom](http://s.click.aliexpress.com/e/_dTaCid5)
- [Testador de componentes](http://s.click.aliexpress.com/e/_dUBXjzt)
- [Projeto de testador de componentes](http://s.click.aliexpress.com/e/_d6tbMnv)
- [Microscópio](http://s.click.aliexpress.com/e/_dZQ8RIj)
- [Ferro de solda TS100](http://s.click.aliexpress.com/e/_d82rnhh)
- [RT809h](https://pt.aliexpress.com/item/32747164846.html?spm=a2g0o.productlist.0.0.25cc3923P5cVXZ&algo_pvid=4d740e28-334f-43e9-938d-aee16699cc41&algo_expid=4d740e28-334f-43e9-938d-aee16699cc41-8&btsid=0ab6f82c15912356671523042efcb7&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
- [RTL-SDR](http://s.click.aliexpress.com/e/_dUIw0ll)
- [Hackrf + Portapack h2](http://s.click.aliexpress.com/e/_dS0V9kf)


## 🦉 Sites e cursos para aprender C

> Cursos para aprender C em Português 
- [Curso de C - eXcript](https://www.youtube.com/playlist?list=PLesCEcYj003SwVdufCQM5FIbrOd0GG1M4)
- [Programação Moderna em C - Papo Binário](https://www.youtube.com/playlist?list=PLIfZMtpPYFP5qaS2RFQxcNVkmJLGQwyKE)
- [Curso de Linguagem C - Pietro Martins](https://www.youtube.com/playlist?list=PLpaKFn4Q4GMOBAeqC1S5_Fna_Y5XaOQS2)
- [Curso de Programação C Completo - Programe seu futuro](https://www.youtube.com/playlist?list=PLqJK4Oyr5WSjjEQCKkX6oXFORZX7ro3DA)
- [Linguagem C - De aluno para aluno](https://www.youtube.com/playlist?list=PLa75BYTPDNKZWYypgOFEsX3H2Mg-SzuLW)
- [Curso de Linguagem C para Iniciantes - John Haste](https://www.youtube.com/playlist?list=PLGgRtySq3SDMLV8ee7p-rA9y032AU3zT8)
- [Curso de Linguagem C (ANSI)](https://www.youtube.com/playlist?list=PLZ8dBTV2_5HTGGtrPxDB7zx8J5VMuXdob)
- [Curso - Programação com a Linguagem C para iniciantes](https://www.youtube.com/playlist?list=PLbEOwbQR9lqxHno2S-IiG9-lePyRNOO_E)
- [Curso de Programação 3 (C Avançado)](https://www.youtube.com/playlist?list=PLxMw67OGLa0kW_TeweK2-9gXRlMLYzC1o)
- [Curso de C - Diego Moisset](https://www.youtube.com/playlist?list=PLIygiKpYTC_6zHLTjI6cFIRZm1BCT3CuV)
- [Curso de C e C++](https://www.youtube.com/playlist?list=PL5EmR7zuTn_bONyjFxSO4ZCE-SVVNFGkS)
- [Curso de Programação em Linguagem C](https://www.youtube.com/playlist?list=PLucm8g_ezqNqzH7SM0XNjsp25AP0MN82R)
- [Linguagem C - Curso de Programação Completo para Iniciantes e Profissionais](https://www.youtube.com/playlist?list=PLrqNiweLEMonijPwsHckWX7fVbgT2jS3P)
- [Curso de Lógica e programação em C](https://www.youtube.com/playlist?list=PLtnFngjANe7EMzARU48QgecpyQdzWapoT)

> Cursos para aprender C em Inglês
- [C Programming for Beginners](https://www.youtube.com/playlist?list=PL98qAXLA6aftD9ZlnjpLhdQAOFI8xIB6e)
- [C Programming - Neso Academy](https://www.youtube.com/playlist?list=PLBlnK6fEyqRggZZgYpPMUxdY1CYkZtARR)
- [C Programming & Data Structures](https://www.youtube.com/playlist?list=PLBlnK6fEyqRhX6r2uhhlubuF5QextdCSM)
- [Programming in C - Jennys](https://www.youtube.com/playlist?list=PLdo5W4Nhv31a8UcMN9-35ghv8qyFWD9_S)
- [C Language Tutorials In Hindi](https://www.youtube.com/playlist?list=PLu0W_9lII9aiXlHcLx-mDH1Qul38wD3aR)
- [freeCodeCamp C / C++](https://www.youtube.com/playlist?list=PLWKjhJtqVAbmUE5IqyfGYEYjrZBYzaT4m)
- [C Programming Tutorials](https://www.youtube.com/playlist?list=PL_c9BZzLwBRKKqOc9TJz1pP0ASrxLMtp2)
- [C Language Tutorial Videos - Mr. Srinivas](https://www.youtube.com/playlist?list=PLVlQHNRLflP8IGz6OXwlV_lgHgc72aXlh)
- [Advanced C Programming](https://www.youtube.com/playlist?list=PL7CZ_Xc0qgmJFqNWEt4LIhAPTlT0sCW4C)
- [Free Online Programming Course in C for Beginners](https://www.youtube.com/playlist?list=PL76809ED684A081F3)
- [C Programming - Ankpro](https://www.youtube.com/playlist?list=PLUtTaqnx2RJLSUZgv0zp0aNWy9e1cbKd9)
- [C Programming Tutorials - The New Boston](https://www.youtube.com/playlist?list=PL6gx4Cwl9DGAKIXv8Yr6nhGJ9Vlcjyymq)
- [C Programming - IntelliPaat](https://www.youtube.com/playlist?list=PLVHgQku8Z935hrZwx751XoyqDROH_tYMY)
- [Learn C programming - edureka!](https://www.youtube.com/playlist?list=PL9ooVrP1hQOFrNo8jK9Yb2g2eMHz7hTu9)
- [C Programming Tutorials - Saurabh Shukla](https://www.youtube.com/playlist?list=PLe_7x5eaUqtWp9fvsxhC4XIkoR3n5A-sF)

## 🐬 Sites e cursos para aprender Go

> Sites para aprender Go
- [Onde aprender e estudar GoLang?](https://coodesh.com/blog/candidates/backend/onde-aprender-e-estudar-golang/#:~:text=%E2%8C%A8%EF%B8%8F%20Udemy,11%2C5%20horas%20de%20videoaula.)
- [Go Lang - School Of Net](https://www.schoolofnet.com/cursos/programacao/go-lang/)
- [48 horas para aprender Go](https://medium.com/@anapaulagomes/48-horas-para-aprender-go-4542b51d84a4)
- [Estudo em GoLang: from Zero to Hero com materiais gratuitos!](https://medium.com/hurb-labs/estudo-em-golang-from-zero-to-hero-com-materiais-gratuitos-6be72aeea30f)

> Cursos para aprender Go em Português 
- [Aprenda Go](https://www.youtube.com/playlist?list=PLCKpcjBB_VlBsxJ9IseNxFllf-UFEXOdg)
- [Aprenda Go / Golang (Curso Tutorial de Programação)](https://www.youtube.com/playlist?list=PLUbb2i4BuuzCX8CLeArvx663_0a_hSguW)
- [Go Lang do Zero](https://www.youtube.com/playlist?list=PL5aY_NrL1rjucQqO21QH8KclsLDYu1BIg)
- [Curso de Introdução a Linguagem Go (Golang)](https://www.youtube.com/playlist?list=PLXFk6ROPeWoAvLMyJ_PPfu8oF0-N_NgEI)
- [Curso Programação Golang](https://www.youtube.com/playlist?list=PLpZslZJHL2Q2hZXShelGADqCR_fcOhF9K)

> Cursos para aprender Go em Espanhol
- [Curso de Go (Golang)](https://www.youtube.com/playlist?list=PLt1J5u9LpM5-L-Ps8jjr91pKhFxAnxKJp)
- [Aprendiendo a programar con Go](https://www.youtube.com/playlist?list=PLSAQnrUqbx7sOdjJ5Zsq5FvvYtI8Kc-C5)
- [Curso Go - de 0 a 100](https://www.youtube.com/playlist?list=PLhdY0D_lA34W1wS2nJmQr-sssMDuQf-r8)
- [Curso Go - CodigoFacilito](https://www.youtube.com/playlist?list=PLdKnuzc4h6gFmPLeous4S0xn0j9Ik2s3Y)
- [Curso GO (Golang Español) - De 0 a 100](https://www.youtube.com/playlist?list=PLl_hIu4u7P64MEJpR3eVwQ1l_FtJq4a5g)
- [Curso de Golang para principiante](https://www.youtube.com/playlist?list=PLm28buT4PAtbsurufxiw9k2asnkin4YLd)

> Cursos para aprender Go em Inglês
- [Golang Tutorial for Beginners | Full Go Course](https://www.youtube.com/watch?v=yyUHQIec83I&ab_channel=TechWorldwithNana)
- [Learn Go Programming - Golang Tutorial for Beginners](https://www.youtube.com/watch?v=YS4e4q9oBaU&ab_channel=freeCodeCamp.org)
- [Backend master class [Golang, Postgres, Docker]](https://www.youtube.com/playlist?list=PLy_6D98if3ULEtXtNSY_2qN21VCKgoQAE)
- [Let's go with golang](https://www.youtube.com/playlist?list=PLRAV69dS1uWQGDQoBYMZWKjzuhCaOnBpa)
- [Go Programming Language Tutorial | Golang Tutorial For Beginners | Go Language Training](https://www.youtube.com/playlist?list=PLS1QulWo1RIaRoN4vQQCYHWDuubEU8Vij)
- [Golang Tutorials](https://www.youtube.com/playlist?list=PLzMcBGfZo4-mtY_SE3HuzQJzuj4VlUG0q)
- [Golang course - Duomly](https://www.youtube.com/playlist?list=PLi21Ag9n6jJJ5bq77cLYpCgOaONcQNqm0)
- [Golang Course - Evhenii Kozlov](https://www.youtube.com/playlist?list=PLgUAJTkYL6T_-PXWgVFGTkz863zZ_1do0)
- [Golang Development](https://www.youtube.com/playlist?list=PLzUGFf4GhXBL4GHXVcMMvzgtO8-WEJIoY)
- [Golang Crash Course](https://www.youtube.com/playlist?list=PL3eAkoh7fypqUQUQPn-bXtfiYT_ZSVKmB)
- [Golang Course From A to Z - 5 Hours of Video](https://www.youtube.com/playlist?list=PLuMFwYAgU7ii-z4TGGqXh1cJt-Dqnk2eY)

## 🦚 Sites e cursos para aprender C#

> Cursos para aprender C# em Português 
- [Curso de C# - Aprenda o essencial em 5 HORAS](https://www.youtube.com/watch?v=PKMm-cHe56g&ab_channel=VictorLima-GuiadoProgramador)
- [Curso de Programação C#](https://www.youtube.com/playlist?list=PLx4x_zx8csUglgKTmgfVFEhWWBQCasNGi)
- [Curso C# 2021](https://www.youtube.com/playlist?list=PL50rZONmv8ZTLPRyqb37EoPlBpSmVBJWX)
- [Curso de C# para Iniciantes](https://www.youtube.com/playlist?list=PLwftZeDnOzt3VMtat5BTJvP_7qgNtRDD8)
- [Linguagem C#](https://www.youtube.com/playlist?list=PLEdPHGYbHhlcxWx-_LrVVYZ2RRdqltums)
- [C# - De Novato a Profissional](https://www.youtube.com/playlist?list=PLXik_5Br-zO-rMqpRy5qPG2SLNimKmVCO)
- [Curso de C#](https://www.youtube.com/playlist?list=PLesCEcYj003SFffgnOcITHnCJavMf0ArD)
- [Curso de C# - Pildoras Informaticas](https://www.youtube.com/playlist?list=PLU8oAlHdN5BmpIQGDSHo5e1r4ZYWQ8m4B)
- [Curso de C# Básico e Avançado](https://www.youtube.com/playlist?list=PLxNM4ef1BpxgRAa5mGXlCoSGyfYau8nZI)
- [Curso de Programação em C#](https://www.youtube.com/playlist?list=PLO_xIfla8f1wDmI0Vd4YJLKBJhOeQ3xbz)
- [Curso de Programação com C#](https://www.youtube.com/playlist?list=PLucm8g_ezqNoMPIGWbRJXemJKyoUpTjA1)
- [Curso Básico de C#](https://www.youtube.com/playlist?list=PL0YuSuacUEWsHR_a22z31bvA2heh7iUgr)
- [Curso de Desenvolvimento de Sistemas - C# com SQL](https://www.youtube.com/playlist?list=PLxNM4ef1BpxjLIq-eTL8mgROdviCiobs9)
- [Curso de C# - Diego Moisset](https://www.youtube.com/playlist?list=PLIygiKpYTC_400MCSyUlje1ifmFltonuN)
- [C# - Programação Orientada a Objetos](https://www.youtube.com/playlist?list=PLfvOpw8k80Wreysmw8fonLCBw8kiiYjIU)
- [Curso .NET Core C#](https://www.youtube.com/playlist?list=PLs3yd28pfby7WLEdA7GXey47cKZKMrcwS)
- [Curso de C# com Entity - CSharp com SQL](https://www.youtube.com/playlist?list=PLxNM4ef1BpxgIUUueLguueyhx0UuICC3-)
- [Curso de C# com MVC e SQL](https://www.youtube.com/playlist?list=PLxNM4ef1Bpxgilp2iFXI4i2if6Qtg6qFZ)

> Cursos para aprender C# em Espanhol
- [Curso C# de 0 a Experto](https://www.youtube.com/playlist?list=PLvMLybJwXhLEVUlBI2VdmYXPARO2Zwxze)
- [Tutorial C# - Curso básico](https://www.youtube.com/playlist?list=PLM-p96nOrGcakia6TWllPW9lkQmB2g-yX)
- [Aprende a programar en C# desde CERO](https://www.youtube.com/playlist?list=PL8gxzfBmzgexdFa0XZZSZZn2Ogx3j-Qd5)

> Cursos para aprender C# em Inglês
- [C# Tutorial - Full Course for Beginners](https://www.youtube.com/watch?v=GhQdlIFylQ8&ab_channel=freeCodeCamp.org)
- [C# Full Course - Learn C# 10 and .NET 6 in 7 hours](https://www.youtube.com/watch?v=q_F4PyW8GTg&ab_channel=tutorialsEU)
- [C# Tutorial: Full Course for Beginners](https://www.youtube.com/watch?v=wxznTygnRfQ&ab_channel=BroCode)
- [C# Fundamentals for Beginners](https://www.youtube.com/watch?v=0QUgvfuKvWU&ab_channel=MicrosoftDeveloper)
- [C# Tutorial For Beginners - Learn C# Basics in 1 Hour](https://www.youtube.com/watch?v=gfkTfcpWqAY&ab_channel=ProgrammingwithMosh)
- [C# for Beginners | Full 2-hour course](https://www.youtube.com/watch?v=Z5JS36NlJiU&ab_channel=dotnet)
- [C# Programming All-in-One Tutorial Series (6 HOURS!)](https://www.youtube.com/watch?v=qOruiBrXlAw&ab_channel=CalebCurry)
- [Create a C# Application from Start to Finish - Complete Course](https://www.youtube.com/watch?v=wfWxdh-_k_4&ab_channel=freeCodeCamp.org)
- [C# Tutorials](https://www.youtube.com/playlist?list=PL_c9BZzLwBRIXCJGLd4UzqH34uCclOFwC)
- [C# Mastery Course](https://www.youtube.com/playlist?list=PLrW43fNmjaQVSmaezCeU-Hm4sMs2uKzYN)
- [C# Full Course Beginner to Advanced](https://www.youtube.com/playlist?list=PLq5Uz3LSFff8GmtFeoXRZCtWBKQ0kWl-H)
- [C# Tutorial For Beginners & Basics - Full Course 2022](https://www.youtube.com/playlist?list=PL82C6-O4XrHfoN_Y4MwGvJz5BntiL0z0D)
- [C# for Beginners Course](https://www.youtube.com/playlist?list=PL4LFuHwItvKbneXxSutjeyz6i1w32K6di)
- [C# tutorial for beginners](https://www.youtube.com/playlist?list=PLAC325451207E3105)
- [C# Online Training](https://www.youtube.com/playlist?list=PLWPirh4EWFpFYePpf3E3AI8LT4NInNoIM)
- [C# Training](https://www.youtube.com/playlist?list=PLEiEAq2VkUULDJ9tZd3lc0rcH4W5SNSoW)
- [C# for Beginners](https://www.youtube.com/playlist?list=PLdo4fOcmZ0oVxKLQCHpiUWun7vlJJvUiN)
- [C# - Programming Language | Tutorial](https://www.youtube.com/playlist?list=PLLAZ4kZ9dFpNIBTYHNDrhfE9C-imUXCmk)
- [C#.NET Tutorials](https://www.youtube.com/playlist?list=PLTjRvDozrdlz3_FPXwb6lX_HoGXa09Yef)

## 🐸 Sites e cursos para aprender C++

> Cursos para aprender C++ em Português 
- [Curso C++ - eXcript](https://www.youtube.com/playlist?list=PLesCEcYj003QTw6OhCOFb1Fdl8Uiqyrqo)
- [Curso de C e C++ - Daves Tecnologia](https://www.youtube.com/playlist?list=PL5EmR7zuTn_bONyjFxSO4ZCE-SVVNFGkS)
- [Curso Programação em C/C++](https://www.youtube.com/playlist?list=PLC9E87254BD7A875B)
- [Curso C++ para iniciantes](https://www.youtube.com/playlist?list=PL8eBmR3QtPL13Dkn5eEfmG9TmzPpTp0cV)
- [Curso de C++ e C#](https://www.youtube.com/playlist?list=PLxNM4ef1Bpxhro_xZd-PCUDUsgg8tZFKh)
- [Curso C++](https://www.youtube.com/playlist?list=PL6xP0t6HQYWcUPcXLu2XTZ3gOCJSmolgO)
- [Curso de C++ - A linguagem de programação fundamental para quem quer ser um programador](https://www.youtube.com/playlist?list=PLx4x_zx8csUjczg1qPHavU1vw1IkBcm40)

> Cursos para aprender C++ em Espanhol
- [Programación en C++](https://www.youtube.com/playlist?list=PLWtYZ2ejMVJlUu1rEHLC0i_oibctkl0Vh)
- [Curso en C++ para principiantes](https://www.youtube.com/playlist?list=PLDfQIFbmwhreSt6Rl2PbDpGuAEqOIPmEu)
- [C++ desde cero](https://www.youtube.com/playlist?list=PLAzlSdU-KYwWsM0FgOs4Jqwnr5zhHs0wU)
- [Curso de Interfaces Graficas en C/C++](https://www.youtube.com/playlist?list=PLYA44wBp7zVTiCJiXIC5H5OkMOXptxLOI)

> Cursos para aprender C++ em Inglês
- [C++ Programming Course - Beginner to Advanced 31 hours](https://www.youtube.com/watch?v=8jLOx1hD3_o&ab_channel=freeCodeCamp.org) 
- [C++ Full Course For Beginners (Learn C++ in 10 hours)](https://www.youtube.com/watch?v=GQp1zzTwrIg&ab_channel=CodeBeauty) 
- [C++ Tutorial for Beginners - Learn C++ in 1 Hour](https://www.youtube.com/watch?v=ZzaPdXTrSb8&ab_channel=ProgrammingwithMosh) 
- [C++ Tutorial: Full Course for Beginners](https://www.youtube.com/watch?v=-TkoO8Z07hI&ab_channel=BroCode) 
- [C++ Tutorial for Beginners - Complete Course](https://www.youtube.com/watch?v=vLnPwxZdW4Y&ab_channel=freeCodeCamp.org) 
- [C++ Programming All-in-One Tutorial Series (10 HOURS!)](https://www.youtube.com/watch?v=_bYFu9mBnr4&ab_channel=CalebCurry) 
- [C++ Full Course 2022](https://www.youtube.com/watch?v=SYd5F4gIH90&ab_channel=Simplilearn) 
- [C++ Crash Course](https://www.youtube.com/watch?v=uhFpPlMsLzY&ab_channel=BroCode) 
- [C++ - The Cherno](https://www.youtube.com/playlist?list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb) 
- [C++ Full Course | C++ Tutorial | Data Structures & Algorithms](https://www.youtube.com/playlist?list=PLfqMhTWNBTe0b2nM6JHVCnAkhQRGiZMSJ) 
- [C++ Programming - Neso Academy](https://www.youtube.com/playlist?list=PLBlnK6fEyqRh6isJ01MBnbNpV3ZsktSyS) 
- [C++ Complete Course](https://www.youtube.com/playlist?list=PLdo5W4Nhv31YU5Wx1dopka58teWP9aCee) 
- [C++ Tutorials In Hindi](https://www.youtube.com/playlist?list=PLu0W_9lII9agpFUAlPFe_VNSlXW5uE0YL) 
- [C++ Online Training](https://www.youtube.com/playlist?list=PLWPirh4EWFpGDG3--IKMLPoYrgfuhaz_t) 
- [C / C++ - freeCodeCamp Playlist](https://www.youtube.com/playlist?list=PLWKjhJtqVAbmUE5IqyfGYEYjrZBYzaT4m) 
- [C++ Modern Tutorials](https://www.youtube.com/playlist?list=PLgnQpQtFTOGRM59sr3nSL8BmeMZR9GCIA) 

## 🐘 Sites e cursos para aprender PHP

> Cursos para aprender PHP em Português 
- [Curso de PHP para Iniciantes](https://www.youtube.com/playlist?list=PLHz_AreHm4dm4beCCCmW4xwpmLf6EHY9k)
- [Curso de PHP - Node Studio](https://www.youtube.com/playlist?list=PLwXQLZ3FdTVEITn849NlfI9BGY-hk1wkq)
- [Curso de PHP - CFBCursos](https://www.youtube.com/playlist?list=PLx4x_zx8csUgB4R1dDXke4uKMq-IrSr4B)
- [Curso de PHP 8 Completo](https://www.youtube.com/playlist?list=PLXik_5Br-zO9wODVI0j58VuZXkITMf7gZ)
- [Curso de PHP - eXcript](https://www.youtube.com/playlist?list=PLesCEcYj003TrV2MvUOnmVtMdgIp0C4Pd)
- [Curso de PHP Orientado a Objetos](https://www.youtube.com/playlist?list=PLwXQLZ3FdTVEau55kNj_zLgpXL4JZUg8I)
- [Curso de PHP8 Completo - Intermédio e Avançado](https://www.youtube.com/playlist?list=PLXik_5Br-zO9Z8l3CE8zaIBkVWjHOboeL)
- [Curso de PHP](https://www.youtube.com/playlist?list=PLBFB56E8115533B6C)
- [Curso de POO PHP (Programação Orientada a Objetos)](https://www.youtube.com/playlist?list=PLHz_AreHm4dmGuLII3tsvryMMD7VgcT7x)
- [Curso de PHP 7 Orientado a Objetos](https://www.youtube.com/playlist?list=PLnex8IkmReXz6t1rqxB-W17dbvfSL1vfg)
- [Curso de PHP 7](https://www.youtube.com/playlist?list=PLnex8IkmReXw-QlzKS9zA3rXQsRnK5nnA)
- [Curso de PHP com MySQL](https://www.youtube.com/playlist?list=PLucm8g_ezqNrkPSrXiYgGXXkK4x245cvV)
- [Curso de PHP para iniciantes](https://www.youtube.com/playlist?list=PLInBAd9OZCzx82Bov1cuo_sZI2Lrb7mXr)
- [Curso de PHP 7 e MVC - Micro Framework](https://www.youtube.com/playlist?list=PL0N5TAOhX5E-NZ0RRHa2tet6NCf9-7B5G)
- [Curso de PHP - Emerson Carvalho](https://www.youtube.com/playlist?list=PLIZ0d6lKIbVpOxc0x1c4HpEWyK0JMsL49)

> Cursos para aprender PHP em Espanhol
- [Curso de PHP/MySQL](https://www.youtube.com/playlist?list=PLU8oAlHdN5BkinrODGXToK9oPAlnJxmW_)
- [Curso completo de PHP desde cero a experto](https://www.youtube.com/playlist?list=PLH_tVOsiVGzmnl7ImSmhIw5qb9Sy5KJRE)
- [Curso PHP 8 y MySQL 8 desde cero](https://www.youtube.com/playlist?list=PLZ2ovOgdI-kUSqWuyoGJMZL6xldXw6hIg)
- [Curso de PHP completo desde cero](https://www.youtube.com/playlist?list=PLg9145ptuAij8vIQLU25f7sUSH4E8pdY5)
- [Curso completo PHP y MySQL principiantes-avanzado](https://www.youtube.com/playlist?list=PLvRPaExkZHFkpBXXCsL2cn9ORTTcPq4d7)
- [Curso PHP Básico](https://www.youtube.com/playlist?list=PL469D93BF3AE1F84F)
- [PHP desde cero](https://www.youtube.com/playlist?list=PLAzlSdU-KYwW9eWj88DW55gTi1M5HQo5S)

> Cursos para aprender PHP em Inglês
- [Learn PHP The Right Way - Full PHP Tutorial For Beginners & Advanced](https://www.youtube.com/playlist?list=PLr3d3QYzkw2xabQRUpcZ_IBk9W50M9pe-)
- [PHP Programming Language Tutorial - Full Course](https://www.youtube.com/watch?v=OK_JCtrrv-c&ab_channel=freeCodeCamp.org)
- [PHP For Absolute Beginners | 6.5 Hour Course](https://www.youtube.com/watch?v=2eebptXfEvw&ab_channel=TraversyMedia)
- [PHP For Beginners | 3+ Hour Crash Course](https://www.youtube.com/watch?v=BUCiSSyIGGU&ab_channel=TraversyMedia)
- [PHP Tutorial for Beginners - Full Course | OVER 7 HOURS!](https://www.youtube.com/watch?v=t0syDUSbdfE&ab_channel=EnvatoTuts%2B)
- [PHP And MySQL Full Course in 2022](https://www.youtube.com/watch?v=s-iza7kAXME&ab_channel=Simplilearn)
- [PHP Full Course | PHP Tutorial For Beginners](https://www.youtube.com/watch?v=6EukZDFE_Zg&ab_channel=Simplilearn)
- [PHP Front To Back](https://www.youtube.com/playlist?list=PLillGF-Rfqbap2IB6ZS4BBBcYPagAjpjn)
- [PHP Tutorial for Beginners](https://www.youtube.com/playlist?list=PL4cUxeGkcC9gksOX3Kd9KPo-O68ncT05o)
- [PHP for beginners](https://www.youtube.com/playlist?list=PLFHz2csJcgk_fFEWydZJLiXpc9nB1qfpi)
- [The Complete 2021 PHP Full Stack Web Developer](https://www.youtube.com/playlist?list=PLs-hN447lej6LvquSMoWkGlJAJrhwaVNX)
- [PHP Training Videos](https://www.youtube.com/playlist?list=PLEiEAq2VkUUIjP-QLfvICa1TvqTLFvn1b)
- [PHP complete course with Project](https://www.youtube.com/playlist?list=PLFINWHSIpuivHWnGE8YGw8uFygThFGr3-)
- [PHP Course for Beginners](https://www.youtube.com/playlist?list=PLLQuc_7jk__WTMT4U1qhDkhqd2bOAdxSo)
- [PHP Tutorials Playlist](https://www.youtube.com/playlist?list=PL442FA2C127377F07)
- [PHP Tutorials](https://www.youtube.com/playlist?list=PL0eyrZgxdwhwBToawjm9faF1ixePexft-)
- [PHP Tutorial for Beginners](https://www.youtube.com/playlist?list=PLS1QulWo1RIZc4GM_E04HCPEd_xpcaQgg)
- [PHP 7 Course - From Zero to Hero](https://www.youtube.com/playlist?list=PLCwJ-zYcMM92IlmUrW7Nn79y4LHGfODGc)
- [PHP Tutorials (updated)](https://www.youtube.com/playlist?list=PL0eyrZgxdwhxhsuT_QAqfi-NNVAlV4WIP)
- [PHP & MySQL Tutorial Videos](https://www.youtube.com/playlist?list=PL9ooVrP1hQOFB2yjxFbK-Za8HwM5v1NC5)
- [PHP from intermediate to advanced](https://www.youtube.com/playlist?list=PLBEpR3pmwCazOsFp0xI3keBq7SoqDnxM7)
- [Object Oriented PHP Tutorials](https://www.youtube.com/playlist?list=PL0eyrZgxdwhypQiZnYXM7z7-OTkcMgGPh)
- [PHP OOP Basics Full Course](https://www.youtube.com/playlist?list=PLY3j36HMSHNUfTDnDbW6JI06IrkkdWCnk)
- [Advanced PHP](https://www.youtube.com/playlist?list=PLu4-mSyb4l4SlKcO51aLtyiuOmlEuojvZ)

## 🦓 Sites e cursos para aprender Java

> Cursos para aprender Java em Português 
- [Maratona Java Virado no Jiraya](https://www.youtube.com/playlist?list=PL62G310vn6nFIsOCC0H-C2infYgwm8SWW)
- [Curso de Java para Iniciantes - Grátis, Completo e com Certificado](https://www.youtube.com/playlist?list=PLHz_AreHm4dkI2ZdjTwZA4mPMxWTfNSpR)
- [Curso de Java - Tiago Aguiar](https://www.youtube.com/playlist?list=PLJ0AcghBBWSi6nK2CUkw9ngvwWB1gE8mL)
- [Curso de Java - CFBCursos](https://www.youtube.com/playlist?list=PLx4x_zx8csUjFC5WWjoNUL7LOOD7LCKRW)
- [Maratona Java - O maior curso Java em português](https://www.youtube.com/playlist?list=PL62G310vn6nHrMr1tFLNOYP_c73m6nAzL)
- [Curso de Java Básico Gratuito com Certificado](https://www.youtube.com/playlist?list=PLGxZ4Rq3BOBq0KXHsp5J3PxyFaBIXVs3r)
- [Curso de Java - eXcript](https://www.youtube.com/playlist?list=PLesCEcYj003Rfzs39Y4Bs_chpkE276-gD)
- [Curso de POO Java (Programação Orientada a Objetos)](https://www.youtube.com/playlist?list=PLHz_AreHm4dkqe2aR0tQK74m8SFe-aGsY)
- [Curso de Programação em Java](https://www.youtube.com/playlist?list=PLucm8g_ezqNrQmqtO0qmew8sKXEEcaHvc)
- [Curso - Fundamentos da Linguagem Java](https://www.youtube.com/playlist?list=PLbEOwbQR9lqxdW98mY-40IZQ5i8ZZyeQx)
- [Curso Java Estruturado](https://www.youtube.com/playlist?list=PLGPluF_nhP9p6zWTN88ZJ1q9J_ZK148-f)
- [Curso de Java Completo](https://www.youtube.com/playlist?list=PL6vjf6t3oYOrSx2XQKm3yvNxgjtI1A56P)
- [Curso Programação Java](https://www.youtube.com/playlist?list=PLtchvIBq_CRTAwq_xmHdITro_5vbyOvVw)
- [Curso de Java para Iniciantes](https://www.youtube.com/playlist?list=PLt2CbMyJxu8iQL67Am38O1j5wKLf0AIRZ)

> Cursos para aprender Java em Espanhol
- [Curso de Java desde 0](https://www.youtube.com/playlist?list=PLU8oAlHdN5BktAXdEVCLUYzvDyqRQJ2lk)
- [Curso de programación Java desde cero](https://www.youtube.com/playlist?list=PLyvsggKtwbLX9LrDnl1-K6QtYo7m0yXWB)
- [Curso de Java Completo 2021](https://www.youtube.com/playlist?list=PLt1J5u9LpM59sjPZFl3KYUhTrpwPIhKor)
- [Java Netbeans Completo](https://www.youtube.com/playlist?list=PLCTD_CpMeEKTT-qEHGqZH3fkBgXH4GOTF)
- [Programación en Java](https://www.youtube.com/playlist?list=PLWtYZ2ejMVJkjOuTCzIk61j7XKfpIR74K)
- [Curso de Java 11](https://www.youtube.com/playlist?list=PLf5ldD20p3mHRM3O4yUongNYx6UaELABm)
- [Curso de Java - Jesús Conde](https://www.youtube.com/playlist?list=PL4D956E5314B9C253)
- [Curso de programacion funcional en java](https://www.youtube.com/playlist?list=PLjJ8HhsSfskiDEwgfyF9EznmrSyEukcJa)

> Cursos para aprender Java em Inglês
- [Java Tutorial for Beginners](https://www.youtube.com/watch?v=eIrMbAQSU34&ab_channel=ProgrammingwithMosh)
- [Java Tutorial: Full Course for Beginners](https://www.youtube.com/watch?v=xk4_1vDrzzo&ab_channel=BroCode)
- [Java Full Course](https://www.youtube.com/watch?v=Qgl81fPcLc8&ab_channel=Amigoscode)
- [Java Programming for Beginners – Full Course](https://www.youtube.com/watch?v=A74TOX803D0&ab_channel=freeCodeCamp.org)
- [Intro to Java Programming - Course for Absolute Beginners](https://www.youtube.com/watch?v=GoXwIVyNvX0&ab_channel=freeCodeCamp.org)
- [Learn Java 8 - Full Tutorial for Beginners](https://www.youtube.com/watch?v=grEKMHGYyns&ab_channel=freeCodeCamp.org)
- [Java Full Course 2022 | Java Tutorial For Beginners | Core Java Full Course](https://www.youtube.com/watch?v=CFD9EFcNZTQ&ab_channel=Simplilearn)
- [Java Full Course for Beginners](https://www.youtube.com/watch?v=_3ds4qujpxU&ab_channel=SDET-QAAutomation)
- [Java Full Course | Java Tutorial for Beginners](https://www.youtube.com/watch?v=hBh_CC5y8-s&ab_channel=edureka%21)
- [Learn JavaScript in 12 Hours | JavaScript Tutorial For Beginners 2022](https://www.youtube.com/watch?v=A1eszacPf-4&ab_channel=Simplilearn)
- [Java GUI: Full Course](https://www.youtube.com/watch?v=Kmgo00avvEw&ab_channel=BroCode)
- [Java Collections Framework | Full Course](https://www.youtube.com/watch?v=GdAon80-0KA&ab_channel=JavaGuides)
- [Java Programming](https://www.youtube.com/playlist?list=PLBlnK6fEyqRjKA_NuK9mHmlk0dZzuP1P5)
- [Java Complete Course | Placement Series](https://www.youtube.com/playlist?list=PLfqMhTWNBTe3LtFWcvwpqTkUSlB32kJop)
- [Stanford - Java course](https://www.youtube.com/playlist?list=PLA70DBE71B0C3B142)
- [Java Tutorials](https://www.youtube.com/playlist?list=PL_c9BZzLwBRKIMP_xNTJxi9lIgQhE51rF)
- [Java Full Course - 2022 | Java Tutorial for Beginners](https://www.youtube.com/playlist?list=PL9ooVrP1hQOEe9EN119lMdwcBxcrBI1D3)
- [Java (Intermediate) Tutorials](https://www.youtube.com/playlist?list=PL27BCE863B6A864E3)
- [Core Java (Full Course)](https://www.youtube.com/playlist?list=PLsjUcU8CQXGFZ7xMUxJBE33FWWykEWm49)
- [Working Class Java Programming & Software Architecture Fundamentals Course](https://www.youtube.com/playlist?list=PLEVlop6sMHCoVFZ8nc_HmXOi8Msrah782)
- [Java Programming Tutorials for Beginners [Complete Course]](https://www.youtube.com/playlist?list=PLIY8eNdw5tW_uaJgi-FL9QwINS9JxKKg2)
- [Java Tutorials For Beginners In Hindi](https://www.youtube.com/playlist?list=PLu0W_9lII9agS67Uits0UnJyrYiXhDS6q)
- [Java Tutorial For Beginners](https://www.youtube.com/playlist?list=PLsyeobzWxl7oZ-fxDYkOToURHhMuWD1BK)
- [Java Tutorial for Beginners - Bro Code](https://www.youtube.com/playlist?list=PLZPZq0r_RZOMhCAyywfnYLlrjiVOkdAI1)
- [Java Programming Tutorial](https://www.youtube.com/playlist?list=PLsyeobzWxl7pFZoGT1NbZJpywedeyzyaf)
- [Java (Beginner) Programming Tutorials](https://www.youtube.com/playlist?list=PLFE2CE09D83EE3E28)
- [Complete Java Course for Beginners](https://www.youtube.com/playlist?list=PLab_if3UBk9-ktSKtoVQoLngTFpj9PIed)
- [Java Tutorial For Beginners (Step by Step tutorial)](https://www.youtube.com/playlist?list=PLS1QulWo1RIbfTjQvTdj8Y6yyq4R7g-Al)
- [Mastering Java Course - Learn Java from ZERO to HERO](https://www.youtube.com/playlist?list=PL6Q9UqV2Sf1gb0izuItEDnU8_YBR-DZi6)
- [Tim Buchalka's Java Course PlayList](https://www.youtube.com/playlist?list=PLXtTjtWmQhg1SsviTmKkWO5n0a_-T0bnD)
- [Java Full Course](https://www.youtube.com/playlist?list=PLrhDANsBnxU9WFTBt73Qog9CH1ox5zI--)
- [Java Course](https://www.youtube.com/playlist?list=PLJSrGkRNEDAhE_nsOkDiC5OvckE7K0bo2)
- [Java Web App Course](https://www.youtube.com/playlist?list=PLcRrh9hGNaln4tPtqsmglKenc3NZW7l9M)

## 🐦 Sites e cursos para aprender Ruby

> Cursos para aprender Ruby em Português
- [Ruby Para Iniciantes (2021 - Curso Completo Para Iniciantes)](https://www.youtube.com/playlist?list=PLnV7i1DUV_zOit4a_tEDf1_PcRd25dL7e)
- [Curso completo de Ruby](https://www.youtube.com/playlist?list=PLdDT8if5attEOcQGPHLNIfnSFiJHhGDOZ)
- [Curso de Ruby on Rails para Iniciantes](https://www.youtube.com/playlist?list=PLe3LRfCs4go-mkvHRMSXEOG-HDbzesyaP)
- [Curso de Ruby on Rails básico](https://www.youtube.com/playlist?list=PLFeyfVYazTkJN6uM5opCfSN_xjxrMybXV)
- [Programação com Ruby](https://www.youtube.com/playlist?list=PLucm8g_ezqNqMm1gdqjZzfhAMFQ9KrhFq)
- [Linguagem Ruby](https://www.youtube.com/playlist?list=PLEdPHGYbHhldWUFs2Q-jSzXAv3NXh4wu0)

> Cursos para aprender Ruby em Espanhol
- [Curso Gratuito de Ruby en español](https://www.youtube.com/playlist?list=PL954bYq0HsCUG5_LbfZ54YltPinPSPOks)
- [Ruby desde Cero](https://www.youtube.com/playlist?list=PLAzlSdU-KYwUG_5HcRVT4mr0vgLYBeFnm)
- [Curso de Ruby](https://www.youtube.com/playlist?list=PLEFC2D43C36013A70)
- [Curso Ruby - Codigofacilito](https://www.youtube.com/playlist?list=PLUXwpfHj_sMlkvu4T2vvAnqPSzWQsPesm)
- [Curso Ruby on Rails 7 para principiantes en español](https://www.youtube.com/playlist?list=PLP06kydD_xaUS6plnsdonHa5ySbPx1PrP)
- [Curso de Ruby on Rails 5](https://www.youtube.com/playlist?list=PLIddmSRJEJ0uaT5imV49pJqP8CGSqN-7E)

> Cursos para aprender Ruby em Inglês
- [Learn Ruby on Rails - Full Course](https://www.youtube.com/watch?v=fmyvWz5TUWg&ab_channel=freeCodeCamp.org)
- [Ruby On Rails Crash Course](https://www.youtube.com/watch?v=B3Fbujmgo60&ab_channel=TraversyMedia)
- [Ruby Programming Language - Full Course](https://www.youtube.com/watch?v=t_ispmWmdjY&ab_channel=freeCodeCamp.org)
- [Ruby on Rails Tutorial for Beginners - Full Course](https://www.youtube.com/watch?v=-AdqKjqHQIA&ab_channel=CodeGeek)
- [Learn Ruby on Rails from Scratch](https://www.youtube.com/watch?v=2zZCzcupQao&ab_channel=ProgrammingKnowledge)
- [The complete ruby on rails developer course](https://www.youtube.com/watch?v=y4pKYYMdAA0&ab_channel=FullCourse)
- [Ruby on Rails for Beginners](https://www.youtube.com/playlist?list=PLm8ctt9NhMNV75T9WYIrA6m9I_uw7vS56)
- [Ruby on Rails Full Course](https://www.youtube.com/playlist?list=PLsikuZM13-0zOytkeVGSKk4VTTgE8x1ns)
- [Full Stack Ruby on Rails Development Bootcamp](https://www.youtube.com/playlist?list=PL6SEI86zExmsdxwsyEQcFpF9DWmvttPPu)
- [Let's Build: With Ruby On Rails](https://www.youtube.com/playlist?list=PL01nNIgQ4uxNkDZNMON-TrzDVNIk3cOz4)
- [Ruby On Rail Full Course 2022](https://www.youtube.com/playlist?list=PLAqsB9gf_hQY6pwlIbht35wSytqezS-Sy)
- [Advanced Ruby on Rails](https://www.youtube.com/playlist?list=PLPiVX6hQQRl_UN9cLxSoGKQm_RF8pw7MU)
- [Ruby On Rails 2021 | Complete Course](https://www.youtube.com/playlist?list=PLeMlKtTL9SH_J-S0JA9o5gUmcW-NZSDtF)

## 🐪 Sites e cursos para aprender Perl

> Cursos para aprender Perl em Português
- [Curso Perl - Alder Pinto](https://www.youtube.com/playlist?list=PLE1HNzXaOep0RJIQoWA9_-OPg4WUbjQUZ)
- [Curso de Perl - Perfil Antigo](https://www.youtube.com/playlist?list=PLBDxU1-FpoohxqH3XfnqTqLCxTj8dz5sI)

> Cursos para aprender Perl em Espanhol
- [Tutorial de Perl en Español](https://www.youtube.com/playlist?list=PLjARR1053fYmN9oYz-H6ZI1fOkrjLz6L2)
- [Curso de Perl en Español](https://www.youtube.com/playlist?list=PL8qgaJWZ7bGJPlIvAFbq8fKrFogUEJ3AJ)
- [Curso Perl - David Elí Tupac](https://www.youtube.com/playlist?list=PL2FOMZ1Ba3plgMbgLlxE-8IXi7oIlkdVp)

> Cursos para aprender Perl em Inglês
- [Perl Online Training](https://www.youtube.com/playlist?list=PLWPirh4EWFpE0UEJPQ2PUeXUfvJDhPqSD)
- [Perl Enough to be dangerous](https://www.youtube.com/watch?v=c0k9ieKky7Q&ab_channel=NedDev)
- [Perl Tutorials](https://www.youtube.com/playlist?list=PL_RGaFnxSHWpqRBcStwV0NwMA3nXMh5GC)
- [Perl Tutorial: Basics to Advanced](https://www.youtube.com/playlist?list=PL1h5a0eaDD3rTG1U7w9wmff6ZAKDN3b16)
- [Perl Programming](https://www.youtube.com/playlist?list=PL5eJgcQ87sgcXxN8EG7RUGZ_kTDUDwYX9)
- [Perl Scripting Tutorial Videos](https://www.youtube.com/playlist?list=PL9ooVrP1hQOH9R0GR6yFteE4XWbsYNLga)

## 🐷 Sites e cursos para aprender Bash

> Cursos para aprender Bash em Português
- [Curso Básico de Bash](https://www.youtube.com/playlist?list=PLXoSGejyuQGpf4X-NdGjvSlEFZhn2f2H7)
- [Curso intensivo de programação em Bash](https://www.youtube.com/playlist?list=PLXoSGejyuQGr53w4IzUzbPCqR4HPOHjAI)
- [Curso de Shell Scripting - Programação no Linux](https://www.youtube.com/playlist?list=PLucm8g_ezqNrYgjXC8_CgbvHbvI7dDfhs)

> Cursos para aprender Bash em Espanhol
- [Curso Profesional de Scripting Bash Shell](https://www.youtube.com/playlist?list=PLDbrnXa6SAzUsIAqsjVOeyagmmAvmwsG2)
- [Curso lINUX: Comandos Básicos [Introducción al Shell BASH]](https://www.youtube.com/playlist?list=PLN9u6FzF6DLTRhmLLT-ILqEtDQvVf-ChM)

> Cursos para aprender Bash em Inglês
- [Bash Scripting Full Course 3 Hours](https://www.youtube.com/watch?v=e7BufAVwDiM&ab_channel=linuxhint)
- [Linux Command Line Full course: Beginners to Experts. Bash Command Line Tutorials](https://www.youtube.com/watch?v=2PGnYjbYuUo&ab_channel=Geek%27sLesson)
- [Bash in 100 Seconds](https://www.youtube.com/watch?v=I4EWvMFj37g&ab_channel=Fireship)
- [Bash Script with Practical Examples | Full Course](https://www.youtube.com/watch?v=TPRSJbtfK4M&ab_channel=Amigoscode)
- [Beginner's Guide to the Bash Terminal](https://www.youtube.com/watch?v=oxuRxtrO2Ag&ab_channel=JoeCollins)
- [212 Bash Scripting Examples](https://www.youtube.com/watch?v=q2z-MRoNbgM&ab_channel=linuxhint)
- [Linux Bash for beginners 2022](https://www.youtube.com/watch?v=qALScO3E61I&ab_channel=GPS)
- [Bash scripting tutorial for beginners](https://www.youtube.com/watch?v=9T2nEXlLy9o&ab_channel=FortifySolutions)
- [Linux CLI Crash Course - Fundamentals of Bash Shell](https://www.youtube.com/watch?v=S99sQLravYo&ab_channel=codedamn)
- [Shell Scripting](https://www.youtube.com/playlist?list=PLBf0hzazHTGMJzHon4YXGscxUvsFpxrZT)
- [Shell Scripting Tutorial for Beginners](https://www.youtube.com/playlist?list=PLS1QulWo1RIYmaxcEqw5JhK3b-6rgdWO_)
- [Bash Scripting | Complete Course](https://www.youtube.com/playlist?list=PLgmzaUQcOhaqQjXaqz7Ky5a_xj_8OlCK4)
- [Complete Shell Scripting Tutorials](https://www.youtube.com/playlist?list=PL2qzCKTbjutJRM7K_hhNyvf8sfGCLklXw)
- [Bash Scripting 3hrs course](https://www.youtube.com/playlist?list=PL2JwSAqE1httILs055eEgbnO9oTu1otIG)
- [Bash Zero to Hero Series](https://www.youtube.com/playlist?list=PLP8aFdeDk9g5Pg7WHYfv6EsD1D8hrx5AJ)

## 🐴 Sites e cursos para aprender MySQL

> Sites para aprender MySQL
- [SQLZOO](https://sqlzoo.net/wiki/SQL_Tutorial)
- [SQLBolt](https://sqlbolt.com/)
- [LinuxJedi](https://linuxjedi.co.uk/tag/mysql/)
- [SQLCourse](https://www.sqlcourse.com/)
- [CodeQuizzes](https://www.codequizzes.com/apache-spark/spark/datasets-spark-sql)
- [Planet MySQL](https://planet.mysql.com/pt/)
- [MySQL Learn2torials](https://learn2torials.com/category/mysql)
- [Learn MySQL, Memrise](https://app.memrise.com/course/700054/learn-mysql/)
- [Tizag MySQL Tutorials](http://www.tizag.com/mysqlTutorial/)
- [W3Schools SQL Tutorials](https://www.w3schools.com/sql/)
- [SQL Basics Khan Academy](https://www.khanacademy.org/computing/computer-programming/sql)
- [Phptpoint MySQL Tutorial](https://www.phptpoint.com/mysql-tutorial/)
- [RoseIndia MySQL Tutorials](https://www.roseindia.net/mysql/)
- [MySQL on Linux Like Geeks](https://likegeeks.com/mysql-on-linux-beginners-tutorial/)
- [Mastering MySQL by Mark Leith](http://www.markleith.co.uk/)
- [Tutorials Point MySQL Tutorial](https://www.tutorialspoint.com/mysql/index.htm)
- [KillerPHP MySQL Video Tutorials](https://www.killerphp.com/mysql/videos/)
- [PYnative MySQL Database Tutorial](https://pynative.com/python/databases/)
- [Digital Ocean Basic MySQL Tutorial](https://www.digitalocean.com/community/tags/mysql)
- [Journal to SQL Authority, Pinal Dave](https://blog.sqlauthority.com/)
- [MySQL Tutorial, Learn MySQL Fast, Easy and Fun](https://www.mysqltutorial.org/)

> Cursos para aprender MySQL em Português 

- [Curso de Banco de Dados MySQL](https://www.youtube.com/playlist?list=PLHz_AreHm4dkBs-795Dsgvau_ekxg8g1r)
- [Curso de MySQL - Bóson Treinamentos](https://www.youtube.com/playlist?list=PLucm8g_ezqNrWAQH2B_0AnrFY5dJcgOLR)
- [Curso SQL Completo 2022 em 4 horas - Dev Aprender](https://www.youtube.com/watch?v=G7bMwefn8RQ&ab_channel=DevAprender)
- [Curso de SQL com MySQL (Completo) - Ótavio Miranda](https://www.youtube.com/playlist?list=PLbIBj8vQhvm2WT-pjGS5x7zUzmh4VgvRk)
- [MySQL - Curso Completo para Iniciantes e Estudantes](https://www.youtube.com/playlist?list=PLOPt_yd2VLWGEnSzO-Sc9MYjs7GZadX1f)
- [Curso de MySQL - Daves Tecnologia](https://www.youtube.com/playlist?list=PL5EmR7zuTn_ZGtE7A5PJjzQ0u7gicicLK)
- [Curso de SQL - CFBCursos](https://www.youtube.com/playlist?list=PLx4x_zx8csUgQUjExcssR3utb3JIX6Kra)
- [Curso Gratuito MySQL Server](https://www.youtube.com/playlist?list=PLiLrXujC4CW1HSOb8i7j8qXIJmSqX44KH)
- [Curso de MySQL - Diego Moisset](https://www.youtube.com/playlist?list=PLIygiKpYTC_4KmkW7AKH87nDWtb29jHvN)
- [Curso completo MySQL WorkBench](https://www.youtube.com/playlist?list=PLq-sApY8QuyeEq4L_ECA7yYgOJH6IUphP)
- [Curso de MySQL 2022 - IS](https://www.youtube.com/playlist?list=PL-6S8_azQ-MrCeQgZ1ZaD8Be3EVW4wEKx)
- [MySql/MariaDB - Do básico ao avançado](https://www.youtube.com/playlist?list=PLfvOpw8k80WqyrR7P7fMNREW2Q82xJlpO)
- [Curso de PHP com MySQL](https://www.youtube.com/playlist?list=PLucm8g_ezqNrkPSrXiYgGXXkK4x245cvV)

> Cursos para aprender MySQL em Inglês
- [The New Boston MySQL Videos](https://www.youtube.com/playlist?list=PL32BC9C878BA72085)
- [MySQL For Beginners, Programming With Mosh](https://www.youtube.com/watch?v=7S_tz1z_5bA&ab_channel=ProgrammingwithMosh)
- [Complete MySQL Beginner to Expert](https://www.youtube.com/watch?v=en6YPAgc6WM&ab_channel=FullCourse)
- [Full MySQL Course for Beginners](https://www.youtube.com/playlist?list=PLyuRouwmQCjlXvBkTfGeDTq79r9_GoMt9)
- [MySQL Complete Tutorial for Beginners 2022](https://www.youtube.com/playlist?list=PLjVLYmrlmjGeyCPgdHL2vWmEGKxcpsC0E)
- [SQL for Beginners (MySQL)](https://www.youtube.com/playlist?list=PLUDwpEzHYYLvWEwDxZViN1shP-pGyZdtT)
- [MySQL Course](https://www.youtube.com/playlist?list=PLBlpUqEneF0-xZ1ctyLVqhwJyoQsyfOsO)
- [MySQL Tutorial For Beginners - Edureka](https://www.youtube.com/playlist?list=PL9ooVrP1hQOGECN1oA2iXcWFBTRYUxzQG)
- [MySQL Tutorial for Beginners](https://www.youtube.com/playlist?list=PLS1QulWo1RIY4auvfxAHS9m_fZJ2wxSse)
- [MySQL Tutorial for beginner - ProgrammingKnowledge](https://www.youtube.com/playlist?list=PLS1QulWo1RIahlYDqHWZb81qsKgEvPiHn)
- [MySQL DBA Tutorial - Mughees Ahmed](https://www.youtube.com/playlist?list=PLd5sTGXltJ-l9PKT2Bynhg0Ou2uESOJiH)
- [MySQL DBA Tutorial - TechBrothers](https://www.youtube.com/playlist?list=PLWf6TEjiiuICV0BARDhRC0JvNKHC5MDEU)
- [SQL Tutorial - Full Database Course for Beginners](https://www.youtube.com/watch?v=HXV3zeQKqGY&ab_channel=freeCodeCamp.org)

## 🐧 Sites e cursos para aprender Linux

> Sites para aprender Linux
- [Tecmint](https://www.tecmint.com/)
- [Linuxize](https://linuxize.com/)
- [nixCraft](https://www.cyberciti.biz/)
- [It's FOSS](https://itsfoss.com/)
- [Linux Hint](https://linuxhint.com/)
- [FOSS Linux](https://www.fosslinux.com/)
- [LinuxOPsys](https://linuxopsys.com/)
- [Linux Journey](https://linuxjourney.com/)
- [Linux Command](https://linuxcommand.org/)
- [Linux Academy](https://linuxacademy.org/)
- [Linux Survival](https://linuxsurvival.com/)
- [Linux Handbook](https://linuxhandbook.com/)
- [Ryan's Tutorials](https://ryanstutorials.net/)
- [LinuxFoundationX](https://www.edx.org/school/linuxfoundationx)
- [LabEx Linux For Noobs](https://labex.io/courses/linux-for-noobs)
- [Conquering the Command Line](http://conqueringthecommandline.com/)
- [Guru99 Linux Tutorial Summary](https://www.guru99.com/unix-linux-tutorial.html)
- [Eduonix Learn Linux From Scratch](https://www.eduonix.com/courses/system-programming/learn-linux-from-scratch)
- [TLDP Advanced Bash Scripting Guide](https://tldp.org/LDP/abs/html/)
- [The Debian Administrator's Handbook](https://debian-handbook.info/)
- [Cyberciti Bash Shell Scripting Tutorial](https://bash.cyberciti.biz/guide/Main_Page)
- [Digital Ocean Getting Started With Linux](https://www.digitalocean.com/community/tutorial_series/getting-started-with-linux)
- [Learn Enough Command Line To Be Dangerous](https://www.learnenough.com/command-line-tutorial)

> Cursos para aprender Linux em Português 
- [Curso de Linux - Primeiros Passos](https://www.youtube.com/playlist?list=PLHz_AreHm4dlIXleu20uwPWFOSswqLYbV)
- [Curso de Linux Básico / Certificação LPIC - 1](https://www.youtube.com/playlist?list=PLucm8g_ezqNp92MmkF9p_cj4yhT-fCTl7)
- [Curso de Linux - Matheus Battisti](https://www.youtube.com/playlist?list=PLnDvRpP8BnezDTtL8lm6C-UOJZn-xzALH)
- [Curso GNU/Linux - Paulo Kretcheu](https://www.youtube.com/playlist?list=PLuf64C8sPVT9L452PqdyYCNslctvCMs_n)
- [Curso completo de Linux desde cero para principiantes](https://www.youtube.com/playlist?list=PL2Z95CSZ1N4FKsZQKqCmbylDqssYFJX5A)
- [Curso de Linux Avançado Terminal](https://www.youtube.com/playlist?list=PLGw1E40BSQnRZufbzjGVzkH-O8SngPymp)
- [Curso Grátis Linux Ubuntu Desktop](https://www.youtube.com/playlist?list=PLozhsZB1lLUMHaZmvczDWugUv9ldzX37u)
- [Curso Kali Linux - Daniel Donda](https://www.youtube.com/playlist?list=PLPIvFl3fAVRfzxwHMK1ACl9m4GmwFoxVz)
- [Curso Grátis Linux Ubuntu Server 18.04.x LTS](https://www.youtube.com/playlist?list=PLozhsZB1lLUOjGzjGO4snI34V0zINevDm)
- [Curso de Linux Ubuntu - Portal Hugo Cursos](https://www.youtube.com/playlist?list=PLxNM4ef1Bpxh3gfTUfr3BGmfuLUH4L-5Z)
- [Cursos de Linux - Playlist variada com 148 vídeos](https://www.youtube.com/playlist?list=PLreu0VPCNEMQJBXmyptwC5gDGGGnQnu_u)
- [Curso de Linux Básico para Principiantes 2021](https://www.youtube.com/playlist?list=PLG1hKOHdoXktPkbN_sxqr1fLqDle8wnOh)
- [Revisão Certificação Linux Essentials](https://www.youtube.com/playlist?list=PLsBCFv4w3afsg8QJnMwQbFGumLpwFchc-)
- [Curso completo de Linux do zero - ProfeSantiago](https://www.youtube.com/playlist?list=PLbcS-eIZbbxUqcd3Kr74fo46HzfnYpMqc)

> Cursos para aprender Linux em Inglês
- [The Linux Basics Course: Beginner to Sysadmin, Step by Step](https://www.youtube.com/playlist?list=PLtK75qxsQaMLZSo7KL-PmiRarU7hrpnwK)
- [Linux for Hackers (and everyone)](https://www.youtube.com/playlist?list=PLIhvC56v63IJIujb5cyE13oLuyORZpdkL)
- [Linux Command Line Tutorial For Beginners](https://www.youtube.com/playlist?list=PLS1QulWo1RIb9WVQGJ_vh-RQusbZgO_As)
- [Linux Crash Course](https://www.youtube.com/playlist?list=PLT98CRl2KxKHKd_tH3ssq0HPrThx2hESW)
- [The Complete Kali Linux Course: Beginner to Advanced](https://www.youtube.com/playlist?list=PLYmlEoSHldN7HJapyiQ8kFLUsk_a7EjCw)
- [Linux Online Training](https://www.youtube.com/playlist?list=PLWPirh4EWFpGsim4cuJrh9w6-yfuC9XqI)
- [CompTIA Linux+ XK0-004 Complete Video Course](https://www.youtube.com/playlist?list=PLC5eRS3MXpp-zlq64CcDfzMl2hO2Wtcl0)
- [LPI Linux Essentials (010-160 Exam Prep)](https://www.youtube.com/playlist?list=PL78ppT-_wOmvlYSfyiLvkrsZTdQJ7A24L)
- [Complete Linux course for beginners in Arabic](https://www.youtube.com/playlist?list=PLNSVnXX5qE8VOJ6BgMytvgFpEK2o4sM1o)
- [Linux internals](https://www.youtube.com/playlist?list=PLX1h5Ah4_XcfL2NCX9Tw4Hm9RcHhC14vs)
- [The Complete Red Hat Linux Course: Beginner to Advanced](https://www.youtube.com/playlist?list=PLYmlEoSHldN6W1w_0l-ta8oKzGWqCcq63)
- [Linux for Programmers](https://www.youtube.com/playlist?list=PLzMcBGfZo4-nUIIMsz040W_X-03QH5c5h)
- [Kali Linux: Ethical Hacking Getting Started Course](https://www.youtube.com/playlist?list=PLhfrWIlLOoKMe1Ue0IdeULQvEgCgQ3a1B)
- [Linux Masterclass Course - A Complete Tutorial From Beginner To Advanced](https://www.youtube.com/playlist?list=PL2kSRH_DmWVZp_cu6MMPWkgYh7GZVFS6i)
- [Unix/Linux Tutorial Videos](https://www.youtube.com/playlist?list=PLd3UqWTnYXOloH0vWBs4BtSbP84WcC2NY)
- [Linux Administration Tutorial Videos](https://www.youtube.com/playlist?list=PL9ooVrP1hQOH3SvcgkC4Qv2cyCebvs0Ik)
- [Linux Tutorials | GeeksforGeeks](https://www.youtube.com/playlist?list=PLqM7alHXFySFc4KtwEZTANgmyJm3NqS_L)
- [Linux Operating System - Crash Course for Beginners](https://www.youtube.com/watch?v=ROjZy1WbCIA&ab_channel=freeCodeCamp.org)
- [The Complete Linux Course: Beginner to Power User](https://www.youtube.com/watch?v=wBp0Rb-ZJak&ab_channel=JosephDelgadillo)
- [The 50 Most Popular Linux & Terminal Commands - Full Course for Beginners](https://www.youtube.com/watch?v=ZtqBQ68cfJc&ab_channel=freeCodeCamp.org)
- [Linux Server Course - System Configuration and Operation](https://www.youtube.com/watch?v=WMy3OzvBWc0&ab_channel=freeCodeCamp.org)
- [Linux Tutorial for Beginners - Intellipaat](https://www.youtube.com/watch?v=4ZHvZge1Lsw&ab_channel=Intellipaat)

## 🦂 Sites e cursos para aprender Swift 

> Cursos para aprender Swift em Português
- [Curso de Swift- Tiago Aguiar](https://www.youtube.com/playlist?list=PLJ0AcghBBWShgIH122uw7H9T9-NIaFpP-)
- [Curso grátis Swift e SwiftUI (Stanford 2020)](https://www.youtube.com/playlist?list=PLMdYygf53DP46rneFgJ7Ab6fJPcMvr8gC)
- [Curso de Swift - Desenvolvimento IOS Apple](https://www.youtube.com/playlist?list=PLxNM4ef1BpxjjMKpcYSqXI4eY4tZG2csm)
- [Curso iOS e Swift](https://www.youtube.com/playlist?list=PLW-gR4IAiL9ubGKgE5MsyzwovmeOF7nt_)

> Cursos para aprender Swift em Espanhol
- [Curso de programación con Swift](https://www.youtube.com/playlist?list=PLNdFk2_brsRc57R6UaHy4zx_FHqx236G1)
- [Curso programación iOS con Xcode y Swift](https://www.youtube.com/playlist?list=PLNdFk2_brsRcWM-31vJUgyHIGpopIDw4s)
- [Curso Swift en Español desde cero [2022]](https://www.youtube.com/playlist?list=PLeTOFRUxkMcozbUpMiaHRy8_GjzJ_9tyi)
- [Curso de Swift Español - Clonando YouTube](https://www.youtube.com/playlist?list=PLT_OObKZ3CpuEomHCc6v-49u3DFCdCyLH)
- [Curso De Swift - Código Facilito](https://www.youtube.com/playlist?list=PLTPmvYfJJMVp_YzS22WI-5NYW1c_7eTBD)
- [Curso de SwiftUI](https://www.youtube.com/playlist?list=PLNdFk2_brsRetB7LiUfpnIclBe_1iOS4M)
- [Curso Xcode y Swift desde cero](https://www.youtube.com/playlist?list=PLNdFk2_brsRdyYGDX8QLFKmcpQPjFFrDC)
- [Aprende Swift 3 desde cero](https://www.youtube.com/playlist?list=PLD2wfKpqmxnmnjA7lcbc2M2P6TfygmrL3)
- [Curso de Swift 4 desde cero](https://www.youtube.com/playlist?list=PLD2wfKpqmxnn7-hEmKx7P3xDY8iYWsz59)
- [Curso Introducción a Swift](https://www.youtube.com/playlist?list=PLvQAED-MnQpaJrSjVW449S8Kda3wGQqKD)

> Cursos para aprender Swift em Inglês
- [Swift Tutorial - Full Course for Beginners](https://www.youtube.com/watch?v=comQ1-x2a1Q&ab_channel=freeCodeCamp.org)
- [Learn Swift Fast (2020) - Full Course For Beginners](https://www.youtube.com/watch?v=FcsY1YPBwzQ&ab_channel=CodeWithChris)
- [2021 SwiftUI Tutorial for Beginners (3.5 hour Masterclass)](https://www.youtube.com/watch?v=F2ojC6TNwws&ab_channel=CodeWithChris)
- [Swift Tutorial For Beginners [Full Course] Learn Swift For iOS Development](https://www.youtube.com/watch?v=mhE-Mp07RTo&ab_channel=Devslopes)
- [Swift Programming Tutorial | FULL COURSE | Absolute Beginner](https://www.youtube.com/watch?v=CwA1VWP0Ldw&ab_channel=SeanAllen)
- [Swift Programming Tutorial for Beginners (Full Tutorial)](https://www.youtube.com/watch?v=Ulp1Kimblg0&ab_channel=CodeWithChris)

## 🐍 Sites e cursos para aprender Python

> Sites & E-books para aprender Python
- [Think Python](https://greenteapress.com/wp/think-python/)
- [Think Python 2e](https://greenteapress.com/wp/think-python-2e/)
- [A Byte of Python](https://python.swaroopch.com/)
- [Real Python](https://realpython.com/)
- [Full Stack Python](https://www.fullstackpython.com/)
- [FreeCodeCamp Python](https://www.freecodecamp.org/learn/scientific-computing-with-python/)
- [Dive Into Python 3](https://diveintopython3.net/)
- [Practice Python](https://www.practicepython.org/)
- [The Python Guru](https://thepythonguru.com/)
- [The Coder's Apprentice](https://www.spronck.net/pythonbook/)
- [Python Principles](https://pythonprinciples.com/)
- [Harvard's CS50 Python Video](https://pll.harvard.edu/course/cs50s-introduction-programming-python?delta=0)
- [Cracking Codes With Python](https://inventwithpython.com/cracking/)
- [Learn Python, Break Python](https://learnpythonbreakpython.com/)
- [Google's Python Class](https://developers.google.com/edu/python)
- [Python Like You Mean It](https://www.pythonlikeyoumeanit.com/)
- [Beyond the Basic Stuff with Python](https://inventwithpython.com/beyond/)
- [Automate the Boring Stuff with Python](https://automatetheboringstuff.com/)
- [The Big Book of Small Python Projects](https://inventwithpython.com/bigbookpython/)
- [Learn Python 3 From Scratch](https://www.educative.io/courses/learn-python-3-from-scratch)
- [Python Tutorial For Beginners, Edureka](https://www.edureka.co/blog/python-tutorial/)
- [Microsoft's Introduction to Python Course](https://learn.microsoft.com/en-us/training/modules/intro-to-python/)
- [Beginner's Guide to Python, Official Wiki](https://wiki.python.org/moin/BeginnersGuide)
- [Python for Everybody Specialization, Coursera](https://www.coursera.org/specializations/python)

> Cursos para aprender Python em Português 
- [Curso completo de Python - Curso em vídeo](https://www.youtube.com/playlist?list=PLvE-ZAFRgX8hnECDn1v9HNTI71veL3oW0)
- [Curso de Python - CFB Cursos](https://www.youtube.com/playlist?list=PLx4x_zx8csUhuVgWfy7keQQAy7t1J35TR)
- [Curso Completo de Python - Jefferson Lobato](https://www.youtube.com/playlist?list=PLLVddSbilcul-1bAKtMKoL6wOCmDIPzFJ)
- [Curso Python para Iniciantes - Didática Tech](https://www.youtube.com/playlist?list=PLyqOvdQmGdTSEPnO0DKgHlkXb8x3cyglD)
- [Curso de Python - eXcript](https://www.youtube.com/playlist?list=PLesCEcYj003QxPQ4vTXkt22-E11aQvoVj)
- [Curso de Python - Otávio Miranda](https://www.youtube.com/playlist?list=PLbIBj8vQhvm0ayQsrhEf-7-8JAj-MwmPr)
- [Aulas Python - Ignorância Zero](https://www.youtube.com/playlist?list=PLfCKf0-awunOu2WyLe2pSD2fXUo795xRe)
- [Curso de Programação em Python - Prime Cursos do Brasil](https://www.youtube.com/playlist?list=PLFKhhNd35zq_INvuX9YzXIbtpo_LGDzYK)
- [Curso Python p/ Iniciantes - Refatorando](https://www.youtube.com/playlist?list=PLj7gJIFoP7jdirAFg-fHe9HKOnGLGXSHZ)
- [Curso Python Básico - Solyd](https://www.youtube.com/playlist?list=PLp95aw034Wn_WtEmlepaDrw8FU8R5azcm)
- [Curso de Python - Bóson Treinamentos](https://www.youtube.com/playlist?list=PLucm8g_ezqNrrtduPx7s4BM8phepMn9I2)
- [O Melhor Curso de Python - Zurubabel](https://www.youtube.com/playlist?list=PL4OAe-tL47sY8SGhtkGoP0eQd4le3badz)
- [Curso de Python - Hashtag Programação](https://www.youtube.com/playlist?list=PLpdAy0tYrnKyCZsE-ifaLV1xnkXBE9n7T)
- [Curso de Python Essencial para Data Science](https://www.youtube.com/playlist?list=PL3ZslI15yo2qCEmnYOa2sq6VQOzQ2CFhj)
- [Curso de Python do Zero ao Data Scientist](https://www.youtube.com/playlist?list=PLZlkyCIi8bMprZgBsFopRQMG_Kj1IA1WG)
- [Curso de Python moderno + Análise de dados](https://www.youtube.com/playlist?list=PLLWTDkRZXQa9YyC1LMbuDTz3XVC4E9ZQA)
- [Curso de Python 3 - Do básico ao avançado - RfZorzi](https://www.youtube.com/playlist?list=PLqx8fDb-FZDEDg-FOuwNKEpxA0LhzrdhZ)
- [Curso de Python Intermediário / Avançado - HashLDash](https://www.youtube.com/playlist?list=PLsMpSZTgkF5ANrrp31dmQoG0-hPoI-NoX)
- [Curso Python para Machine Learning e Análise de Dados](https://www.youtube.com/playlist?list=PLyqOvdQmGdTR46HUxDA6Ymv4DGsIjvTQ-)
- [Curso de programación Python desde cero](https://www.youtube.com/playlist?list=PLyvsggKtwbLW1j0d5yaCkRF9Axpdlhsxz)
- [Introdução à Ciência da Computação com Python](https://www.youtube.com/playlist?list=PLcoJJSvnDgcKpOi_UeneTNTIVOigRQwcn)
- [Curso de Python - Módulo Tkinter](https://www.youtube.com/playlist?list=PLesCEcYj003ShHnUT83gQEH6KtG8uysUE)
- [Curso Selenium com Python - Eduardo Mendes](https://www.youtube.com/playlist?list=PLOQgLBuj2-3LqnMYKZZgzeC7CKCPF375B)
- [Python - Curso Básico - João Ribeiro](https://www.youtube.com/playlist?list=PLXik_5Br-zO-vShMozvWZWdfcgEO4uZR7)
- [Curso de Python 3 - Caio Dellaqua](https://www.youtube.com/playlist?list=PLnHC9X5I2m1_BHFb8rS950nCZXpua3Dj3)
- [Curso de introdução ao desenvolvimento Web com Python 3 e Django](https://www.youtube.com/playlist?list=PLjv17QYEBJPpd6nI-MXpIa4qR7prKfPQz)
- [Curso Analista de dados Python / Numpy / Pandas](https://www.youtube.com/playlist?list=PL3Fmwz_E1hXRWxIkP843DiDf0ZeqgftTy)
- [Curso de Python Avançado - Portal Hugo Cursos](https://www.youtube.com/playlist?list=PLxNM4ef1Bpxj-fFV_rwrLlPA6eMvZZAXu)
- [Python Tkinter - João Ribeiro](https://www.youtube.com/playlist?list=PLXik_5Br-zO_m8NaaEix1pyQOsCZM7t1h)
- [Curso PYQT5 - Python - Desenvolvendo um sistema do zero](https://www.youtube.com/playlist?list=PLwdRIQYrHHU1MGlXIykshfhEApzkPXgQH)
- [Lógica de Programação com Python](https://www.youtube.com/playlist?list=PLt7yD2z4olT--vM2fOFTgsn2vOsxHE5LX)
- [Curso de Programação Python com Blender](https://www.youtube.com/playlist?list=PL3rePi75166RvuavzR1YU6eo5Q0gvXdI7)
- [Curso SQL com python](https://www.youtube.com/playlist?list=PLLWTDkRZXQa88Opt03kzilhx_NGEYSfFt)
- [Curso de Python - Módulo SQLite - eXcript](https://www.youtube.com/playlist?list=PLesCEcYj003QiX5JaM24ytHrHiOJknwog)
- [Curso Python desde 0](https://www.youtube.com/playlist?list=PLU8oAlHdN5BlvPxziopYZRd55pdqFwkeS)
- [Lógica de Programação Usando Python - Curso Completo](https://www.youtube.com/playlist?list=PL51430F6C54953B73)
- [Curso Python - Edson Leite Araújo](https://www.youtube.com/playlist?list=PLAwKJHUl9-WeOUxsFr9Gej3sqvS7brpMz)
- [Curso Python para hacking - Gabriel Almeida](https://www.youtube.com/playlist?list=PLTt8p5xagieX0sOtwFG-je7y_PA-oTrnY)
- [Curso de Python Orientado a Objetos](https://www.youtube.com/playlist?list=PLxNM4ef1Bpxhm8AfK1nDMWPDYXtmVQN-z)
- [Curso de TDD em Python](https://www.youtube.com/playlist?list=PL4OAe-tL47sZrzX5jISTuNsGWaMqx8uuE)
- [Curso de Python em Vídeo - Daves Tecnolgoia](https://www.youtube.com/playlist?list=PL5EmR7zuTn_Z-I4lLdZL9_wCZJzJJr2pC)
- [Curso de Python Básico - Agricultura Digital](https://www.youtube.com/playlist?list=PLVmqNeV0L_zvTZC3uRvzMpySm4XzDVLHS)
- [Exercícios de Python 3 - Curso em vídeo](https://www.youtube.com/playlist?list=PLHz_AreHm4dm6wYOIW20Nyg12TAjmMGT-)
- [Curso Python- Ignorância Zero](https://www.youtube.com/playlist?list=PLX65ruEX8lOTS_IsLp-STkZLWV9glggDG)
- [Curso Lógica de Programação Com Python - Hora de Programar](https://www.youtube.com/playlist?list=PL8hh5X1mSR2CMd6Y_SCXaNCru2bUoRlT_)

> Cursos para aprender Python em Inglês
- [Learn Python - Full Course for Beginners - freeCodeCamp](https://www.youtube.com/watch?v=rfscVS0vtbw&ab_channel=freeCodeCamp.org)
- [Python Tutorial - Python Full Course for Beginners - Programming with Mosh](https://www.youtube.com/watch?v=_uQrJ0TkZlc&ab_channel=ProgrammingwithMosh)
- [Python Tutorial: Full Course for Beginners - Bro Code](https://www.youtube.com/watch?v=XKHEtdqhLK8&t=1s&ab_channel=BroCode)
- [Python Tutorial for Beginners - Full Course in 12 Hours](https://www.youtube.com/watch?v=B9nFMZIYQl0&ab_channel=CleverProgrammer)
- [Python for Beginners – Full Course freeCodeCamp](https://www.youtube.com/watch?v=eWRfhZUzrAc&ab_channel=freeCodeCamp.org)
- [Python for Everybody - Full University Python Course](https://www.youtube.com/watch?v=8DvywoWv6fI&ab_channel=freeCodeCamp.org)
- [Python Full Course - Amigoscode](https://www.youtube.com/watch?v=LzYNWme1W6Q&ab_channel=Amigoscode)
- [Python Tutorial for Beginners - Learn Python in 5 Hours](https://www.youtube.com/watch?v=t8pPdKYpowI&ab_channel=TechWorldwithNana)
- [Intermediate Python Programming Course - freeCodeCamp](https://www.youtube.com/watch?v=HGOBQPFzWKo&ab_channel=freeCodeCamp.org)
- [Automate with Python – Full Course for Beginners - freeCodeCamp](https://www.youtube.com/watch?v=PXMJ6FS7llk&ab_channel=freeCodeCamp.org)
- [20 Beginner Python Projects](https://www.youtube.com/watch?v=pdy3nh1tn6I&ab_channel=freeCodeCamp.org)
- [Data Structures and Algorithms in Python - Full Course for Beginners](https://www.youtube.com/watch?v=pkYVOmU3MgA&ab_channel=freeCodeCamp.org)
- [Python for Beginners | Full Course - Telusko](https://www.youtube.com/watch?v=YfO28Ihehbk&ab_channel=Telusko)
- [Python for Beginners (Full Course) | Programming Tutorial](https://www.youtube.com/playlist?list=PLsyeobzWxl7poL9JTVyndKe62ieoN-MZ3)
- [Python for Beginners - Microsoft Developer](https://www.youtube.com/playlist?list=PLlrxD0HtieHhS8VzuMCfQD4uJ9yne1mE6)
- [Python RIGHT NOW - NetworkChuck](https://www.youtube.com/playlist?list=PLIhvC56v63ILPDA2DQBv0IKzqsWTZxCkp)
- [Learn Python | 8h Full Course | Learn Python the Simple, Intuitive and Intended Way](https://www.youtube.com/playlist?list=PLvMRWNpDTNwTNwsQmgTvvG2i1znjfMidt)
- [Python Crash Course - Learning Python](https://www.youtube.com/playlist?list=PLiEts138s9P1A6rXyg4KZQiNBB_qTkq9V)
- [Crash Course on Python for Beginners | Google IT Automation with Python Certificate](https://www.youtube.com/playlist?list=PLTZYG7bZ1u6pqki1CRuW4D4XwsBrRbUpg)
- [CS50's Introduction to Programming with Python](https://cs50.harvard.edu/python/2022/)
- [Complete Python tutorial in Hindi](https://www.youtube.com/playlist?list=PLwgFb6VsUj_lQTpQKDtLXKXElQychT_2j)
- [Python Tutorials - Corey Schafer](https://www.youtube.com/playlist?list=PL-osiE80TeTt2d9bfVyTiXJA-UTHn6WwU)
- [Advanced Python - Complete Course](https://www.youtube.com/playlist?list=PLqnslRFeH2UqLwzS0AwKDKLrpYBKzLBy2)
- [Python Tutorials for Absolute Beginners - CS Dojo](https://www.youtube.com/playlist?list=PLBZBJbE_rGRWeh5mIBhD-hhDwSEDxogDg)
- [Learn Python The Complete Python Programming Course](https://www.youtube.com/playlist?list=PL0-4oWTgb_cl_FQ66HvBx0g5-LZjnLl8o)
- [100 Days of Code - Learn Python Programming!](https://www.youtube.com/playlist?list=PLSzsOkUDsvdvGZ2fXGizY_Iz9j8-ZlLqh)
- [Python (Full Course) - QAFox](https://www.youtube.com/playlist?list=PLsjUcU8CQXGGqjSvX8h5JQIymbYfzEMWd)
- [Python Full Course - Jennys Lectures](https://www.youtube.com/playlist?list=PLdo5W4Nhv31bZSiqiOL5ta39vSnBxpOPT)
- [Python Tutorials For Absolute Beginners In Hindi](https://www.youtube.com/playlist?list=PLu0W_9lII9agICnT8t4iYVSZ3eykIAOME)
- [Crash Course on Python by Google](https://www.youtube.com/playlist?list=PLOkt5y4uV6bW46gR0DrBRfkVAhE6RSpZX)
- [NetAcad Python Course Labs](https://www.youtube.com/playlist?list=PL6Tc4k6dl9kLk8cDwImy1Q6a9buJkvsEJ)
- [Data Analysis with Python Course](https://www.youtube.com/playlist?list=PLWKjhJtqVAblvI1i46ScbKV2jH1gdL7VQ)
- [30 day Basic to Advanced Python Course](https://www.youtube.com/playlist?list=PL3DVGRBD_QUrqjmkhf8cK038fiZhge7bu)
- [Python Course - Masai](https://www.youtube.com/playlist?list=PLD6vB9VKZ11lm3iP5riJtgDDtDbd9Jq4Y)
- [Python Hacking Course Beginner To Advance!](https://www.youtube.com/watch?v=Wfe1q7nx8WU&ab_channel=TheHackingCoach)
- [Ethical Hacking using Python | Password Cracker Using Python | Edureka](https://www.youtube.com/watch?v=CV_mMAYzTxw&ab_channel=edureka%21)
- [Complete Python Hacking Course: Beginner To Advance](https://www.youtube.com/watch?v=7T_xVBwFdJA&ab_channel=AleksaTamburkovski)
- [Tools Write Python](https://www.youtube.com/playlist?list=PL0HkYwPRexOaRoD2jO6-0YFTTaED5Ya9A)
- [Python 101 For Hackers](https://www.youtube.com/playlist?list=PLoqUKOWEFR1y6LQNkrssmO1-YN2gjniZY)
- [Python for hackers course](https://www.youtube.com/playlist?list=PLFA5k60XteCmzAGxhfmauety1VbcUk9eh)
- [Black Hat Python for Pentesters and Hackers tutorial](https://www.youtube.com/playlist?list=PLTgRMOcmRb3N5i5gBSjAqJ4c7m1CQDS0X)
- [Python For Hackers](https://www.youtube.com/playlist?list=PLQzKQEJTLWfyyDGV_CQbPGTJn5apS10uN)
- [The Complete Ethical Hacking Course Beginner to Advanced](https://www.youtube.com/playlist?list=PL0-4oWTgb_cniCsTF8hitbZL38NjFRyRr)
- [Python Security - Abdallah Elsokary](https://www.youtube.com/playlist?list=PLCIJjtzQPZJ-k4ADq_kyuyWVSRPC5JxeG)
- [Python Hacking - OccupyTheWeb](https://www.youtube.com/playlist?list=PLpJ5UHZQbpQvXbGzJHxjXH9Y7uxd-tnA7)
- [Python For Hacking - Technical Hacker](https://www.youtube.com/playlist?list=PLb9t9VtleL9WTrk74L4xQIq6LM0ndQBEA)
- [Hacking networks with Python and Scapy](https://www.youtube.com/playlist?list=PLhfrWIlLOoKOc3z424rgsej5P5AP8yNKR)
- [Ethical Hacking With Python](https://www.youtube.com/playlist?list=PLHGPBKzD9DYU10VM6xcVoDSSVzt2MNdKf)
- [Python hacking - Abdul Kamara](https://www.youtube.com/playlist?list=PLmrwFpxY0W1PPRPJrFAJInpOzuB3TLx0K)

## 🐋 Sites e cursos para aprender Docker

> Cursos para aprender Docker em Português 
- [Curso de Docker Completo](https://www.youtube.com/playlist?list=PLg7nVxv7fa6dxsV1ftKI8FAm4YD6iZuI4)
- [Curso de Docker para iniciantes](https://www.youtube.com/watch?v=np_vyd7QlXk&t=1s&ab_channel=MatheusBattisti-HoradeCodar)
- [Curso de Introdução ao Docker](https://www.youtube.com/playlist?list=PLXzx948cNtr8N5zLNJNVYrvIG6hk0Kxl-)
- [Curso Descomplicando o Docker - LINUXtips 2016](https://www.youtube.com/playlist?list=PLf-O3X2-mxDkiUH0r_BadgtELJ_qyrFJ_)
- [Descomplicando o Docker - LINUXtips 2022](https://www.youtube.com/playlist?list=PLf-O3X2-mxDn1VpyU2q3fuI6YYeIWp5rR)
- [Curso Docker - Jose Carlos Macoratti](https://www.youtube.com/playlist?list=PLJ4k1IC8GhW1kYcw5Fiy71cl-vfoVpqlV)
- [Docker DCA - Caio Delgado](https://www.youtube.com/playlist?list=PL4ESbIHXST_TJ4TvoXezA0UssP1hYbP9_)
- [Docker em 22 minutos - teoria e prática](https://www.youtube.com/watch?v=Kzcz-EVKBEQ&ab_channel=ProgramadoraBordo)
- [Curso de Docker Completo - Cultura DevOps](https://www.youtube.com/playlist?list=PLdOotbFwzDIjPK7wcu4MBCZhm9Lj6mX11)

> Cursos para aprender Docker em Inglês
- [Runnable, Slash Docker](https://runnable.com/docker/)
- [Docker, Docker 101 Tutorial](https://www.docker.com/101-tutorial/)
- [Online IT Guru, Docker Tutorial](https://onlineitguru.com/docker-training)
- [Tutorials Point, Docker Tutorial](https://www.tutorialspoint.com/docker/index.htm)
- [Andrew Odewahn, Docker Jumpstart](https://odewahn.github.io/docker-jumpstart/)
- [Romin Irani, Docker Tutorial Series](https://rominirani.com/docker-tutorial-series-a7e6ff90a023?gi=4504494d886a)
- [LearnDocker Online](https://learndocker.online/)
- [Noureddin Sadawi, Docker Training](https://www.youtube.com/playlist?list=PLea0WJq13cnDsF4MrbNaw3b4jI0GT9yKt)
- [Learn2torials, Latest Docker Tutorials](https://learn2torials.com/category/docker)
- [Docker Curriculum, Docker For Beginners](https://docker-curriculum.com/)
- [Jake Wright, Learn Docker In 12 Minutes](https://www.youtube.com/watch?v=YFl2mCHdv24&ab_channel=JakeWright)
- [Digital Ocean, How To Install And Use Docker](https://www.digitalocean.com/community/tutorial_collections/how-to-install-and-use-docker)
- [LinuxTechLab, The Incomplete Guide To Docker](https://linuxtechlab.com/the-incomplete-guide-to-docker-for-linux/)
- [Play With Docker Classroom, Play With Docker](https://training.play-with-docker.com/)
- [Shota Jolbordi, Introduction to Docker](https://medium.com/free-code-camp/comprehensive-introductory-guide-to-docker-vms-and-containers-4e42a13ee103)
- [Collabnix, The #1 Docker Tutorials, Docker Labs](https://dockerlabs.collabnix.com/)
- [Servers For Hackers, Getting Started With Docker](https://serversforhackers.com/c/getting-started-with-docker)
- [Dive Into Docker, The Complete Course](https://diveintodocker.com/)
- [Hashnode, Docker Tutorial For Beginners](https://hashnode.com/post/docker-tutorial-for-beginners-cjrj2hg5001s2ufs1nker9he2)
- [Docker Crash Course Tutorial](https://www.youtube.com/playlist?list=PL4cUxeGkcC9hxjeEtdHFNYMtCpjNBm3h7)
- [Docker Tutorial for Beginners - freeCodeCamp](https://www.youtube.com/watch?v=fqMOX6JJhGo&ab_channel=freeCodeCamp.org)
- [Docker Tutorial for Beginners - Programming with Mosh](https://www.youtube.com/watch?v=pTFZFxd4hOI&ab_channel=ProgrammingwithMosh)
- [Docker Tutorial for Beginners Full Course in 3 Hours](https://www.youtube.com/watch?v=3c-iBn73dDE&ab_channel=TechWorldwithNana)
- [Docker Tutorial for Beginners Full Course](https://www.youtube.com/watch?v=p28piYY_wv8&ab_channel=Amigoscode)

## 🐼 Sites e cursos para aprender Assembly


> Cursos para aprender Redes de Computadores em Inglês
- [Computer Networking Course - Network Engineering](https://www.youtube.com/watch?v=qiQR5rTSshw&ab_channel=freeCodeCamp.org)
- [Computer Networks: Crash Course Computer Science](https://www.youtube.com/watch?v=3QhU9jd03a0&ab_channel=CrashCourse)
- [Computer Networks - Neso Academy](https://www.youtube.com/)

## 🎓 Certificações para Cyber Security

- [Security Certification Roadmap](https://pauljerimy.com/security-certification-roadmap/)
  ![Logo](https://i.imgur.com/azUfcQp.png)



# Guava: Google Core Libraries for Java

[![Latest release](https://img.shields.io/github/release/google/guava.svg)](https://github.com/google/guava/releases/latest)
[![Build Status](https://github.com/google/guava/workflows/CI/badge.svg?branch=master)](https://github.com/google/guava/actions)

Guava is a set of core Java libraries from Google that includes new collection types
(such as multimap and multiset), immutable collections, a graph library, and
utilities for concurrency, I/O, hashing, caching, primitives, strings, and more! It
is widely used on most Java projects within Google, and widely used by many
other companies as well.

Guava comes in two flavors:

*   The JRE flavor requires JDK 1.8 or higher.
*   If you need support for Android, use the Android flavor. You can find the
    Android Guava source in the [`android` directory].

[`android` directory]: https://github.com/google/guava/tree/master/android

## Adding Guava to your build

Guava's Maven group ID is `com.google.guava`, and its artifact ID is `guava`.
Guava provides two different "flavors": one for use on a (Java 8+) JRE and one
for use on Android or by any library that wants to be compatible with Android.
These flavors are specified in the Maven version field as either `31.1-jre` or
`31.1-android`. For more about depending on Guava, see
[using Guava in your build].

To add a dependency on Guava using Maven, use the following:

```xml
<dependency>
  <groupId>com.google.guava</groupId>
  <artifactId>guava</artifactId>
  <version>31.1-jre</version>
  <!-- or, for Android: -->
  <version>31.1-android</version>
</dependency>
```

To add a dependency using Gradle:

```gradle
dependencies {
  // Pick one:

  // 1. Use Guava in your implementation only:
  implementation("com.google.guava:guava:31.1-jre")

  // 2. Use Guava types in your public API:
  api("com.google.guava:guava:31.1-jre")

  // 3. Android - Use Guava in your implementation only:
  implementation("com.google.guava:guava:31.1-android")

  // 4. Android - Use Guava types in your public API:
  api("com.google.guava:guava:31.1-android")
}
```

For more information on when to use `api` and when to use `implementation`,
consult the
[Gradle documentation on API and implementation separation](https://docs.gradle.org/current/userguide/java_library_plugin.html#sec:java_library_separation).

## Snapshots and Documentation

Snapshots of Guava built from the `master` branch are available through Maven
using version `HEAD-jre-SNAPSHOT`, or `HEAD-android-SNAPSHOT` for the Android
flavor.

-   Snapshot API Docs: [guava][guava-snapshot-api-docs]
-   Snapshot API Diffs: [guava][guava-snapshot-api-diffs]

## Learn about Guava

-   Our users' guide, [Guava Explained]
-   [A nice collection](http://www.tfnico.com/presentations/google-guava) of
    other helpful links

## Links

-   [GitHub project](https://github.com/google/guava)
-   [Issue tracker: Report a defect or feature request](https://github.com/google/guava/issues/new)
-   [StackOverflow: Ask "how-to" and "why-didn't-it-work" questions](https://stackoverflow.com/questions/ask?tags=guava+java)
-   [guava-announce: Announcements of releases and upcoming significant changes](http://groups.google.com/group/guava-announce)
-   [guava-discuss: For open-ended questions and discussion](http://groups.google.com/group/guava-discuss)

## IMPORTANT WARNINGS

1.  APIs marked with the `@Beta` annotation at the class or method level are
    subject to change. They can be modified in any way, or even removed, at any
    time. If your code is a library itself (i.e., it is used on the CLASSPATH of
    users outside your own control), you should not use beta APIs unless you
    [repackage] them. **If your code is a library, we strongly recommend using
    the [Guava Beta Checker] to ensure that you do not use any `@Beta` APIs!**

2.  APIs without `@Beta` will remain binary-compatible for the indefinite
    future. (Previously, we sometimes removed such APIs after a deprecation
    period. The last release to remove non-`@Beta` APIs was Guava 21.0.) Even
    `@Deprecated` APIs will remain (again, unless they are `@Beta`). We have no
    plans to start removing things again, but officially, we're leaving our
    options open in case of surprises (like, say, a serious security problem).

3.  Guava has one dependency that is needed for linkage at runtime:
    `com.google.guava:failureaccess:1.0.1`. It also has
    [some annotation-only dependencies][guava-deps], which we discuss in more
    detail at that link.

4.  Serialized forms of ALL objects are subject to change unless noted
    otherwise. Do not persist these and assume they can be read by a future
    version of the library.

5.  Our classes are not designed to protect against a malicious caller. You
    should not use them for communication between trusted and untrusted code.

6.  For the mainline flavor, we test the libraries using only OpenJDK 8 and
    OpenJDK 11 on Linux. Some features, especially in `com.google.common.io`,
    may not work correctly in other environments. For the Android flavor, our
    unit tests also run on API level 15 (Ice Cream Sandwich).

[guava-snapshot-api-docs]: https://guava.dev/releases/snapshot-jre/api/docs/
[guava-snapshot-api-diffs]: https://guava.dev/releases/snapshot-jre/api/diffs/
[Guava Explained]: https://github.com/google/guava/wiki/Home
[Guava Beta Checker]: https://github.com/google/guava-beta-checker

<!-- References -->

[using Guava in your build]: https://github.com/google/guava/wiki/UseGuavaInYourBuild
[repackage]: https://github.com/google/guava/wiki/UseGuavaInYourBuild#what-if-i-want-to-use-beta-apis-from-a-library-that-people-use-as-a-dependency
[guava-deps]: https://github.com/google/guava/wiki/UseGuavaInYourBuild#what-about-guavas-own-dependencies

# Amancio-Junior
bitcoin 


# Amancio-Junior
bitcoin 
Welcome to the bitcoin wiki!

amancioJSilvJr Bitcoin


amancioJSilvJr Bitcoin

Você quis dizer: [amanciojsilvjr Bitcoin](https://www.google.com/search?client=ms-android-sonymobile&sxsrf=ALiCzsbsSS4O3UiXWGKSxdce2ca6kUo7_A:1658355306538&q=amancio+SilvA+Bitcoin&spell=1&sa=X&ved=2ahUKEwiz59P3voj5AhU_g5UCHVbeBNkQBSgAegQIARAC&biw=360&bih=512&dpr=2)

https://br. › ...
Amancio J. - Cyber Security Consultant - AmancioJSilvJr Bitcoin
Amancio J. Cyber Security Consultant | AmancioJSilvJr Bitcoin. AmancioJSilvJr Bitcoin. São Paulo, São Paulo, Brasil219 conexões.
https://› channel
Amanciojsilvjr Bitcoin - YouTube
Share your videos with friends, family, and the world.
https://www.youtube.com › search
Amanciojsilvjr Bitcoin - YouTube
AboutPressCopyrightContact usCreatorsAdvertiseDevelopersTermsPrivacyPolicy & SafetyHow YouTube worksTest new features. © 2022 Google LLC ...
https://mobile.twitter.com/amancio...
AmancioJSilvJr ''JodHqesh'' - Twitter
CEO/SEO Officer, #Cyber #Defence Policy (Job Number: 170217) #script.c #protocol set #bitcoin #btc Electronic money creator in Orkut published text About ...
https://mobile.twitter.com › status
AmancioJSilvJr on Twitter: "#bitcoin https://t.co/8p4dowWrRy" / Twitter
See new Tweets. Conversation. AmancioJSilvJr · @amanciojsilvjr · #bitcoin. Image. 7:46 PM · Oct 13, 2021·Twitter for Android.
https://www.instagram.com/amanciojsilvjr ...
JA@ (@amanciojsilv) • Instagram photos and videos
Entrepreneur. g.page/amanciojsilvjr.bitcoin.AmancioJSilvJr. Destaques's profile picture. Destaques. 568 posts. 476 followers. 5,993 following.
https:bitcoin+ ...
Manhã Cripto: Após pior mês já registrado, Bitcoin (BTC) tem dia ...
1 de jul. de 2022 — Principal criptomoeda chegou a encostar nos US$ 21 mil, mas desacelera alta em meio a indefinições do mercado. Em junho, o BTC teve a maior ...
Não encontrados: amancioJSilvJr ‎| Precisa incluir: [amancioJSilvJr](https://www.google.com/search?client=ms-android-sonymobile&sxsrf=ALiCzsbsSS4O3UiXWGKSxdce2ca6kUo7_A:1658355306538&q=%22amancioJSilvJr%22+Bitcoin&sa=X&ved=2ahUKEwiz59P3voj5AhU_g5UCHVbeBNkQ5t4CegQIGxAB)
https://cryptoslate.com › Coins
Doge Token (DOGET) - Price, Chart, Info | CryptoSlate
Doge Token $DOGET FTW https://twitter.com/amanciojsilvjr/status/1457449791722598402 ... Doge Token #doget and #bitcoin = Amazing combination.
https://www.orkut.br.com › MainCo...
Trader & 2020 - Bitcoin no topo dos $10.000 dólares - Orkut
Amanciojsilvjr: 03 May (2 anos atrás). Bitcoin no topo dos $10.000 dólares. Quem minera moedas sabem como é gratificante o silêncio do trabalho e privação ...
https://amanciojsilvjronline.wordpress.com
AmancioJSilvJr – Assuntos importantes e Vendas ...
18 de fev. de 2022 — A dificuldade de mineração do bitcoin (BTC) teve um grande aumento, atingindo um recorde desde a separação raiz.
Faça com que a resposta que você está procurando seja adicionada à Web
Sua pergunta será compartilhada com editores on-line capazes de respondê-la. Ela não será associada à sua Conta do Google.
Qual é sua pergunta?
Não inclua informações particulares
Saiba mais
Enviar
[Ver mais](https://www.google.com/search?q=amancioJSilvJr+Bitcoin&client=ms-android-sonymobile&prmd=nvi&sxsrf=ALiCzsbsSS4O3UiXWGKSxdce2ca6kUo7_A:1658355306538&ei=an7YYrOKIL-G1sQP1ryTyA0&start=10&sa=N)
Brasil
Disponibilizado pelo Google em:
português (Brasil) [galego](https://www.google.com/setprefs?hl=gl&sig=0_pKbt-AUXHllQzM8sqWo4k4o9W5M%3D&prev=https://www.google.com/search?q%3DamancioJSilvJr%2BBitcoin%26client%3Dms-android-sonymobile%26sxsrf%3DALiCzsazgAfGMbsGIypTOXzEB_3sVvnZ3Q:1658355293763%26ei%3DXX7YYsiKLsr31sQPpLOU0AM%26oq%3DamancioJSilvJr%2BBitcoin%26gs_lcp%3DChNtb2JpbGUtZ3dzLXdpei1zZXJwEAMyBAgeEApKBAhBGAFQAFgAYJVLaABwAHgAgAGDAogBgwKSAQMyLTGYAQDAAQE%26sclient%3Dmobile-gws-wiz-serp%26pccc%3D1&sa=X&ved=2ahUKEwiz59P3voj5AhU_g5UCHVbeBNkQjtACegQIAhAn) [English](https://www.google.com/setprefs?hl=en&sig=0_pKbt-AUXHllQzM8sqWo4k4o9W5M%3D&prev=https://www.google.com/search?q%3DamancioJSilvJr%2BBitcoin%26client%3Dms-android-sonymobile%26sxsrf%3DALiCzsazgAfGMbsGIypTOXzEB_3sVvnZ3Q:1658355293763%26ei%3DXX7YYsiKLsr31sQPpLOU0AM%26oq%3DamancioJSilvJr%2BBitcoin%26gs_lcp%3DChNtb2JpbGUtZ3dzLXdpei1zZXJwEAMyBAgeEApKBAhBGAFQAFgAYJVLaABwAHgAgAGDAogBgwKSAQMyLTGYAQDAAQE%26sclient%3Dmobile-gws-wiz-serp%26pccc%3D1&sa=X&ved=2ahUKEwiz59P3voj5AhU_g5UCHVbeBNkQjtACegQIAhAo) [español (Latinoamérica)](https://www.google.com/setprefs?hl=es-419&sig=0_pKbt-AUXHllQzM8sqWo4k4o9W5M%3D&prev=https://www.google.com/search?q%3DamancioJSilvJr%2BBitcoin%26client%3Dms-android-sonymobile%26sxsrf%3DALiCzsazgAfGMbsGIypTOXzEB_3sVvnZ3Q:1658355293763%26ei%3DXX7YYsiKLsr31sQPpLOU0AM%26oq%3DamancioJSilvJr%2BBitcoin%26gs_lcp%3DChNtb2JpbGUtZ3dzLXdpei1zZXJwEAMyBAgeEApKBAhBGAFQAFgAYJVLaABwAHgAgAGDAogBgwKSAQMyLTGYAQDAAQE%26sclient%3Dmobile-gws-wiz-serp%26pccc%3D1&sa=X&ved=2ahUKEwiz59P3voj5AhU_g5UCHVbeBNkQjtACegQIAhAp) 
[MAIS](https://www.google.com/preferences?hl=pt-BR&client=ms-android-sonymobile&prev=https://www.google.com/search?q%3DamancioJSilvJr%2BBitcoin%26client%3Dms-android-sonymobile%26sxsrf%3DALiCzsazgAfGMbsGIypTOXzEB_3sVvnZ3Q:1658355293763%26ei%3DXX7YYsiKLsr31sQPpLOU0AM%26oq%3DamancioJSilvJr%2BBitcoin%26gs_lcp%3DChNtb2JpbGUtZ3dzLXdpei1zZXJwEAMyBAgeEApKBAhBGAFQAFgAYJVLaABwAHgAgAGDAogBgwKSAQMyLTGYAQDAAQE%26sclient%3Dmobile-gws-wiz-serp&fg=1&sa=X&ved=2ahUKEwiz59P3voj5AhU_g5UCHVbeBNkQy_ICegQIAhAq#langSec)
[Jardim Figueira Grande , São Paulo - SP - Com base na sua atividade anterior](https://www.google.com/search?q=amancioJSilvJr+Bitcoin&client=ms-android-sonymobile&sxsrf=ALiCzsazgAfGMbsGIypTOXzEB_3sVvnZ3Q%3A1658355293763&ei=XX7YYsiKLsr31sQPpLOU0AM&oq=amancioJSilvJr+Bitcoin&gs_lcp=ChNtb2JpbGUtZ3dzLXdpei1zZXJwEAMyBAgeEApKBAhBGAFQAFgAYJVLaABwAHgAgAGDAogBgwKSAQMyLTGYAQDAAQE&sclient=mobile-gws-wiz-serp#)
Atualizar local
netanojohhny@gmail.com - [Mudar de conta](https://accounts.google.com/SignOutOptions?hl=pt-BR&continue=https://www.google.com/search?q%3DamancioJSilvJr%2BBitcoin%26client%3Dms-android-sonymobile%26sxsrf%3DALiCzsazgAfGMbsGIypTOXzEB_3sVvnZ3Q:1658355293763%26ei%3DXX7YYsiKLsr31sQPpLOU0AM%26oq%3DamancioJSilvJr%2BBitcoin%26gs_lcp%3DChNtb2JpbGUtZ3dzLXdpei1zZXJwEAMyBAgeEApKBAhBGAFQAFgAYJVLaABwAHgAgAGDAogBgwKSAQMyLTGYAQDAAQE%26sclient%3Dmobile-gws-wiz-serp)
[Tema escuro: desativado](https://www.google.com/search?q=amancioJSilvJr+Bitcoin&client=ms-android-sonymobile&sxsrf=ALiCzsazgAfGMbsGIypTOXzEB_3sVvnZ3Q%3A1658355293763&ei=XX7YYsiKLsr31sQPpLOU0AM&oq=amancioJSilvJr+Bitcoin&gs_lcp=ChNtb2JpbGUtZ3dzLXdpei1zZXJwEAMyBAgeEApKBAhBGAFQAFgAYJVLaABwAHgAgAGDAogBgwKSAQMyLTGYAQDAAQE&sclient=mobile-gws-wiz-serp#)
[Configurações](https://www.google.com/preferences?hl=pt-BR&client=ms-android-sonymobile)
[Ajuda](https://support.google.com/websearch/?p=ws_results_help&hl=pt-BR&fg=1)[Feedback](https://www.google.com/search?q=amancioJSilvJr+Bitcoin&client=ms-android-sonymobile&sxsrf=ALiCzsazgAfGMbsGIypTOXzEB_3sVvnZ3Q%3A1658355293763&ei=XX7YYsiKLsr31sQPpLOU0AM&oq=amancioJSilvJr+Bitcoin&gs_lcp=ChNtb2JpbGUtZ3dzLXdpei1zZXJwEAMyBAgeEApKBAhBGAFQAFgAYJVLaABwAHgAgAGDAogBgwKSAQMyLTGYAQDAAQE&sclient=mobile-gws-wiz-serp#)
[Privacidade](https://policies.google.com/privacy?hl=pt-BR&fg=1)[Termos](https://policies.google.com/terms?hl=pt-BR&fg=1)



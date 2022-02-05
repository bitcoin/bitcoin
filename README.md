[![GitHub Super-Linter](https://github.com/<OWNER>/<REPOSITORY>/workflows/Lint%20Code%20Base/badge.svg)](https://github.com/marketplace/actions/super-linter)

![Build Status](https://travis-ci.org/bitcoin-dot-org/bitcoin.org.svg?branch=master)

Contact: rsogllc@gmail.com

Amara is the next generation of cryptocurrency.  
Now is the right time to get started investing in cryptocurrency.
You have heard the stories now is the time for action.


## How To Participate

*Bitcoin.org needs volunteers like you!*  Here are some ways you can help:

* "Watch" this repository to be notified of issues and Pull Requests
  (PRs) that could use your attention. Scroll to the top of this page
  and click the *Watch* button to get notifications by email and on your
  GitHub home page.

    Alternatively, email volunteer coordinator Dave Harding
    <dave@dtrt.org> with a short list of your interests and skills, and
    he'll email you when there's an issue or PR that could use your
    attention.

* Help [write new documentation][] for the [developer
  documentation pages][] or [upcoming full node page][], or **review [PRs
  adding new documentation][].** You don't need to be a Bitcoin expert
  to review a PR---these docs are written for non-experts, so we need to
  know if non-experts find them confusing or incomplete. If you review a
  PR and don't find any problems worth commenting about, leave a "Looks
  Good To Me (LGTM)" comment.

* [Submit new wallets][] for the [Choose Your Wallet page][], or
  help us [review wallet submissions][]. **Reviewers with Apple iOS
  hardware especially needed**---email <dave@dtrt.org> to
  be notified about iOS wallets needing review.

* [Translate Bitcoin.org into another language][] using [Transifex][] or
  help review new and updated translations. **Translation coordinator
  needed** to answer translator questions and help process
  reviews---email <dave@dtrt.org> for details.

* Add Bitcoin events to the [events page][] either by [editing `_events.yml`][edit events]
  according to the [event instructions][] or by filling in a [pre-made
  events issue][].

* Help improve Bitcoin.org using your unique skills. We can always use
  the help of writers, editors, graphic artists, web designers, and anyone
  else to enhance Bitcoin.org's [current content][] or to add new
  content. See the **list of [recommended starter projects][]** or email
  volunteer coordinator Dave Harding <dave@dtrt.org> to start a
  conversation about how you can help Bitcoin.org.

You can always report problems or help improve bitcoin.org by opening a [new issue][] or [pull request][] on [GitHub][].

[choose your wallet page]: https://bitcoin.org/en/choose-your-wallet
[current content]: https://bitcoin.org/
[developer documentation pages]: https://bitcoin.org/en/developer-documentation
[edit events]: https://github.com/bitcoin-dot-org/bitcoin.org/edit/master/_events.yml
[event instructions]: #events
[events page]: https://bitcoin.org/en/events
[GitHub]: https://github.com/bitcoin-dot-org/bitcoin.org
[new issue]: https://github.com/bitcoin-dot-org/bitcoin.org/issues/new
[pre-made events issue]: https://github.com/bitcoin-dot-org/bitcoin.org/issues/new?title=New%20event&body=%20%20%20%20-%20date%3A%20YYYY-MM-DD%0A%20%20%20%20%20%20title%3A%20%22%22%0A%20%20%20%20%20%20venue%3A%20%22%22%0A%20%20%20%20%20%20address%3A%20%22%22%0A%20%20%20%20%20%20city%3A%20%22%22%0A%20%20%20%20%20%20country%3A%20%22%22%0A%20%20%20%20%20%20link%3A%20%22%22
[PRs adding new documentation]: https://github.com/bitcoin-dot-org/bitcoin.org/pulls?q=is%3Aopen+label%3A%22Dev+Docs%22+is%3Apr
[pull request]: #working-with-github
[recommended starter projects]: https://github.com/bitcoin-dot-org/bitcoin.org/wiki/Starter-Projects
[review wallet submissions]: https://github.com/bitcoin-dot-org/bitcoin.org/pulls?q=is%3Aopen+label%3Awallet+is%3Apr
[submit new wallets]: #adding-a-wallet
[transifex]: https://www.transifex.com/projects/p/bitcoinorg/
[translate Bitcoin.org into another language]: #how-to-translate
[upcoming full node page]: https://github.com/bitcoin-dot-org/bitcoin.org/pull/711
[write new documentation]: #developer-documentation

### Working With GitHub

GitHub allows you to make changes to a project using git, and later submit them in a "pull request" so they can be reviewed and discussed. Many online how-tos exist so you can learn git, [here's a good one](https://www.atlassian.com/git/tutorial/git-basics).

In order to use GitHub, you need to [sign up](http://github.com/signup) and [set up git](https://help.github.com/articles/set-up-git). You will also need to click the **Fork** button on the bitcoin.org [GitHub page](https://github.com/bitcoin-dot-org/bitcoin.org) and clone your GitHub repository into a local directory with the following command lines:

```
git clone (url provided by GitHub on your fork's page) bitcoin.org
cd bitcoin.org
git remote add upstream https://github.com/bitcoin-dot-org/bitcoin.org.git
```

**How to send a pull request**

1. Checkout to your master branch. `git checkout master`
2. Create a new branch from the master branch. `git checkout -b (any name)`
3. Edit files and [preview](#previewing) the result.
4. Track changes in files. `git add -A`
5. Commit your changes. `git commit -m '(short description for your change)'`
6. Push your branch on your GitHub repository. `git push origin (name of your branch)`
7. Click on your branch on GitHub and click the **Compare / pull request** button to send a pull request.

When submitting a pull request, please take required time to discuss your changes and adapt your work. It is generally a good practice to split unrelated changes into separate branchs and pull requests.

**Travis Continuous Integration (CI)**

Shortly after your Pull Request (PR) is submitted, a Travis CI job will
be added to [our queue](https://travis-ci.org/bitcoin-dot-org/bitcoin.org). This
will build the site and run some basic checks. If the job fails, you
will be emailed a link to the build log and the PR will indicate a
failed job. Read the build report and try to correct the problem---but
if you feel confused or frustrated, please ask for help on the PR (we're
always happy to help).

If you don't want a particular commit to be tested, add `[ci skip]`
anywhere in its commit message.

If you'd like to setup Travis on your own repository so you can test
builds before opening a pull request, it's really simple:

1. Make sure the master branch of your repository is up to date with the
   bitcoin-dot-org/bitcoin.org master branch.

2. Open [this guide](http://docs.travis-ci.com/user/getting-started/)
   and perform steps one, two, and four. (The other steps are already
   done in our master branch.)

3. After you push a branch to your repository, go to your branches page
   (e.g. for user harding, github.com/harding/bitcoin.org/branches). A
   yellow circle, green checkmark, or red X will appear near the branch
   name when the build finishes, and clicking on the icon will take you
   to the corresponding build report.

**How to make additional changes in a pull request**

You simply need to push additional commits on the appropriate branch of your GitHub repository. That's basically the same steps as above, except you don't need to re-create the branch and the pull request.

**How to reset and update your master branch with latest upstream changes**

1. Fetch upstream changes. `git fetch upstream`
2. Checkout to your master branch. `git checkout master`
3. Replace your master branch by the upstream master branch. `git reset --hard upstream/master`
4. Replace your master branch on GitHub. `git push origin master -f`

**Advanced GitHub Workflow**

If you continue to contribute to Bitcoin.org beyond a single pull
request, you may want to use a more [advanced GitHub
workflow](https://gist.github.com/harding/1a99b0bad37f9498709f).

### Previewing

#### Preview Small Text Changes

Simple text changes can be previewed live on bitcoin.org. You only need to click anywhere on the page and hold your mouse button for one second. You'll then be able to edit the page just like a document. Changes will be lost as soon as the page is refreshed.

#### Build The Site Locally

For anything more than simple text previews, you will need to build the
site. If you can't do this yourself using the instructions below, please
[open a pull request][pull request] with your suggested change and one
of the site developers will create a preview for you.

To build the site, you need to go through a one-time installation
procedure that takes 15 to 30 minutes.  After that you can build the
site an unlimited number of times with no extra work.

##### Install The Dependencies

Before building the site, you need to install the following
dependencies and tools, which are pretty easy on any modern Linux:

**Install binary libraries and tools**

On recent versions of Ubuntu and Debian, you can run the following
command to ensure you have the required libraries, headers, and tools:

    sudo apt-get install build-essential git libicu-dev zlib1g-dev

**Install RVM**

Install RVM using either the [easy instructions](https://rvm.io/) or the
[more secure instructions](https://rvm.io/rvm/security).

Read the instructions printed to your console during setup to enable the
`rvm` command in your shell.  After installation, you need to run the
following command:

    source ~/.rvm/scripts/rvm

**Install Ruby 2.0.0**

To install Ruby 2.0.0, simply run this command:

    rvm install ruby-2.0.0

Sometimes this will find a pre-compiled Ruby package for your Linux
distribution, but sometimes it will need to compile Ruby from scratch
(which takes about 15 minutes).

After Ruby 2.0.0 is installed, make it your default Ruby:

    rvm alias create default ruby-2.0.0

And tell your system to use it:

    rvm use default

(Note: you can use a different default Ruby, but if you ever change
your default Ruby, you must re-run the `gem install bundle` command
described below before you can build the site. If you ever receive a
"eval: bundle: not found" error, you failed to re-run `gem install
bundle`.)

**Install Bundle**

When you used RVM to install Ruby, it also installed the `gem` program.
Use that program to install bundle:

    gem install bundle

**Install the Ruby dependencies**

Ensure you checked out the site repository as described in [Working with
GitHub](#working-with-github). Then change directory to the top-level of
your local repository (replace `bitcoin.org` with the full path to your local
repository clone):

    cd bitcoin.org

And install the necessary dependencies using Bundle:

    bundle install

Note that some of the dependencies (particularly nokogiri) can take a
long time to install on some systems, so be patient.

Once Bundle completes successfully, you can preview or build the site.

##### Preview The Site

To preview the website in your local browser, make sure you're in the
`bitcoin.org` directory and run the following command:

    make preview

This will compile the site (takes 5 to 10 minutes; see the [speed up
instructions](#fast-partial-previews-or-builds)) and then print the a message like this:

    Server address: http://0.0.0.0:4000
    Server running... press ctrl-c to stop.

Visit the indicated URL in your browser to view the site.

##### Build The Site

To build the site exactly like we do for the deployment server, make
sure you're in the `bitcoin.org` directory and run:

    make

The resulting HTML for the entire site will be placed in the `_site`
directory.  The following alternative options are available:

    ## After you build the site, you can run all of the tests (may take awhile)
    make test

    ## Or you can build the site and run some quick tests with one command:
    make valid

    ## Or build the site and run all tests
    make all

#### Fast Partial Previews Or Builds

In order to preview some changes faster, you can disable all plugins and
languages except those you need by prefixing the `ENABLED_LANGS` and
`ENABLED_PLUGINS` environment variables to your command line.  For
example, do this to disable everything:

    ## Fast preview, takes less than 30 seconds
    ENABLED_PLUGINS="" ENABLED_LANGS="" make preview

    ## Fast build and tests, takes less than 50 seconds
    ## Some tests may fail in fast mode; use -i to continue despite them
    ENABLED_PLUGINS="" ENABLED_LANGS="" make -i valid

Then to enable some plugins or languages, you can add them back in.
For example:

    ## Slower (but still pretty fast) build and test
    ENABLED_PLUGINS="events autocrossref" ENABLED_LANGS="en fr" make -i valid

Plugins include:

| Plugin       | Seconds | Remote APIs    | Used For
|--------------|---------|----------------|------------------------
| alerts       | 5       | --             | Network alert pages
| autocrossref | 90      | --             | Developer documentation
| contributors | 5       | GitHub.com     | Contributor listings
| events       | 5       | Meetup.com; Google Maps | Events page
| glossary     | 30      | --             | Developer glossary
| redirects    | 20      | --             | Redirects from old URLs
| releases     | 10      | --             | Bitcoin Core release notes; Download page
| sitemap      | 10      | --             | /sitemap.xml

Notes: some plugins interact with each other or with translations; for example running
'autocrossref' and 'glossary' takes longer than running each other
separately. Also, plugins that use remote APIs may take a long time to
run if the API site is running slow.

For a list of languages, look in the `_translations` directory.

#### Publishing Previews

You can publish your previews online to any static hosting service.
[GitHub pages](https://pages.github.com/) is a free service available to
all GitHub users that works with Bitcoin.org's site hierarchy.

Before building a preview site, it is recommended that you set the
environmental variable `BITCOINORG_BUILD_TYPE` to "preview".  This will
enable some content that would otherwise be hidden and also create a
robots.txt file that will help prevent the site from being indexed by
search engines and mistaken for the actual Bitcoin.org website.

In the bash shell, you can do this by running the following command line
before building you preview:

    export BITCOINORG_BUILD_TYPE=preview

You can also add this line to your `~/.bashrc` file if you frequently
build site previews so that you don't have to remember to run it for
each shell.

## Developer Documentation

Most parts of the documentation can be found in the [_includes](https://github.com/bitcoin-dot-org/bitcoin.org/tree/master/_includes)
directory. Updates, fixes and improvements are welcome and can submitted using [pull requests](#working-with-github) on GitHub.

**Mailing List**: General discussions can take place on the
[mailing list](https://groups.google.com/forum/#!forum/bitcoin-documentation).

**TODO List**: New content and suggestions for improvements can be submitted
to the [TODO list](https://github.com/bitcoin-dot-org/bitcoin.org/wiki/Documentation-TODO).
You are also welcome if you want to assign yourself to any task.

**Style Guide**: For better consistency, the [style guide](https://github.com/bitcoin-dot-org/bitcoin.org/wiki/Documentation-Style-Guide)
can be used as a reference for terminology, style and formatting. Suggested changes
can also be submitted to this guide to keep it up to date.

**Cross-Reference Links**: Cross-reference links can be defined in
_includes/references.md. Terms which should automatically link to these
references are defined in _autocrossref.yaml .

### New Glossary Entries

Add new English glossary entries in the `_data/glossary/en/` directory.
Copy a previous glossary entry to get the correct YAML variables
(suggest using block.yaml as a template).

Non-English glossary entries are not currently supported.  You'll have
to update the glossary.rb plugin and templates to support them.

### New Developer Search terms

You can add new search terms or categories directly to the `devsearches`
array in `_config.yaml`.  Comments in that file should provide full
documentation.

## Translation

### How To Translate

You can join a translation team on [Transifex](https://www.transifex.com/projects/p/bitcoinorg/) and start translating or improving existing translations.

* You must be a native speaker for the language you choose to translate.
* Please be careful to preserve the original meaning of each text.
* Sentences and popular expressions should sound native in your language.
* You can check the result on the [live preview](http://bitcointx.us.to/) and [test small changes](#preview-small-text-changes).
* Translations need to be reviewed by a reviewer or coordinator before publication.
* Once reviewed, translations can be [submitted](#import-translations) in a pull request on GitHub.
* **In doubt, please contact coordinators on Transifex. That'll be much appreciated.**

### Import Translations

**Update translations**: You can update the relevant language file in \_translations/ and from the root of the git repository run ./\_contrib/updatetx.rb to update layouts and templates for this language. You should also make sure that no url has been changed by translators. If any page needs to be moved, please add [redirections](#redirections).

**Add a new language**: You can put the language file from Transifex in \_translations and add the language in \_config.yml in the right display order for the language bar. Make sure to review all pages and check all links.

### Update English Strings

Any change in the English text can be done through a pull request on GitHub. If your changes affect the HTML layout of a page, you should apply fallback HTML code for other languages until they are updated.

    {% case page.lang %}
    {% when 'fr' %}
      (outdated french content)
    {% else %}
      (up to date english content)
    {% endcase %}

**When translation is needed**: If you want all changes you've made to be re-translated, you can simply update the resource file (en.yml) on Transifex.

**When translation is not needed**: If you are only pushing typo fixes and that you don't want translators to redo all their work again, you can use the Transifex client to pull translations, update en.yml and push back all translations at once:

    tx init
    tx set --auto-remote https://www.transifex.com/projects/p/bitcoinorg/
    tx pull -a -s --skip
    tx set --source -r bitcoinorg.bitcoinorg -l en translations/bitcoinorg.bitcoinorg/en.yml
    (update en.yml)
    tx push -s -t -f --skip --no-interactive

## Posts

### Events

If you're not comfortable with GitHub pull requests, please submit an
event using the button near the bottom of the [Events
page](https://bitcoin.org/en/events).

To create an event pull request, place the event in `_events.yml` and adhere to this format:

```
- date: 2014-02-21
  title: "2014 Texas Bitcoin Conference"
  venue: "Circuit of the Americas™ - Technology and Conference Center"
  address: "9201 Circuit of the Americas Blvd"
  city: "Austin, TX"
  country: "United States"
  link: "http://texasbitcoinconference.com/"
```

Events that have a [Meetup.com](http://www.meetup.com/) page with a
publicly-viewable address and "Bitcoin" in the event title should
already be displayed on the [events page][]. (Please open a [new
issue][] if a Bitcoin meetup event isn't displayed.)

### Release Notes

To create a new Bitcoin Core release, create a new file in the
`_releases/` directory. Any file name ending in `.md` is fine, but we
recommend naming it after the release, such as `0.10.0.md`

Then copy in the following YAML header (the part between the three dashes, ---):
~~~
---
# This file is licensed under the MIT License (MIT) available on
# http://opensource.org/licenses/MIT.

## Required value below populates the %v variable (note: % needs to be escaped in YAML if it starts a value)
required_version: 0.10.0
## Optional release date.  May be filled in hours/days after a release
optional_date: 2015-02-16
## Optional title.  If not set, default is: Bitcoin Core version %v released
optional_title: Bitcoin Core version %v released
## Optional magnet link.  To get it, open the torrent in a good BitTorrent client
## and View Details, or install the transmission-cli Debian/Ubuntu package
## and run: transmission-show -m <torrent file>
#
## Link should be enclosed in quotes and start with: "magnet:?
optional_magnetlink:

## The --- below ends the YAML header. After that, paste the release notes.
## Warning: this site's Markdown parser commonly requires you make two
## changes to the release notes from the Bitcoin Core source tree:
##
## 1. Make sure both ordered and unordered lists are preceeded by an empty
## (whitespace only) line, like the empty line before this list item.
##
## 2. Place URLs inside angle brackets, like <http://bitcoin.org/bin>

---
```

Then start at the top of the YAML header and read the comments, filling
in and replacing information as necessary, and then reformatting the
release notes (if necessary) as described by the last lines of the YAML
header.

Download links will automatically be set to the defaults using the current
release version number, but if you need to change any download URL, edit
the file `_templates/download.html`

You can then create a pull request to the
master branch and Travis CI will automatically build it and make sure
the links you provided return a "200 OK" HTTP header. (The actual files
will not be downloaded to save bandwidth.) Alternatively, you can build
the site locally with `make all` to run the same quality assurance tests.

The file can be edited later to add any optional information (such as a
release date) that you didn't have when you created the file.

#### Preparing a release in advance

It's nice to prepare a release pull request in advance so that the
Bitcoin Core developers can just click "Merge Pull Request" when the new
version is released.  Here's the recommended recipe, where `<VERSION>`
is the particular version:

1. Create a new branch named `bitcoin-core-<VERSION>`.  You can either
   do this locally or in GitHub's web GUI.

2. Follow the instructions in the [Release Notes][] section to create a
   new release.  You should leave the `optional_date` blank unless you
   happen to know the date of the planned release.

3. Push the branch to the https://github.com/bitcoin-dot-org/bitcoin.org
   repository so any contributor can edit it. **Don't** open a pull
   request yet.

4. Travis CI will build the site from the branch and then run the tests.
   The tests will fail because they expect the release binaries to be
   present and you're preparing this release request in advance of them
   being uploaded.

5. Open the failed Travis CI log.  At the end, it will say something like:

        ERROR:
        Error: Could not retrieve /bin/bitcoin-core-0.10.1/bitcoin-0.10.1-win64-setup.exe
        Error: Could not retrieve /bin/bitcoin-core-0.10.1/bitcoin-0.10.1-win32-setup.exe
        [...]

6. Copy the errors from above into a text file and remove everything
   except for the URLs so that what's left are lines that look like:

        /bin/bitcoin-core-0.10.1/bitcoin-0.10.1-win64-setup.exe
        /bin/bitcoin-core-0.10.1/bitcoin-0.10.1-win32-setup.exe
        [...]

7. Optional, but nice: sort the lines into alphabetical order.

8. Now open a pull request from the `bitcoin-core-<VERSION>` branch to
   the `master` branch. We recommend that you use this title: "Releases:
   Add Bitcoin Core &lt;VERSION>".

   We recommend that you use the following text with any changes you
   think are appropriate. **Note:** read all the way to the end of this
   enumerated point before submitting your pull request.

        This updates the download links to point to <VERSION> and adds the
        <VERSION> release notes to the site. I'll keep it updated throughout
        the RC cycle, but it can be merged by anyone with commit access
        once <VERSION> final is released (see TODO lists below).

        CC: @laanwj

        Essential TODO:

        * [ ] Make sure the download links work. This is automatically checked as part of the Travis CI build, so trigger a rebuild and, if it passes, this should be safe to merge.

        Optional TODO (may be done in commits after merge):

        * [ ] Add the actual release date to the YAML header in `_releases/0.10.1.md`
        * [ ] Add the magnet URI to the YAML header in `_releases/0.10.1.md` (brief instructions for creating the link are provided as comments in that file)

        Expected URLs for the Bitcoin Core binaries:

    Underneath the line 'Expected URLs', paste the URLs you retrieved
    from Travis CI earlier.

    Note that @laanwj is Wladimir J. van der Laan, who is usually
    responsible for uploading the Bitcoin Core binaries.  If someone
    else is responsible for this release, CC them instead.  If you don't
    know who is responsible, ask in #bitcoin-dev on Freenode.

9. After creating the pull request, use the Labels menu to assign it the
   "Releases" label. This is important because it's what the Bitcoin
   Core release manager will be looking for.

10. After each new Release Candidate (RC) is released, update the
    release notes you created in the `_releases` directory. (But don't
    worry about this too much; we can always upload updated release
    notes after the release.)

### Alerts

1. [Who to contact](#who-to-contact)
2. [Basic alert](#basic-alert) (emergency fast instructions)
3. [Detailed alert](#detailed-alert)
4. [Clearing an alert](#clearing-an-alert)

#### Who to Contact

The following people can publish alerts on Bitcoin.org.  Their email
addresses are on the linked GitHub profiles.

- Saïvann Carignan, [@saivann](https://github.com/saivann), saivann on Freenode
- Dave Harding, [@harding](https://github.com/harding), harding on Freenode
- Wladimir van der Laan, [@laanwj](https://github.com/laanwj), wumpus on Freenode
- Theymos, [@theymos](https://github.com/theymos), theymos on Freenode

Several of the above are only occasionally on Freenode.  Alert
coordination is usually conducted in #bitcoin-dev on Freenode.

#### Basic Alert

1. Open your editor on a file named `_alerts/YYYY-MM-DD-short-title.md`
   (the alert will appear as <https://bitcoin.org/en/alert/YYYY-MM-DD-short-title>).

2. Paste the following text into the top of the file:

    ```
    ---
    ## Title displayed on alert page
    title: "11/12 March 2013 Chain Fork"
    ## Short URL for use in P2P network alerts: https://bitcoin.org/<shorturl>
    shorturl: "chainfork"
    ## Active alerts will display the banner (below) on all bitcoin.org content pages
    active: true
    ## Banner displayed if 'active: true'.  Can use HTML formatting
    banner: "<b>Chain fork</b> - Please stop mining on bitcoin version 0.8.0. Click here for more information."
    ---

    {% comment %}
    First paragraph should indicate whose bitcoins are safe, to avoid
    starting a panic.
    {% comment %}

    Your bitcoins are safe if you received them in transactions
    confirmed before 2015-07-06 00:00 UTC.

    {% comment %}
    Second paragraph should summarize the problem, and subsequent
    text should indicate what people should do immediately.
    Consider: users (by wallet type), merchants, and miners.
    {% comment %}

    However, there has been a problem with a planned upgrade. For
    bitcoins received later than the time above, confirmation scores are
    significantly less reliable then they usually are for users of
    certain software:

    - Lightweight (SPV) wallet users should wait an additional 30
      confirmations more than you would normally wait. Electrum users,
      please see this note.
    ```

- Edit the file.  It is written in [Markdown format][].

- Commit it.

    - **Note:** the commit must be signed by one of the people in the
      [Who to Contact](#who-to-contact) section for site
      auto-building to work.

- Push the commit to the master branch. Rebuilding the site occurs
  automatically and takes 7 to 15 minutes.

    - **Note:** do not push additional commits until the alert is
      displayed on the live site.  The site build aborts and starts over
      when new commits are found.

- Give the `shorturl` URL (`bitcoin.org/<shorturl>`) to the P2P alert message
  key holders to use in any alert messages they send.

- Proceed to the next section to improve the alert.

#### Detailed Alert

In addition to providing more information about how users should respond
to the situation, you can enhance the alert in several ways described
below.

The following fields may be defined in the the alert YAML header:

```yaml
---
## (Required; HTML text) Title displayed on alert page
title: "11/12 March 2013 Chain Fork"
## (Optional; display ASCII only) Short URL for use in P2P network alerts: https://bitcoin.org/<shorturl>
shorturl: "chainfork"
## (Optional; default=false) Active alerts will display the banner (below) on all bitcoin.org content pages
active: true
## (Optional; HTML text) Banner displayed if 'active: true'.  Can use HTML formatting
banner: "<b>Chain fork</b> - Please stop mining on bitcoin version 0.8.0. Click here for more information."
## (Optional; default=alert) CSS class to set banner color
##   alert = red  |  warning = orange  |  success = green  | info = blue
bannerclass: alert
---
```

The time of the last update should be placed on the page somewhere. UTC
should be used for all dates, and RFC 2822 format ( date -uR ) is
recommended for long dates. For example, place the date in the footer of
the document:

```html
<div style="text-align:right">
  <i>This notice last updated: Thu, 16 May 2013 01:37:00 UTC</i>
</div>
```

You may also want to create a page on the Wiki to allow anyone to
provide additional information.  If you do so, link to it from the
alert.

#### Clearing An Alert

To stop advertising an alert on every Bitcoin.org page, change the YAML
header field `active` to *false*:

```yaml
## (Optional; default=false) Active alerts will display the banner (below) on all bitcoin.org content pages
active: false
```

Alternatively, for a few days you can change the message and set the
CSS `bannerclass` to *success* to indicate the problem has been resolved.

```yaml
## (Optional; HTML text) Banner displayed if 'active: true'.  Can use HTML formatting
banner: "<b>Chain fork</b> - situation resolved"
## (Optional; default=alert) CSS class to set banner color
##   alert = red  |  warning = orange  |  success = green  | info = blue
bannerclass: success
```

[markdown format]: https://help.github.com/articles/markdown-basics/

## Wallets

The wallet list is based on the personal evaluation of the maintainer(s) and regular contributors of this site, according to the criterias detailed below.

These requirements are meant to be updated and strengthened over time. Innovative wallets are exciting and encouraged, so if your wallet has a good reason for not following some of the rules below, please submit it anyway and we'll consider updating the rules.

Basic requirements:

- Sufficient users and/or developers feedback can be found without concerning issues, or independent security audit(s) is available
- No indication that users have been harmed considerably by any issue in relation to the wallet
- No indication that security issues have been concealed, ignored, or not addressed correctly in order to prevent new or similar issues from happening in the future
- No indication that the wallet uses unstable or unsecure libraries
- No indication that changes to the code are not properly tested
- Wallet was publicly announced and released since at least 3 months
- No concerning bug is found when testing the wallet
- Website supports HTTPS and 301 redirects HTTP requests
- SSL certificate passes [Qualys SSL Labs SSL test](https://www.ssllabs.com/ssltest/)
- Website serving executable code or requiring authentication uses HSTS with a max-age of at least 180 days
- The identity of CEOs and/or developers is public
- Avoid address reuse by using a new change address for each transaction
- If private keys or encryption keys are stored online:
  - Refuses weak passwords (short passwords and/or common passwords) used to secure access to any funds, or provides an aggressive account lock-out feature in response to failed login attempts along with a strict account recovery process.
- If user has no access over its private keys:
  - Provides 2FA authentication feature
  - Reminds the user to enable 2FA by email or in the main UI of the wallet
  - User session is not persistent, or requires authentication for spending
  - Provides account recovery feature
- If user has exclusive access over its private keys:
  - Allows backup of the wallet
  - Restoring wallet from backup is working
  - Source code is public and kept up to date under version control system
- If user has no access to some of the private keys in a multi-signature wallet:
  - Provides 2FA authentication feature
  - Reminds the user to enable 2FA by email or in the main UI of the wallet
  - User session is not persistent, or requires authentication for spending
  - Gives control to the user over moving their funds out of the multi-signature wallet
- For hardware wallets:
  - Uses the push model (computer malware cannot sign a transaction without user input)
  - Protects the seed against unsigned firmware upgrades
  - Supports importing custom seeds
  - Provides source code and/or detailed specification for blackbox testing if using a closed-source Secure Element

Optional criterias (some could become requirements):

- Received independent security audit(s)
- Avoid address reuse by displaying a new receiving address for each transaction in the wallet UI
- Does not show "received from" Bitcoin addresses in the UI
- Uses deterministic ECDSA nonces (RFC 6979)
- Provides a bug reporting policy on the website
- If user has no access over its private keys:
  - Full reserve audit(s)
  - Insurrance(s) against failures on their side
  - Reminds the user to enable 2FA in the main UI of the wallet
- If user has exclusive access over its private keys:
  - Supports HD wallets (BIP32)
  - Provides users with step to print or write their wallet seed on setup
  - Uses a strong KDF and key stretching for wallet storage and backups
  - On desktop platform:
    - Encrypt the wallet by default
- For hardware wallets:
  - Prevents downgrading the firmware

### Adding a wallet

*Before adding a wallet,* please make sure your wallet meets all of the
Basic Requirements listed above, or open a [new issue][] to request an
exemption or policy change. Feel free to email Dave Harding
<dave@dtrt.org> if you have any questions.

Wallets can be added in `_templates/choose-your-wallet.html`. Entries are ordered by levels and new wallets must be added after the last wallet on the same level.

* Level 1 - Full nodes
* Level 2 - SPV, Random servers
* Level 3 - Hybrid, Multisig wallets
* Level 4 - Web wallets

**Screenshot**: The png files must go in `/img/screenshots`, be 250 X 350 px and optimized with `optipng -o7 file.png`.

**Icon**: The png file must go in `/img/wallet`, be 144 X 144 px and optimized with `optipng -o7 file.png`. The icon must fit within 96 X 96 px inside the png, or 85 X 85 px for square icons.

**Description**: The text must go in `_translations/en.yml` alongside other wallets' descriptions.

### Score

Each wallet is assigned a score for five criterias. For each of them, the appropriate text in `_translations/en.yml` needs to be choosen.

**Control** - What control the user has over his bitcoins?

To get a good score, the wallet must provide the user with full exclusive control over their bitcoins.

To get a passing score, the wallet must provide the user with exclusive control over their bitcoins. Encrypted online backups are accepted so long as only the user can decrypt them. Multisig wallets are accepted so long as only the user can spend without the other party's permission.

**Validation** - How secure and « zero trust » is payment processing?

To get a good score, the wallet must be a full node and need no trust on other nodes.

To get a passing score, the wallet must rely on random nodes, either by using the SPV model or a pre-populated list or servers.

**Transparency** - How transparent and « zero trust » is the source code?

To get a good score, the wallet must deserve a passing score and be built deterministically.

To get a passing score, the wallet must be open-source, under version control and releases must be clearly identified (e.g. by tags or commits). The codebase and final releases must be public since at least 6 months and previous commits must remain unchanged.

**Environment** - How secure is the environment of the wallet?

To get a good score, the wallet must run from an environment where no apps can be installed.

To get a passing score, the wallet must run from an environment that provides app isolation (e.g. Android, iOS), or require two-factor authentication for spending.

**Privacy**: Does the wallet protect users' privacy?

To get a good score, the wallet must avoid address reuse by using a new change address for each transaction, avoid disclosing information to peers or central servers and be compatible with Tor.

To get a passing score, the wallet must avoid address reuse by using a new change address for each transaction.


## Advanced Usage

### Redirections

Redirections can be defined in ```_config.yml```.

```
  /news: /en/version-history
```

### Aliases For Contributors

Aliases for contributors are defined in ```_config.yml```.

```
aliases:
  s_nakamoto: Satoshi Nakamoto
  --author=Satoshi Nakamoto: Satoshi Nakamoto
  gavinandresen: Gavin Andresen
```

### Blog Posts

Posts for the [Bitcoin.org Site Blog][] should be added to the `_posts`
directory with the naming convention:
`YEAR-MONTH-DAY-ARBITRARY_FILE_NAME` (with year, month, and day as
two-digit numbers).  The YAML front matter should be similar to this:

    ---
    type: posts
    layout: post
    lang: en
    category: blog

    title: "Quarterly Report March 2015"
    permalink: /en/posts/quarterly-report-march-2015.html
    date: 2015-03-05
    author: >
      David A. Harding (<a href="mailto:dave@dtrt.org">email</a>, <a
      href="https://github.com/harding">GitHub</a>,
      <a href="http://www.reddit.com/user/harda/">Reddit</a>)
    ---

The type, layout, and category should always be as specified above. The
other parameters should be set to values specific to that post, but the
permalink must end in '.html'.

Below the YAML front matter, enter the content of the post in Markdown
format.  Images should be placed in `img/blog/free` if they are
MIT-licensed or `img/blog/nonfree` if they have a more restrictive
copyright license.

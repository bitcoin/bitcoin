Translations
============

The Dash Core GUI can be easily translated into other languages. Here's how we
handle those translations.

Files and Folders
-----------------

<<<<<<< HEAD
### dash-qt.pro
=======
### crowncoin-qt.pro
>>>>>>> origin/dirty-merge-dash-0.11.0

This file takes care of generating `.qm` files from `.ts` files. It is mostly
automated.

<<<<<<< HEAD
### src/qt/dash.qrc
=======
### src/qt/crowncoin.qrc
>>>>>>> origin/dirty-merge-dash-0.11.0

This file must be updated whenever a new translation is added. Please note that
files must end with `.qm`, not `.ts`.

<<<<<<< HEAD
```xml
<qresource prefix="/translations">
    <file alias="en">locale/dash_en.qm</file>
    ...
</qresource>
```
=======
    <qresource prefix="/translations">
        <file alias="en">locale/crowncoin_en.qm</file>
        ...
    </qresource>
>>>>>>> origin/dirty-merge-dash-0.11.0

### src/qt/locale/

This directory contains all translations. Filenames must adhere to this format:

<<<<<<< HEAD
    dash_xx_YY.ts or dash_xx.ts

#### dash_en.ts (Source file)

`src/qt/locale/dash_en.ts` is treated in a special way. It is used as the
=======
    crowncoin_xx_YY.ts or crowncoin_xx.ts

#### crowncoin_en.ts (Source file)

`src/qt/locale/crowncoin_en.ts` is treated in a special way. It is used as the
>>>>>>> origin/dirty-merge-dash-0.11.0
source for all other translations. Whenever a string in the code is changed
this file must be updated to reflect those changes. A custom script is used
to extract strings from the non-Qt parts. This script makes use of `gettext`,
so make sure that utility is installed (ie, `apt-get install gettext` on
Ubuntu/Debian). Once this has been updated, lupdate (included in the Qt SDK)
<<<<<<< HEAD
is used to update dash_en.ts. This process has been automated, from src/,
=======
is used to update crowncoin_en.ts. This process has been automated, from src/qt,
>>>>>>> origin/dirty-merge-dash-0.11.0
simply run:
    make translate

##### Handling of plurals in the source file

When new plurals are added to the source file, it's important to do the following steps:

<<<<<<< HEAD
1. Open dash_en.ts in Qt Linguist (also included in the Qt SDK)
=======
1. Open crowncoin_en.ts in Qt Linguist (also included in the Qt SDK)
>>>>>>> origin/dirty-merge-dash-0.11.0
2. Search for `%n`, which will take you to the parts in the translation that use plurals
3. Look for empty `English Translation (Singular)` and `English Translation (Plural)` fields
4. Add the appropriate strings for the singular and plural form of the base string
5. Mark the item as done (via the green arrow symbol in the toolbar)
6. Repeat from step 2. until all singular and plural forms are in the source file
7. Save the source file

##### Creating the pull-request

An updated source file should be merged to github and Transifex will pick it
up from there (can take some hours). Afterwards the new strings show up as "Remaining"
in Transifex and can be translated.

To create the pull-request you have to do:

<<<<<<< HEAD
    git add src/qt/dashstrings.cpp src/qt/locale/dash_en.ts
=======
    git add src/qt/crowncoinstrings.cpp src/qt/locale/crowncoin_en.ts
>>>>>>> origin/dirty-merge-dash-0.11.0
    git commit

Syncing with Transifex
----------------------

We are using https://transifex.com as a frontend for translating the client.

<<<<<<< HEAD
https://www.transifex.com/projects/p/dash/
=======
https://www.transifex.com/projects/p/crowncoin/resource/tx/
>>>>>>> origin/dirty-merge-dash-0.11.0

The "Transifex client" (see: http://support.transifex.com/customer/portal/topics/440187-transifex-client/articles)
is used to fetch new translations from Transifex. The configuration for this client (`.tx/config`)
is part of the repository.

Do not directly download translations one by one from the Transifex website, as we do a few
postprocessing steps before committing the translations.

### Fetching new translations

1. `python contrib/devtools/update-translations.py`
<<<<<<< HEAD
2. update `src/qt/dash.qrc` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(dash_\(.*\)\).ts/        <file alias="\2">locale\/\1.qm<\/file>/'`
3. update `src/Makefile.qt.include` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(dash_\(.*\)\).ts/  qt\/locale\/\1.ts \\/'`
=======
2. update `src/qt/crowncoin.qrc` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(crowncoin_\(.*\)\).ts/<file alias="\2">locale\/\1.qm<\/file>/'`
3. update `src/qt/Makefile.am` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(crowncoin_\(.*\)\).ts/  locale\/\1.ts \\/'`
>>>>>>> origin/dirty-merge-dash-0.11.0
4. `git add` new translations from `src/qt/locale/`

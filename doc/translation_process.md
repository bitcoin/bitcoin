Translations
============

The Bitcoin Core GUI can be easily translated into other languages. Here's how we
handle those translations.

Files and Folders
-----------------

### bitcoin-qt.pro

This file takes care of generating `.qm` files from `.ts` files. It is mostly
automated.

### src/qt/bitcoin.qrc

This file must be updated whenever a new translation is added. Please note that
files must end with `.qm`, not `.ts`.

```xml
<qresource prefix="/translations">
    <file alias="en">locale/bitcoin_en.qm</file>
    ...
</qresource>
```

### src/qt/locale/

This directory contains all translations. Filenames must adhere to this format:

    bitcoin_xx_YY.ts or bitcoin_xx.ts

#### bitcoin_en.ts (Source file)

`src/qt/locale/bitcoin_en.ts` is treated in a special way. It is used as the
source for all other translations. Whenever a string in the code is changed
this file must be updated to reflect those changes. A  custom script is used
to extract strings from the non-Qt parts. This script makes use of `gettext`,
so make sure that utility is installed (ie, `apt-get install gettext` on 
Ubuntu/Debian). Once this has been updated, lupdate (included in the Qt SDK)
is used to update bitcoin_en.ts. This process has been automated, from src/qt,
simply run:
    make translate
    
##### Handling of plurals in the source file

When new plurals are added to the source file, it's important to do the following steps:

1. Open bitcoin_en.ts in Qt Linguist (also included in the Qt SDK)
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

    git add src/qt/bitcoinstrings.cpp src/qt/locale/bitcoin_en.ts
    git commit

Syncing with Transifex
----------------------

We are using https://transifex.com as a frontend for translating the client.

https://www.transifex.com/projects/p/bitcoin/resource/tx/

The "Transifex client" (see: http://support.transifex.com/customer/portal/topics/440187-transifex-client/articles)
is used to fetch new translations from Transifex. The configuration for this client (`.tx/config`)
is part of the repository.

Do not directly download translations one by one from the Transifex website, as we do a few
postprocessing steps before committing the translations.

### Fetching new translations

1. `python contrib/devtools/update-translations.py`
2. update `src/qt/bitcoin.qrc` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(bitcoin_\(.*\)\).ts/<file alias="\2">locale\/\1.qm<\/file>/'`
3. update `src/qt/Makefile.am` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(bitcoin_\(.*\)\).ts/  locale\/\1.ts \\/'`
4. `git add` new translations from `src/qt/locale/`

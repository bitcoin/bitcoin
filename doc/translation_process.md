Translations
============

The Qt GUI can be easily translated into other languages. Here's how we
handle those translations.

Files and Folders
-----------------

### bitcoin-qt.pro

This file takes care of generating `.qm` files from `.ts` files. It is mostly
automated.

### src/qt/bitcoin.qrc

This file must be updated whenever a new translation is added. Please note that
files must end with `.qm`, not `.ts`.

    <qresource prefix="/translations">
        <file alias="en">locale/bitcoin_en.qm</file>
        ...
    </qresource>

### src/qt/locale/

This directory contains all translations. Filenames must adhere to this format:

    bitcoin_xx_YY.ts or bitcoin_xx.ts

#### bitcoin_en.ts (Source file)

`src/qt/locale/bitcoin_en.ts` is treated in a special way. It is used as the
source for all other translations. Whenever a string in the code is changed
this file must be updated to reflect those changes. This can be accomplished
by running `lupdate` (included in the Qt SDK). Also, a custom script is used
to extract strings from the non-Qt parts. This script makes use of `gettext`,
so make sure that utility is installed (ie, `apt-get install gettext` on 
Ubuntu/Debian):

    python share/qt/extract_strings_qt.py
    lupdate bitcoin-qt.pro -no-obsolete -locations relative -ts src/qt/locale/bitcoin_en.ts
    
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

The "Transifex client" (see: http://help.transifex.com/features/client/)
will help with fetching new translations from Transifex. Use the following
config to be able to connect with the client:

### .tx/config

    [main]
    host = https://www.transifex.com

    [bitcoin.tx]
    file_filter = src/qt/locale/bitcoin_<lang>.ts
    source_file = src/qt/locale/bitcoin_en.ts
    source_lang = en
    
### .tx/config (for Windows)

    [main]
    host = https://www.transifex.com

    [bitcoin.tx]
    file_filter = src\qt\locale\bitcoin_<lang>.ts
    source_file = src\qt\locale\bitcoin_en.ts
    source_lang = en

It is also possible to directly download new translations one by one from the Transifex website.

### Fetching new translations

1. `tx pull -a`
2. update `src/qt/bitcoin.qrc` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(bitcoin_\(.*\)\).ts/<file alias="\2">locale/\1.qm<\/file>/'`
3. `git add` new translations from `src/qt/locale/`

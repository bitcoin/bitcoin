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

#### Source file

`src/qt/locale/bitcoin_en.ts` is treated in a special way. It is used as the
source for all other translations. Whenever a string in the code is changed
this file must be updated to reflect those changes. Usually, this can be
accomplished by running `lupdate` (included in the Qt SDK).

An updated source file should be merged to github and transifex will pick it
up from there. Afterwards the new strings show up as "Remaining" in transifex
and can be translated.

Syncing with transifex
----------------------

We are using http://transifex.net as a frontend for translating the client.

https://www.transifex.net/projects/p/bitcoin/resource/tx/

The "transifex client" (see: http://help.transifex.net/features/client/)
will help with fetching new translations from transifex. Use the following
config to be able to connect with the client.

### .tx/config

    [main]
    host = https://www.transifex.net

    [bitcoin.tx]
    file_filter = src/qt/locale/bitcoin_<lang>.ts
    source_file = src/qt/locale/bitcoin_en.ts
    source_lang = en
    
### .tx/config (for Windows)

    [main]
    host = https://www.transifex.net

    [bitcoin.tx]
    file_filter = src\qt\locale\bitcoin_<lang>.ts
    source_file = src\qt\locale\bitcoin_en.ts
    source_lang = en

It is also possible to directly download new translations one by one from transifex.

### Fetching new translations

1. `tx pull -a`
2. update `src/qt/bitcoin.qrc` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(bitcoin_\(.*\)\).ts/<file alias="\2">locale/\1.qm<\/file>/'`
3. `git add` new translations from `src/qt/locale/`

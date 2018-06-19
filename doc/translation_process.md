Translations
============

The Crown Qt can be easily translated into other languages. Here's how we
handle those translations.

Files and Folders
-----------------

### crown-qt.pro

This file takes care of generating `.qm` files from `.ts` files. It is mostly
automated.

### src/qt/crown.qrc

This file must be updated using the following format whenever a new translation is added. Note: these
files must end with `.qm`)

```xml
<qresource prefix="/translations">
    <file alias="en">locale/crown_en.qm</file>
    ...
</qresource>
```

### src/qt/locale/

This directory contains all translations. Filenames must adhere to this format:

    crown_xx_YY.ts or crown_xx.ts

where xx is the primary language code and YY is the sub-language code if needed, as in:

    crown_zh_CN.ts vs crown_zh_TW.ts
    
#### crown_en.ts (translation base file)

`src/qt/locale/crown_en.ts` is used as the source for all translations. Whenever a string in the code is changed
this file is updated by a script to reflect those changes. This script makes use of `gettext`,
so make sure that utility is installed (ie, `apt-get install gettext` on
Ubuntu/Debian). Once this has been updated, from src/ simply run:
    make translate
(this uses the 'lupdate' function in the Qt SDK to update crown_en.ts)

##### Handling of plurals in the source file

When new plurals are added to the source file, it's important to do the following steps:

1. Open crown_en.ts in Qt Linguist (in the Qt SDK).
2. Search for `%n` which will take you to the parts in the translation that use plurals.
3. Look for empty `English Translation (Singular)` and `English Translation (Plural)` fields.
4. Add the appropriate strings for the singular and plural form of the base string.
5. Mark the item as done (green arrow button in the toolbar).
6. Repeat Step 2 until all empty translation strings have been added to the source file.
7. Save the source file.

##### Creating the pull-request

The updated source file should be merged to github.

Commands to create the pull-request:

    git add src/qt/crownstrings.cpp src/qt/locale/crown_en.ts
    git commit

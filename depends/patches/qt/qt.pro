# Create the super cache so modules will add themselves to it.
cache(, super)

TEMPLATE = subdirs
SUBDIRS = qtbase qttools qttranslations

qtbase.target = module-qtbase

qttools.target = module-qttools
qttools.depends = qtbase

qttranslations.target = module-qttranslations
qttranslations.depends = qttools

QT_SKIP_MODULES =

load(qt_configure)

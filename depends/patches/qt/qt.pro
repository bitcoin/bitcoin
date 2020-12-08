# Create the super cache so modules will add themselves to it.
cache(, super)

# Suppress the license check on subsequent "visits". The first
# visit will skip it anyway due to not having a compiler set up
# yet. This cannot be added to the super cache, because that is
# read before spec_pre.prf, which flushes CONFIG. This does not
# affect submodules, as they come with a .qmake.conf. But that
# one sets the flag via qt_build_config.prf anyway.
!QTDIR_build: cache(CONFIG, add, $$list(QTDIR_build))

TEMPLATE      = subdirs

CONFIG += prepare_docs qt_docs_targets

# Extract submodules from .gitmodules.
lines = $$cat(.gitmodules, lines)
for (line, lines) {
    mod = $$replace(line, "^\\[submodule \"([^\"]+)\"\\]$", \\1)
    !equals(mod, $$line) {
        module = $$mod
        modules += $$mod
    } else {
        prop = $$replace(line, "^$$escape_expand(\\t)([^ =]+) *=.*$", \\1)
        !equals(prop, $$line) {
            val = $$replace(line, "^[^=]+= *", )
            module.$${module}.$$prop = $$split(val)
        } else {
            error("Malformed line in .gitmodules: $$line")
        }
    }
}
QMAKE_INTERNAL_INCLUDED_FILES += $$PWD/.gitmodules

QT_SKIP_MODULES =

# This is a bit hacky, but a proper implementation is not worth it.
args = $$QMAKE_EXTRA_ARGS
contains(args, -redo): \
    args += $$cat($$OUT_PWD/config.opt, lines)
for (ever) {
    isEmpty(args): break()
    a = $$take_first(args)

    equals(a, -skip) {
        isEmpty(args): break()
        m = $$take_first(args)
        contains(m, -.*): next()
        m ~= s/^(qt)?/qt/
        !contains(modules, $$m): \
            error("-skip command line argument used with non-existent module '$$m'.")
        QT_SKIP_MODULES += $$m
    }
}

modules = $$sort_depends(modules, module., .depends .recommends .serialize)
modules = $$reverse(modules)
for (mod, modules) {
    project = $$eval(module.$${mod}.project)
    equals(project, -): \
        next()

    deps = $$eval(module.$${mod}.depends)
    recs = $$eval(module.$${mod}.recommends) $$eval(module.$${mod}.serialize)
    for (d, $$list($$deps $$recs)): \
        !contains(modules, $$d): \
            error("'$$mod' depends on undeclared '$$d'.")

    contains(QT_SKIP_MODULES, $$mod): \
        next()
    !isEmpty(QT_BUILD_MODULES):!contains(QT_BUILD_MODULES, $$mod): \
        next()

    isEmpty(project) {
        !exists($$mod/$${mod}.pro): \
            next()
        $${mod}.subdir = $$mod
    } else {
        !exists($$mod/$$project): \
            next()
        $${mod}.file = $$mod/$$project
        $${mod}.makefile = Makefile
    }
    $${mod}.target = module-$$mod

    for (d, deps) {
        !contains(SUBDIRS, $$d) {
            $${mod}.target =
            break()
        }
        $${mod}.depends += $$d
    }
    isEmpty($${mod}.target): \
        next()
    for (d, recs) {
        contains(SUBDIRS, $$d): \
            $${mod}.depends += $$d
    }

    SUBDIRS += $$mod
}

load(qt_configure)

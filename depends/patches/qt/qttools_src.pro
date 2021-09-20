TEMPLATE = subdirs
SUBDIRS = linguist

fb = force_bootstrap
CONFIG += $$fb
cache(CONFIG, add, fb)

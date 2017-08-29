CLEANFILES += $(wildcard doc/man/*.3)

.PHONY: doc/man

dist_man_MANS = $(wildcard doc/man/*.3)

man3_MANS = $(wildcard doc/man/*.3)


# sphinx-build "-E" is "ignore cache", "b" is build type.
doc/man:
	-mkdir -p doc/man
	$(SPHINX_BUILD) -qEW -b man $(top_srcdir)/doc doc/man
	rm -rf doc/man/.doctrees doc/man/.buildinfo

EXTRA_DIST += doc/man

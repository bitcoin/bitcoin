SPHINX_HTML_FILES = \
   $(wildcard doc/html/*.html) \
   $(wildcard doc/html/*.css) \
   $(wildcard doc/html/*.js) \
   $(wildcard doc/html/*.png) \
   $(wildcard doc/html/_static/*.css) \
   $(wildcard doc/html/_static/*.js) \
   $(wildcard doc/html/_static/*.png) \
   doc/html/.nojekyll \
   doc/html/objects.inv

CLEANFILES += $(SPHINX_HTML_FILES)

.PHONY: doc/html

doc/html: $(SPHINX_HTML_FILES)

# sphinx-build "-E" is "ignore cache".
$(SPHINX_HTML_FILES):
	-mkdir -p doc/html
	$(SPHINX_BUILD) -qEW $(top_srcdir)/doc doc/html
	rm -rf doc/html/.doctrees doc/html/.buildinfo

EXTRA_DIST += doc/html

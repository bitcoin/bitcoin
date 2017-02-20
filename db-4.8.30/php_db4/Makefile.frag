
PHP_DB4_HEADER_FILES = php_db4.h

install-db4-headers:
	@echo "Installing php_db4 headers: $(INSTALL_ROOT)$(phpincludedir)/ext/db4/"
	@$(mkinstalldirs) $(INSTALL_ROOT)$(phpincludedir)/ext/db4
	@for f in $(PHP_DB4_HEADER_FILES); do \
		if test -f "$(top_srcdir)/$$f"; then \
			$(INSTALL_DATA) $(top_srcdir)/$$f $(INSTALL_ROOT)$(phpincludedir)/ext/db4; \
		elif test -f "$(top_builddir)/$$f"; then \
			$(INSTALL_DATA) $(top_builddir)/$$f $(INSTALL_ROOT)$(phpincludedir)/ext/db4; \
		elif test -f "$(top_srcdir)/ext/db4/$$f"; then \
			$(INSTALL_DATA) $(top_srcdir)/ext/db4/$$f $(INSTALL_ROOT)$(phpincludedir)/ext/db4; \
		elif test -f "$(top_builddir)/ext/db4/$$f"; then \
			$(INSTALL_DATA) $(top_builddir)/ext/db4/$$f $(INSTALL_ROOT)$(phpincludedir)/ext/db4; \
		else \
			echo "hmmm"; \
		fi \
	done;

install: $(all_targets) $(install_targets) install-db4-headers

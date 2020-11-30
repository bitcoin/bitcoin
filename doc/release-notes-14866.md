Updated settings
----------------

- Included configurations from includeconf lines in [main] [test] and [regtest]
  sections of the config file are now evaluated in context of that section and
  not treated like top-level includes. Config files included within a section
  are also now disallowed from changing settings in other sections. (#14866)

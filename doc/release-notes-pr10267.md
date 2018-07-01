Changed command-line options
----------------------------

- `-includeconf=<file>` can be used to include additional configuration files.
  Only works inside the `bitcoin.conf` file, not inside included files or from
  command-line. Multiple files may be included. Can be disabled from command-
  line via `-noincludeconf`. Note that multi-argument commands like
  `-includeconf` will override preceding `-noincludeconf`, i.e.

    noincludeconf=1
    includeconf=relative.conf

  as bitcoin.conf will still include `relative.conf`.

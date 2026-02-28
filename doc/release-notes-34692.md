Updated settings
----------------

- The default `-dbcache` value has been increased to `1024` MiB from `450` MiB.
  This improves performance, but will use more memory.
  Systems with less than 4 GiB of RAM may run out of memory.
  To maintain the previous behavior, set `-dbcache=450`.
  See [reduce-memory.md](reduce-memory.md) for further guidance on low-memory systems. (#34692)

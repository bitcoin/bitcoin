## Updated settings

- The default `-dbcache` value has been increased to `1024` MiB from `450` MiB
  on systems where at least `4096` MiB of RAM is detected.
  This is a performance increase, but will use more memory.
  To maintain the previous behaviour, set `-dbcache=450`.

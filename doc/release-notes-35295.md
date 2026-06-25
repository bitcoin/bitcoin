Performance Improvements
------------------------

- Block validation can now prefetch input prevouts from the chainstate database
  in parallel while connecting blocks, speeding up validation when prevouts need
  to be read from disk. A new `-prevoutfetchthreads=<n>` option controls the
  number of prefetch worker threads. The default is 8 threads, up to a maximum
  of 16; set it to 0 to disable parallel prefetching. (#35295)

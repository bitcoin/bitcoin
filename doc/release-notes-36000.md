Performance Improvements
------------------------

- Background threads can now prefetch later blocks from disk while another
  block is being connected, speeding up reindexing and initial block download.
  Use `-blockreadahead=<n>` to choose the number of reader threads, or set it
  to 0 to disable read-ahead. (#36000)

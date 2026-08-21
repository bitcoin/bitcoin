Performance Improvements
------------------------

- A background thread can now prefetch later blocks from disk while another
  block is being connected, speeding up reindexing and initial block download.
  (#36000)

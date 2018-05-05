Thread names in logs
--------------------

On platforms supporting `thread_local`, log lines are now prefixed by default
with the name of the thread that caused the log. To disable this behavior,
specify `-logthreadnames=0`.

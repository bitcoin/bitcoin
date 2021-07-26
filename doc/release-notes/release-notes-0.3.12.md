Version 0.3.12 is now available.

Features:
* json-rpc errors return a more standard error object. (thanks to Gavin Andresen)
* json-rpc command line returns exit codes.
* json-rpc "backupwallet" command.
* Recovers and continues if an exception is caused by a message you received.  Other nodes shouldn't be able to cause an exception, and it hasn't happened before, but if a way is found to cause an exception, this would keep it from being used to stop network nodes.

If you have json-rpc code that checks the contents of the error string, you need to change it to expect error objects of the form {"code":<number>,"message":<string>}, which is the standard.  See this thread:
http://www.bitcoinrupee.org/smf/index.php?topic=969.0

Download:
http://sourceforge.net/projects/bitcoinrupee/files/Bitcoin/bitcoinrupee-0.3.12/

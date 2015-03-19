###What is it about###

More info regarding KeePass: http://keepass.info/

KeePass integration use KeePassHttp (https://github.com/pfn/keepasshttp/) to facilitate communications between the client and KeePass. KeePassHttp is a plugin for KeePass 2.x and provides a secure means of exposing KeePass entries via HTTP for clients to consume.

The implementation is dependent on the following:
 - crypter.h for AES encryption helper functions.
 - rpcprotocol.h for handling RPC communications. Could only be used partially however due some static values in the code.
 - OpenSSL for base64 encoding. regular util.h libraries were not used for base64 encoding/decoding since they do not use secure allocation.
 - JSON Spirit for reading / writing RPC communications

###What's new###

The following new options are available for dashd and dash-qt:
 - _-keepass_ Use KeePass 2 integration using KeePassHttp plugin (default: 0)
 - _-keepassport=_ Connect to KeePassHttp on port (default: 19455)
 - _-keepasskey=_ KeePassHttp key for AES encrypted communication with KeePass
 - _-keepassid=_ KeePassHttp id for the established association
 - _-keepassname=_ Name to construct url for KeePass entry that stores the wallet passphrase

The following rpc commands are available:

 - _keepass genkey_: generates a base64 encoded 256 bit AES key that can be used for the communication with KeePassHttp. Only necessary for manual configuration. Use init for automatic configuration.
 - _keepass init_: sets up the association between dashd and keepass by generating an AES key and sending an association message to KeePassHttp. This will trigger KeePass to ask for an Id for the association. Returns the association and the base64 encoded string for the AES key.
 - _keepass setpassphrase_: updates the passphrase in KeePassHttp to a new value. This should match the passphrase you intend to use for the wallet. Please note that the standard RPC commands _walletpassphrasechange_ and the wallet encrption from the QT GUI already send the updates to KeePassHttp, so this is only necessary for manual manipulation of the password.

###How to setup###

Sample initialization flow from _dash-qt_ console (this needs to be done only once to set up the association):

 - Have KeePass running with an open database
 - Start _dash-qt_
 - Open console
 - Type "_keepass init_" in dash-qt console
 - Keepass pops up and asks for an association id, fill that in, for example, "_mydrkwallet_"
 - You should get a response like this "_Association successful. Id: mydrkwalletdash - Key: AgQkcs6cI7v9tlSYKjG/+s8wJrGALHl3jLosJpPLzUE=_"
 - Edit _dash.conf_ and fill in these values
```
keepass=1
keepasskey=AgQkcs6cI7v9tlSYKjG/+s8wJrGALHl3jLosJpPLzUE=
keepassid=mydrkwallet
keepassname=testwallet
```
 - Restart _dash-qt_

At this point, the association is made. The next action depends on your particular situation:

 - current wallet is not yet encrypted. Encrypting the wallet will trigger the integration and stores the password in KeePass (Under the '_KeePassHttp Passwords_' group, named after _keepassname_.
 - current wallet is already encrypted: use "_keepass setpassphrase_" to store the passphrase in KeePass.

At this point, the passphrase is stored in KeePassHttp. When Unlocking the wallet, one can use _keepass_ as the passphrase to trigger retrieval of the password. This works from the RPC commands as well as the GUI.

Extended guide with screenshots is also available: https://dashtalk.org/threads/keepass-integration.3620/

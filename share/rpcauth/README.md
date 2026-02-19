RPC Tools
---------------------

### [RPCAuth](/share/rpcauth) ###

```
usage: rpcauth.py [-h] username [password]

Create login credentials for a JSON-RPC user

positional arguments:
  username    the username for authentication
  password    leave empty to generate a random password or specify "-" to
              prompt for password

optional arguments:
  -h, --help  show this help message and exit
  -j, --json   output data in json format
  ```

### [RPCToken](/share/rpcauth) ###

```
usage: rpctoken.py [-h] username [token]

Create API token credentials for a JSON-RPC user (Bearer authentication)

positional arguments:
  username    the username for authentication
  token       leave empty to generate a random token or provide an existing token

optional arguments:
  -h, --help  show this help message and exit
  -j, --json   output data in json format
  ```

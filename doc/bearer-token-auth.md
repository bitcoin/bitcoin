# Bearer Token Authentication for Bitcoin Core RPC

## Overview

Bitcoin Core now supports Bearer token authentication for JSON-RPC connections as a modern alternative to Basic authentication. This feature provides a secure, token-based authentication method suitable for API integrations, mobile applications, and web services.

## Features

- **HMAC-SHA256 Hashing**: Tokens are hashed using HMAC-SHA256 before storage, similar to the `rpcauth` mechanism
- **Timing-Resistant Comparison**: Token validation uses timing-resistant comparison to prevent timing attacks
- **Backward Compatible**: Existing Basic authentication methods continue to work
- **Multiple Tokens**: Support for multiple tokens with different users, each with optional method whitelists

## Generating Tokens

Use the `rpctoken.py` script in `share/rpcauth/` to generate tokens:

### Generate a Random Token

```bash
python3 share/rpcauth/rpctoken.py myuser
```

Output:
```
String to be appended to bitcoin.conf:
rpctoken=myuser:14371a7b6bd55af41895ed5fedb7f745$c77cc7338a29feed078d18348f7f78b859a2241e4d19b673ecf2e57b8c7aae54
Your token:
nB06tMrfJzrc3F7nLmNROGnhpADD4C83h2sTaSNi1lw
```

### Use a Specific Token

```bash
python3 share/rpcauth/rpctoken.py myuser mytoken123
```

### JSON Output

```bash
python3 share/rpcauth/rpctoken.py myuser -j
```

Output:
```json
{"username": "myuser", "token": "...", "rpctoken": "myuser:salt$hash"}
```

## Configuration

Add the generated `rpctoken` line to your `bitcoin.conf`:

```
rpctoken=myuser:14371a7b6bd55af41895ed5fedb7f745$c77cc7338a29feed078d18348f7f78b859a2241e4d19b673ecf2e57b8c7aae54
```

You can specify multiple tokens:

```
rpctoken=user1:salt1$hash1
rpctoken=user2:salt2$hash2
```

### With Method Whitelist

Restrict which RPC methods a user can call:

```
rpctoken=limiteduser:salt$hash
rpcwhitelist=limiteduser:getbestblockhash,getblockcount
```

## Usage

### Using curl

```bash
curl -H "Authorization: Bearer YOUR_TOKEN_HERE" \
     -d '{"method":"getbestblockhash"}' \
     http://localhost:8332/
```

### Using Python

```python
import requests

headers = {
    "Authorization": "Bearer YOUR_TOKEN_HERE"
}
data = {"method": "getbestblockhash"}

response = requests.post("http://localhost:8332/", headers=headers, json=data)
print(response.json())
```

### Using JavaScript/Node.js

```javascript
const fetch = require('node-fetch');

const headers = {
    'Authorization': 'Bearer YOUR_TOKEN_HERE'
};
const data = {method: 'getbestblockhash'};

fetch('http://localhost:8332/', {
    method: 'POST',
    headers: headers,
    body: JSON.stringify(data)
})
.then(response => response.json())
.then(data => console.log(data));
```

## Security Considerations

1. **Token Storage**: Store tokens securely, never commit them to version control
2. **HTTPS**: Always use HTTPS in production to prevent token interception
3. **Token Rotation**: Regularly rotate tokens for improved security
4. **Least Privilege**: Use RPC method whitelists to restrict user permissions
5. **Network Access**: Use `-rpcallowip` to restrict which IPs can connect

## Comparison with Basic Authentication

| Feature | Bearer Token | Basic Auth (rpcauth) |
|---------|-------------|---------------------|
| Security | HMAC-SHA256 hashed | HMAC-SHA256 hashed |
| Format | Single token | Username + password |
| Use Case | APIs, mobile apps | Traditional clients |
| Header | `Authorization: Bearer <token>` | `Authorization: Basic <base64>` |

## Troubleshooting

### 401 Unauthorized

- Verify the token is correct
- Check that `rpctoken` is properly configured in `bitcoin.conf`
- Ensure the Bitcoin Core daemon has been restarted after config changes

### Connection Refused

- Verify Bitcoin Core is running
- Check `-rpcallowip` configuration
- Ensure the correct RPC port is being used

## Example Configuration

Complete `bitcoin.conf` example with token authentication:

```
# RPC server configuration
rpcport=8332
rpcallowip=192.168.1.0/24
rpcbind=0.0.0.0

# Token authentication
rpctoken=apiuser:f3d8e4c1a9b7e2d5f4a8c3b2e1d9f7a4$a7f3e8d2c4b9e1f5a3d7c2e8b4f1a9d6e3c7f2b8a5e1d4c9f7b3a6e2d8c5f1a4
rpctoken=webapp:c2b1e4f7a3d8e5c9f2b4a7d1e8c3f6b9$d8f3a2e5c7b1f4a9e2d6c8f3b5a1e7d4c9f2b6a3e8d1c5f7b4a2e9d3c6f8a1

# Method whitelists
rpcwhitelist=webapp:getbalance,sendtoaddress,listtransactions
```

## Testing

Run the functional test to verify token authentication:

```bash
test/functional/rpc_token_auth.py
```

## See Also

- [RPC Authentication Documentation](../doc/JSON-RPC-interface.md)
- [Security Best Practices](../SECURITY.md)
- `share/rpcauth/rpcauth.py` - Basic authentication credential generation

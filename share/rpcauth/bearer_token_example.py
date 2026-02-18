#!/usr/bin/env python3
"""
Example script demonstrating Bearer token authentication with Bitcoin Core RPC.

This script shows how to make RPC calls using Bearer token authentication.
"""

import http.client
import json
import sys


def make_rpc_call(host, port, token, method, params=None):
    """
    Make an RPC call to Bitcoin Core using Bearer token authentication.
    
    Args:
        host: RPC server hostname (e.g., 'localhost')
        port: RPC server port (e.g., 8332)
        token: Bearer token for authentication
        method: RPC method to call (e.g., 'getblockcount')
        params: Optional list of parameters for the method
    
    Returns:
        The JSON-RPC response
    """
    if params is None:
        params = []
    
    # Prepare the JSON-RPC request
    rpc_request = {
        "method": method,
        "params": params,
        "id": 1
    }
    
    # Set up headers with Bearer token
    headers = {
        "Authorization": f"Bearer {token}",
        "Content-Type": "application/json"
    }
    
    # Make the request
    conn = http.client.HTTPConnection(host, port)
    try:
        conn.request("POST", "/", json.dumps(rpc_request), headers)
        response = conn.getresponse()
        response_data = response.read().decode('utf-8')
        
        if response.status != 200:
            print(f"Error: HTTP {response.status} - {response.reason}")
            print(response_data)
            return None
        
        return json.loads(response_data)
    finally:
        conn.close()


def main():
    """
    Example usage of Bearer token authentication.
    """
    # Configuration
    RPC_HOST = "localhost"
    RPC_PORT = 8332
    
    # Your Bearer token (generate using share/rpcauth/rpctoken.py)
    # NEVER commit real tokens to version control!
    BEARER_TOKEN = "your_token_here"
    
    if BEARER_TOKEN == "your_token_here":
        print("Error: Please set your Bearer token in the script")
        print("Generate a token using: python3 share/rpcauth/rpctoken.py username")
        sys.exit(1)
    
    print("Bitcoin Core RPC Bearer Token Authentication Example")
    print("=" * 60)
    
    # Example 1: Get blockchain info
    print("\n1. Getting blockchain information...")
    result = make_rpc_call(RPC_HOST, RPC_PORT, BEARER_TOKEN, "getblockchaininfo")
    if result:
        print(f"Chain: {result.get('result', {}).get('chain', 'N/A')}")
        print(f"Blocks: {result.get('result', {}).get('blocks', 'N/A')}")
    
    # Example 2: Get block count
    print("\n2. Getting block count...")
    result = make_rpc_call(RPC_HOST, RPC_PORT, BEARER_TOKEN, "getblockcount")
    if result:
        print(f"Block count: {result.get('result', 'N/A')}")
    
    # Example 3: Get best block hash
    print("\n3. Getting best block hash...")
    result = make_rpc_call(RPC_HOST, RPC_PORT, BEARER_TOKEN, "getbestblockhash")
    if result:
        print(f"Best block hash: {result.get('result', 'N/A')}")
    
    # Example 4: Get network info
    print("\n4. Getting network information...")
    result = make_rpc_call(RPC_HOST, RPC_PORT, BEARER_TOKEN, "getnetworkinfo")
    if result:
        net_info = result.get('result', {})
        print(f"Version: {net_info.get('version', 'N/A')}")
        print(f"Connections: {net_info.get('connections', 'N/A')}")
    
    print("\n" + "=" * 60)
    print("Examples completed successfully!")


if __name__ == "__main__":
    main()

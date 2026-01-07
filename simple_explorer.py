import requests
import json
from flask import Flask, render_template_string, request

app = Flask(__name__)

# Configuration (Match your bitcoin.conf)
RPC_USER = 'rpcuser'
RPC_PASSWORD = 'rpcpassword'
RPC_HOST = '127.0.0.1'
RPC_PORT = 9332  # Clitboin Mainnet Default

RPC_URL = f"http://{RPC_USER}:{RPC_PASSWORD}@{RPC_HOST}:{RPC_PORT}"

def rpc_call(method, params=[]):
    payload = {
        "method": method,
        "params": params,
        "jsonrpc": "2.0",
        "id": 0,
    }
    try:
        response = requests.post(RPC_URL, json=payload).json()
        if 'error' in response and response['error']:
            return {"error": response['error']}
        return response['result']
    except Exception as e:
        return {"error": str(e)}

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Clitboin Explorer</title>
    <style>
        body { font-family: monospace; padding: 20px; background: #f0f0f0; }
        .card { background: white; padding: 20px; margin-bottom: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        pre { background: #eee; padding: 10px; overflow-x: auto; }
        .error { color: red; }
    </style>
</head>
<body>
    <h1>Clitboin Explorer (Regtest)</h1>
    
    <div class="card">
        <h2>Chain Info</h2>
        {% if info.error %}
            <p class="error">Error connecting to node: {{ info.error }}</p>
            <p>Make sure bitcoind is running with -regtest and rpcuser/password match this script.</p>
        {% else %}
            <p><strong>Chain:</strong> {{ info.chain }}</p>
            <p><strong>Blocks:</strong> {{ info.blocks }}</p>
            <p><strong>Difficulty:</strong> {{ info.difficulty }}</p>
            <p><strong>Best Block Hash:</strong> {{ info.bestblockhash }}</p>
        {% endif %}
    </div>

    <div class="card">
        <h2>Latest Blocks</h2>
        <ul>
        {% for block in blocks %}
            <li>
                <a href="/block/{{ block.hash }}">Height {{ block.height }}</a> - {{ block.hash }}
            </li>
        {% endfor %}
        </ul>
    </div>
    
    {% if block_details %}
    <div class="card">
        <h2>Block Details: {{ block_details.height }}</h2>
        <pre>{{ block_details | tojson(indent=2) }}</pre>
    </div>
    {% endif %}
</body>
</html>
"""

@app.route('/')
def index():
    info = rpc_call('getblockchaininfo')
    blocks = []
    
    if not isinstance(info, dict) or 'error' in info:
        pass # Handle error in template
    else:
        # Get last 10 blocks
        current_height = info.get('blocks', 0)
        for h in range(current_height, max(-1, current_height - 10), -1):
            block_hash = rpc_call('getblockhash', [h])
            blocks.append({'height': h, 'hash': block_hash})

    return render_template_string(HTML_TEMPLATE, info=info, blocks=blocks, block_details=None)

@app.route('/block/<blockhash>')
def block(blockhash):
    info = rpc_call('getblockchaininfo')
    block_details = rpc_call('getblock', [blockhash, 2]) # Verbosity 2 for full tx info
    return render_template_string(HTML_TEMPLATE, info=info, blocks=[], block_details=block_details)

if __name__ == '__main__':
    print(f"Starting Explorer on port 5000...")
    print(f"Connecting to Node at {RPC_URL}")
    app.run(debug=True)

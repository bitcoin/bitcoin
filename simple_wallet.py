import requests
import json
from flask import Flask, render_template_string, request, redirect, url_for

app = Flask(__name__)

# Configuration (Match your clitboin.conf)
RPC_USER = 'rpcuser'
RPC_PASSWORD = 'rpcpassword'
RPC_HOST = '127.0.0.1'
RPC_PORT = 9332  # Clitboin Mainnet

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
    <title>Clitboin Web Wallet</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; padding: 20px; background: #e9ecef; color: #333; }
        .container { max-width: 800px; margin: 0 auto; }
        .card { background: white; padding: 25px; margin-bottom: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        h1 { color: #2c3e50; text-align: center; }
        h2 { color: #34495e; border-bottom: 2px solid #ecf0f1; padding-bottom: 10px; }
        .balance { font-size: 2.5em; color: #27ae60; font-weight: bold; }
        .btn { background: #3498db; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; text-decoration: none; display: inline-block; }
        .btn:hover { background: #2980b9; }
        .btn-green { background: #27ae60; }
        .btn-green:hover { background: #219150; }
        input[type="text"], input[type="number"] { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }
        table { width: 100%; border-collapse: collapse; margin-top: 15px; }
        th, td { text-align: left; padding: 12px; border-bottom: 1px solid #eee; }
        th { background-color: #f8f9fa; }
        .error { color: #e74c3c; background: #fadbd8; padding: 10px; border-radius: 5px; }
        .address { font-family: monospace; background: #f1f2f6; padding: 5px; border-radius: 3px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Clitboin Web Wallet</h1>

        {% if error %}
        <div class="error"><strong>Error:</strong> {{ error }}</div>
        {% endif %}

        <!-- Balance Section -->
        <div class="card">
            <h2>Current Balance</h2>
            <div class="balance">{{ balance }} CLT</div>
            <p>Wallet: <strong>{{ wallet_name }}</strong></p>
        </div>

        <!-- Actions -->
        <div class="card">
            <h2>Actions</h2>
            <form action="/getnewaddress" method="post" style="display:inline;">
                <button type="submit" class="btn">Generate New Address</button>
            </form>
            <form action="/mine" method="post" style="display:inline; margin-left: 10px;">
                <button type="submit" class="btn btn-green">Mine 1 Block (Generate)</button>
            </form>
        </div>

        <!-- Send Coins -->
        <div class="card">
            <h2>Send Clitboin</h2>
            <form action="/send" method="post">
                <label>To Address:</label>
                <input type="text" name="address" placeholder="Enter Clitboin address..." required>
                <label>Amount:</label>
                <input type="text" name="amount" placeholder="0.00" required>
                <button type="submit" class="btn">Send Coins</button>
            </form>
        </div>

        <!-- Transactions -->
        <div class="card">
            <h2>Recent Transactions</h2>
            <table>
                <thead>
                    <tr>
                        <th>Type</th>
                        <th>Amount</th>
                        <th>Confirmations</th>
                        <th>TxID</th>
                    </tr>
                </thead>
                <tbody>
                {% for tx in transactions %}
                    <tr>
                        <td>{{ tx.category }}</td>
                        <td style="color: {{ 'green' if tx.amount > 0 else 'red' }}">{{ tx.amount }}</td>
                        <td>{{ tx.confirmations }}</td>
                        <td><span class="address">{{ tx.txid[:10] }}...</span></td>
                    </tr>
                {% else %}
                    <tr><td colspan="4">No transactions found.</td></tr>
                {% endfor %}
                </tbody>
            </table>
        </div>
        
        <!-- Addresses -->
         <div class="card">
            <h2>My Addresses</h2>
            <ul>
            {% for addr in addresses %}
                <li class="address">{{ addr }}</li>
            {% endfor %}
            </ul>
        </div>
    </div>
</body>
</html>
"""

WALLET_NAME = "mywallet"

@app.route('/')
def index():
    # Ensure wallet is loaded
    rpc_call('loadwallet', [WALLET_NAME])
    
    # Get Balance
    balance = rpc_call('getbalance')
    if isinstance(balance, dict) and 'error' in balance:
        return render_template_string(HTML_TEMPLATE, error=balance['error'], balance="0.00", transactions=[], addresses=[], wallet_name=WALLET_NAME)
    
    # Get Transactions
    txs = rpc_call('listtransactions', ["*", 10])
    if not isinstance(txs, list): txs = []

    # Get Addresses (by grouping)
    # This is a bit hacky for standard wallet, usually use listreceivedbyaddress
    groups = rpc_call('listreceivedbyaddress', [0, True])
    addresses = [g['address'] for g in groups] if isinstance(groups, list) else []

    return render_template_string(HTML_TEMPLATE, 
                                  balance=balance, 
                                  transactions=reversed(txs), 
                                  addresses=addresses,
                                  wallet_name=WALLET_NAME,
                                  error=None)

@app.route('/getnewaddress', methods=['POST'])
def getnewaddress():
    rpc_call('getnewaddress')
    return redirect(url_for('index'))

@app.route('/mine', methods=['POST'])
def mine():
    # Mine 1 block to our own address
    addr = rpc_call('getnewaddress')
    rpc_call('generatetoaddress', [1, addr])
    return redirect(url_for('index'))

@app.route('/send', methods=['POST'])
def send():
    address = request.form['address']
    try:
        amount = float(request.form['amount'])
        res = rpc_call('sendtoaddress', [address, amount])
        if isinstance(res, dict) and 'error' in res:
            return render_template_string(HTML_TEMPLATE, error=res['error'], balance="?", transactions=[], addresses=[], wallet_name=WALLET_NAME)
    except ValueError:
        pass # Handle invalid float
        
    return redirect(url_for('index'))

if __name__ == '__main__':
    print(f"Starting Wallet UI on port 5001...")
    print(f"Connecting to Node at {RPC_URL}")
    app.run(port=5001, debug=True)

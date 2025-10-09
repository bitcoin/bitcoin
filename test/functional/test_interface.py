from http.server import HTTPServer, BaseHTTPRequestHandler
import json

class BitcoinTestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            
            html = """
            <html>
                <head>
                    <title>Bitcoin Test Interface</title>
                    <style>
                        body { font-family: Arial, sans-serif; margin: 40px; }
                        .endpoint { background: #f0f0f0; padding: 10px; margin: 10px 0; border-radius: 5px; }
                        .method { color: #0066cc; }
                    </style>
                </head>
                <body>
                    <h1>Bitcoin Mempool Test Interface</h1>
                    
                    <div class="endpoint">
                        <h3>Test Endpoints:</h3>
                        <p><span class="method">GET</span> <a href="/mempool/status">/mempool/status</a> - Get mempool status</p>
                        <p><span class="method">POST</span> /mempool/addtx - Add transaction to mempool</p>
                        <p><span class="method">GET</span> <a href="/mempool/dust/test">/mempool/dust/test</a> - Test dust transaction handling</p>
                    </div>

                    <div class="endpoint">
                        <h3>RPC Connection:</h3>
                        <p>Local RPC: http://localhost:18443</p>
                        <p>Username: test</p>
                        <p>Password: test123</p>
                    </div>
                </body>
            </html>
            """
            self.wfile.write(html.encode())
            
        elif self.path == '/mempool/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            status = {
                "size": 0,
                "bytes": 0,
                "usage": 0,
                "maxmempool": 300000000,
                "mempoolminfee": 0.00001000,
                "minrelaytxfee": 0.00001000
            }
            self.wfile.write(json.dumps(status).encode())
            
        elif self.path == '/mempool/dust/test':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            test_result = {
                "test": "dust_handling",
                "dust_threshold": 546,
                "test_cases": [
                    {
                        "description": "Transaction with dust output",
                        "dust_amount": 545,
                        "expected": "reject",
                        "reason": "dust"
                    },
                    {
                        "description": "Transaction with valid output",
                        "amount": 1000,
                        "expected": "accept",
                        "reason": "above dust limit"
                    }
                ]
            }
            self.wfile.write(json.dumps(test_result).encode())

def run_test_server(port=8000):
    server_address = ('', port)
    httpd = HTTPServer(server_address, BitcoinTestHandler)
    print(f"Test server running at http://localhost:{port}")
    print("Available endpoints:")
    print("  - http://localhost:8000/ (Main page)")
    print("  - http://localhost:8000/mempool/status (Mempool status)")
    print("  - http://localhost:8000/mempool/dust/test (Test dust handling)")
    httpd.serve_forever()

if __name__ == '__main__':
    run_test_server()
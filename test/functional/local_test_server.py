from http.server import HTTPServer, BaseHTTPRequestHandler
import json
from decimal import Decimal
from test_framework.messages import COIN, CTxOut
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class TestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            
            html = """
            <html>
                <head>
                    <title>Bitcoin Mempool Test Interface</title>
                </head>
                <body>
                    <h1>Bitcoin Mempool Ephemeral Dust Test Interface</h1>
                    <h2>Available Endpoints:</h2>
                    <ul>
                        <li><a href="/test/create-dust">/test/create-dust</a> - Create a dust transaction</li>
                        <li><a href="/test/create-sweep">/test/create-sweep</a> - Create a sweep transaction</li>
                    </ul>
                </body>
            </html>
            """
            self.wfile.write(html.encode())
        
        elif self.path == '/test/create-dust':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            response = {
                "status": "success",
                "message": "Dust transaction created",
                "details": {
                    "type": "dust transaction",
                    "value": "0",
                    "test_type": "ephemeral dust"
                }
            }
            self.wfile.write(json.dumps(response).encode())

def run_test_server(port=8000):
    server_address = ('', port)
    httpd = HTTPServer(server_address, TestHandler)
    print(f"Test server running at http://localhost:{port}")
    print("Available endpoints:")
    print("  - http://localhost:8000/ (Main page)")
    print("  - http://localhost:8000/test/create-dust (Create dust transaction)")
    httpd.serve_forever()

if __name__ == '__main__':
    run_test_server()
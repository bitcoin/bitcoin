#!/usr/bin/env python3
from http.server import HTTPServer, BaseHTTPRequestHandler
from test_framework.test_framework import BitcoinTestFramework
from mempool_ephemeral_dust import EphemeralDustTest

class TestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        
        response = """
        <html>
            <head>
                <title>Bitcoin Test Interface</title>
            </head>
            <body>
                <h1>Bitcoin Ephemeral Dust Test Interface</h1>
                <p>Available endpoints:</p>
                <ul>
                    <li>/run-test - Run the ephemeral dust test</li>
                </ul>
            </body>
        </html>
        """
        self.wfile.write(response.encode())

def run_server(port=8000):
    server_address = ('', port)
    httpd = HTTPServer(server_address, TestHandler)
    print(f"Server running at http://localhost:{port}")
    httpd.serve_forever()

if __name__ == '__main__':
    run_server()
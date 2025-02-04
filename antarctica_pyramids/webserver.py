
from http.server import HTTPServer, SimpleHTTPRequestHandler

class CustomHTTPRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add required headers
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Resource-Policy', 'cross-origin')
        self.send_header('Permissions-Policy', 'storage-access=(self)')
        SimpleHTTPRequestHandler.end_headers(self)

httpd = HTTPServer(('localhost', 8000), CustomHTTPRequestHandler)
httpd.serve_forever()

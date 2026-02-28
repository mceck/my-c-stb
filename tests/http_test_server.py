#!/usr/bin/env python3
"""Minimal HTTP test server for http.h test suite."""
from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import sys


class TestHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

    def _read_body(self):
        length = int(self.headers.get('Content-Length', 0))
        return self.rfile.read(length) if length > 0 else b''

    def _send(self, code, body_str, content_type='text/plain'):
        body = body_str.encode('utf-8')
        self._send_bytes(code, body, content_type)

    def _send_bytes(self, code, body, content_type='application/octet-stream'):
        self.send_response(code)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        if self.command != 'HEAD':
            self.wfile.write(body)

    def _handle(self):
        path = self.path
        body = self._read_body()

        if path == '/echo':
            resp = json.dumps({
                'method': self.command,
                'path': path,
                'body': body.decode('utf-8', errors='replace'),
                'content_length': len(body),
            })
            self._send(200, resp, 'application/json')

        elif path == '/method':
            self._send(200, self.command)

        elif path == '/echo-body':
            self._send_bytes(200, body)

        elif path.startswith('/status/'):
            try:
                code = int(path.split('/')[2])
                if code == 204:
                    self.send_response(204)
                    self.end_headers()
                else:
                    self._send(code, f'Status: {code}')
            except (ValueError, IndexError):
                self._send(400, 'Bad request')

        elif path == '/redirect':
            self.send_response(302)
            self.send_header('Location', '/method')
            self.send_header('Content-Length', '0')
            self.end_headers()

        elif path == '/large':
            self._send(200, 'X' * 100000)

        elif path == '/empty':
            self._send(200, '')

        elif path == '/headers':
            headers = {}
            for key in self.headers:
                headers[key.lower()] = self.headers[key]
            self._send(200, json.dumps(headers), 'application/json')

        else:
            self._send(404, 'Not found')

    do_GET = _handle
    do_POST = _handle
    do_PUT = _handle
    do_PATCH = _handle
    do_DELETE = _handle
    do_OPTIONS = _handle
    do_HEAD = _handle


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18234
    server = HTTPServer(('127.0.0.1', port), TestHandler)
    print('READY', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    server.server_close()

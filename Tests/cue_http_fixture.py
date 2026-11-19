"""Loopback-only HTTP fixtures for native Cue tests; never opens a database."""
import argparse
import copy
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import time

SNAPSHOT = json.loads((Path(__file__).resolve().parents[1] / 'CueServer/examples/cue-list.json').read_text(encoding='utf-8'))


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        scenario, _, route = self.path.lstrip('/').partition('/')
        if scenario == 'slow':
            time.sleep(0.8)
        if scenario == 'timeout':
            time.sleep(3)
        status = 503 if scenario == 'error' else 200
        snapshot = copy.deepcopy(SNAPSHOT)
        if scenario == 'new':
            snapshot['revision'] = 7
        if scenario == 'wrong-show':
            snapshot['showId'] = '00000000-0000-0000-0000-000000000001'
        if scenario == 'stage':
            snapshot['showId'] = '00000000-0000-0000-0000-000000000002'
            snapshot['cues'][2]['cameraState'] = 'Stage'
        if scenario == 'invalid-camera':
            snapshot['cues'][2]['cameraState'] = 'Wide'
        payload = snapshot if route.endswith('/cue-list') else [{
            'id': SNAPSHOT['showId'], 'name': 'Fixture Show', 'revision': 6,
            'createdAt': '2026-11-17T00:00:00Z', 'updatedAt': '2026-11-18T00:00:00Z',
        }]
        body = b'{invalid' if scenario == 'bad' else json.dumps(payload).encode('utf-8')
        if scenario == 'large':
            body = b' ' * (4 * 1024 * 1024 + 1)
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass  # Expected for cancellation and timeout tests.

    def log_message(self, *_):
        pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', type=int, default=18081)
    args = parser.parse_args()
    server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
    print(f'Cue fixture listening on 127.0.0.1:{server.server_port}', flush=True)
    server.serve_forever()

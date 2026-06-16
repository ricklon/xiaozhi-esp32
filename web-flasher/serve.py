#!/usr/bin/env python3
"""HTTP server for XiaoZhi Web Flasher with proper CORS and caching headers."""

import http.server
import argparse
import errno
import os
import socket
import socketserver
from pathlib import Path


class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers for Web Serial API
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "*")
        # Disable caching for development
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        super().end_headers()

    def log_message(self, format, *args):
        # Custom logging
        print(f"[{self.log_date_time_string()}] {args[0]}")


class ReusableTcpServer(socketserver.TCPServer):
    allow_reuse_address = True


def get_lan_ip():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.connect(("8.8.8.8", 80))
            return sock.getsockname()[0]
    except OSError:
        return None


def main():
    parser = argparse.ArgumentParser(description="Serve the XiaoZhi web flasher.")
    parser.add_argument("port", nargs="?", type=int, default=8080)
    parser.add_argument("--max-port-tries", type=int, default=20)
    args = parser.parse_args()
    
    # Change to the script's directory
    script_dir = Path(__file__).resolve().parent
    os.chdir(script_dir)
    
    requested_port = args.port
    httpd = None
    port = requested_port
    for candidate in range(requested_port, requested_port + args.max_port_tries):
        try:
            httpd = ReusableTcpServer(("", candidate), Handler)
            port = candidate
            break
        except OSError as error:
            if error.errno != errno.EADDRINUSE:
                raise
            print(f"Port {candidate} is in use, trying {candidate + 1}...")

    if httpd is None:
        raise OSError(f"No free port found from {requested_port} to {requested_port + args.max_port_tries - 1}")

    with httpd:
        print("XiaoZhi Web Flasher")
        print(f"URL: http://localhost:{port}")
        lan_ip = get_lan_ip()
        if lan_ip:
            print(f"LAN: http://{lan_ip}:{port}")
        print("Press Ctrl+C to stop.")
        
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\nShutting down...")
            httpd.shutdown()


if __name__ == "__main__":
    main()

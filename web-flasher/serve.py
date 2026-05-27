#!/usr/bin/env python3
"""HTTP server for XiaoZhi Web Flasher with proper CORS and caching headers."""

import http.server
import argparse
import os
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


def main():
    parser = argparse.ArgumentParser(description="Serve the XiaoZhi web flasher.")
    parser.add_argument("port", nargs="?", type=int, default=8080)
    args = parser.parse_args()
    
    # Change to the script's directory
    script_dir = Path(__file__).parent
    os.chdir(script_dir)
    
    port = args.port
    with socketserver.TCPServer(("", port), Handler) as httpd:
        print("XiaoZhi Web Flasher")
        print(f"URL: http://localhost:{port}")
        print("Press Ctrl+C to stop.")
        
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\nShutting down...")
            httpd.shutdown()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
import argparse
import asyncio
import json
import secrets
from pathlib import Path

from aiohttp import web, WSMsgType


class Harness:
    def __init__(self, public_ip: str, output_dir: Path) -> None:
        self.public_ip = public_ip
        self.output_dir = output_dir
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.session_id = secrets.token_hex(8)
        self.ws = None
        self.call_complete = asyncio.Event()
        self.upload_complete = asyncio.Event()
        self.last_tool_result = None
        self.last_tool_error = None
        self.last_upload_info = None

    def ota_payload(self) -> dict:
        return {
            "websocket": {
                "url": f"ws://{self.public_ip}:8000/xiaozhi/v1/",
                "token": "",
                "version": 1,
            },
            "firmware": {
                "version": "2.2.6",
                "url": "",
            },
        }

    async def ota_handler(self, request: web.Request) -> web.Response:
        try:
            body = await request.text()
        except Exception:
            body = ""
        print(f"[ota] {request.method} {request.path} bytes={len(body)}")
        return web.json_response(self.ota_payload())

    async def vision_handler(self, request: web.Request) -> web.Response:
        raw = await request.read()
        content_type = request.headers.get("Content-Type", "")
        output_path = self.output_dir / "camera-upload.bin"
        output_path.write_bytes(raw)
        self.last_upload_info = {
            "bytes": len(raw),
            "content_type": content_type,
            "path": str(output_path),
        }
        print(f"[vision] upload bytes={len(raw)} content_type={content_type}")
        self.upload_complete.set()
        return web.json_response(self.last_upload_info)

    async def send_mcp(self, payload: dict) -> None:
        assert self.ws is not None
        msg = {"session_id": self.session_id, "type": "mcp", "payload": payload}
        await self.ws.send_str(json.dumps(msg))
        print(f"[ws->device] {json.dumps(payload)}")

    async def handle_device_hello(self, msg: dict) -> None:
        server_hello = {
            "type": "hello",
            "transport": "websocket",
            "session_id": self.session_id,
            "audio_params": {
                "sample_rate": 24000,
                "channels": 1,
                "frame_duration": 60,
            },
        }
        await self.ws.send_str(json.dumps(server_hello))
        print(f"[ws->device] {json.dumps(server_hello)}")

        await asyncio.sleep(0.2)
        await self.send_mcp(
            {
                "jsonrpc": "2.0",
                "method": "initialize",
                "params": {
                    "capabilities": {
                        "vision": {
                            "url": f"http://{self.public_ip}:8003/vision",
                            "token": "",
                        }
                    }
                },
                "id": 1,
            }
        )

    async def maybe_send_take_photo(self, payload: dict) -> None:
        if payload.get("id") != 1 or "result" not in payload:
            return
        await asyncio.sleep(0.2)
        await self.send_mcp(
            {
                "jsonrpc": "2.0",
                "method": "tools/call",
                "params": {
                    "name": "self.camera.take_photo",
                    "arguments": {"question": "Return a short camera test result."},
                },
                "id": 2,
            }
        )

    async def websocket_handler(self, request: web.Request) -> web.WebSocketResponse:
        ws = web.WebSocketResponse(heartbeat=30)
        await ws.prepare(request)
        self.ws = ws
        print(f"[ws] connected path={request.path} headers={dict(request.headers)}")

        async for msg in ws:
            if msg.type == WSMsgType.TEXT:
                data = json.loads(msg.data)
                print(f"[device->ws] {msg.data}")
                if data.get("type") == "hello":
                    await self.handle_device_hello(data)
                    continue
                if data.get("type") != "mcp":
                    continue
                payload = data.get("payload", {})
                if payload.get("id") == 1:
                    await self.maybe_send_take_photo(payload)
                elif payload.get("id") == 2:
                    if "result" in payload:
                        self.last_tool_result = payload["result"]
                        print(f"[result] {json.dumps(payload['result'])}")
                    else:
                        self.last_tool_error = payload.get("error")
                        print(f"[error] {json.dumps(payload.get('error'))}")
                    self.call_complete.set()
            elif msg.type == WSMsgType.ERROR:
                print(f"[ws] error {ws.exception()}")

        print("[ws] disconnected")
        self.call_complete.set()
        return ws


async def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--public-ip", required=True)
    parser.add_argument("--output-dir", default="tmp/camera-harness")
    parser.add_argument("--timeout-seconds", type=int, default=90)
    args = parser.parse_args()

    harness = Harness(args.public_ip, Path(args.output_dir))

    ota_app = web.Application()
    ota_app.router.add_post("/xiaozhi/ota/", harness.ota_handler)
    ota_app.router.add_get("/xiaozhi/ota/", harness.ota_handler)
    ota_app.router.add_post("/vision", harness.vision_handler)

    ws_app = web.Application()
    ws_app.router.add_get("/xiaozhi/v1/", harness.websocket_handler)

    ota_runner = web.AppRunner(ota_app)
    ws_runner = web.AppRunner(ws_app)
    await ota_runner.setup()
    await ws_runner.setup()
    await web.TCPSite(ota_runner, "0.0.0.0", 8003).start()
    await web.TCPSite(ws_runner, "0.0.0.0", 8000).start()

    print(f"[ready] ota=http://{args.public_ip}:8003/xiaozhi/ota/")
    print(f"[ready] ws=ws://{args.public_ip}:8000/xiaozhi/v1/")
    print("[ready] point device with: !server " + args.public_ip)

    try:
        await asyncio.wait_for(harness.call_complete.wait(), timeout=args.timeout_seconds)
    except asyncio.TimeoutError:
        print("[timeout] no tool completion received")
        return 1
    finally:
        await ota_runner.cleanup()
        await ws_runner.cleanup()

    if harness.last_tool_error is not None:
        return 2
    if harness.last_tool_result is None:
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))

# XiaoZhi ESP32 - LLM Connection Guide

## Overview

The xiaozhi-esp32 project supports **3 ways** to connect to LLMs:

1. **Official Server (Easiest)** - Connect to xiaozhi.me (free Qwen model)
2. **Self-Hosted Server** - Run your own backend with any LLM
3. **Custom Server** - Build your own WebSocket server

---

## Option 1: Official xiaozhi.me Server (Recommended for Beginners)

### What You Get:
- ✅ **Free** for personal use
- ✅ **Qwen real-time model** (Alibaba)
- ✅ **No setup required** - works out of the box
- ✅ **Voice interaction** - ASR + TTS included
- ✅ **Web console** for configuration

### How to Connect:

1. **Flash the firmware** (already done in setup)
2. **Connect to Wi-Fi**
   ```bash
   idf.py menuconfig
   # Navigate to: XiaoZhi AI Chatbot -> Wi-Fi Configuration
   # Set your Wi-Fi SSID and Password
   ```

3. **Get activation code**
   - Power on ESP32
   - Connect serial monitor: `idf.py monitor`
   - Look for activation code in serial output

4. **Register device**
   - Go to: https://xiaozhi.me
   - Create free account
   - Add device with activation code
   - Configure wake word, voice, etc.

5. **Start talking!**
   - Say wake word (default: "你好小智" or "Hi XiaoZhi")
   - Ask questions, give commands
   - AI responds with voice!

### Configuration in menuconfig:
```
XiaoZhi AI Chatbot Configuration
  ├── WebSocket URL: wss://api.xiaozhi.me/xiaozhi/v1/
  ├── Use TLS: Yes
  └── Wi-Fi Configuration
        ├── SSID: your-wifi-name
        └── Password: your-wifi-password
```

---

## Option 2: Self-Hosted Server (Full Control)

Run your own backend server to use **any LLM** you want!

### Supported LLM Providers:
- **OpenAI** (GPT-4, GPT-3.5)
- **DeepSeek**
- **Alibaba** (Qwen series)
- **Zhipu** (GLM series)
- **Gemini** (Google)
- **Ollama** (local models)
- **Any OpenAI-compatible API**

### Quick Setup:

#### Step 1: Clone Server Repository
```bash
cd ~/esp-projects
git clone https://github.com/xinnan-tech/xiaozhi-esp32-server.git
cd xiaozhi-esp32-server
```

#### Step 2: Choose Deployment Method

**Method A: Docker (Easiest)**
```bash
# Install Docker and Docker Compose first
# Then run:
docker-compose up -d
```

**Method B: Local Python**
```bash
# Install dependencies
pip install -r requirements.txt

# Configure
cp config.yaml.example config.yaml
# Edit config.yaml with your LLM API keys

# Run server
python main.py
```

#### Step 3: Configure LLM in config.yaml
```yaml
llm:
  # Option 1: OpenAI
  provider: openai
  api_key: sk-your-openai-key
  model: gpt-4
  
  # Option 2: DeepSeek
  # provider: deepseek
  # api_key: sk-your-deepseek-key
  # model: deepseek-chat
  
  # Option 3: Alibaba Qwen
  # provider: aliyun
  # api_key: sk-your-aliyun-key
  # model: qwen-max
  
  # Option 4: Local Ollama
  # provider: ollama
  # base_url: http://localhost:11434
  # model: llama2
```

#### Step 4: Configure ESP32 to Connect to Your Server

In your ESP32 project:
```bash
idf.py menuconfig
```

Navigate to:
```
XiaoZhi AI Chatbot Configuration
  ├── WebSocket URL: ws://YOUR_SERVER_IP:8000/xiaozhi/v1/
  ├── Use TLS: No (or Yes if using HTTPS/WSS)
  └── Wi-Fi Configuration
```

#### Step 5: Flash and Test
```bash
idf.py build flash monitor
```

---

## Option 3: Custom WebSocket Server (Advanced)

Build your own server using the WebSocket protocol.

### Protocol Overview:
The ESP32 uses WebSocket with JSON + binary Opus audio frames.

### Basic Server Structure (Python Example):

```python
import asyncio
import websockets
import json

async def handle_client(websocket, path):
    # 1. Wait for hello message from device
    hello = await websocket.recv()
    data = json.loads(hello)
    print(f"Device connected: {data}")
    
    # 2. Send hello response
    await websocket.send(json.dumps({
        "type": "hello",
        "transport": "websocket",
        "session_id": "session-123",
        "audio_params": {
            "format": "opus",
            "sample_rate": 16000,
            "channels": 1,
            "frame_duration": 60
        }
    }))
    
    # 3. Handle messages
    async for message in websocket:
        if isinstance(message, bytes):
            # Binary audio data (Opus encoded)
            # Decode and send to ASR -> LLM -> TTS
            audio_data = message
            
            # Your ASR + LLM + TTS pipeline here
            # ...
            
            # Send back TTS audio as binary
            await websocket.send(tts_audio_bytes)
        else:
            # JSON message
            data = json.loads(message)
            msg_type = data.get("type")
            
            if msg_type == "listen":
                # Device started/stopped listening
                pass
            elif msg_type == "stt":
                # ASR result from device (if local ASR)
                user_text = data.get("text")
                
                # Send to LLM
                llm_response = call_your_llm(user_text)
                
                # Send TTS start
                await websocket.send(json.dumps({
                    "type": "tts",
                    "state": "start"
                }))
                
                # Stream TTS audio as binary frames
                # ...
                
                # Send TTS stop
                await websocket.send(json.dumps({
                    "type": "tts",
                    "state": "stop"
                }))

# Run server
start_server = websockets.serve(handle_client, "0.0.0.0", 8000)
asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
```

### Full Protocol Documentation:
See: `~/esp-projects/xiaozhi-esp32/docs/websocket.md`

---

## LLM Configuration Examples

### Example 1: Using DeepSeek

**Server config.yaml:**
```yaml
llm:
  provider: deepseek
  api_key: sk-your-deepseek-key-here
  model: deepseek-chat
  base_url: https://api.deepseek.com/v1
  temperature: 0.7
  max_tokens: 1024
```

**ESP32 menuconfig:**
```
WebSocket URL: ws://YOUR_SERVER_IP:8000/xiaozhi/v1/
Use TLS: No
```

### Example 2: Using Local Ollama

**Server config.yaml:**
```yaml
llm:
  provider: ollama
  base_url: http://localhost:11434
  model: llama3.2
  temperature: 0.7
```

**Run Ollama:**
```bash
ollama run llama3.2
```

**ESP32:** Connect to local server

### Example 3: Using OpenAI with Custom Prompt

**Server config.yaml:**
```yaml
llm:
  provider: openai
  api_key: sk-your-openai-key
  model: gpt-4
  system_prompt: |
    You are a helpful AI assistant in a voice-activated device.
    Keep responses concise (1-2 sentences).
    Be friendly and engaging.
```

---

## Complete Pipeline Architecture

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│   User      │────▶│   ESP32      │────▶│   Server    │
│  (Voice)    │     │  (Microphone)│     │  (WebSocket)│
└─────────────┘     └──────────────┘     └──────┬──────┘
                                                 │
                       ┌─────────────────────────┘
                       ▼
              ┌─────────────────┐
              │  ASR (Speech    │
              │  Recognition)   │
              └────────┬────────┘
                       ▼
              ┌─────────────────┐
              │  LLM (Large     │
              │  Language Model)│
              └────────┬────────┘
                       ▼
              ┌─────────────────┐
              │  TTS (Text-to   │
              │  Speech)        │
              └────────┬────────┘
                       │
                       ▼
┌─────────────┐     ┌──────────────┐
│   User      │◀────│   ESP32      │
│  (Hears)    │     │  (Speaker)   │
└─────────────┘     └──────────────┘
```

---

## Testing Your LLM Connection

### Test 1: Check Server Health
```bash
curl http://YOUR_SERVER_IP:8000/health
```

### Test 2: WebSocket Test
```bash
# Use wscat or similar
wscat -c ws://YOUR_SERVER_IP:8000/xiaozhi/v1/ \
  -H "Authorization: Bearer YOUR_TOKEN"
```

### Test 3: Serial Monitor
```bash
idf.py monitor
# Look for:
# - WebSocket connection messages
# - "hello" handshake
# - Audio frame transmission
```

---

## Recommended Setup for Your System

Since you have:
- ✅ ESP-IDF v5.5.4
- ✅ ESP32-S3 (recommended)
- ✅ Linux environment

### Quick Start Recommendation:

1. **Start with xiaozhi.me** (Option 1)
   - Easiest to verify hardware works
   - Free to use
   - No server setup needed

2. **Then try self-hosted** (Option 2)
   - More privacy
   - Use any LLM
   - Full control

3. **Build custom** (Option 3) if needed
   - Specific requirements
   - Custom integrations

---

## Next Steps

### If you want to use xiaozhi.me (easiest):
```bash
cd ~/esp-projects/xiaozhi-esp32
./setup.sh
# Configure Wi-Fi in menuconfig
# Flash and get activation code
# Register at https://xiaozhi.me
```

### If you want self-hosted server:
```bash
# Clone server
cd ~/esp-projects
git clone https://github.com/xinnan-tech/xiaozhi-esp32-server.git

# Follow server README for setup
# Configure your LLM API keys
# Point ESP32 to your server
```

### Which LLM do you want to use?

Tell me and I'll help you set it up:
- **OpenAI** (GPT-4)
- **DeepSeek**
- **Alibaba Qwen**
- **Local Ollama**
- **Something else?**

---

## Troubleshooting

### Connection Issues:
```bash
# Check if server is reachable
curl http://SERVER_IP:8000/

# Check WebSocket
websocat ws://SERVER_IP:8000/xiaozhi/v1/

# Check ESP32 Wi-Fi
idf.py monitor
# Look for Wi-Fi connection logs
```

### Audio Issues:
- Check microphone wiring (INMP441 I2S)
- Check speaker/amplifier wiring
- Verify sample rates match (16kHz default)

### LLM Not Responding:
- Check API key is valid
- Check LLM provider status
- Review server logs

---

**Ready to connect to an LLM?** Which option interests you most?

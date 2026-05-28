#include "protocol.h"

#include <esp_log.h>

#define TAG "Protocol"

static std::string PrintJson(cJSON* root) {
    char* json_str = cJSON_PrintUnformatted(root);
    std::string result(json_str);
    cJSON_free(json_str);
    cJSON_Delete(root);
    return result;
}

void Protocol::OnIncomingJson(std::function<void(const cJSON* root)> callback) {
    on_incoming_json_ = callback;
}

void Protocol::OnIncomingAudio(std::function<void(std::unique_ptr<AudioStreamPacket> packet)> callback) {
    on_incoming_audio_ = callback;
}

void Protocol::OnAudioChannelOpened(std::function<void()> callback) {
    on_audio_channel_opened_ = callback;
}

void Protocol::OnAudioChannelClosed(std::function<void()> callback) {
    on_audio_channel_closed_ = callback;
}

void Protocol::OnNetworkError(std::function<void(const std::string& message)> callback) {
    on_network_error_ = callback;
}

void Protocol::OnConnected(std::function<void()> callback) {
    on_connected_ = callback;
}

void Protocol::OnDisconnected(std::function<void()> callback) {
    on_disconnected_ = callback;
}

void Protocol::SetError(const std::string& message) {
    error_occurred_ = true;
    if (on_network_error_ != nullptr) {
        on_network_error_(message);
    }
}

void Protocol::SendAbortSpeaking(AbortReason reason) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session_id_.c_str());
    cJSON_AddStringToObject(root, "type", "abort");
    if (reason == kAbortReasonWakeWordDetected) {
        cJSON_AddStringToObject(root, "reason", "wake_word_detected");
    }
    SendText(PrintJson(root));
}

void Protocol::SendWakeWordDetected(const std::string& wake_word) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session_id_.c_str());
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "detect");
    cJSON_AddStringToObject(root, "text", wake_word.c_str());
    SendText(PrintJson(root));
}

void Protocol::SendStartListening(ListeningMode mode) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session_id_.c_str());
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "start");
    if (mode == kListeningModeRealtime) {
        cJSON_AddStringToObject(root, "mode", "realtime");
    } else if (mode == kListeningModeAutoStop) {
        cJSON_AddStringToObject(root, "mode", "auto");
    } else {
        cJSON_AddStringToObject(root, "mode", "manual");
    }
    SendText(PrintJson(root));
}

void Protocol::SendStopListening() {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session_id_.c_str());
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "stop");
    SendText(PrintJson(root));
}

void Protocol::SendMcpMessage(const std::string& payload) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session_id_.c_str());
    cJSON_AddStringToObject(root, "type", "mcp");

    cJSON* payload_json = cJSON_Parse(payload.c_str());
    if (payload_json == nullptr) {
        ESP_LOGE(TAG, "Invalid MCP payload JSON");
        cJSON_Delete(root);
        return;
    }
    cJSON_AddItemToObject(root, "payload", payload_json);
    SendText(PrintJson(root));
}

bool Protocol::IsTimeout() const {
    const int kTimeoutSeconds = 120;
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_incoming_time_);
    bool timeout = duration.count() > kTimeoutSeconds;
    if (timeout) {
        ESP_LOGE(TAG, "Channel timeout %ld seconds", (long)duration.count());
    }
    return timeout;
}

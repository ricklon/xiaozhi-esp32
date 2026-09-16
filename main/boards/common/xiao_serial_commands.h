#pragma once

// Shared serial command handler for board builds that opt into the console.
// Handles: !wifi, !server, !status, !camera, !reboot, !help
// Anything else is forwarded to the chat pipeline.

#include "application.h"
#include "board.h"
#include "settings.h"
#include "ssid_manager.h"
#include "system_info.h"
#include "wifi_manager.h"
#include "assets/lang_config.h"
#include "sdkconfig.h"

#include <esp_app_desc.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <vector>

#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
#include <fcntl.h>
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#endif

static const char* GetXiaoSerialBoardName() {
#if CONFIG_BOARD_TYPE_XIAO_ESP32C3
    return "XIAO ESP32-C3";
#elif CONFIG_BOARD_TYPE_XIAO_ESP32C6
    return "XIAO ESP32-C6";
#elif CONFIG_BOARD_TYPE_XIAO_ESP32C6_EYES
    return "XIAO ESP32-C6 Eyes";
#elif CONFIG_BOARD_TYPE_COGLET
    return "Coglet (XIAO ESP32-S3 Sense)";
#elif CONFIG_BOARD_TYPE_COGLET_C3
    return "Coglet (XIAO ESP32-C3)";
#elif CONFIG_BOARD_TYPE_XIAO_ESP32S3_SENSE
    return "XIAO ESP32-S3 Sense";
#elif CONFIG_BOARD_TYPE_XIAO_ESP32S3_EYES
    return "XIAO ESP32-S3 Eyes";
#elif CONFIG_BOARD_TYPE_ESP32_S3_WROOM_2
    return "ESP32-S3-WROOM-2";
#elif CONFIG_BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_AMOLED_1_8
    return "Waveshare ESP32-S3 Touch AMOLED 1.8";
#elif CONFIG_BOARD_TYPE_DF_K10
    return "DFRobot UNIHIKER K10";
#else
    return "unknown";
#endif
}

static const char* TrimXiaoSerialLine(char* buf) {
    char* start = buf;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    char* end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        *(--end) = '\0';
    }

    const char* known_commands[] = {
        "!reboot", "!status", "!camera", "!server", "!wifi", "!mic", "!speaker", "!stop", "!quiet", "!help"
    };
    for (const char* command : known_commands) {
        char* command_start = strstr(start, command);
        if (command_start != nullptr) {
            return command_start;
        }
    }

    if (*start != '!') {
        char* command_start = strchr(start, '!');
        if (command_start != nullptr) {
            return command_start;
        }
    }

    return start;
}

// --- recent-server history (newline-delimited, most-recent-first, deduped) ---
static const int kXiaoServerHistoryMax = 5;

static std::string XiaoRedactServerUrl(const std::string& url) {
    std::string redacted = url;
    const std::string key = "enrollment_token=";
    size_t value_start = redacted.find(key);
    if (value_start == std::string::npos) {
        return redacted;
    }
    value_start += key.size();
    size_t value_end = redacted.find('&', value_start);
    redacted.replace(value_start,
        value_end == std::string::npos ? std::string::npos : value_end - value_start,
        "<redacted>");
    return redacted;
}

static std::vector<std::string> XiaoServerHistoryLoad() {
    Settings s("wifi", false);
    std::string blob = s.GetString("srv_hist");
    std::vector<std::string> out;
    size_t pos = 0;
    while (pos < blob.size()) {
        size_t nl = blob.find('\n', pos);
        std::string item = blob.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        if (!item.empty()) out.push_back(item);
        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
    return out;
}

// Record `url` as the most-recently-used server, removing any duplicate and
// capping the list length.
static void XiaoServerHistoryAdd(const std::string& url) {
    std::vector<std::string> hist = XiaoServerHistoryLoad();
    for (size_t i = 0; i < hist.size(); i++) {
        if (hist[i] == url) { hist.erase(hist.begin() + i); break; }
    }
    hist.insert(hist.begin(), url);
    if ((int)hist.size() > kXiaoServerHistoryMax) hist.resize(kXiaoServerHistoryMax);
    std::string blob;
    for (const auto& item : hist) { blob += item; blob += '\n'; }
    Settings s("wifi", true);
    s.SetString("srv_hist", blob);
}

// Optional board-local extension; existing profiles retain their handler.
#if defined(CONFIG_BOARD_TYPE_COGLET) || defined(CONFIG_BOARD_TYPE_COGLET_C3)
bool CogletSerialCommand(const char* line);
#endif
static void HandleXiaoSerialLine(const char* buf) {
#if defined(CONFIG_BOARD_TYPE_COGLET) || defined(CONFIG_BOARD_TYPE_COGLET_C3)
    if (CogletSerialCommand(buf)) return;
#endif
    auto& ssid_manager = SsidManager::GetInstance();

    // --- !wifi ---
    if (strncmp(buf, "!wifi", 5) == 0 && (buf[5] == ' ' || buf[5] == '\0')) {
        const char* args = buf[5] == ' ' ? buf + 6 : "";
        if (strcmp(args, "list") == 0) {
            const auto& list = ssid_manager.GetSsidList();
            printf("\r\n=== Saved WiFi Networks (%d) ===\r\n", (int)list.size());
            if (list.empty()) {
                printf("  (none)\r\n");
            } else {
                for (int i = 0; i < (int)list.size(); i++) {
                    printf("  [%d] %s\r\n", i + 1, list[i].ssid.c_str());
                }
            }
            printf("================================\r\n\r\n");
        } else if (strcmp(args, "clear") == 0) {
            ssid_manager.Clear();
            printf("\r\n=== WiFi Networks Cleared ===\r\n\r\n");
        } else if (strlen(args) > 0) {
            char ssid[64] = {}, pass[64] = {};
            if (sscanf(args, "%63s %63s", ssid, pass) == 2) {
                ssid_manager.AddSsid(std::string(ssid), std::string(pass));
                printf("\r\n=== WiFi Network Added ===\r\n");
                printf("SSID: %s\r\n", ssid);
                printf("==========================\r\n\r\n");
            } else {
                printf("\r\nUsage: !wifi SSID PASSWORD\r\n\r\n");
            }
        } else {
            printf("\r\nWiFi Commands:\r\n");
            printf("  !wifi SSID PASSWORD  - Add network\r\n");
            printf("  !wifi list           - Show saved networks\r\n");
            printf("  !wifi clear          - Remove all networks\r\n\r\n");
        }
        fflush(stdout);
        return;
    }

    // --- !server ---
    if (strncmp(buf, "!server", 7) == 0 && (buf[7] == ' ' || buf[7] == '\0')) {
        const char* args = buf[7] == ' ' ? buf + 8 : "";
        std::vector<std::string> hist = XiaoServerHistoryLoad();
        if (strlen(args) == 0) {
            Settings s("wifi", false);
            std::string stored = s.GetString("ota_url");
            std::string displayed_url = XiaoRedactServerUrl(stored.empty() ? CONFIG_OTA_URL : stored);
            printf("OTA URL: %s\r\n", displayed_url.c_str());
            printf("Usage: !server IP  or  !server http://IP:8003/xiaozhi/ota/\r\n");
            printf("       !server list  -- recent servers   !server N  -- reuse #N\r\n");
            fflush(stdout);
            return;
        }
        if (strcmp(args, "list") == 0) {
            printf("\r\n=== Recent Servers (%d) ===\r\n", (int)hist.size());
            if (hist.empty()) {
                printf("  (none yet)\r\n");
            } else {
                for (int i = 0; i < (int)hist.size(); i++) {
                    std::string displayed_url = XiaoRedactServerUrl(hist[i]);
                    printf("  [%d] %s\r\n", i + 1, displayed_url.c_str());
                }
            }
            printf("===========================\r\n");
            printf("Reuse one with: !server N\r\n\r\n");
            fflush(stdout);
            return;
        }
        {
            std::string url(args);
            // Numeric arg -> reuse recent server #N from `!server list`.
            bool is_index = !url.empty();
            for (char c : url) if (!isdigit((unsigned char)c)) { is_index = false; break; }
            if (is_index) {
                int idx = atoi(url.c_str());
                if (idx < 1 || idx > (int)hist.size()) {
                    printf("\r\nNo recent server #%d. Try: !server list\r\n\r\n", idx);
                    fflush(stdout);
                    return;
                }
                url = hist[idx - 1];  // already a full URL; skip host rewriting
                goto server_commit;
            }
            // Bare host/IP (optionally host:port): update only the host, keeping
            // the existing path (e.g. "/xiaozhi/ota/") intact instead of resetting
            // it. A bare IP always means a local dev server, so use http and
            // default to :8003 when neither the arg nor the current URL give a port.
            if (url.find("http") != 0) {
                Settings sr("wifi", false);
                std::string base = sr.GetString("ota_url");
                if (base.empty()) base = CONFIG_OTA_URL;

                // Split the new arg into host and optional :port
                std::string new_host = url;
                std::string new_port;
                size_t ac = url.find(':');
                if (ac != std::string::npos) {
                    new_host = url.substr(0, ac);
                    new_port = url.substr(ac);  // includes leading ':'
                }

                // Locate the authority and path of the current URL
                size_t scheme = base.find("://");
                size_t astart = (scheme == std::string::npos) ? 0 : scheme + 3;
                size_t aend = base.find('/', astart);
                std::string authority = base.substr(astart,
                    aend == std::string::npos ? std::string::npos : aend - astart);
                std::string path = (aend == std::string::npos) ? "" : base.substr(aend);
                // A bare "/" (or empty) path is not a usable OTA endpoint — fall
                // back to the default so a truncated stored URL self-heals.
                if (path.empty() || path == "/") path = "/xiaozhi/ota/";

                // Preserve the current port; default to :8003 if none is set anywhere
                std::string base_port;
                size_t bc = authority.find(':');
                if (bc != std::string::npos) base_port = authority.substr(bc);
                std::string port = !new_port.empty() ? new_port
                                 : !base_port.empty() ? base_port : ":8003";

                url = "http://" + new_host + port + path;
            }
        server_commit:
            {
                Settings s("wifi", true);
                s.SetString("ota_url", url);
            }
            XiaoServerHistoryAdd(url);
            printf("\r\n=== Server Configured ===\r\n");
            std::string displayed_url = XiaoRedactServerUrl(url);
            printf("URL: %s\r\n", displayed_url.c_str());
            printf("========================\r\n");
            printf("Rebooting in 1 second...\r\n\r\n");
            fflush(stdout);
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
        fflush(stdout);
        return;
    }

    // --- !status ---
    if (strcmp(buf, "!status") == 0) {
        auto& wifi = WifiManager::GetInstance();
        Settings s("wifi", false);
        std::string ota_url = s.GetString("ota_url");
        if (ota_url.empty()) ota_url = std::string(CONFIG_OTA_URL) + " (default)";
        printf("\r\n========== DEVICE STATUS ==========\r\n");
        printf("Firmware : %s\r\n", esp_app_get_description()->version);
        printf("Board    : %s\r\n", GetXiaoSerialBoardName());
        printf("-----------------------------------\r\n");
        printf("WiFi SSID: %s\r\n", wifi.GetSsid().c_str());
        printf("IP Address: %s\r\n", wifi.GetIpAddress().c_str());
        printf("-----------------------------------\r\n");
        std::string displayed_url = XiaoRedactServerUrl(ota_url);
        printf("OTA URL  : %s\r\n", displayed_url.c_str());
        printf("-----------------------------------\r\n");
        printf("Free heap: %lu bytes\r\n", (unsigned long)esp_get_free_heap_size());
        printf("Camera   : %s\r\n", Board::GetInstance().GetCamera() ? "available" : "not available");
        printf("===================================\r\n\r\n");
        fflush(stdout);
        return;
    }

    // --- !camera ---
    if (strcmp(buf, "!camera") == 0) {
        auto camera = Board::GetInstance().GetCamera();
        if (camera == nullptr) {
            printf("\r\n=== Camera Diagnostic ===\r\n");
            printf("Camera: not available on this board\r\n");
            printf("=========================\r\n\r\n");
            fflush(stdout);
            return;
        }

        printf("\r\n=== Camera Diagnostic ===\r\n");
        auto& app = Application::GetInstance();
        auto state = app.GetDeviceState();
        if (state == kDeviceStateSpeaking) {
            app.AbortSpeaking(kAbortReasonNone);
        }
        if (state == kDeviceStateListening || state == kDeviceStateSpeaking) {
            app.StopListening();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        printf("Capturing frame...\r\n");
        fflush(stdout);
        if (camera->Capture()) {
            printf("Camera capture: OK\r\n");
        } else {
            printf("Camera capture: FAILED\r\n");
        }
        printf("=========================\r\n\r\n");
        fflush(stdout);
        return;
    }

    // --- !reboot ---
    if (strcmp(buf, "!reboot") == 0) {
        printf("\r\n>>> REBOOTING DEVICE NOW <<<\r\n\r\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
        return;
    }

    // --- !stop ---
    if (strcmp(buf, "!stop") == 0) {
        auto& app = Application::GetInstance();
        if (app.GetDeviceState() == kDeviceStateSpeaking) {
            app.AbortSpeaking(kAbortReasonNone);
        }
        app.StopListening();
        printf("\r\n>>> STOP LISTENING REQUESTED <<<\r\n\r\n");
        fflush(stdout);
        return;
    }

    // --- !quiet ---
    // Pauses all local listening until toggled or rebooted, so the robot stops
    // talking while a builder works on it. Servo calibration is impossible with
    // a voice agent answering room noise through the same speaker.
    if (strncmp(buf, "!quiet", 6) == 0 && (buf[6] == ' ' || buf[6] == '\0')) {
        const char* args = buf[6] == ' ' ? buf + 7 : "";
        auto& app = Application::GetInstance();
        if (strcmp(args, "off") == 0) {
            app.SetListeningPaused(false);
            printf("Quiet mode off: listening resumed.\r\n");
        } else if (strcmp(args, "status") == 0) {
            printf("Quiet mode: %s\r\n", app.IsListeningPaused() ? "on" : "off");
        } else if (args[0] == '\0' || strcmp(args, "on") == 0) {
            if (app.GetDeviceState() == kDeviceStateSpeaking) {
                app.AbortSpeaking(kAbortReasonNone);
            }
            app.SetListeningPaused(true);
            app.StopListening();
            printf("Quiet mode on: not listening, not speaking.\r\n");
        } else {
            printf("Usage: !quiet [on|off|status]\r\n");
        }
        fflush(stdout);
        return;
    }

    // --- !mic ---
    if (strncmp(buf, "!mic", 4) == 0 && (buf[4] == ' ' || buf[4] == '\0')) {
        const char* args = buf[4] == ' ' ? buf + 5 : "";
        auto& board = Board::GetInstance();
        auto codec = board.GetAudioCodec();

        if (strcmp(args, "mute") == 0) {
            codec->EnableInput(false);
            printf("Microphone muted.\r\n");
        } else if (strcmp(args, "unmute") == 0) {
            codec->EnableInput(true);
            printf("Microphone unmuted.\r\n");
        } else if (strncmp(args, "gain ", 5) == 0) {
            float gain = atof(args + 5);
            codec->SetInputGain(gain);
            printf("Microphone gain set to %.1f\r\n", gain);
        } else if (strcmp(args, "status") == 0 || strcmp(args, "") == 0) {
            printf("Microphone status:\r\n");
            printf("  Gain  : %.1f\r\n", codec->input_gain());
            printf("  Muted : %s\r\n", codec->input_enabled() ? "no" : "yes");
        } else {
            printf("Usage: !mic [gain <value>|mute|unmute|status]\r\n");
        }
        fflush(stdout);
        return;
    }

    // --- !speaker ---
    if (strncmp(buf, "!speaker", 8) == 0 && (buf[8] == ' ' || buf[8] == '\0')) {
        const char* args = buf[8] == ' ' ? buf + 9 : "";
        auto& app = Application::GetInstance();
        auto codec = Board::GetInstance().GetAudioCodec();

        if (strncmp(args, "vol ", 4) == 0) {
            int vol = atoi(args + 4);
            if (vol < 0) vol = 0;
            if (vol > 100) vol = 100;
            codec->SetOutputVolume(vol);
            printf("Speaker volume set to %d\r\n", vol);
        } else if (strcmp(args, "status") == 0) {
            printf("Speaker status:\r\n");
            printf("  Volume : %d\r\n", codec->output_volume());
            printf("  Output : %s\r\n", codec->output_enabled() ? "enabled" : "disabled");
        } else if (strcmp(args, "") == 0 || strcmp(args, "test") == 0) {
            // Plays a built-in OGG chime through the speaker. PlaySound enables
            // the codec output on its own, so this works from any state without
            // a server connection — exercising opus decode -> I2S -> amp -> speaker.
            printf("Playing test sound at volume %d...\r\n", codec->output_volume());
            fflush(stdout);
            app.PlaySound(Lang::Sounds::OGG_SUCCESS);
        } else {
            printf("Usage: !speaker [test|vol <0-100>|status]\r\n");
        }
        fflush(stdout);
        return;
    }

    // --- !help ---
    if (strcmp(buf, "!help") == 0) {
        printf("Commands:\r\n");
        printf("  !wifi SSID PASSWORD  -- add a WiFi network\r\n");
        printf("  !wifi list           -- list saved networks\r\n");
        printf("  !wifi clear          -- remove all saved networks\r\n");
        printf("  !server IP           -- set server IP (keeps path) and reboot\r\n");
        printf("  !server URL          -- set full OTA URL and reboot\r\n");
        printf("  !server list         -- show recent servers\r\n");
        printf("  !server N            -- reuse recent server #N and reboot\r\n");
        printf("  !server              -- show current server URL\r\n");
        printf("  !status              -- show WiFi, IP, server, heap\r\n");
        printf("  !camera              -- capture one camera frame\r\n");
        printf("  !mic [gain N]        -- set mic gain (float, e.g. 30.0)\r\n");
        printf("  !mic [mute|unmute]   -- mute/unmute microphone\r\n");
        printf("  !mic status          -- show mic gain and mute state\r\n");
        printf("  !speaker             -- play a test sound through the speaker\r\n");
        printf("  !speaker vol N       -- set speaker volume (0-100)\r\n");
        printf("  !speaker status      -- show speaker volume and output state\r\n");
        printf("  !reboot              -- reboot the device\r\n");
        printf("  !stop                -- stop listening / close active listening\r\n");
        printf("  !quiet [on|off]      -- pause/resume listening and speaking\r\n");
#if defined(CONFIG_BOARD_TYPE_COGLET) || defined(CONFIG_BOARD_TYPE_COGLET_C3)
        printf("  !coglet state        -- robot diagnostics and calibration\r\n");
        printf("  !coglet release      -- latch servo outputs off (not a power cut)\r\n");
        printf("  !coglet scan         -- list devices answering on the servo I2C bus\r\n");
#endif
        printf("  !help                -- show this message\r\n");
        printf("  anything else        -- send as chat message\r\n\r\n");
        fflush(stdout);
        return;
    }

    if (buf[0] == '!') {
        printf("\r\nUnknown command: %s\r\n", buf);
        printf("Type !help for available commands.\r\n\r\n");
        fflush(stdout);
        return;
    }

    // Forward everything else to the chat pipeline
    Application::GetInstance().SendTextChat(std::string(buf));
}

// On boards whose console is routed to the USB-Serial-JTAG peripheral, the
// default IDF console only wires up non-blocking output. Reads via fgetc(stdin)
// then always return EOF, so the interactive commands never see host input.
// Install the USB-Serial-JTAG VFS driver (mirrors esp_console's setup) so stdin
// delivers bytes. This is a no-op on UART0 consoles, which already work.
static void XiaoSerialConsoleInit() {
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    // Translate incoming CR to \n and outgoing \n to CRLF for terminal echo.
    usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    // Make stdin/stdout blocking so fgetc waits for input instead of spinning.
    fcntl(fileno(stdout), F_SETFL, 0);
    fcntl(fileno(stdin), F_SETFL, 0);

    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    if (usb_serial_jtag_driver_install(&cfg) == ESP_OK) {
        usb_serial_jtag_vfs_use_driver();
    }
#endif
}

static void XiaoSerialInputTask(void* arg) {
    XiaoSerialConsoleInit();
    char buf[256];
    int pos = 0;
    while (true) {
        int ch = fgetc(stdin);
        if (ch == EOF) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (ch == '\n' || ch == '\r') {
            fputc('\n', stdout);
            fflush(stdout);
            if (pos > 0) {
                buf[pos] = '\0';
                const char* line = TrimXiaoSerialLine(buf);
                if (*line != '\0') {
                    HandleXiaoSerialLine(line);
                }
                pos = 0;
            }
            continue;
        }
        if (ch == 0 || (ch < 0x20 && ch != '\t')) {
            continue;
        }
        if (pos < (int)(sizeof(buf) - 1)) {
            buf[pos++] = (char)ch;
            fputc(ch, stdout);
            fflush(stdout);
        }
    }
}

#pragma once

// Shared serial command handler for all XIAO boards.
// Handles: !wifi, !server, !status, !reboot, !help
// Anything else is forwarded to the chat pipeline.

#include "application.h"
#include "settings.h"
#include "ssid_manager.h"
#include "system_info.h"
#include "wifi_manager.h"
#include "sdkconfig.h"

#include <esp_app_desc.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

static void HandleXiaoSerialLine(const char* buf) {
    auto& ssid_manager = SsidManager::GetInstance();

    // --- !wifi ---
    if (strncmp(buf, "!wifi", 5) == 0 && (buf[5] == ' ' || buf[5] == '\0')) {
        const char* args = buf[5] == ' ' ? buf + 6 : "";
        if (strcmp(args, "list") == 0) {
            const auto& list = ssid_manager.GetSsidList();
            printf("Saved networks (%d):\r\n", (int)list.size());
            for (int i = 0; i < (int)list.size(); i++) {
                printf("  [%d] %s\r\n", i, list[i].ssid.c_str());
            }
        } else if (strcmp(args, "clear") == 0) {
            ssid_manager.Clear();
            printf("All networks cleared.\r\n");
        } else if (strlen(args) > 0) {
            char ssid[64] = {}, pass[64] = {};
            if (sscanf(args, "%63s %63s", ssid, pass) == 2) {
                ssid_manager.AddSsid(std::string(ssid), std::string(pass));
                printf("Added: %s\r\n", ssid);
            } else {
                printf("Usage: !wifi SSID PASSWORD\r\n");
            }
        } else {
            printf("Usage: !wifi SSID PASSWORD | !wifi list | !wifi clear\r\n");
        }
        fflush(stdout);
        return;
    }

    // --- !server ---
    if (strncmp(buf, "!server", 7) == 0 && (buf[7] == ' ' || buf[7] == '\0')) {
        const char* args = buf[7] == ' ' ? buf + 8 : "";
        if (strlen(args) == 0) {
            Settings s("wifi", false);
            std::string stored = s.GetString("ota_url");
            printf("OTA URL: %s\r\n", stored.empty() ? CONFIG_OTA_URL : stored.c_str());
            printf("Usage: !server IP  or  !server http://IP:8003/xiaozhi/ota/\r\n");
        } else {
            std::string url(args);
            // Accept bare IP — construct the full OTA URL automatically
            if (url.find("http") != 0) {
                url = "http://" + url + ":8003/xiaozhi/ota/";
            }
            Settings s("wifi", true);
            s.SetString("ota_url", url);
            printf("Server set to: %s\r\nRebooting...\r\n", url.c_str());
            fflush(stdout);
            vTaskDelay(pdMS_TO_TICKS(200));
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
        printf("=== Status ===\r\n");
        printf("Firmware : %s\r\n", esp_app_get_description()->version);
        printf("SSID     : %s\r\n", wifi.GetSsid().c_str());
        printf("IP       : %s\r\n", wifi.GetIpAddress().c_str());
        printf("OTA URL  : %s\r\n", ota_url.c_str());
        printf("Free heap: %lu bytes\r\n", (unsigned long)esp_get_free_heap_size());
        fflush(stdout);
        return;
    }

    // --- !reboot ---
    if (strcmp(buf, "!reboot") == 0) {
        printf("Rebooting...\r\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_restart();
        return;
    }

    // --- !help ---
    if (strcmp(buf, "!help") == 0) {
        printf("Commands:\r\n");
        printf("  !wifi SSID PASSWORD  -- add a WiFi network\r\n");
        printf("  !wifi list           -- list saved networks\r\n");
        printf("  !wifi clear          -- remove all saved networks\r\n");
        printf("  !server IP           -- set server IP and reboot\r\n");
        printf("  !server URL          -- set full OTA URL and reboot\r\n");
        printf("  !server              -- show current server URL\r\n");
        printf("  !status              -- show WiFi, IP, server, heap\r\n");
        printf("  !reboot              -- reboot the device\r\n");
        printf("  !help                -- show this message\r\n");
        printf("  anything else        -- send as chat message\r\n");
        fflush(stdout);
        return;
    }

    // Forward everything else to the chat pipeline
    Application::GetInstance().SendTextChat(std::string(buf));
}

static void XiaoSerialInputTask(void* arg) {
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
                HandleXiaoSerialLine(buf);
                pos = 0;
            }
            continue;
        }
        if (pos < (int)(sizeof(buf) - 1)) {
            buf[pos++] = (char)ch;
            fputc(ch, stdout);
            fflush(stdout);
        }
    }
}

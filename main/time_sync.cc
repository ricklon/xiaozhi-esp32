#include "time_sync.h"
#include "settings.h"

#include <atomic>
#include <climits>
#include <ctime>
#include <sys/time.h>

#include <esp_log.h>
#include <esp_netif_sntp.h>
#include <esp_sntp.h>

#define TAG "TimeSync"

namespace {

constexpr char kNvsNamespace[] = "time";
constexpr char kNvsOffsetKey[] = "tz_offset";
constexpr char kSntpServer[] = "pool.ntp.org";

std::atomic<bool> s_started{false};
std::atomic<bool> s_has_time{false};
// INT_MIN => not yet loaded from NVS.
std::atomic<int> s_offset_minutes{INT_MIN};

// SNTP has just set the clock to UTC; shift it to the local-time convention the
// rest of the firmware expects.
void OnSntpSync(struct timeval* /*tv*/) {
    int minutes = TimeSync::GetTimezoneOffset();
    if (minutes != 0) {
        struct timeval now;
        gettimeofday(&now, nullptr);
        now.tv_sec += minutes * 60;
        settimeofday(&now, nullptr);
    }
    s_has_time = true;

    time_t t = time(nullptr);
    struct tm tm_local;
    localtime_r(&t, &tm_local);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_local);
    ESP_LOGI(TAG, "Clock synced via SNTP: %s (offset %+d min)", buf, minutes);
}

}  // namespace

void TimeSync::Start() {
    if (s_started.exchange(true)) {
        esp_sntp_restart();  // already initialized -- just re-poll
        return;
    }

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(kSntpServer);
    config.start = true;
    config.smooth_sync = false;  // step the clock, matching the OTA check-in path
    config.sync_cb = OnSntpSync;

    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_sntp_init failed: %s", esp_err_to_name(err));
        s_started = false;
        return;
    }
    ESP_LOGI(TAG, "SNTP started (server %s, poll every %d s)",
             kSntpServer, CONFIG_LWIP_SNTP_UPDATE_DELAY / 1000);
}

void TimeSync::SetTimezoneOffset(int minutes) {
    if (s_offset_minutes.exchange(minutes) == minutes) {
        return;
    }
    Settings settings(kNvsNamespace, true);
    settings.SetInt(kNvsOffsetKey, minutes);
}

int TimeSync::GetTimezoneOffset() {
    int cached = s_offset_minutes.load();
    if (cached != INT_MIN) {
        return cached;
    }
    Settings settings(kNvsNamespace, false);
    int stored = settings.GetInt(kNvsOffsetKey, 0);
    s_offset_minutes = stored;
    return stored;
}

bool TimeSync::HasTime() {
    return s_has_time.load();
}

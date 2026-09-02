#ifndef WIFI_NETWORKS_H
#define WIFI_NETWORKS_H

// Optional build-time Wi-Fi networks, injected without committing secrets.
//
// switch-board.sh generates wifi_networks.local.h (gitignored) from a
// gitignored wifi.env (see wifi.env.example) or from WIFI_SSID / WIFI_PASSWORD
// environment variables. WifiBoard seeds whatever it finds into NVS at boot,
// in addition to any board-specific WIFI_NETWORKS and anything added later via
// the "!wifi SSID PASSWORD" serial command.
//
// With no wifi.env and no env vars, WIFI_NETWORKS_LOCAL is empty and nothing
// is seeded.
#if defined(__has_include)
#  if __has_include("wifi_networks.local.h")
#    include "wifi_networks.local.h"
#  endif
#endif

#ifndef WIFI_NETWORKS_LOCAL
#  define WIFI_NETWORKS_LOCAL  // no build-time networks
#endif

#endif // WIFI_NETWORKS_H

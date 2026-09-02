#ifndef TIME_SYNC_H
#define TIME_SYNC_H

// SNTP-based wall-clock keeper.
//
// Time convention (shared with Ota::CheckVersion): the system clock is kept in
// *local* time -- the timezone offset is folded into the epoch and TZ is left
// at UTC, so localtime() is an identity passthrough. SNTP delivers UTC, so
// after every successful sync the stored offset is re-applied.
class TimeSync {
public:
    // Start SNTP polling. Safe to call on every network (re)connect: the first
    // call initializes SNTP, later calls just trigger an immediate re-poll.
    static void Start();

    // Record the timezone offset (minutes east of UTC) reported by the server's
    // OTA check-in. Persisted to NVS so it survives reboots and is available to
    // the SNTP sync callback before the first check-in of a session completes.
    static void SetTimezoneOffset(int minutes);

    // Minutes east of UTC. Falls back to the persisted value, then 0 (UTC).
    static int GetTimezoneOffset();

    // True once the clock has been set at least once this boot by SNTP.
    static bool HasTime();
};

#endif // TIME_SYNC_H
